/**
 * file psb_msvdxinit.c
 * MSVDX initialization and mtx-firmware upload
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
#include "drm.h"
#include "psb_drv.h"
#include "psb_msvdx.h"
#include <linux/firmware.h>

/*MSVDX FW header*/
struct msvdx_fw
{
  uint32_t ver;
  uint32_t text_size;
  uint32_t data_size;
  uint32_t data_location;
};

int
psb_wait_for_register (struct drm_psb_private *dev_priv,
		       uint32_t ui32Offset,
		       uint32_t ui32Value, uint32_t ui32Enable)
{
  uint32_t ui32Temp;
  uint32_t ui32PollCount = 1000;
  while (ui32PollCount)
    {
      ui32Temp = PSB_RMSVDX32 (ui32Offset);
      if (ui32Value == (ui32Temp & ui32Enable))	/* All the bits are reset   */
	return 0;		/* So exit                      */

      /* Wait a bit */
      DRM_UDELAY (100);
      ui32PollCount--;
    }
  PSB_DEBUG_GENERAL
    ("MSVDX: Timeout while waiting for register %08x: expecting %08x (mask %08x), got %08x\n",
     ui32Offset, ui32Value, ui32Enable, ui32Temp);
  return 1;
}

int
psb_poll_mtx_irq (struct drm_psb_private *dev_priv)
{
  int ret = 0;
  uint32_t MtxInt = 0;
  REGIO_WRITE_FIELD_LITE (MtxInt, MSVDX_INTERRUPT_STATUS, CR_MTX_IRQ, 1);

  ret = psb_wait_for_register (dev_priv, MSVDX_INTERRUPT_STATUS, MtxInt,	/* Required value */
			       MtxInt /* Enabled bits */ );
  if (ret)
    {
      PSB_DEBUG_GENERAL
	("MSVDX: Error Mtx did not return int within a resonable time\n");

      return ret;
    }

  PSB_DEBUG_GENERAL ("MSVDX: Got MTX Int\n");

  /* Got it so clear the bit */
  PSB_WMSVDX32 (MtxInt, MSVDX_INTERRUPT_CLEAR);

  return ret;
}

void
psb_write_mtx_core_reg (struct drm_psb_private *dev_priv,
			const uint32_t ui32CoreRegister,
			const uint32_t ui32Val)
{
  uint32_t ui32Reg = 0;

  /* Put data in MTX_RW_DATA */
  PSB_WMSVDX32 (ui32Val, MSVDX_MTX_REGISTER_READ_WRITE_DATA);

  /* DREADY is set to 0 and request a write */
  ui32Reg = ui32CoreRegister;
  REGIO_WRITE_FIELD_LITE (ui32Reg, MSVDX_MTX_REGISTER_READ_WRITE_REQUEST,
			  MTX_RNW, 0);
  REGIO_WRITE_FIELD_LITE (ui32Reg, MSVDX_MTX_REGISTER_READ_WRITE_REQUEST,
			  MTX_DREADY, 0);
  PSB_WMSVDX32 (ui32Reg, MSVDX_MTX_REGISTER_READ_WRITE_REQUEST);

  psb_wait_for_register (dev_priv, MSVDX_MTX_REGISTER_READ_WRITE_REQUEST, MSVDX_MTX_REGISTER_READ_WRITE_REQUEST_MTX_DREADY_MASK,	/* Required Value */
			 MSVDX_MTX_REGISTER_READ_WRITE_REQUEST_MTX_DREADY_MASK);
}

