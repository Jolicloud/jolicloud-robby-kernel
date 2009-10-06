/**
 * file lnc_topaz.c
 * TOPAZ I/O operations and IRQ handling
 *
 */

/**************************************************************************
 *
 * Copyright (c) 2007 Intel Corporation, Hillsboro, OR, USA
 * Copyright (c) Imagination Technologies Limited, UK
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/* include headers */
/* #define DRM_DEBUG_CODE 2 */

#include <drm/drmP.h>
#include <drm/drm_os_linux.h>

#include "psb_drv.h"
#include "psb_drm.h"
#include "lnc_topaz.h"

#include <linux/io.h>
#include <linux/delay.h>

static int drm_psb_ospmxxx = 0x0;

/* static function define */
static int lnc_topaz_deliver_command(struct drm_device *dev,
				     struct ttm_buffer_object *cmd_buffer,
				     unsigned long cmd_offset,
				     unsigned long cmd_size,
				     void **topaz_cmd, uint32_t sequence,
				     int copy_cmd);
static int lnc_topaz_send(struct drm_device *dev, void *cmd,
			unsigned long cmd_size, uint32_t sync_seq);
static int lnc_mtx_send(struct drm_psb_private *dev_priv, const void *cmd);
static int lnc_topaz_dequeue_send(struct drm_device *dev);
static int lnc_topaz_save_command(struct drm_device *dev, void *cmd,
			unsigned long cmd_size, uint32_t sequence);

void lnc_topaz_interrupt(struct drm_device *dev, uint32_t topaz_stat)
{
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *)dev->dev_private;
	uint32_t clr_flag = lnc_topaz_queryirq(dev);

	lnc_topaz_clearirq(dev, clr_flag);

	/* ignore non-SYNC interrupts */
	if ((CCB_CTRL_SEQ(dev_priv) & 0x8000) == 0)
		return;

	dev_priv->topaz_current_sequence =
		*(uint32_t *)dev_priv->topaz_sync_addr;

	PSB_DEBUG_IRQ("TOPAZ:Got SYNC IRQ,sync seq:0x%08x (MTX) vs 0x%08x\n",
		dev_priv->topaz_current_sequence,
		dev_priv->sequence[LNC_ENGINE_ENCODE]);

	psb_fence_handler(dev, LNC_ENGINE_ENCODE);

	dev_priv->topaz_busy = 1;
	lnc_topaz_dequeue_send(dev);
}

