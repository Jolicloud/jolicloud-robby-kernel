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

#include "drmP.h"
#include "drm_os_linux.h"
#include "psb_drv.h"
#include "psb_drm.h"
#include "psb_msvdx.h"

#include <asm/io.h>
#include <linux/delay.h>

#ifndef list_first_entry
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)
#endif

static int psb_msvdx_send (struct drm_device *dev, void *cmd,
			   unsigned long cmd_size);

int
psb_msvdx_dequeue_send (struct drm_device *dev)
{
  struct drm_psb_private *dev_priv = dev->dev_private;
  struct psb_msvdx_cmd_queue *msvdx_cmd = NULL;
  int ret = 0;

  if (list_empty (&dev_priv->msvdx_queue))
    {
      PSB_DEBUG_GENERAL ("MSVDXQUE: msvdx list empty.\n");
      dev_priv->msvdx_busy = 0;
      return -EINVAL;
    }
  msvdx_cmd =
    list_first_entry (&dev_priv->msvdx_queue, struct psb_msvdx_cmd_queue,
		      head);
  PSB_DEBUG_GENERAL ("MSVDXQUE: Queue has id %08x\n", msvdx_cmd->sequence);
  ret = psb_msvdx_send (dev, msvdx_cmd->cmd, msvdx_cmd->cmd_size);
  if (ret)
    {
      PSB_DEBUG_GENERAL ("MSVDXQUE: psb_msvdx_send failed\n");
      ret = -EINVAL;
    }
  list_del (&msvdx_cmd->head);
  kfree (msvdx_cmd->cmd);
  drm_free (msvdx_cmd, sizeof (struct psb_msvdx_cmd_queue), DRM_MEM_DRIVER);
  return ret;
}

int
psb_msvdx_map_command (struct drm_device *dev,
		       struct drm_buffer_object *cmd_buffer,
		       unsigned long cmd_offset, unsigned long cmd_size,
		       void **msvdx_cmd, uint32_t sequence, int copy_cmd)
{
  struct drm_psb_private *dev_priv = dev->dev_private;
  int ret = 0;
  unsigned long cmd_page_offset = cmd_offset & ~PAGE_MASK;
  unsigned long cmd_size_remaining;
  struct drm_bo_kmap_obj cmd_kmap;
  void *cmd, *tmp, *cmd_start;
  int is_iomem;

  /* command buffers may not exceed page boundary */
  if (cmd_size + cmd_page_offset > PAGE_SIZE)
    return -EINVAL;

  ret = drm_bo_kmap (cmd_buffer, cmd_offset >> PAGE_SHIFT, 2, &cmd_kmap);

  if (ret)
    {
      PSB_DEBUG_GENERAL ("MSVDXQUE:ret:%d\n", ret);
      return ret;
    }

  cmd_start =
    (void *) drm_bmo_virtual (&cmd_kmap, &is_iomem) + cmd_page_offset;
  cmd = cmd_start;
  cmd_size_remaining = cmd_size;

  while (cmd_size_remaining > 0)
    {
      uint32_t mmu_ptd;
      uint32_t cur_cmd_size = MEMIO_READ_FIELD (cmd, FWRK_GENMSG_SIZE);
      uint32_t cur_cmd_id = MEMIO_READ_FIELD (cmd, FWRK_GENMSG_ID);
      PSB_DEBUG_GENERAL
	("cmd start at %08x cur_cmd_size = %d cur_cmd_id = %02x fence = %08x\n",
	 (uint32_t) cmd, cur_cmd_size, cur_cmd_id, sequence);
      if ((cur_cmd_size % sizeof (uint32_t))
	  || (cur_cmd_size > cmd_size_remaining))
	{
	  ret = -EINVAL;
	  PSB_DEBUG_GENERAL ("MSVDX: ret:%d\n", ret);
	  goto out;
	}

      switch (cur_cmd_id)
	{
	case VA_MSGID_RENDER:
	  /* Fence ID */
	  MEMIO_WRITE_FIELD (cmd, FW_VA_RENDER_FENCE_VALUE, sequence);

	  mmu_ptd = psb_get_default_pd_addr (dev_priv->mmu);
          if (atomic_cmpxchg(&dev_priv->msvdx_mmu_invaldc, 1, 0) == 1)
            {
                mmu_ptd |= 1;
                PSB_DEBUG_GENERAL ("MSVDX: Setting MMU invalidate flag\n");
	    }  
	  /* PTD */
	  MEMIO_WRITE_FIELD (cmd, FW_VA_RENDER_MMUPTD, mmu_ptd);
	  break;

	default:
	  /* Msg not supported */
	  ret = -EINVAL;
	  PSB_DEBUG_GENERAL ("MSVDX: ret:%d\n", ret);
	  goto out;
	}

      cmd += cur_cmd_size;
      cmd_size_remaining -= cur_cmd_size;
    }

  if (copy_cmd)
    {
      PSB_DEBUG_GENERAL
	("MSVDXQUE: psb_msvdx_map_command copying command...\n");
      tmp = drm_calloc (1, cmd_size, DRM_MEM_DRIVER);
      if (tmp == NULL)
	{
	  ret = -ENOMEM;
	  PSB_DEBUG_GENERAL ("MSVDX: ret:%d\n", ret);
	  goto out;
	}
      memcpy (tmp, cmd_start, cmd_size);
      *msvdx_cmd = tmp;
    }
  else
    {
      PSB_DEBUG_GENERAL
	("MSVDXQUE: psb_msvdx_map_command did NOT copy command...\n");
      ret = psb_msvdx_send (dev, cmd_start, cmd_size);
      if (ret)
	{
	  PSB_DEBUG_GENERAL ("MSVDXQUE: psb_msvdx_send failed\n");
	  ret = -EINVAL;
	}
    }

out:
  drm_bo_kunmap (&cmd_kmap);

  return ret;
}