void
psb_upload_fw (struct drm_psb_private *dev_priv, const uint32_t ui32DataMem,
	       uint32_t ui32RamBankSize, uint32_t ui32Address,
	       const unsigned int uiWords, const uint32_t * const pui32Data)
{
  uint32_t ui32Loop, ui32Ctrl, ui32RamId, ui32Addr, ui32CurrBank =
    (uint32_t) ~ 0;
  uint32_t ui32AccessControl;

  /* Save the access control register... */
  ui32AccessControl = PSB_RMSVDX32 (MSVDX_MTX_RAM_ACCESS_CONTROL);

  /* Wait for MCMSTAT to become be idle 1 */
  psb_wait_for_register (dev_priv, MSVDX_MTX_RAM_ACCESS_STATUS, 1,	/* Required Value */
			 0xffffffff /* Enables */ );

  for (ui32Loop = 0; ui32Loop < uiWords; ui32Loop++)
    {
      ui32RamId = ui32DataMem + (ui32Address / ui32RamBankSize);

      if (ui32RamId != ui32CurrBank)
	{
	  ui32Addr = ui32Address >> 2;

	  ui32Ctrl = 0;

	  REGIO_WRITE_FIELD_LITE (ui32Ctrl,
				  MSVDX_MTX_RAM_ACCESS_CONTROL,
				  MTX_MCMID, ui32RamId);
	  REGIO_WRITE_FIELD_LITE (ui32Ctrl,
				  MSVDX_MTX_RAM_ACCESS_CONTROL,
				  MTX_MCM_ADDR, ui32Addr);
	  REGIO_WRITE_FIELD_LITE (ui32Ctrl,
				  MSVDX_MTX_RAM_ACCESS_CONTROL, MTX_MCMAI, 1);

	  PSB_WMSVDX32 (ui32Ctrl, MSVDX_MTX_RAM_ACCESS_CONTROL);

	  ui32CurrBank = ui32RamId;
	}
      ui32Address += 4;

      PSB_WMSVDX32 (pui32Data[ui32Loop], MSVDX_MTX_RAM_ACCESS_DATA_TRANSFER);

      /* Wait for MCMSTAT to become be idle 1 */
      psb_wait_for_register (dev_priv, MSVDX_MTX_RAM_ACCESS_STATUS, 1,	/* Required Value */
			     0xffffffff /* Enables */ );
    }
  PSB_DEBUG_GENERAL ("MSVDX: Upload done\n");

  /* Restore the access control register... */
  PSB_WMSVDX32 (ui32AccessControl, MSVDX_MTX_RAM_ACCESS_CONTROL);
}

static int
psb_verify_fw (struct drm_psb_private *dev_priv,
	       const uint32_t ui32RamBankSize,
	       const uint32_t ui32DataMem, uint32_t ui32Address,
	       const uint32_t uiWords, const uint32_t * const pui32Data)
{
  uint32_t ui32Loop, ui32Ctrl, ui32RamId, ui32Addr, ui32CurrBank =
    (uint32_t) ~ 0;
  uint32_t ui32AccessControl;
  int ret = 0;

  /* Save the access control register... */
  ui32AccessControl = PSB_RMSVDX32 (MSVDX_MTX_RAM_ACCESS_CONTROL);

  /* Wait for MCMSTAT to become be idle 1 */
  psb_wait_for_register (dev_priv, MSVDX_MTX_RAM_ACCESS_STATUS, 1,	/* Required Value */
			 0xffffffff /* Enables */ );

  for (ui32Loop = 0; ui32Loop < uiWords; ui32Loop++)
    {
      uint32_t ui32ReadBackVal;
      ui32RamId = ui32DataMem + (ui32Address / ui32RamBankSize);

      if (ui32RamId != ui32CurrBank)
	{
	  ui32Addr = ui32Address >> 2;
	  ui32Ctrl = 0;
	  REGIO_WRITE_FIELD_LITE (ui32Ctrl,
				  MSVDX_MTX_RAM_ACCESS_CONTROL,
				  MTX_MCMID, ui32RamId);
	  REGIO_WRITE_FIELD_LITE (ui32Ctrl,
				  MSVDX_MTX_RAM_ACCESS_CONTROL,
				  MTX_MCM_ADDR, ui32Addr);
	  REGIO_WRITE_FIELD_LITE (ui32Ctrl,
				  MSVDX_MTX_RAM_ACCESS_CONTROL, MTX_MCMAI, 1);
	  REGIO_WRITE_FIELD_LITE (ui32Ctrl,
				  MSVDX_MTX_RAM_ACCESS_CONTROL, MTX_MCMR, 1);

	  PSB_WMSVDX32 (ui32Ctrl, MSVDX_MTX_RAM_ACCESS_CONTROL);

	  ui32CurrBank = ui32RamId;
	}
      ui32Address += 4;

      /* Wait for MCMSTAT to become be idle 1 */
      psb_wait_for_register (dev_priv, MSVDX_MTX_RAM_ACCESS_STATUS, 1,	/* Required Value */
			     0xffffffff /* Enables */ );

      ui32ReadBackVal = PSB_RMSVDX32 (MSVDX_MTX_RAM_ACCESS_DATA_TRANSFER);
      if (pui32Data[ui32Loop] != ui32ReadBackVal)
	{
	  DRM_ERROR
	    ("psb: Firmware validation fails at index=%08x\n", ui32Loop);
	  ret = 1;
	  break;
	}
    }

  /* Restore the access control register... */
  PSB_WMSVDX32 (ui32AccessControl, MSVDX_MTX_RAM_ACCESS_CONTROL);

  return ret;
}