static int lnc_submit_encode_cmdbuf(struct drm_device *dev,
			struct ttm_buffer_object *cmd_buffer,
			unsigned long cmd_offset, unsigned long cmd_size,
			struct ttm_fence_object *fence)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	unsigned long irq_flags;
	int ret = 0;
	void *cmd;
	uint32_t sequence = dev_priv->sequence[LNC_ENGINE_ENCODE];

	PSB_DEBUG_GENERAL("TOPAZ: command submit\n");

	/* # lock topaz's mutex [msvdx_mutex] */
	mutex_lock(&dev_priv->topaz_mutex);

	PSB_DEBUG_GENERAL("TOPAZ: topaz busy = %d\n", dev_priv->topaz_busy);

	if (dev_priv->topaz_fw_loaded == 0) {
		/* #.# load fw to driver */
		PSB_DEBUG_INIT("TOPAZ: load /lib/firmware/topaz_fw.bin\n");
		ret = topaz_init_fw(dev);
		if (ret != 0) {
			mutex_unlock(&dev_priv->topaz_mutex);

			/* FIXME: find a proper return value */
			DRM_ERROR("TOPAX:load /lib/firmware/topaz_fw.bin fail,"
				"ensure udevd is configured correctly!\n");

			return -EFAULT;
		}
		dev_priv->topaz_fw_loaded = 1;
	} else {
		/* OSPM power state change */
		/* FIXME: why here? why not in the NEW_CODEC case? */
		if (drm_psb_ospmxxx & ENABLE_TOPAZ_OSPM_D0IX) {
			psb_power_up_topaz(dev);
			lnc_topaz_restore_mtx_state(dev);
		}
	}

	/* # schedule watchdog */
	/* psb_schedule_watchdog(dev_priv); */

	/* # spin lock irq save [msvdx_lock] */
	spin_lock_irqsave(&dev_priv->topaz_lock, irq_flags);

	/* # if topaz need to reset, reset it */
	if (dev_priv->topaz_needs_reset) {
		/* #.# reset it */
		spin_unlock_irqrestore(&dev_priv->topaz_lock, irq_flags);
		PSB_DEBUG_GENERAL("TOPAZ: needs reset.\n");

		if (lnc_topaz_reset(dev_priv)) {
			mutex_unlock(&dev_priv->topaz_mutex);
			ret = -EBUSY;
			DRM_ERROR("TOPAZ: reset failed.\n");
			return ret;
		}

		PSB_DEBUG_GENERAL("TOPAZ: reset ok.\n");

		/* #.# reset any related flags */
		dev_priv->topaz_needs_reset = 0;
		dev_priv->topaz_busy = 0;
		PSB_DEBUG_GENERAL("XXX: does we need idle flag??\n");
		dev_priv->topaz_start_idle = 0;

		/* #.# init topaz */
		lnc_topaz_init(dev);

		/* avoid another fw init */
		dev_priv->topaz_fw_loaded = 1;

		spin_lock_irqsave(&dev_priv->topaz_lock, irq_flags);
	}

	if (!dev_priv->topaz_busy) {
		/* # direct map topaz command if topaz is free */
		PSB_DEBUG_GENERAL("TOPAZ:direct send command,sequence %08x \n",
			   sequence);

		dev_priv->topaz_busy = 1;
		spin_unlock_irqrestore(&dev_priv->topaz_lock, irq_flags);

		ret = lnc_topaz_deliver_command(dev, cmd_buffer, cmd_offset,
						cmd_size, NULL, sequence, 0);

		if (ret) {
			DRM_ERROR("TOPAZ: failed to extract cmd...\n");
			mutex_unlock(&dev_priv->topaz_mutex);
			return ret;
		}
	} else {
		PSB_DEBUG_GENERAL("TOPAZ: queue command,sequence %08x \n",
				sequence);
		cmd = NULL;

		spin_unlock_irqrestore(&dev_priv->topaz_lock, irq_flags);

		ret = lnc_topaz_deliver_command(dev, cmd_buffer, cmd_offset,
						cmd_size, &cmd, sequence, 1);
		if (cmd == NULL || ret) {
			DRM_ERROR("TOPAZ: map command for save fialed\n");
			mutex_unlock(&dev_priv->topaz_mutex);
			return ret;
		}

		ret = lnc_topaz_save_command(dev, cmd, cmd_size, sequence);
		if (ret)
			DRM_ERROR("TOPAZ: save command failed\n");
	}

	/* OPSM D0IX power state change */
	if (drm_psb_ospmxxx & ENABLE_TOPAZ_OSPM_D0IX)
		lnc_topaz_save_mtx_state(dev);

	mutex_unlock(&dev_priv->topaz_mutex);

	return ret;
}

static int lnc_topaz_save_command(struct drm_device *dev, void *cmd,
			unsigned long cmd_size, uint32_t sequence)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct lnc_topaz_cmd_queue *topaz_cmd;
	unsigned long irq_flags;

	PSB_DEBUG_GENERAL("TOPAZ: queue command,sequence: %08x..\n",
			sequence);

	topaz_cmd = kcalloc(1, sizeof(struct lnc_topaz_cmd_queue), GFP_KERNEL);
	if (topaz_cmd == NULL) {
		mutex_unlock(&dev_priv->topaz_mutex);
		DRM_ERROR("TOPAZ: out of memory....\n");
		return -ENOMEM;
	}

	topaz_cmd->cmd = cmd;
	topaz_cmd->cmd_size = cmd_size;
	topaz_cmd->sequence = sequence;

	spin_lock_irqsave(&dev_priv->topaz_lock, irq_flags);
	list_add_tail(&topaz_cmd->head, &dev_priv->topaz_queue);
	if (!dev_priv->topaz_busy) {
		/* dev_priv->topaz_busy = 1; */
		PSB_DEBUG_GENERAL("TOPAZ: need immediate dequeue...\n");
		lnc_topaz_dequeue_send(dev);
		PSB_DEBUG_GENERAL("TOPAZ: after dequeue command\n");
	}

	spin_unlock_irqrestore(&dev_priv->topaz_lock, irq_flags);

	return 0;
}