int
psb_submit_video_cmdbuf (struct drm_device *dev,
			 struct drm_buffer_object *cmd_buffer,
			 unsigned long cmd_offset, unsigned long cmd_size,
			 struct drm_fence_object *fence)
{
  struct drm_psb_private *dev_priv = dev->dev_private;
  uint32_t sequence = fence->sequence;
  unsigned long irq_flags;
  int ret = 0;

  mutex_lock (&dev_priv->msvdx_mutex);
  psb_schedule_watchdog (dev_priv);

  spin_lock_irqsave (&dev_priv->msvdx_lock, irq_flags);
  dev_priv->msvdx_power_saving = 0;

  if (dev_priv->msvdx_needs_reset)
    {
      spin_unlock_irqrestore (&dev_priv->msvdx_lock, irq_flags);
      PSB_DEBUG_GENERAL ("MSVDX: Needs reset\n");
      if (psb_msvdx_reset (dev_priv))
	{
	  mutex_unlock (&dev_priv->msvdx_mutex);
	  ret = -EBUSY;
	  PSB_DEBUG_GENERAL ("MSVDX: Reset failed\n");
	  return ret;
	}
      PSB_DEBUG_GENERAL ("MSVDX: Reset ok\n");
      dev_priv->msvdx_needs_reset = 0;
      dev_priv->msvdx_busy = 0;
      dev_priv->msvdx_start_idle = 0;

      psb_msvdx_init (dev);
      psb_msvdx_irq_preinstall (dev_priv);
      psb_msvdx_irq_postinstall (dev_priv);
      PSB_DEBUG_GENERAL ("MSVDX: Init ok\n");
      spin_lock_irqsave (&dev_priv->msvdx_lock, irq_flags);
    }