static uint32_t *
msvdx_get_fw (struct drm_device *dev,
	      const struct firmware **raw, uint8_t * name)
{
  int rc;
  int *ptr = NULL;

  rc = request_firmware (raw, name, &dev->pdev->dev);
  if (rc < 0)
    {
      DRM_ERROR ("MSVDX: %s request_firmware failed: Reason %d\n", name, rc);
      return NULL;
    }

  if ((*raw)->size < sizeof (struct msvdx_fw))
    {
      PSB_DEBUG_GENERAL ("MSVDX: %s is is not correct size(%zd)\n",
			 name, (*raw)->size);
      return NULL;
    }

  ptr = (int *) ((*raw))->data;

  if (!ptr)
    {
      PSB_DEBUG_GENERAL ("MSVDX: Failed to load %s\n", name);
      return NULL;
    }
  /*another sanity check... */
  if ((*raw)->size !=
      (sizeof (struct msvdx_fw) +
       sizeof (uint32_t) * ((struct msvdx_fw *) ptr)->text_size +
       sizeof (uint32_t) * ((struct msvdx_fw *) ptr)->data_size))
    {
      PSB_DEBUG_GENERAL ("MSVDX: %s is is not correct size(%zd)\n",
			 name, (*raw)->size);
      return NULL;
    }
  return ptr;
}