int lnc_cmdbuf_video(struct drm_file *priv,
		struct list_head *validate_list,
		uint32_t fence_type,
		struct drm_psb_cmdbuf_arg *arg,
		struct ttm_buffer_object *cmd_buffer,
		struct psb_ttm_fence_rep *fence_arg)
{
	struct drm_device *dev = priv->minor->dev;
	struct ttm_fence_object *fence = NULL;
	int ret;

	ret = lnc_submit_encode_cmdbuf(dev, cmd_buffer, arg->cmdbuf_offset,
				      arg->cmdbuf_size, fence);
	if (ret)
		return ret;

#if LNC_TOPAZ_NO_IRQ /* workaround for interrupt issue */
	psb_fence_or_sync(priv, LNC_ENGINE_ENCODE, fence_type, arg->fence_flags,
			  validate_list, fence_arg, &fence);

	if (fence)
		ttm_fence_object_unref(&fence);
#endif

	mutex_lock(&cmd_buffer->mutex);
	if (cmd_buffer->sync_obj != NULL)
		ttm_fence_sync_obj_unref(&cmd_buffer->sync_obj);
	mutex_unlock(&cmd_buffer->mutex);

	return 0;
}

static int lnc_topaz_sync(struct drm_device *dev, uint32_t sync_seq)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	uint32_t sync_cmd[3];
	int count = 10000;
#if 0
	struct ttm_fence_device *fdev = &dev_priv->fdev;
	struct ttm_fence_class_manager *fc =
	    &fdev->fence_class[LNC_ENGINE_ENCODE];
	unsigned long irq_flags;
#endif
	uint32_t *sync_p = (uint32_t *)dev_priv->topaz_sync_addr;

	/* insert a SYNC command here */
	dev_priv->topaz_sync_cmd_seq = (1 << 15) | dev_priv->topaz_cmd_seq++;
	sync_cmd[0] = MTX_CMDID_SYNC | (3 << 8) |
		(dev_priv->topaz_sync_cmd_seq << 16);
	sync_cmd[1] = dev_priv->topaz_sync_offset;
	sync_cmd[2] = sync_seq;

	PSB_DEBUG_GENERAL("TOPAZ:MTX_CMDID_SYNC: size(3),cmd seq (0x%04x),"
			"sync_seq (0x%08x)\n",
			dev_priv->topaz_sync_cmd_seq, sync_seq);

	lnc_mtx_send(dev_priv, sync_cmd);

#if LNC_TOPAZ_NO_IRQ /* workaround for interrupt issue */
	/* # poll topaz register for certain times */
	while (count && *sync_p != sync_seq) {
		DRM_UDELAY(100);
		--count;
	}
	if ((count == 0) && (*sync_p != sync_seq)) {
		DRM_ERROR("TOPAZ: wait sycn timeout (0x%08x),actual 0x%08x\n",
			sync_seq, *sync_p);
		return -EBUSY;
	}
	PSB_DEBUG_GENERAL("TOPAZ: SYNC done, seq=0x%08x\n", *sync_p);

	dev_priv->topaz_busy = 0;

	/* XXX: check psb_fence_handler is suitable for topaz */
	dev_priv->topaz_current_sequence = *sync_p;
#if 0
	write_lock_irqsave(&fc->lock, irq_flags);
	ttm_fence_handler(fdev, LNC_ENGINE_ENCODE,
			dev_priv->topaz_current_sequence,
			_PSB_FENCE_TYPE_EXE, 0);
	write_unlock_irqrestore(&fc->lock, irq_flags);
#endif
#endif
	return 0;
}