  if (!dev_priv->msvdx_busy)
    {
      dev_priv->msvdx_busy = 1;
      spin_unlock_irqrestore (&dev_priv->msvdx_lock, irq_flags);
      PSB_DEBUG_GENERAL
	("MSVDXQUE: nothing in the queue sending sequence:%08x..\n",
	 sequence);
      ret =
	psb_msvdx_map_command (dev, cmd_buffer, cmd_offset, cmd_size,
			       NULL, sequence, 0);
      if (ret)
	{
	  mutex_unlock (&dev_priv->msvdx_mutex);
	  PSB_DEBUG_GENERAL ("MSVDXQUE: Failed to extract cmd...\n");
	  return ret;
	}
    }
  else
    {
      struct psb_msvdx_cmd_queue *msvdx_cmd;
      void *cmd = NULL;

      spin_unlock_irqrestore (&dev_priv->msvdx_lock, irq_flags);
      /*queue the command to be sent when the h/w is ready */
      PSB_DEBUG_GENERAL ("MSVDXQUE: queueing sequence:%08x..\n", sequence);
      msvdx_cmd =
	drm_calloc (1, sizeof (struct psb_msvdx_cmd_queue), DRM_MEM_DRIVER);
      if (msvdx_cmd == NULL)
	{
	  mutex_unlock (&dev_priv->msvdx_mutex);
	  PSB_DEBUG_GENERAL ("MSVDXQUE: Out of memory...\n");
	  return -ENOMEM;
	}

      ret =
	psb_msvdx_map_command (dev, cmd_buffer, cmd_offset, cmd_size,
			       &cmd, sequence, 1);
      if (ret)
	{
	  mutex_unlock (&dev_priv->msvdx_mutex);
	  PSB_DEBUG_GENERAL ("MSVDXQUE: Failed to extract cmd...\n");
	  drm_free (msvdx_cmd, sizeof (struct psb_msvdx_cmd_queue),
		    DRM_MEM_DRIVER);
	  return ret;
	}
      msvdx_cmd->cmd = cmd;
      msvdx_cmd->cmd_size = cmd_size;
      msvdx_cmd->sequence = sequence;
      spin_lock_irqsave (&dev_priv->msvdx_lock, irq_flags);
      list_add_tail (&msvdx_cmd->head, &dev_priv->msvdx_queue);
      if (!dev_priv->msvdx_busy)
	{
	  dev_priv->msvdx_busy = 1;
	  PSB_DEBUG_GENERAL ("MSVDXQUE: Need immediate dequeue\n");
	  psb_msvdx_dequeue_send (dev);
	}
      spin_unlock_irqrestore (&dev_priv->msvdx_lock, irq_flags);
    }
  mutex_unlock (&dev_priv->msvdx_mutex);
  return ret;
}

int
psb_msvdx_send (struct drm_device *dev, void *cmd, unsigned long cmd_size)
{
  int ret = 0;
  struct drm_psb_private *dev_priv = dev->dev_private;

  while (cmd_size > 0)
    {
      uint32_t cur_cmd_size = MEMIO_READ_FIELD (cmd, FWRK_GENMSG_SIZE);
      if (cur_cmd_size > cmd_size)
	{
	  ret = -EINVAL;
	  PSB_DEBUG_GENERAL
	    ("MSVDX: cmd_size = %d cur_cmd_size = %d\n",
	     (int) cmd_size, cur_cmd_size);
	  goto out;
	}
      /* Send the message to h/w */
      ret = psb_mtx_send (dev_priv, cmd);
      if (ret)
	{
	  PSB_DEBUG_GENERAL ("MSVDX: ret:%d\n", ret);
	  goto out;
	}
      cmd += cur_cmd_size;
      cmd_size -= cur_cmd_size;
    }

out:
  PSB_DEBUG_GENERAL ("MSVDX: ret:%d\n", ret);
  return ret;
}

/***********************************************************************************
 * Function Name      : psb_mtx_send
 * Inputs             :
 * Outputs            :
 * Returns            :
 * Description        :
 ************************************************************************************/