static int
psb_setup_fw (struct drm_device *dev)
{
  struct drm_psb_private *dev_priv = dev->dev_private;
  int ret = 0;

  uint32_t ram_bank_size;
  struct msvdx_fw *fw;
  uint32_t *fw_ptr = NULL;
  uint32_t *text_ptr = NULL;
  uint32_t *data_ptr = NULL;
  const struct firmware *raw = NULL;
  /* todo : Assert the clock is on - if not turn it on to upload code */

  PSB_DEBUG_GENERAL ("MSVDX: psb_setup_fw\n");

  /* Reset MTX */
  PSB_WMSVDX32 (MSVDX_MTX_SOFT_RESET_MTX_RESET_MASK, MSVDX_MTX_SOFT_RESET);

  /* Initialses Communication controll area to 0 */
  if(dev_priv->psb_rev_id >= POULSBO_D1)
   {
	PSB_DEBUG_GENERAL("MSVDX: Detected Poulsbo D1 or later revision.\n");
	PSB_WMSVDX32 (MSVDX_DEVICE_NODE_FLAGS_DEFAULT_D1, MSVDX_COMMS_OFFSET_FLAGS);
   }
  else 
   {
	PSB_DEBUG_GENERAL("MSVDX: Detected Poulsbo D0 or earlier revision.\n");
        PSB_WMSVDX32 (MSVDX_DEVICE_NODE_FLAGS_DEFAULT_D0, MSVDX_COMMS_OFFSET_FLAGS);
   }

  PSB_WMSVDX32 (0, MSVDX_COMMS_MSG_COUNTER);
  PSB_WMSVDX32 (0, MSVDX_COMMS_SIGNATURE);
  PSB_WMSVDX32 (0, MSVDX_COMMS_TO_HOST_RD_INDEX);
  PSB_WMSVDX32 (0, MSVDX_COMMS_TO_HOST_WRT_INDEX);
  PSB_WMSVDX32 (0, MSVDX_COMMS_TO_MTX_RD_INDEX);
  PSB_WMSVDX32 (0, MSVDX_COMMS_TO_MTX_WRT_INDEX);
  PSB_WMSVDX32 (0, MSVDX_COMMS_FW_STATUS);

  /* read register bank size */
  {
    uint32_t ui32BankSize, ui32Reg;
    ui32Reg = PSB_RMSVDX32 (MSVDX_MTX_RAM_BANK);
    ui32BankSize =
      REGIO_READ_FIELD (ui32Reg, MSVDX_MTX_RAM_BANK, CR_MTX_RAM_BANK_SIZE);
    ram_bank_size = (uint32_t) (1 << (ui32BankSize + 2));
  }

  PSB_DEBUG_GENERAL ("MSVDX: RAM bank size = %d bytes\n", ram_bank_size);

  fw_ptr = msvdx_get_fw (dev, &raw, "msvdx_fw.bin");

  if (!fw_ptr)
    {
      DRM_ERROR ("psb: No valid msvdx_fw.bin firmware found.\n");
      ret = 1;
      goto out;
    }

  fw = (struct msvdx_fw *) fw_ptr;
  if (fw->ver != 0x02)
    {
      DRM_ERROR
	("psb: msvdx_fw.bin firmware version mismatch, got version=%02x expected version=%02x\n",
	 fw->ver, 0x02);
      ret = 1;
      goto out;
    }

  text_ptr = (uint32_t *) ((uint8_t *) fw_ptr + sizeof (struct msvdx_fw));
  data_ptr = text_ptr + fw->text_size;

  PSB_DEBUG_GENERAL ("MSVDX: Retrieved pointers for firmware\n");
  PSB_DEBUG_GENERAL ("MSVDX: text_size: %d\n", fw->text_size);
  PSB_DEBUG_GENERAL ("MSVDX: data_size: %d\n", fw->data_size);
  PSB_DEBUG_GENERAL ("MSVDX: data_location: 0x%x\n", fw->data_location);
  PSB_DEBUG_GENERAL ("MSVDX: First 4 bytes of text: 0x%x\n", *text_ptr);
  PSB_DEBUG_GENERAL ("MSVDX: First 4 bytes of data: 0x%x\n", *data_ptr);

  PSB_DEBUG_GENERAL ("MSVDX: Uploading firmware\n");
  psb_upload_fw (dev_priv, MTX_CORE_CODE_MEM, ram_bank_size,
		 PC_START_ADDRESS - MTX_CODE_BASE, fw->text_size, text_ptr);
  psb_upload_fw (dev_priv, MTX_CORE_DATA_MEM, ram_bank_size,
		 fw->data_location - MTX_DATA_BASE, fw->data_size, data_ptr);

  /*todo :  Verify code upload possibly only in debug */
  if (psb_verify_fw
      (dev_priv, ram_bank_size, MTX_CORE_CODE_MEM,
       PC_START_ADDRESS - MTX_CODE_BASE, fw->text_size, text_ptr))
    {
      /* Firmware code upload failed */
      ret = 1;
      goto out;
    }
  if (psb_verify_fw
      (dev_priv, ram_bank_size, MTX_CORE_DATA_MEM,
       fw->data_location - MTX_DATA_BASE, fw->data_size, data_ptr))
    {
      /* Firmware data upload failed */
      ret = 1;
      goto out;
    }

  /*      -- Set starting PC address      */
  psb_write_mtx_core_reg (dev_priv, MTX_PC, PC_START_ADDRESS);

  /*      -- Turn on the thread   */
  PSB_WMSVDX32 (MSVDX_MTX_ENABLE_MTX_ENABLE_MASK, MSVDX_MTX_ENABLE);

  /* Wait for the signature value to be written back */
  ret = psb_wait_for_register (dev_priv, MSVDX_COMMS_SIGNATURE, MSVDX_COMMS_SIGNATURE_VALUE,	/* Required value */
			       0xffffffff /* Enabled bits */ );
  if (ret)
    {
      DRM_ERROR ("psb: MSVDX firmware fails to initialize.\n");
      goto out;
    }

  PSB_DEBUG_GENERAL ("MSVDX: MTX Initial indications OK\n");
  PSB_DEBUG_GENERAL ("MSVDX: MSVDX_COMMS_AREA_ADDR = %08x\n",
		     MSVDX_COMMS_AREA_ADDR);
out:
  if (raw)
    {
      PSB_DEBUG_GENERAL ("MSVDX releasing firmware resouces....\n");
      release_firmware (raw);
    }
  return ret;
}

static void
psb_free_ccb (struct drm_buffer_object **ccb)
{
  drm_bo_usage_deref_unlocked (ccb);
  *ccb = NULL;
}