int
lnc_topaz_deliver_command(struct drm_device *dev,
			  struct ttm_buffer_object *cmd_buffer,
			  unsigned long cmd_offset, unsigned long cmd_size,
			  void **topaz_cmd, uint32_t sequence,
			  int copy_cmd)
{
	unsigned long cmd_page_offset = cmd_offset & ~PAGE_MASK;
	struct ttm_bo_kmap_obj cmd_kmap;
	bool is_iomem;
	int ret;
	unsigned char *cmd_start, *tmp;

	ret = ttm_bo_kmap(cmd_buffer, cmd_offset >> PAGE_SHIFT, 2,
			&cmd_kmap);
	if (ret) {
		DRM_ERROR("TOPAZ: drm_bo_kmap failed: %d\n", ret);
		return ret;
	}
	cmd_start = (unsigned char *) ttm_kmap_obj_virtual(&cmd_kmap,
					&is_iomem) + cmd_page_offset;

	if (copy_cmd) {
		PSB_DEBUG_GENERAL("TOPAZ: queue commands\n");
		tmp = kcalloc(1, cmd_size, GFP_KERNEL);
		if (tmp == NULL) {
			ret = -ENOMEM;
			goto out;
		}
		memcpy(tmp, cmd_start, cmd_size);
		*topaz_cmd = tmp;
	} else {
		PSB_DEBUG_GENERAL("TOPAZ: directly send the command\n");
		ret = lnc_topaz_send(dev, cmd_start, cmd_size, sequence);
		if (ret) {
			DRM_ERROR("TOPAZ: commit commands failed.\n");
			ret = -EINVAL;
		}
	}

out:
	PSB_DEBUG_GENERAL("TOPAZ:cmd_size(%ld), sequence(%d) copy_cmd(%d)\n",
			cmd_size, sequence, copy_cmd);

	ttm_bo_kunmap(&cmd_kmap);

	return ret;
}

int
lnc_topaz_send(struct drm_device *dev, void *cmd,
	unsigned long cmd_size, uint32_t sync_seq)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	int ret = 0;
	unsigned char *command = (unsigned char *) cmd;
	struct topaz_cmd_header *cur_cmd_header;
	uint32_t cur_cmd_size, cur_cmd_id;
	uint32_t codec;

	PSB_DEBUG_GENERAL("TOPAZ: send the command in the buffer one by one\n");

	while (cmd_size > 0) {
		cur_cmd_header = (struct topaz_cmd_header *) command;
		cur_cmd_size = cur_cmd_header->size * 4;
		cur_cmd_id = cur_cmd_header->id;

		switch (cur_cmd_id) {
		case MTX_CMDID_SW_NEW_CODEC:
			codec = *((uint32_t *) cmd + 1);

			PSB_DEBUG_GENERAL("TOPAZ: setup new codec %s (%d)\n",
					codec_to_string(codec), codec);
			if (topaz_setup_fw(dev, codec)) {
				DRM_ERROR("TOPAZ: upload FW to HW failed\n");
				return -EBUSY;
			}

			dev_priv->topaz_cur_codec = codec;
			break;

		case MTX_CMDID_SW_ENTER_LOWPOWER:
			PSB_DEBUG_GENERAL("TOPAZ: enter lowpower.... \n");
			PSB_DEBUG_GENERAL("XXX: implement it\n");
			break;

		case MTX_CMDID_SW_LEAVE_LOWPOWER:
			PSB_DEBUG_GENERAL("TOPAZ: leave lowpower... \n");
			PSB_DEBUG_GENERAL("XXX: implement it\n");
			break;

			/* ordinary commmand */
		case MTX_CMDID_START_PIC:
			/* XXX: specially handle START_PIC hw command */
			CCB_CTRL_SET_QP(dev_priv,
					*(command + cur_cmd_size - 4));
			/* strip the QP parameter (it's software arg) */
			cur_cmd_header->size--;
		default:
			cur_cmd_header->seq = 0x7fff &
				dev_priv->topaz_cmd_seq++;

			PSB_DEBUG_GENERAL("TOPAZ: %s: size(%d),"
					" seq (0x%04x)\n",
					cmd_to_string(cur_cmd_id),
					cur_cmd_size, cur_cmd_header->seq);
			ret = lnc_mtx_send(dev_priv, command);
			if (ret) {
				DRM_ERROR("TOPAZ: error -- ret(%d)\n", ret);
				goto out;
			}
			break;
		}

		command += cur_cmd_size;
		cmd_size -= cur_cmd_size;
	}
	lnc_topaz_sync(dev, sync_seq);
out:
	return ret;
}

