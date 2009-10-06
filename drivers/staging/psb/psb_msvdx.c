/**
 * file psb_msvdx.c
 * MSVDX I/O operations and IRQ handling
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

#include <drm/drmP.h>
#include <drm/drm_os_linux.h>
#include "psb_drv.h"
#include "psb_drm.h"
#include "psb_msvdx.h"

#include <linux/io.h>
#include <linux/delay.h>

#ifndef list_first_entry
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)
#endif


static int psb_msvdx_send(struct drm_device *dev, void *cmd,
			  unsigned long cmd_size);

int psb_msvdx_dequeue_send(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_msvdx_cmd_queue *msvdx_cmd = NULL;
	int ret = 0;

	if (list_empty(&dev_priv->msvdx_queue)) {
		PSB_DEBUG_GENERAL("MSVDXQUE: msvdx list empty.\n");
		dev_priv->msvdx_busy = 0;
		return -EINVAL;
	}
	msvdx_cmd = list_first_entry(&dev_priv->msvdx_queue,
				struct psb_msvdx_cmd_queue, head);
	PSB_DEBUG_GENERAL("MSVDXQUE: Queue has id %08x\n", msvdx_cmd->sequence);
	ret = psb_msvdx_send(dev, msvdx_cmd->cmd, msvdx_cmd->cmd_size);
	if (ret) {
		DRM_ERROR("MSVDXQUE: psb_msvdx_send failed\n");
		ret = -EINVAL;
	}
	list_del(&msvdx_cmd->head);
	kfree(msvdx_cmd->cmd);
	kfree(msvdx_cmd);

	return ret;
}

int psb_msvdx_map_command(struct drm_device *dev,
			  struct ttm_buffer_object *cmd_buffer,
			  unsigned long cmd_offset, unsigned long cmd_size,
			  void **msvdx_cmd, uint32_t sequence, int copy_cmd)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	int ret = 0;
	unsigned long cmd_page_offset = cmd_offset & ~PAGE_MASK;
	unsigned long cmd_size_remaining;
	struct ttm_bo_kmap_obj cmd_kmap;
	void *cmd, *tmp, *cmd_start;
	bool is_iomem;

	/* command buffers may not exceed page boundary */
	if (cmd_size + cmd_page_offset > PAGE_SIZE)
		return -EINVAL;

	ret = ttm_bo_kmap(cmd_buffer, cmd_offset >> PAGE_SHIFT, 2, &cmd_kmap);
	if (ret) {
		DRM_ERROR("MSVDXQUE:ret:%d\n", ret);
		return ret;
	}

	cmd_start = (void *)ttm_kmap_obj_virtual(&cmd_kmap, &is_iomem)
		+ cmd_page_offset;
	cmd = cmd_start;
	cmd_size_remaining = cmd_size;

	while (cmd_size_remaining > 0) {
		uint32_t cur_cmd_size = MEMIO_READ_FIELD(cmd, FWRK_GENMSG_SIZE);
		uint32_t cur_cmd_id = MEMIO_READ_FIELD(cmd, FWRK_GENMSG_ID);
		uint32_t mmu_ptd = 0, tmp = 0;

		PSB_DEBUG_GENERAL("cmd start at %08x cur_cmd_size = %d"
				" cur_cmd_id = %02x fence = %08x\n",
			(uint32_t) cmd, cur_cmd_size, cur_cmd_id, sequence);
		if ((cur_cmd_size % sizeof(uint32_t))
		    || (cur_cmd_size > cmd_size_remaining)) {
			ret = -EINVAL;
			DRM_ERROR("MSVDX: ret:%d\n", ret);
			goto out;
		}

		switch (cur_cmd_id) {
		case VA_MSGID_RENDER:
			/* Fence ID */
			MEMIO_WRITE_FIELD(cmd, FW_VA_RENDER_FENCE_VALUE,
					  sequence);
			mmu_ptd = psb_get_default_pd_addr(dev_priv->mmu);
			tmp = atomic_cmpxchg(&dev_priv->msvdx_mmu_invaldc,
					1, 0);
			if (tmp == 1) {
				mmu_ptd |= 1;
				PSB_DEBUG_GENERAL("MSVDX:Set MMU invalidate\n");
			}

			/* PTD */
			MEMIO_WRITE_FIELD(cmd, FW_VA_RENDER_MMUPTD, mmu_ptd);
			break;

		default:
			/* Msg not supported */
			ret = -EINVAL;
			PSB_DEBUG_GENERAL("MSVDX: ret:%d\n", ret);
			goto out;
		}

		cmd += cur_cmd_size;
		cmd_size_remaining -= cur_cmd_size;
	}

	if (copy_cmd) {
		PSB_DEBUG_GENERAL("MSVDXQUE:copying command\n");

		tmp = kcalloc(1, cmd_size, GFP_KERNEL);
		if (tmp == NULL) {
			ret = -ENOMEM;
			DRM_ERROR("MSVDX: fail to callc,ret=:%d\n", ret);
			goto out;
		}
		memcpy(tmp, cmd_start, cmd_size);
		*msvdx_cmd = tmp;
	} else {
		PSB_DEBUG_GENERAL("MSVDXQUE:did NOT copy command\n");
		ret = psb_msvdx_send(dev, cmd_start, cmd_size);
		if (ret) {
			DRM_ERROR("MSVDXQUE: psb_msvdx_send failed\n");
			ret = -EINVAL;
		}
	}