int
psb_mtx_send (struct drm_psb_private *dev_priv, const void *pvMsg)
{

  static uint32_t padMessage[FWRK_PADMSG_SIZE];

  const uint32_t *pui32Msg = (uint32_t *) pvMsg;
  uint32_t msgNumWords, wordsFree, readIndex, writeIndex;
  int ret = 0;

  PSB_DEBUG_GENERAL ("MSVDX: psb_mtx_send\n");

  /* we need clocks enabled before we touch VEC local ram */
  PSB_WMSVDX32 (clk_enable_all, MSVDX_MAN_CLK_ENABLE);

  msgNumWords = (MEMIO_READ_FIELD (pvMsg, FWRK_GENMSG_SIZE) + 3) / 4;

  if (msgNumWords > NUM_WORDS_MTX_BUF)
    {
      ret = -EINVAL;
      PSB_DEBUG_GENERAL ("MSVDX: ret:%d\n", ret);
      goto out;
    }

  readIndex = PSB_RMSVDX32 (MSVDX_COMMS_TO_MTX_RD_INDEX);
  writeIndex = PSB_RMSVDX32 (MSVDX_COMMS_TO_MTX_WRT_INDEX);

  if (writeIndex + msgNumWords > NUM_WORDS_MTX_BUF)
    {				/* message would wrap, need to send a pad message */
      BUG_ON (MEMIO_READ_FIELD (pvMsg, FWRK_GENMSG_ID) == FWRK_MSGID_PADDING);	/* Shouldn't happen for a PAD message itself */
      /* if the read pointer is at zero then we must wait for it to change otherwise the write
       * pointer will equal the read pointer,which should only happen when the buffer is empty
       *
       * This will only happens if we try to overfill the queue, queue management should make
       * sure this never happens in the first place.
       */
      BUG_ON (0 == readIndex);
      if (0 == readIndex)
	{
	  ret = -EINVAL;
	  PSB_DEBUG_GENERAL ("MSVDX: ret:%d\n", ret);
	  goto out;
	}
      /* Send a pad message */
      MEMIO_WRITE_FIELD (padMessage, FWRK_GENMSG_SIZE,
			 (NUM_WORDS_MTX_BUF - writeIndex) << 2);
      MEMIO_WRITE_FIELD (padMessage, FWRK_GENMSG_ID, FWRK_MSGID_PADDING);
      psb_mtx_send (dev_priv, padMessage);
      writeIndex = PSB_RMSVDX32 (MSVDX_COMMS_TO_MTX_WRT_INDEX);
    }

  wordsFree =
    (writeIndex >=
     readIndex) ? NUM_WORDS_MTX_BUF - (writeIndex -
				       readIndex) : readIndex - writeIndex;

  BUG_ON (msgNumWords > wordsFree);
  if (msgNumWords > wordsFree)
    {
      ret = -EINVAL;
      PSB_DEBUG_GENERAL ("MSVDX: ret:%d\n", ret);
      goto out;
    }

  while (msgNumWords > 0)
    {
      PSB_WMSVDX32 (*pui32Msg++, MSVDX_COMMS_TO_MTX_BUF + (writeIndex << 2));
      msgNumWords--;
      writeIndex++;
      if (NUM_WORDS_MTX_BUF == writeIndex)
	{
	  writeIndex = 0;
	}
    }
  PSB_WMSVDX32 (writeIndex, MSVDX_COMMS_TO_MTX_WRT_INDEX);

  /* Make sure clocks are enabled before we kick */
  PSB_WMSVDX32 (clk_enable_all, MSVDX_MAN_CLK_ENABLE);

  /* signal an interrupt to let the mtx know there is a new message */
  PSB_WMSVDX32 (1, MSVDX_MTX_KICKI);

out:
  return ret;
}

/*
 * MSVDX MTX interrupt
 */