static int lnc_mtx_send(struct drm_psb_private *dev_priv, const void *cmd)
{
	struct topaz_cmd_header *cur_cmd_header =
		(struct topaz_cmd_header *) cmd;
	uint32_t cmd_size = cur_cmd_header->size;
	uint32_t read_index, write_index;
	const uint32_t *cmd_pointer = (uint32_t *) cmd;

	int ret = 0;

	/* <msvdx does> # enable all clock */

	write_index = dev_priv->topaz_cmd_windex;
	if (write_index + cmd_size + 1 > dev_priv->topaz_ccb_size) {
		int free_space = dev_priv->topaz_ccb_size - write_index;

		PSB_DEBUG_GENERAL("TOPAZ: -------will wrap CCB write point.\n");
		if (free_space > 0) {
			struct topaz_cmd_header pad_cmd;

			pad_cmd.id = MTX_CMDID_NULL;
			pad_cmd.size = free_space;
			pad_cmd.seq = 0x7fff & dev_priv->topaz_cmd_seq++;

			PSB_DEBUG_GENERAL("TOPAZ: MTX_CMDID_NULL:"
					" size(%d),seq (0x%04x)\n",
					pad_cmd.size, pad_cmd.seq);

			TOPAZ_BEGIN_CCB(dev_priv);
			TOPAZ_OUT_CCB(dev_priv, pad_cmd.val);
			TOPAZ_END_CCB(dev_priv, 1);
		}
		POLL_WB_RINDEX(dev_priv, 0);
		if (ret == 0)
			dev_priv->topaz_cmd_windex = 0;
		else {
			DRM_ERROR("TOPAZ: poll rindex timeout\n");
			return ret; /* HW may hang, need reset */
		}
		PSB_DEBUG_GENERAL("TOPAZ: -------wrap CCB was done.\n");
	}

	read_index = CCB_CTRL_RINDEX(dev_priv);/* temperily use CCB CTRL */
	write_index = dev_priv->topaz_cmd_windex;

	PSB_DEBUG_GENERAL("TOPAZ: write index(%d), read index(%d,WB=%d)\n",
			write_index, read_index, WB_CCB_CTRL_RINDEX(dev_priv));
	TOPAZ_BEGIN_CCB(dev_priv);
	while (cmd_size > 0) {
		TOPAZ_OUT_CCB(dev_priv, *cmd_pointer++);
		--cmd_size;
	}
	TOPAZ_END_CCB(dev_priv, 1);

	POLL_WB_RINDEX(dev_priv, dev_priv->topaz_cmd_windex);

#if 0
	DRM_UDELAY(1000);
	lnc_topaz_clearirq(dev,
			lnc_topaz_queryirq(dev));
	LNC_TRACEL("TOPAZ: after clear, query again\n");
	lnc_topaz_queryirq(dev_priv);
#endif

	return ret;
}

int lnc_topaz_dequeue_send(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct lnc_topaz_cmd_queue *topaz_cmd = NULL;
	int ret;

	PSB_DEBUG_GENERAL("TOPAZ: dequeue command and send it to topaz\n");

	if (list_empty(&dev_priv->topaz_queue)) {
		dev_priv->topaz_busy = 0;
		return 0;
	}

	topaz_cmd = list_first_entry(&dev_priv->topaz_queue,
				struct lnc_topaz_cmd_queue, head);

	PSB_DEBUG_GENERAL("TOPAZ: queue has id %08x\n", topaz_cmd->sequence);
	ret = lnc_topaz_send(dev, topaz_cmd->cmd, topaz_cmd->cmd_size,
			topaz_cmd->sequence);
	if (ret) {
		DRM_ERROR("TOPAZ: lnc_topaz_send failed.\n");
		ret = -EINVAL;
	}

	list_del(&topaz_cmd->head);
	kfree(topaz_cmd->cmd);
	kfree(topaz_cmd);

	return ret;
}