out:
	ttm_bo_kunmap(&cmd_kmap);

	return ret;
}

int psb_submit_video_cmdbuf(struct drm_device *dev,
			    struct ttm_buffer_object *cmd_buffer,
			    unsigned long cmd_offset, unsigned long cmd_size,
			    struct ttm_fence_object *fence)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	uint32_t sequence =  dev_priv->sequence[PSB_ENGINE_VIDEO];
	unsigned long irq_flags;
	int ret = 0;

	mutex_lock(&dev_priv->msvdx_mutex);

	psb_schedule_watchdog(dev_priv);

	spin_lock_irqsave(&dev_priv->msvdx_lock, irq_flags);
	if (dev_priv->msvdx_needs_reset) {
		spin_unlock_irqrestore(&dev_priv->msvdx_lock, irq_flags);
		PSB_DEBUG_GENERAL("MSVDX: will reset msvdx\n");
		if (psb_msvdx_reset(dev_priv)) {
			mutex_unlock(&dev_priv->msvdx_mutex);
			ret = -EBUSY;
			DRM_ERROR("MSVDX: Reset failed\n");
			return ret;
		}
		dev_priv->msvdx_needs_reset = 0;
		dev_priv->msvdx_busy = 0;

		psb_msvdx_init(dev);
		psb_msvdx_irq_preinstall(dev_priv);
		psb_msvdx_irq_postinstall(dev_priv);
		spin_lock_irqsave(&dev_priv->msvdx_lock, irq_flags);
	}

	if (!dev_priv->msvdx_fw_loaded) {
		spin_unlock_irqrestore(&dev_priv->msvdx_lock, irq_flags);
		PSB_DEBUG_GENERAL("MSVDX:load /lib/firmware/msvdx_fw.bin"
				" by udevd\n");

		ret = psb_setup_fw(dev);
		if (ret) {
			mutex_unlock(&dev_priv->msvdx_mutex);

			DRM_ERROR("MSVDX:is there a /lib/firmware/msvdx_fw.bin,"
			    "and udevd is configured correctly?\n");

			/* FIXME: find a proper return value */
			return -EFAULT;
		}
		dev_priv->msvdx_fw_loaded = 1;

		psb_msvdx_irq_preinstall(dev_priv);
		psb_msvdx_irq_postinstall(dev_priv);
		PSB_DEBUG_GENERAL("MSVDX: load firmware successfully\n");
		spin_lock_irqsave(&dev_priv->msvdx_lock, irq_flags);
	}


	if (!dev_priv->msvdx_busy) {
		dev_priv->msvdx_busy = 1;
		spin_unlock_irqrestore(&dev_priv->msvdx_lock, irq_flags);
		PSB_DEBUG_GENERAL("MSVDX: commit command to HW,seq=0x%08x\n",
				sequence);
		ret = psb_msvdx_map_command(dev, cmd_buffer, cmd_offset,
					cmd_size, NULL, sequence, 0);
		if (ret) {
			mutex_unlock(&dev_priv->msvdx_mutex);
			DRM_ERROR("MSVDXQUE: Failed to extract cmd\n");
			return ret;
		}
	} else {
		struct psb_msvdx_cmd_queue *msvdx_cmd;
		void *cmd = NULL;

		spin_unlock_irqrestore(&dev_priv->msvdx_lock, irq_flags);
		/* queue the command to be sent when the h/w is ready */
		PSB_DEBUG_GENERAL("MSVDXQUE: queueing sequence:%08x..\n",
				sequence);
		msvdx_cmd = kcalloc(1, sizeof(struct psb_msvdx_cmd_queue),
				GFP_KERNEL);
		if (msvdx_cmd == NULL) {
			mutex_unlock(&dev_priv->msvdx_mutex);
			DRM_ERROR("MSVDXQUE: Out of memory...\n");
			return -ENOMEM;
		}

		ret = psb_msvdx_map_command(dev, cmd_buffer, cmd_offset,
					cmd_size, &cmd, sequence, 1);
		if (ret) {
			mutex_unlock(&dev_priv->msvdx_mutex);
			DRM_ERROR("MSVDXQUE: Failed to extract cmd\n");
			kfree(msvdx_cmd);
			return ret;
		}
		msvdx_cmd->cmd = cmd;
		msvdx_cmd->cmd_size = cmd_size;
		msvdx_cmd->sequence = sequence;
		spin_lock_irqsave(&dev_priv->msvdx_lock, irq_flags);
		list_add_tail(&msvdx_cmd->head, &dev_priv->msvdx_queue);
		if (!dev_priv->msvdx_busy) {
			dev_priv->msvdx_busy = 1;
			PSB_DEBUG_GENERAL("MSVDXQUE: Need immediate dequeue\n");
			psb_msvdx_dequeue_send(dev);
		}
		spin_unlock_irqrestore(&dev_priv->msvdx_lock, irq_flags);
	}
	mutex_unlock(&dev_priv->msvdx_mutex);
	return ret;
}