void
psb_msvdx_mtx_interrupt (struct drm_device *dev)
{
  static uint32_t msgBuffer[128];
  uint32_t readIndex, writeIndex;
  uint32_t msgNumWords, msgWordOffset;
  struct drm_psb_private *dev_priv =
    (struct drm_psb_private *) dev->dev_private;

  /* Are clocks enabled  - If not enable before attempting to read from VLR */
  if (PSB_RMSVDX32 (MSVDX_MAN_CLK_ENABLE) != (clk_enable_all))
    {
      PSB_DEBUG_GENERAL
	("MSVDX: Warning - Clocks disabled when Interupt set\n");
      PSB_WMSVDX32 (clk_enable_all, MSVDX_MAN_CLK_ENABLE);
    }

  for (;;)
    {
      readIndex = PSB_RMSVDX32 (MSVDX_COMMS_TO_HOST_RD_INDEX);
      writeIndex = PSB_RMSVDX32 (MSVDX_COMMS_TO_HOST_WRT_INDEX);

      if (readIndex != writeIndex)
	{
	  msgWordOffset = 0;

	  msgBuffer[msgWordOffset] =
	    PSB_RMSVDX32 (MSVDX_COMMS_TO_HOST_BUF + (readIndex << 2));

	  msgNumWords = (MEMIO_READ_FIELD (msgBuffer, FWRK_GENMSG_SIZE) + 3) / 4;	/* round to nearest word */

	  /*ASSERT(msgNumWords <= sizeof(msgBuffer) / sizeof(uint32_t)); */

	  if (++readIndex >= NUM_WORDS_HOST_BUF)
	    readIndex = 0;

	  for (msgWordOffset++; msgWordOffset < msgNumWords; msgWordOffset++)
	    {
	      msgBuffer[msgWordOffset] =
		PSB_RMSVDX32 (MSVDX_COMMS_TO_HOST_BUF + (readIndex << 2));

	      if (++readIndex >= NUM_WORDS_HOST_BUF)
		{
		  readIndex = 0;
		}
	    }

	  /* Update the Read index */
	  PSB_WMSVDX32 (readIndex, MSVDX_COMMS_TO_HOST_RD_INDEX);

	  if (!dev_priv->msvdx_needs_reset)
	    switch (MEMIO_READ_FIELD (msgBuffer, FWRK_GENMSG_ID))
	      {
	      case VA_MSGID_CMD_HW_PANIC:
	      case VA_MSGID_CMD_FAILED:
		{
		  uint32_t ui32Fence = MEMIO_READ_FIELD (msgBuffer,
							 FW_VA_CMD_FAILED_FENCE_VALUE);
		  uint32_t ui32FaultStatus = MEMIO_READ_FIELD (msgBuffer,
							       FW_VA_CMD_FAILED_IRQSTATUS);

		 if(MEMIO_READ_FIELD (msgBuffer, FWRK_GENMSG_ID) == VA_MSGID_CMD_HW_PANIC )
		  PSB_DEBUG_GENERAL
		    ("MSVDX: VA_MSGID_CMD_HW_PANIC: Msvdx fault detected - Fence: %08x, Status: %08x - resetting and ignoring error\n",
		     ui32Fence, ui32FaultStatus);
		 else
		  PSB_DEBUG_GENERAL
		    ("MSVDX: VA_MSGID_CMD_FAILED: Msvdx fault detected - Fence: %08x, Status: %08x - resetting and ignoring error\n",
		     ui32Fence, ui32FaultStatus);

		  dev_priv->msvdx_needs_reset = 1;

		 if(MEMIO_READ_FIELD (msgBuffer, FWRK_GENMSG_ID) == VA_MSGID_CMD_HW_PANIC)
                   {
                     if (dev_priv->
                         msvdx_current_sequence
                         - dev_priv->sequence[PSB_ENGINE_VIDEO] > 0x0FFFFFFF)
                       dev_priv->msvdx_current_sequence++;
                     PSB_DEBUG_GENERAL
                       ("MSVDX: Fence ID missing, assuming %08x\n",
                        dev_priv->msvdx_current_sequence);
                   }
		 else
	           dev_priv->msvdx_current_sequence = ui32Fence;

		  psb_fence_error (dev,
				   PSB_ENGINE_VIDEO,
				   dev_priv->
				   msvdx_current_sequence,
				   DRM_FENCE_TYPE_EXE, DRM_CMD_FAILED);

		  /* Flush the command queue */
		  psb_msvdx_flush_cmd_queue (dev);

		  goto isrExit;
		  break;
		}
	      case VA_MSGID_CMD_COMPLETED:
		{
		  uint32_t ui32Fence = MEMIO_READ_FIELD (msgBuffer,
							 FW_VA_CMD_COMPLETED_FENCE_VALUE);
		  uint32_t ui32Flags =
		    MEMIO_READ_FIELD (msgBuffer, FW_VA_CMD_COMPLETED_FLAGS);

		  PSB_DEBUG_GENERAL
		    ("msvdx VA_MSGID_CMD_COMPLETED: FenceID: %08x, flags: 0x%x\n",
		     ui32Fence, ui32Flags);
		  dev_priv->msvdx_current_sequence = ui32Fence;

		  psb_fence_handler (dev, PSB_ENGINE_VIDEO);


		  if (ui32Flags & FW_VA_RENDER_HOST_INT)
		    {
		      /*Now send the next command from the msvdx cmd queue */
		      psb_msvdx_dequeue_send (dev);
		      goto isrExit;
		    }
		  break;
		}
	      case VA_MSGID_ACK:
		PSB_DEBUG_GENERAL ("msvdx VA_MSGID_ACK\n");
		break;

	      case VA_MSGID_TEST1:
		PSB_DEBUG_GENERAL ("msvdx VA_MSGID_TEST1\n");
		break;

	      case VA_MSGID_TEST2:
		PSB_DEBUG_GENERAL ("msvdx VA_MSGID_TEST2\n");
		break;
		/* Don't need to do anything with these messages */

	      case VA_MSGID_DEBLOCK_REQUIRED:
		{
		  uint32_t ui32ContextId = MEMIO_READ_FIELD (msgBuffer,
							     FW_VA_DEBLOCK_REQUIRED_CONTEXT);

		  /* The BE we now be locked. */

		  /* Unblock rendec by reading the mtx2mtx end of slice */
		  (void) PSB_RMSVDX32 (MSVDX_RENDEC_READ_DATA);

		  PSB_DEBUG_GENERAL
		    ("msvdx VA_MSGID_DEBLOCK_REQUIRED Context=%08x\n",
		     ui32ContextId);
		  goto isrExit;
		  break;
		}

	      default:
		{
		  PSB_DEBUG_GENERAL
		    ("ERROR: msvdx Unknown message from MTX \n");
		}
		break;

	      }
	}
      else
	{
	  /* Get out of here if nothing */
	  break;
	}
    }
isrExit:

#if 1
  if (!dev_priv->msvdx_busy)
  {
    /* check that clocks are enabled before reading VLR */
    if( PSB_RMSVDX32( MSVDX_MAN_CLK_ENABLE ) != (clk_enable_all) )
        PSB_WMSVDX32 (clk_enable_all, MSVDX_MAN_CLK_ENABLE);

   /* If the firmware says the hardware is idle and the CCB is empty then we can power down */
   uint32_t ui32FWStatus = PSB_RMSVDX32( MSVDX_COMMS_FW_STATUS );
   uint32_t ui32CCBRoff = PSB_RMSVDX32 ( MSVDX_COMMS_TO_MTX_RD_INDEX );
   uint32_t ui32CCBWoff = PSB_RMSVDX32 ( MSVDX_COMMS_TO_MTX_WRT_INDEX );

   if( (ui32FWStatus & MSVDX_FW_STATUS_HW_IDLE) && (ui32CCBRoff == ui32CCBWoff))
   {
	PSB_DEBUG_GENERAL("MSVDX_CLOCK: Setting clock to minimal...\n");
        PSB_WMSVDX32 (clk_enable_minimal, MSVDX_MAN_CLK_ENABLE);
   }
   }
#endif
  DRM_MEMORYBARRIER ();
}