void
lnc_topaz_lockup(struct drm_psb_private *dev_priv,
		 int *topaz_lockup, int *topaz_idle)
{
	unsigned long irq_flags;
	uint32_t tmp;

	/* if have printk in this function, you will have plenties here */
	spin_lock_irqsave(&dev_priv->topaz_lock, irq_flags);
	*topaz_lockup = 0;
	*topaz_idle = 1;

	if (!dev_priv->has_topaz) {
		spin_unlock_irqrestore(&dev_priv->topaz_lock, irq_flags);
		return;
	}

	tmp = dev_priv->topaz_current_sequence
		- dev_priv->sequence[LNC_ENGINE_ENCODE];
	if (tmp > 0x0FFFFFFF) {
		if (dev_priv->topaz_current_sequence ==
		    dev_priv->topaz_last_sequence) {
			*topaz_lockup = 1;
		} else {
			dev_priv->topaz_last_sequence =
			    dev_priv->topaz_current_sequence;
			*topaz_idle = 0;
		}

		if (dev_priv->topaz_start_idle)
			dev_priv->topaz_start_idle = 0;
	} else {
		if (dev_priv->topaz_needs_reset == 0) {
			if (dev_priv->topaz_start_idle &&
				(dev_priv->topaz_finished_sequence
					== dev_priv->topaz_current_sequence)) {
				if (time_after_eq(jiffies,
					dev_priv->topaz_idle_start_jiffies +
							TOPAZ_MAX_IDELTIME)) {

					/* XXX: disable clock <msvdx does> */
					dev_priv->topaz_needs_reset = 1;
				} else
					*topaz_idle = 0;
			} else {
				dev_priv->topaz_start_idle = 1;
				dev_priv->topaz_idle_start_jiffies =  jiffies;
				dev_priv->topaz_finished_sequence =
					dev_priv->topaz_current_sequence;
				*topaz_idle = 0;
			}
		}
	}
	spin_unlock_irqrestore(&dev_priv->topaz_lock, irq_flags);
}


void topaz_mtx_kick(struct drm_psb_private *dev_priv, uint32_t kick_count)
{
	PSB_DEBUG_GENERAL("TOPAZ: kick mtx count(%d).\n", kick_count);
	MTX_WRITE32(MTX_CR_MTX_KICK, kick_count);
}

/* power up msvdx, OSPM function */
int psb_power_up_topaz(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *)dev->dev_private;

	if (dev_priv->topaz_power_state == LNC_TOPAZ_POWERON)
		return 0;

	psb_up_island_power(dev, PSB_VIDEO_ENC_ISLAND);

	PSB_DEBUG_GENERAL("FIXME: how to write clock state for topaz?"
			" so many clock\n");
	/* PSB_WMSVDX32(dev_priv->topaz_clk_state, MSVDX_MAN_CLK_ENABLE); */

	PSB_DEBUG_GENERAL("FIXME restore registers or init msvdx\n");

	PSB_DEBUG_GENERAL("FIXME: flush all mmu\n");

	dev_priv->topaz_power_state = LNC_TOPAZ_POWERON;

	return 0;
}

int psb_power_down_topaz(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *)dev->dev_private;

	if (dev_priv->topaz_power_state == LNC_TOPAZ_POWEROFF)
		return 0;

	if (dev_priv->topaz_busy) {
		PSB_DEBUG_GENERAL("FIXME: MSVDX is busy, should wait it\n");
		return -EBUSY;
	}
	PSB_DEBUG_GENERAL("FIXME: how to read clock state for topaz?"
			" so many clock\n");
	/* dev_priv->topaz_clk_state = PSB_RMSVDX32(MSVDX_MAN_CLK_ENABLE); */
	PSB_DEBUG_GENERAL("FIXME: save MSVDX register\n");
	PSB_DEBUG_GENERAL("FIXME: save MSVDX context\n");

	psb_down_island_power(dev, PSB_VIDEO_ENC_ISLAND);

	dev_priv->topaz_power_state = LNC_TOPAZ_POWEROFF;

	return 0;
}

int lnc_prepare_topaz_suspend(struct drm_device *dev)
{
	/* FIXME: need reset when resume?
	 * Is mtx restore enough for encoder continue run? */
	/* dev_priv->topaz_needs_reset = 1; */

	/* make sure all IRQs are seviced */

	/* make sure all the fence is signaled */

	/* save mtx context into somewhere */
	/* lnc_topaz_save_mtx_state(dev); */

	return 0;
}

int lnc_prepare_topaz_resume(struct drm_device *dev)
{
	/* FIXME: need reset when resume?
	 * Is mtx restore enough for encoder continue run? */
	/* dev_priv->topaz_needs_reset = 1; */

	/* make sure IRQ is open */

	/* restore mtx context */
	/* lnc_topaz_restore_mtx_state(dev); */

	return 0;
}