int psb_msvdx_send(struct drm_device *dev, void *cmd, unsigned long cmd_size)
{
	int ret = 0;
	struct drm_psb_private *dev_priv = dev->dev_private;

	while (cmd_size > 0) {
		uint32_t cur_cmd_size = MEMIO_READ_FIELD(cmd, FWRK_GENMSG_SIZE);
		if (cur_cmd_size > cmd_size) {
			ret = -EINVAL;
			DRM_ERROR("MSVDX:cmd_size %lu cur_cmd_size %lu\n",
				cmd_size, (unsigned long)cur_cmd_size);
			goto out;
		}
		/* Send the message to h/w */
		ret = psb_mtx_send(dev_priv, cmd);
		if (ret) {
			PSB_DEBUG_GENERAL("MSVDX: ret:%d\n", ret);
			goto out;
		}
		cmd += cur_cmd_size;
		cmd_size -= cur_cmd_size;
	}

out:
	PSB_DEBUG_GENERAL("MSVDX: ret:%d\n", ret);
	return ret;
}

int psb_mtx_send(struct drm_psb_private *dev_priv, const void *msg)
{
	static uint32_t pad_msg[FWRK_PADMSG_SIZE];
	const uint32_t *p_msg = (uint32_t *) msg;
	uint32_t msg_num, words_free, ridx, widx;
	int ret = 0;

	PSB_DEBUG_GENERAL("MSVDX: psb_mtx_send\n");

	/* we need clocks enabled before we touch VEC local ram */
	PSB_WMSVDX32(clk_enable_all, MSVDX_MAN_CLK_ENABLE);

	msg_num = (MEMIO_READ_FIELD(msg, FWRK_GENMSG_SIZE) + 3) / 4;

	if (msg_num > NUM_WORDS_MTX_BUF) {
		ret = -EINVAL;
		DRM_ERROR("MSVDX: message exceed maximum,ret:%d\n", ret);
		goto out;
	}

	ridx = PSB_RMSVDX32(MSVDX_COMMS_TO_MTX_RD_INDEX);
	widx = PSB_RMSVDX32(MSVDX_COMMS_TO_MTX_WRT_INDEX);

	/* message would wrap, need to send a pad message */
	if (widx + msg_num > NUM_WORDS_MTX_BUF) {
		/* Shouldn't happen for a PAD message itself */
		BUG_ON(MEMIO_READ_FIELD(msg, FWRK_GENMSG_ID)
			== FWRK_MSGID_PADDING);

		/* if the read pointer is at zero then we must wait for it to
		 * change otherwise the write pointer will equal the read
		 * pointer,which should only happen when the buffer is empty
		 *
		 * This will only happens if we try to overfill the queue,
		 * queue management should make
		 * sure this never happens in the first place.
		 */
		BUG_ON(0 == ridx);
		if (0 == ridx) {
			ret = -EINVAL;
			DRM_ERROR("MSVDX: RIndex=0, ret:%d\n", ret);
			goto out;
		}
		/* Send a pad message */
		MEMIO_WRITE_FIELD(pad_msg, FWRK_GENMSG_SIZE,
				  (NUM_WORDS_MTX_BUF - widx) << 2);
		MEMIO_WRITE_FIELD(pad_msg, FWRK_GENMSG_ID,
				  FWRK_MSGID_PADDING);
		psb_mtx_send(dev_priv, pad_msg);
		widx = PSB_RMSVDX32(MSVDX_COMMS_TO_MTX_WRT_INDEX);
	}

	if (widx >= ridx)
		words_free = NUM_WORDS_MTX_BUF - (widx - ridx);
	else
		words_free = ridx - widx;

	BUG_ON(msg_num > words_free);
	if (msg_num > words_free) {
		ret = -EINVAL;
		DRM_ERROR("MSVDX: msg_num > words_free, ret:%d\n", ret);
		goto out;
	}

	while (msg_num > 0) {
		PSB_WMSVDX32(*p_msg++, MSVDX_COMMS_TO_MTX_BUF + (widx << 2));
		msg_num--;
		widx++;
		if (NUM_WORDS_MTX_BUF == widx)
			widx = 0;
	}
	PSB_WMSVDX32(widx, MSVDX_COMMS_TO_MTX_WRT_INDEX);

	/* Make sure clocks are enabled before we kick */
	PSB_WMSVDX32(clk_enable_all, MSVDX_MAN_CLK_ENABLE);

	PSB_WMSVDX32(clk_enable_all, MSVDX_MAN_CLK_ENABLE);

	/* signal an interrupt to let the mtx know there is a new message */
	PSB_WMSVDX32(1, MSVDX_MTX_KICKI);

out:
	return ret;
}