void
psb_msvdx_lockup (struct drm_psb_private *dev_priv,
		  int *msvdx_lockup, int *msvdx_idle)
{
	unsigned long irq_flags;
//	struct psb_scheduler *scheduler = &dev_priv->scheduler;

  spin_lock_irqsave (&dev_priv->msvdx_lock, irq_flags);
  *msvdx_lockup = 0;
  *msvdx_idle = 1;

  if (!dev_priv->has_msvdx)
  {
      spin_unlock_irqrestore (&dev_priv->msvdx_lock, irq_flags);
      return;
  }
#if 0
  PSB_DEBUG_GENERAL ("MSVDXTimer: current_sequence:%d "
		     "last_sequence:%d and last_submitted_sequence :%d\n",
		     dev_priv->msvdx_current_sequence,
		     dev_priv->msvdx_last_sequence,
		     dev_priv->sequence[PSB_ENGINE_VIDEO]);
#endif
  if (dev_priv->msvdx_current_sequence -
      dev_priv->sequence[PSB_ENGINE_VIDEO] > 0x0FFFFFFF)
    {

      if (dev_priv->msvdx_current_sequence == dev_priv->msvdx_last_sequence)
	{
	  PSB_DEBUG_GENERAL
	    ("MSVDXTimer: msvdx locked-up for sequence:%d\n",
	     dev_priv->msvdx_current_sequence);
	  *msvdx_lockup = 1;
	}
      else
	{
	  PSB_DEBUG_GENERAL ("MSVDXTimer: msvdx responded fine so far...\n");
	  dev_priv->msvdx_last_sequence = dev_priv->msvdx_current_sequence;
	  *msvdx_idle = 0;
	}
	if (dev_priv->msvdx_start_idle)
		dev_priv->msvdx_start_idle = 0;
    } 
    else 
    {
	//if (dev_priv->msvdx_needs_reset == 0)
	if (dev_priv->msvdx_power_saving == 0)
	{
	    if (dev_priv->msvdx_start_idle && (dev_priv->msvdx_finished_sequence == dev_priv->msvdx_current_sequence))
	    {
		//if (dev_priv->msvdx_idle_start_jiffies + MSVDX_MAX_IDELTIME >= jiffies)
		if (time_after_eq(jiffies, dev_priv->msvdx_idle_start_jiffies + MSVDX_MAX_IDELTIME))
		{
		    printk("set the msvdx clock to 0 in the %s\n", __FUNCTION__);
      		    PSB_WMSVDX32 (0, MSVDX_MAN_CLK_ENABLE);
		    // MSVDX needn't to be reset for the latter commands after pausing and resuming playing.
		    //dev_priv->msvdx_needs_reset = 1;
		    dev_priv->msvdx_power_saving = 1;
		}
		else
		{
		    *msvdx_idle = 0;
		}
	    }
	    else
	    {
		dev_priv->msvdx_start_idle = 1;
		dev_priv->msvdx_idle_start_jiffies = jiffies;
		dev_priv->msvdx_finished_sequence = dev_priv->msvdx_current_sequence;
		*msvdx_idle = 0;
	    }
	}
    }
    spin_unlock_irqrestore (&dev_priv->msvdx_lock, irq_flags);
}