/*******************************************************************************

 @Function	psb_msvdx_reset

 @Description

 Reset chip and disable interrupts.

 @Input psDeviceNode - device info. structure

 @Return  0 - Success
	  1 - Failure

******************************************************************************/
int
psb_msvdx_reset (struct drm_psb_private *dev_priv)
{
  int ret = 0;

  /* Issue software reset */
  PSB_WMSVDX32 (msvdx_sw_reset_all, MSVDX_CONTROL);

  ret = psb_wait_for_register (dev_priv, MSVDX_CONTROL, 0,	/* Required value */
			       MSVDX_CONTROL_CR_MSVDX_SOFT_RESET_MASK
			       /* Enabled bits */ );

  if (!ret)
    {
      /* Clear interrupt enabled flag */
      PSB_WMSVDX32 (0, MSVDX_HOST_INTERRUPT_ENABLE);

      /* Clear any pending interrupt flags                                                                                    */
      PSB_WMSVDX32 (0xFFFFFFFF, MSVDX_INTERRUPT_CLEAR);
    }
  
  mutex_destroy (&dev_priv->msvdx_mutex);

  return ret;
}

static int
psb_allocate_ccb (struct drm_device *dev,
		  struct drm_buffer_object **ccb,
		  uint32_t * base_addr, int size)
{
  int ret;
  struct drm_bo_kmap_obj tmp_kmap;
  int is_iomem;

  ret = drm_buffer_object_create (dev, size,
				  drm_bo_type_kernel,
				  DRM_BO_FLAG_READ |
				  DRM_PSB_FLAG_MEM_KERNEL |
				  DRM_BO_FLAG_NO_EVICT,
				  DRM_BO_HINT_DONT_FENCE, 0, 0, ccb);
  if (ret)
    {
      PSB_DEBUG_GENERAL ("Failed to allocate CCB.\n");
      *ccb = NULL;
      return 1;
    }

  ret = drm_bo_kmap (*ccb, 0, (*ccb)->num_pages, &tmp_kmap);
  if (ret)
    {
      PSB_DEBUG_GENERAL ("drm_bo_kmap failed ret: %d\n", ret);
      drm_bo_usage_deref_unlocked (ccb);
      *ccb = NULL;
      return 1;
    }

  memset (drm_bmo_virtual (&tmp_kmap, &is_iomem), 0, size);
  drm_bo_kunmap (&tmp_kmap);

  *base_addr = (*ccb)->offset;
  return 0;
}