/*
 * MSVDX MTX interrupt
 */
void psb_msvdx_mtx_interrupt(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *)dev->dev_private;
	static uint32_t buf[128]; /* message buffer */
	uint32_t ridx, widx;
	uint32_t num, ofs; /* message num and offset */

	PSB_DEBUG_GENERAL("MSVDX:Got a MSVDX MTX interrupt\n");

	/* Are clocks enabled  - If not enable before
	 * attempting to read from VLR
	 */
	if (PSB_RMSVDX32(MSVDX_MAN_CLK_ENABLE) != (clk_enable_all)) {
		PSB_DEBUG_GENERAL("MSVDX:Clocks disabled when Interupt set\n");
		PSB_WMSVDX32(clk_enable_all, MSVDX_MAN_CLK_ENABLE);
	}

loop: /* just for coding style check */
	ridx = PSB_RMSVDX32(MSVDX_COMMS_TO_HOST_RD_INDEX);
	widx = PSB_RMSVDX32(MSVDX_COMMS_TO_HOST_WRT_INDEX);

	/* Get out of here if nothing */
	if (ridx == widx)
		goto done;

	ofs = 0;
	buf[ofs] = PSB_RMSVDX32(MSVDX_COMMS_TO_HOST_BUF + (ridx << 2));

	/* round to nearest word */
	num = (MEMIO_READ_FIELD(buf, FWRK_GENMSG_SIZE) + 3) / 4;

	/* ASSERT(num <= sizeof(buf) / sizeof(uint32_t)); */

	if (++ridx >= NUM_WORDS_HOST_BUF)
		ridx = 0;

	for (ofs++; ofs < num; ofs++) {
		buf[ofs] = PSB_RMSVDX32(MSVDX_COMMS_TO_HOST_BUF + (ridx << 2));

		if (++ridx >= NUM_WORDS_HOST_BUF)
			ridx = 0;
	}

	/* Update the Read index */
	PSB_WMSVDX32(ridx, MSVDX_COMMS_TO_HOST_RD_INDEX);

	if (dev_priv->msvdx_needs_reset)
		goto loop;

	switch (MEMIO_READ_FIELD(buf, FWRK_GENMSG_ID)) {
	case VA_MSGID_CMD_HW_PANIC:
	case VA_MSGID_CMD_FAILED: {
		uint32_t fence = MEMIO_READ_FIELD(buf,
					FW_VA_CMD_FAILED_FENCE_VALUE);
		uint32_t fault = MEMIO_READ_FIELD(buf,
					FW_VA_CMD_FAILED_IRQSTATUS);
		uint32_t msg_id = MEMIO_READ_FIELD(buf, FWRK_GENMSG_ID);
		uint32_t diff = 0;

		if (msg_id == VA_MSGID_CMD_HW_PANIC)
			PSB_DEBUG_GENERAL("MSVDX: VA_MSGID_CMD_HW_PANIC:"
					"Fault detected"
					" - Fence: %08x, Status: %08x"
					" - resetting and ignoring error\n",
					fence, fault);
		else
			PSB_DEBUG_GENERAL("MSVDX: VA_MSGID_CMD_FAILED:"
					"Fault detected"
					" - Fence: %08x, Status: %08x"
					" - resetting and ignoring error\n",
					fence, fault);

		dev_priv->msvdx_needs_reset = 1;

		if (msg_id == VA_MSGID_CMD_HW_PANIC) {
			diff = dev_priv->msvdx_current_sequence
				- dev_priv->sequence[PSB_ENGINE_VIDEO];

			if (diff > 0x0FFFFFFF)
				dev_priv->msvdx_current_sequence++;

			PSB_DEBUG_GENERAL("MSVDX: Fence ID missing, "
					"assuming %08x\n",
					dev_priv->msvdx_current_sequence);
		} else {
			dev_priv->msvdx_current_sequence = fence;
		}

		psb_fence_error(dev, PSB_ENGINE_VIDEO,
				dev_priv->msvdx_current_sequence,
				_PSB_FENCE_TYPE_EXE, DRM_CMD_FAILED);

		/* Flush the command queue */
		psb_msvdx_flush_cmd_queue(dev);

		goto done;
	}
	case VA_MSGID_CMD_COMPLETED: {
		uint32_t fence = MEMIO_READ_FIELD(buf,
					FW_VA_CMD_COMPLETED_FENCE_VALUE);
		uint32_t flags = MEMIO_READ_FIELD(buf,
						FW_VA_CMD_COMPLETED_FLAGS);

		PSB_DEBUG_GENERAL("MSVDX:VA_MSGID_CMD_COMPLETED: "
				"FenceID: %08x, flags: 0x%x\n",
				fence, flags);

		dev_priv->msvdx_current_sequence = fence;

		psb_fence_handler(dev, PSB_ENGINE_VIDEO);

		if (flags & FW_VA_RENDER_HOST_INT) {
			/*Now send the next command from the msvdx cmd queue */
			psb_msvdx_dequeue_send(dev);
			goto done;
		}

		break;
	}
	case VA_MSGID_CMD_COMPLETED_BATCH: {
		uint32_t fence = MEMIO_READ_FIELD(buf,
					FW_VA_CMD_COMPLETED_FENCE_VALUE);
		uint32_t tickcnt = MEMIO_READ_FIELD(buf,
					FW_VA_CMD_COMPLETED_NO_TICKS);

		/* we have the fence value in the message */
		PSB_DEBUG_GENERAL("MSVDX:VA_MSGID_CMD_COMPLETED_BATCH:"
			" FenceID: %08x, TickCount: %08x\n",
			fence, tickcnt);
		dev_priv->msvdx_current_sequence = fence;

		break;
	}
	case VA_MSGID_ACK:
		PSB_DEBUG_GENERAL("MSVDX: VA_MSGID_ACK\n");
		break;

	case VA_MSGID_TEST1:
		PSB_DEBUG_GENERAL("MSVDX: VA_MSGID_TEST1\n");
		break;

	case VA_MSGID_TEST2:
		PSB_DEBUG_GENERAL("MSVDX: VA_MSGID_TEST2\n");
		break;
		/* Don't need to do anything with these messages */

	case VA_MSGID_DEBLOCK_REQUIRED:	{
		uint32_t ctxid = MEMIO_READ_FIELD(buf,
					FW_VA_DEBLOCK_REQUIRED_CONTEXT);

		/* The BE we now be locked. */
		/* Unblock rendec by reading the mtx2mtx end of slice */
		(void) PSB_RMSVDX32(MSVDX_RENDEC_READ_DATA);

		PSB_DEBUG_GENERAL("MSVDX: VA_MSGID_DEBLOCK_REQUIRED"
				" Context=%08x\n", ctxid);
		goto done;
	}
	default:
		DRM_ERROR("ERROR: msvdx Unknown message from MTX \n");
		goto done;
	}

done:

#if 1
	if (!dev_priv->msvdx_busy) {
		/* If the firmware says the hardware is idle
		 * and the CCB is empty then we can power down
		 */
		uint32_t fs_status = PSB_RMSVDX32(MSVDX_COMMS_FW_STATUS);
		uint32_t ccb_roff = PSB_RMSVDX32(MSVDX_COMMS_TO_MTX_RD_INDEX);
		uint32_t ccb_woff = PSB_RMSVDX32(MSVDX_COMMS_TO_MTX_WRT_INDEX);

		/* check that clocks are enabled before reading VLR */
		if (PSB_RMSVDX32(MSVDX_MAN_CLK_ENABLE) != (clk_enable_all))
			PSB_WMSVDX32(clk_enable_all, MSVDX_MAN_CLK_ENABLE);

		if ((fs_status & MSVDX_FW_STATUS_HW_IDLE) &&
			(ccb_roff == ccb_woff)) {
			PSB_DEBUG_GENERAL("MSVDX: Setting clock to minimal\n");
			PSB_WMSVDX32(clk_enable_minimal, MSVDX_MAN_CLK_ENABLE);
		}
	}
#endif
	DRM_MEMORYBARRIER();	/* TBD check this... */
}

void psb_msvdx_lockup(struct drm_psb_private *dev_priv,
		int *msvdx_lockup, int *msvdx_idle)
{
	int tmp;
	*msvdx_lockup = 0;
	*msvdx_idle = 1;

	if (!dev_priv->has_msvdx)
		return;
#if 0
	PSB_DEBUG_GENERAL("MSVDXTimer: current_sequence:%d "
			"last_sequence:%d and last_submitted_sequence :%d\n",
			dev_priv->msvdx_current_sequence,
			dev_priv->msvdx_last_sequence,
			dev_priv->sequence[PSB_ENGINE_VIDEO]);
#endif

	tmp = dev_priv->msvdx_current_sequence -
		dev_priv->sequence[PSB_ENGINE_VIDEO];

	if (tmp > 0x0FFFFFFF) {
		if (dev_priv->msvdx_current_sequence ==
			dev_priv->msvdx_last_sequence) {
			DRM_ERROR("MSVDXTimer:locked-up for sequence:%d\n",
				dev_priv->msvdx_current_sequence);
			*msvdx_lockup = 1;
		} else {
			PSB_DEBUG_GENERAL("MSVDXTimer: "
					"msvdx responded fine so far\n");
			dev_priv->msvdx_last_sequence =
				dev_priv->msvdx_current_sequence;
			*msvdx_idle = 0;
		}
	}
}