int
psb_msvdx_init (struct drm_device *dev)
{
  struct drm_psb_private *dev_priv = dev->dev_private;
  uint32_t ui32Cmd;
  int ret;

  PSB_DEBUG_GENERAL ("MSVDX: psb_msvdx_init\n");

  /*Initialize command msvdx queueing */
  INIT_LIST_HEAD (&dev_priv->msvdx_queue);
  mutex_init (&dev_priv->msvdx_mutex);
  spin_lock_init (&dev_priv->msvdx_lock);
  dev_priv->msvdx_busy = 0;

  /*figure out the stepping*/
  pci_read_config_byte(dev->pdev, PSB_REVID_OFFSET, &dev_priv->psb_rev_id );

  /* Enable Clocks */
  PSB_DEBUG_GENERAL ("Enabling clocks\n");
  PSB_WMSVDX32 (clk_enable_all, MSVDX_MAN_CLK_ENABLE);

  /* Enable MMU by removing all bypass bits */
  PSB_WMSVDX32 (0, MSVDX_MMU_CONTROL0);

  PSB_DEBUG_GENERAL ("MSVDX: Setting up RENDEC\n");
  /* Allocate device virtual memory as required by rendec.... */
  if (!dev_priv->ccb0)
    {
      ret =
	psb_allocate_ccb (dev, &dev_priv->ccb0,
			  &dev_priv->base_addr0, RENDEC_A_SIZE);
      if (ret)
	goto err_exit;
    }

  if (!dev_priv->ccb1)
    {
      ret =
	psb_allocate_ccb (dev, &dev_priv->ccb1,
			  &dev_priv->base_addr1, RENDEC_B_SIZE);
      if (ret)
	goto err_exit;
    }

  PSB_DEBUG_GENERAL ("MSVDX: RENDEC A: %08x RENDEC B: %08x\n",
		     dev_priv->base_addr0, dev_priv->base_addr1);

  PSB_WMSVDX32 (dev_priv->base_addr0, MSVDX_RENDEC_BASE_ADDR0);
  PSB_WMSVDX32 (dev_priv->base_addr1, MSVDX_RENDEC_BASE_ADDR1);

  ui32Cmd = 0;
  REGIO_WRITE_FIELD (ui32Cmd, MSVDX_RENDEC_BUFFER_SIZE,
		     RENDEC_BUFFER_SIZE0, RENDEC_A_SIZE / 4096);
  REGIO_WRITE_FIELD (ui32Cmd, MSVDX_RENDEC_BUFFER_SIZE,
		     RENDEC_BUFFER_SIZE1, RENDEC_B_SIZE / 4096);
  PSB_WMSVDX32 (ui32Cmd, MSVDX_RENDEC_BUFFER_SIZE);

  ui32Cmd = 0;
  REGIO_WRITE_FIELD (ui32Cmd, MSVDX_RENDEC_CONTROL1,
		     RENDEC_DECODE_START_SIZE, 0);
  REGIO_WRITE_FIELD (ui32Cmd, MSVDX_RENDEC_CONTROL1, RENDEC_BURST_SIZE_W, 1);
  REGIO_WRITE_FIELD (ui32Cmd, MSVDX_RENDEC_CONTROL1, RENDEC_BURST_SIZE_R, 1);
  REGIO_WRITE_FIELD (ui32Cmd, MSVDX_RENDEC_CONTROL1,
		     RENDEC_EXTERNAL_MEMORY, 1);
  PSB_WMSVDX32 (ui32Cmd, MSVDX_RENDEC_CONTROL1);

  ui32Cmd = 0x00101010;
  PSB_WMSVDX32 (ui32Cmd, MSVDX_RENDEC_CONTEXT0);
  PSB_WMSVDX32 (ui32Cmd, MSVDX_RENDEC_CONTEXT1);
  PSB_WMSVDX32 (ui32Cmd, MSVDX_RENDEC_CONTEXT2);
  PSB_WMSVDX32 (ui32Cmd, MSVDX_RENDEC_CONTEXT3);
  PSB_WMSVDX32 (ui32Cmd, MSVDX_RENDEC_CONTEXT4);
  PSB_WMSVDX32 (ui32Cmd, MSVDX_RENDEC_CONTEXT5);

  ui32Cmd = 0;
  REGIO_WRITE_FIELD (ui32Cmd, MSVDX_RENDEC_CONTROL0, RENDEC_INITIALISE, 1);
  PSB_WMSVDX32 (ui32Cmd, MSVDX_RENDEC_CONTROL0);

  ret = psb_setup_fw (dev);
  if (ret)
    goto err_exit;

  PSB_WMSVDX32 (clk_enable_minimal, MSVDX_MAN_CLK_ENABLE);

  return 0;

err_exit:
  if (dev_priv->ccb0)
    psb_free_ccb (&dev_priv->ccb0);
  if (dev_priv->ccb1)
    psb_free_ccb (&dev_priv->ccb1);

  return 1;
}

int
psb_msvdx_uninit (struct drm_device *dev)
{
  struct drm_psb_private *dev_priv = dev->dev_private;

  /*Reset MSVDX chip */
  psb_msvdx_reset (dev_priv);

//  PSB_WMSVDX32 (clk_enable_minimal, MSVDX_MAN_CLK_ENABLE);
    printk("set the msvdx clock to 0 in the %s\n", __FUNCTION__);
    PSB_WMSVDX32 (0, MSVDX_MAN_CLK_ENABLE);

  /*Clean up resources...*/
  if (dev_priv->ccb0)
    psb_free_ccb (&dev_priv->ccb0);
  if (dev_priv->ccb1)
    psb_free_ccb (&dev_priv->ccb1);

  return 0;
}

int psb_hw_info_ioctl(struct drm_device *dev, void *data,
                            struct drm_file *file_priv)
{
    struct drm_psb_private *dev_priv = dev->dev_private;
    struct drm_psb_hw_info *hw_info = data;
    struct pci_dev * pci_root = pci_get_bus_and_slot(0, 0);

    hw_info->rev_id = dev_priv->psb_rev_id;
   
    /*read the fuse info to determine the caps*/
    pci_write_config_dword(pci_root, 0xD0, PCI_PORT5_REG80_FFUSE);
    pci_read_config_dword(pci_root, 0xD4, &hw_info->caps);

    PSB_DEBUG_GENERAL("MSVDX: PSB caps: 0x%x\n", hw_info->caps);
    return 0;
}