/* power up msvdx, OSPM function */
int psb_power_up_msvdx(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *)dev->dev_private;
	int ret;

	if ((dev_priv->msvdx_state & PSB_PWR_STATE_MASK) != PSB_PWR_STATE_D0i3)
		return -EINVAL;

	PSB_DEBUG_TMP("power up msvdx\n");
	dump_stack();

	psb_up_island_power(dev, PSB_VIDEO_DEC_ISLAND);

	ret = psb_msvdx_init(dev);
	if (ret) {
		DRM_ERROR("failed to init msvdx when power up it\n");
		goto err;
	}
	PSB_WMSVDX32(dev_priv->msvdx_clk_state, MSVDX_MAN_CLK_ENABLE);

	PSB_DEBUG_GENERAL("FIXME restore registers or init msvdx\n");

	PSB_DEBUG_GENERAL("FIXME MSVDX MMU setting up\n");

	dev_priv->msvdx_state = PSB_PWR_STATE_D0i0;
	return 0;

err:
	return -1;
}

int psb_power_down_msvdx(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *)dev->dev_private;

	if ((dev_priv->msvdx_state & PSB_PWR_STATE_MASK) != PSB_PWR_STATE_D0i0)
		return -EINVAL;
	if (dev_priv->msvdx_busy) {
		PSB_DEBUG_GENERAL("FIXME: MSVDX is busy, should wait it\n");
		return -EBUSY;
	}

	dev_priv->msvdx_clk_state = PSB_RMSVDX32(MSVDX_MAN_CLK_ENABLE);
	PSB_DEBUG_GENERAL("FIXME: save MSVDX register\n");

	PSB_DEBUG_GENERAL("FIXME: save MSVDX context\n");
	psb_down_island_power(dev, PSB_VIDEO_DEC_ISLAND);

	dev_priv->msvdx_state = PSB_PWR_STATE_D0i3;

	return 0;
}
