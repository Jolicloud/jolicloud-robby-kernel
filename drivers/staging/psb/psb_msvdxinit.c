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

#include <drm/drmP.h>
#include <drm/drm.h>
#include "psb_drv.h"
#include "psb_msvdx.h"
#include <linux/firmware.h>

#define MSVDX_REG (dev_priv->msvdx_reg)
uint8_t psb_rev_id;
/*MSVDX FW header*/
struct msvdx_fw {
	uint32_t ver;
	uint32_t text_size;
	uint32_t data_size;
	uint32_t data_location;
};

int psb_wait_for_register(struct drm_psb_private *dev_priv,
			  uint32_t offset, uint32_t value, uint32_t enable)
{
	uint32_t tmp;
	uint32_t poll_cnt = 10000;
	while (poll_cnt) {
		tmp = PSB_RMSVDX32(offset);
		if (value == (tmp & enable))	/* All the bits are reset   */
			return 0;	/* So exit                      */

		/* Wait a bit */
		DRM_UDELAY(1000);
		poll_cnt--;
	}
	DRM_ERROR("MSVDX: Timeout while waiting for register %08x:"
		  " expecting %08x (mask %08x), got %08x\n",
		  offset, value, enable, tmp);

	return 1;
}

int psb_poll_mtx_irq(struct drm_psb_private *dev_priv)
{
	int ret = 0;
	uint32_t mtx_int = 0;

	REGIO_WRITE_FIELD_LITE(mtx_int, MSVDX_INTERRUPT_STATUS, CR_MTX_IRQ,
			       1);

	ret = psb_wait_for_register(dev_priv, MSVDX_INTERRUPT_STATUS,
				    /* Required value */
				    mtx_int,
				    /* Enabled bits */
				    mtx_int);

	if (ret) {
		DRM_ERROR("MSVDX: Error Mtx did not return"
			  " int within a resonable time\n");
		return ret;
	}

	PSB_DEBUG_IRQ("MSVDX: Got MTX Int\n");

	/* Got it so clear the bit */
	PSB_WMSVDX32(mtx_int, MSVDX_INTERRUPT_CLEAR);

	return ret;
}

void psb_write_mtx_core_reg(struct drm_psb_private *dev_priv,
			    const uint32_t core_reg, const uint32_t val)
{
	uint32_t reg = 0;

	/* Put data in MTX_RW_DATA */
	PSB_WMSVDX32(val, MSVDX_MTX_REGISTER_READ_WRITE_DATA);

	/* DREADY is set to 0 and request a write */
	reg = core_reg;
	REGIO_WRITE_FIELD_LITE(reg, MSVDX_MTX_REGISTER_READ_WRITE_REQUEST,
			       MTX_RNW, 0);
	REGIO_WRITE_FIELD_LITE(reg, MSVDX_MTX_REGISTER_READ_WRITE_REQUEST,
			       MTX_DREADY, 0);
	PSB_WMSVDX32(reg, MSVDX_MTX_REGISTER_READ_WRITE_REQUEST);

	psb_wait_for_register(dev_priv,
			MSVDX_MTX_REGISTER_READ_WRITE_REQUEST,
			MSVDX_MTX_REGISTER_READ_WRITE_REQUEST_MTX_DREADY_MASK,
			MSVDX_MTX_REGISTER_READ_WRITE_REQUEST_MTX_DREADY_MASK);
}

void psb_upload_fw(struct drm_psb_private *dev_priv,
		   const uint32_t data_mem, uint32_t ram_bank_size,
		   uint32_t address, const unsigned int words,
		   const uint32_t * const data)
{
	uint32_t loop, ctrl, ram_id, addr, cur_bank = (uint32_t) ~0;
	uint32_t access_ctrl;

	/* Save the access control register... */
	access_ctrl = PSB_RMSVDX32(MSVDX_MTX_RAM_ACCESS_CONTROL);

	/* Wait for MCMSTAT to become be idle 1 */
	psb_wait_for_register(dev_priv, MSVDX_MTX_RAM_ACCESS_STATUS,
			      1,	/* Required Value */
			      0xffffffff /* Enables */);

	for (loop = 0; loop < words; loop++) {
		ram_id = data_mem + (address / ram_bank_size);
		if (ram_id != cur_bank) {
			addr = address >> 2;
			ctrl = 0;
			REGIO_WRITE_FIELD_LITE(ctrl,
					       MSVDX_MTX_RAM_ACCESS_CONTROL,
					       MTX_MCMID, ram_id);
			REGIO_WRITE_FIELD_LITE(ctrl,
					       MSVDX_MTX_RAM_ACCESS_CONTROL,
					       MTX_MCM_ADDR, addr);
			REGIO_WRITE_FIELD_LITE(ctrl,
					       MSVDX_MTX_RAM_ACCESS_CONTROL,
					       MTX_MCMAI, 1);
			PSB_WMSVDX32(ctrl, MSVDX_MTX_RAM_ACCESS_CONTROL);
			cur_bank = ram_id;
		}
		address += 4;

		PSB_WMSVDX32(data[loop],
			     MSVDX_MTX_RAM_ACCESS_DATA_TRANSFER);

		/* Wait for MCMSTAT to become be idle 1 */
		psb_wait_for_register(dev_priv, MSVDX_MTX_RAM_ACCESS_STATUS,
				      1,	/* Required Value */
				      0xffffffff /* Enables */);
	}
	PSB_DEBUG_GENERAL("MSVDX: Upload done\n");

	/* Restore the access control register... */
	PSB_WMSVDX32(access_ctrl, MSVDX_MTX_RAM_ACCESS_CONTROL);
}

static int psb_verify_fw(struct drm_psb_private *dev_priv,
			 const uint32_t ram_bank_size,
			 const uint32_t data_mem, uint32_t address,
			 const uint32_t words, const uint32_t * const data)
{
	uint32_t loop, ctrl, ram_id, addr, cur_bank = (uint32_t) ~0;
	uint32_t access_ctrl;
	int ret = 0;

	/* Save the access control register... */
	access_ctrl = PSB_RMSVDX32(MSVDX_MTX_RAM_ACCESS_CONTROL);

	/* Wait for MCMSTAT to become be idle 1 */
	psb_wait_for_register(dev_priv, MSVDX_MTX_RAM_ACCESS_STATUS,
			      1,	/* Required Value */
			      0xffffffff /* Enables */);

	for (loop = 0; loop < words; loop++) {
		uint32_t tmp;
		ram_id = data_mem + (address / ram_bank_size);

		if (ram_id != cur_bank) {
			addr = address >> 2;
			ctrl = 0;
			REGIO_WRITE_FIELD_LITE(ctrl,
					       MSVDX_MTX_RAM_ACCESS_CONTROL,
					       MTX_MCMID, ram_id);
			REGIO_WRITE_FIELD_LITE(ctrl,
					       MSVDX_MTX_RAM_ACCESS_CONTROL,
					       MTX_MCM_ADDR, addr);
			REGIO_WRITE_FIELD_LITE(ctrl,
					       MSVDX_MTX_RAM_ACCESS_CONTROL,
					       MTX_MCMAI, 1);
			REGIO_WRITE_FIELD_LITE(ctrl,
					       MSVDX_MTX_RAM_ACCESS_CONTROL,
					       MTX_MCMR, 1);

			PSB_WMSVDX32(ctrl, MSVDX_MTX_RAM_ACCESS_CONTROL);

			cur_bank = ram_id;
		}
		address += 4;

		/* Wait for MCMSTAT to become be idle 1 */
		psb_wait_for_register(dev_priv, MSVDX_MTX_RAM_ACCESS_STATUS,
				      1,	/* Required Value */
				      0xffffffff /* Enables */);

		tmp = PSB_RMSVDX32(MSVDX_MTX_RAM_ACCESS_DATA_TRANSFER);
		if (data[loop] != tmp) {
			DRM_ERROR("psb: Firmware validation fails"
				  " at index=%08x\n", loop);
			ret = 1;
			break;
		}
	}

	/* Restore the access control register... */
	PSB_WMSVDX32(access_ctrl, MSVDX_MTX_RAM_ACCESS_CONTROL);

	return ret;
}

static uint32_t *msvdx_get_fw(struct drm_device *dev,
			      const struct firmware **raw, uint8_t *name)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	int rc, fw_size;
	int *ptr = NULL;

	rc = request_firmware(raw, name, &dev->pdev->dev);
	if (rc < 0) {
		DRM_ERROR("MSVDX: %s request_firmware failed: Reason %d\n",
			  name, rc);
		return NULL;
	}

	if ((*raw)->size < sizeof(struct msvdx_fw)) {
		DRM_ERROR("MSVDX: %s is is not correct size(%zd)\n",
			  name, (*raw)->size);
		return NULL;
	}

	ptr = (int *) ((*raw))->data;

	if (!ptr) {
		DRM_ERROR("MSVDX: Failed to load %s\n", name);
		return NULL;
	}

	/* another sanity check... */
	fw_size = sizeof(struct msvdx_fw) +
	    sizeof(uint32_t) * ((struct msvdx_fw *) ptr)->text_size +
	    sizeof(uint32_t) * ((struct msvdx_fw *) ptr)->data_size;
	if ((*raw)->size != fw_size) {
		DRM_ERROR("MSVDX: %s is is not correct size(%zd)\n",
			  name, (*raw)->size);
		return NULL;
	}
	dev_priv->msvdx_fw = kcalloc(1, fw_size, GFP_KERNEL);
	if (dev_priv->msvdx_fw == NULL)
		DRM_ERROR("MSVDX: allocate FW buffer failed\n");
	else {
		memcpy(dev_priv->msvdx_fw, ptr, fw_size);
		dev_priv->msvdx_fw_size = fw_size;
	}

	PSB_DEBUG_GENERAL("MSVDX: releasing firmware resouces\n");
	release_firmware(*raw);

	return dev_priv->msvdx_fw;
}

int psb_setup_fw(struct drm_device *dev)
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

	PSB_DEBUG_GENERAL("MSVDX: psb_setup_fw\n");
	PSB_WMSVDX32(clk_enable_all, MSVDX_MAN_CLK_ENABLE);

	/* Reset MTX */
	PSB_WMSVDX32(MSVDX_MTX_SOFT_RESET_MTX_RESET_MASK,
		     MSVDX_MTX_SOFT_RESET);

	/* Initialses Communication controll area to 0 */
	if (psb_rev_id >= POULSBO_D1) {
		PSB_DEBUG_GENERAL("MSVDX: Detected Poulsbo D1"
				  " or later revision.\n");
		PSB_WMSVDX32(MSVDX_DEVICE_NODE_FLAGS_DEFAULT_D1,
			     MSVDX_COMMS_OFFSET_FLAGS);
	} else {
		PSB_DEBUG_GENERAL("MSVDX: Detected Poulsbo D0"
				  " or earlier revision.\n");
		PSB_WMSVDX32(MSVDX_DEVICE_NODE_FLAGS_DEFAULT_D0,
			     MSVDX_COMMS_OFFSET_FLAGS);
	}

	PSB_WMSVDX32(0, MSVDX_COMMS_MSG_COUNTER);
	PSB_WMSVDX32(0, MSVDX_COMMS_SIGNATURE);
	PSB_WMSVDX32(0, MSVDX_COMMS_TO_HOST_RD_INDEX);
	PSB_WMSVDX32(0, MSVDX_COMMS_TO_HOST_WRT_INDEX);
	PSB_WMSVDX32(0, MSVDX_COMMS_TO_MTX_RD_INDEX);
	PSB_WMSVDX32(0, MSVDX_COMMS_TO_MTX_WRT_INDEX);
	PSB_WMSVDX32(0, MSVDX_COMMS_FW_STATUS);

	/* read register bank size */
	{
		uint32_t bank_size, reg;
		reg = PSB_RMSVDX32(MSVDX_MTX_RAM_BANK);
		bank_size =
		    REGIO_READ_FIELD(reg, MSVDX_MTX_RAM_BANK,
				     CR_MTX_RAM_BANK_SIZE);
		ram_bank_size = (uint32_t) (1 << (bank_size + 2));
	}

	PSB_DEBUG_GENERAL("MSVDX: RAM bank size = %d bytes\n",
			  ram_bank_size);

	/* if FW already loaded from storage */
	if (dev_priv->msvdx_fw)
		fw_ptr = dev_priv->msvdx_fw;
	else
		fw_ptr = msvdx_get_fw(dev, &raw, "msvdx_fw.bin");

	if (!fw_ptr) {
		DRM_ERROR("psb: No valid msvdx_fw.bin firmware found.\n");
		ret = 1;
		goto out;
	}

	fw = (struct msvdx_fw *) fw_ptr;
	if (fw->ver != 0x02) {
		DRM_ERROR("psb: msvdx_fw.bin firmware version mismatch,"
			  "got version=%02x expected version=%02x\n",
			  fw->ver, 0x02);
		ret = 1;
		goto out;
	}

	text_ptr =
	    (uint32_t *) ((uint8_t *) fw_ptr + sizeof(struct msvdx_fw));
	data_ptr = text_ptr + fw->text_size;

	PSB_DEBUG_GENERAL("MSVDX: Retrieved pointers for firmware\n");
	PSB_DEBUG_GENERAL("MSVDX: text_size: %d\n", fw->text_size);
	PSB_DEBUG_GENERAL("MSVDX: data_size: %d\n", fw->data_size);
	PSB_DEBUG_GENERAL("MSVDX: data_location: 0x%x\n",
			  fw->data_location);
	PSB_DEBUG_GENERAL("MSVDX: First 4 bytes of text: 0x%x\n",
			  *text_ptr);
	PSB_DEBUG_GENERAL("MSVDX: First 4 bytes of data: 0x%x\n",
			  *data_ptr);

	PSB_DEBUG_GENERAL("MSVDX: Uploading firmware\n");
	psb_upload_fw(dev_priv, MTX_CORE_CODE_MEM, ram_bank_size,
		      PC_START_ADDRESS - MTX_CODE_BASE, fw->text_size,
		      text_ptr);
	psb_upload_fw(dev_priv, MTX_CORE_DATA_MEM, ram_bank_size,
		      fw->data_location - MTX_DATA_BASE, fw->data_size,
		      data_ptr);

#if 0
	/* todo :  Verify code upload possibly only in debug */
	ret = psb_verify_fw(dev_priv, ram_bank_size,
			    MTX_CORE_CODE_MEM,
			    PC_START_ADDRESS - MTX_CODE_BASE,
			    fw->text_size, text_ptr);
	if (ret) {
		/* Firmware code upload failed */
		ret = 1;
		goto out;
	}

	ret = psb_verify_fw(dev_priv, ram_bank_size, MTX_CORE_DATA_MEM,
			    fw->data_location - MTX_DATA_BASE,
			    fw->data_size, data_ptr);
	if (ret) {
		/* Firmware data upload failed */
		ret = 1;
		goto out;
	}
#else
	(void)psb_verify_fw;
#endif
	/*      -- Set starting PC address      */
	psb_write_mtx_core_reg(dev_priv, MTX_PC, PC_START_ADDRESS);

	/*      -- Turn on the thread   */
	PSB_WMSVDX32(MSVDX_MTX_ENABLE_MTX_ENABLE_MASK, MSVDX_MTX_ENABLE);

	/* Wait for the signature value to be written back */
	ret = psb_wait_for_register(dev_priv, MSVDX_COMMS_SIGNATURE,
				MSVDX_COMMS_SIGNATURE_VALUE, /*Required value*/
				0xffffffff /* Enabled bits */);
	if (ret) {
		DRM_ERROR("MSVDX: firmware fails to initialize.\n");
		goto out;
	}

	PSB_DEBUG_GENERAL("MSVDX: MTX Initial indications OK\n");
	PSB_DEBUG_GENERAL("MSVDX: MSVDX_COMMS_AREA_ADDR = %08x\n",
			  MSVDX_COMMS_AREA_ADDR);
#if 0

	/* Send test message */
	{
		uint32_t msg_buf[FW_VA_DEBUG_TEST2_SIZE >> 2];

		MEMIO_WRITE_FIELD(msg_buf, FW_VA_DEBUG_TEST2_MSG_SIZE,
				  FW_VA_DEBUG_TEST2_SIZE);
		MEMIO_WRITE_FIELD(msg_buf, FW_VA_DEBUG_TEST2_ID,
				  VA_MSGID_TEST2);

		ret = psb_mtx_send(dev_priv, msg_buf);
		if (ret) {
			DRM_ERROR("psb: MSVDX sending fails.\n");
			goto out;
		}

		/* Wait for Mtx to ack this message */
		psb_poll_mtx_irq(dev_priv);

	}
#endif
out:

	return ret;
}


static void psb_free_ccb(struct ttm_buffer_object **ccb)
{
	ttm_bo_unref(ccb);
	*ccb = NULL;
}

/**
 * Reset chip and disable interrupts.
 * Return 0 success, 1 failure
 */
int psb_msvdx_reset(struct drm_psb_private *dev_priv)
{
	int ret = 0;

	/* Issue software reset */
	PSB_WMSVDX32(msvdx_sw_reset_all, MSVDX_CONTROL);

	ret = psb_wait_for_register(dev_priv, MSVDX_CONTROL, 0,
				    MSVDX_CONTROL_CR_MSVDX_SOFT_RESET_MASK);

	if (!ret) {
		/* Clear interrupt enabled flag */
		PSB_WMSVDX32(0, MSVDX_HOST_INTERRUPT_ENABLE);

		/* Clear any pending interrupt flags */
		PSB_WMSVDX32(0xFFFFFFFF, MSVDX_INTERRUPT_CLEAR);
	}

	/* mutex_destroy(&dev_priv->msvdx_mutex); */

	return ret;
}

static int psb_allocate_ccb(struct drm_device *dev,
			    struct ttm_buffer_object **ccb,
			    uint32_t *base_addr, int size)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct ttm_bo_device *bdev = &dev_priv->bdev;
	int ret;
	struct ttm_bo_kmap_obj tmp_kmap;
	bool is_iomem;

	PSB_DEBUG_INIT("MSVDX: allocate CCB\n");

	ret = ttm_buffer_object_create(bdev, size,
				       ttm_bo_type_kernel,
				       DRM_PSB_FLAG_MEM_KERNEL |
				       TTM_PL_FLAG_NO_EVICT, 0, 0, 0,
				       NULL, ccb);
	if (ret) {
		DRM_ERROR("MSVDX:failed to allocate CCB.\n");
		*ccb = NULL;
		return 1;
	}

	ret = ttm_bo_kmap(*ccb, 0, (*ccb)->num_pages, &tmp_kmap);
	if (ret) {
		PSB_DEBUG_GENERAL("ttm_bo_kmap failed ret: %d\n", ret);
		ttm_bo_unref(ccb);
		*ccb = NULL;
		return 1;
	}

	memset(ttm_kmap_obj_virtual(&tmp_kmap, &is_iomem), 0,
	       RENDEC_A_SIZE);
	ttm_bo_kunmap(&tmp_kmap);

	*base_addr = (*ccb)->offset;
	return 0;
}

int psb_msvdx_init(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	uint32_t cmd;
	/* uint32_t clk_gate_ctrl = clk_enable_all; */
	int ret;

	if (!dev_priv->ccb0) { /* one for the first time */
		/* Initialize comand msvdx queueing */
		INIT_LIST_HEAD(&dev_priv->msvdx_queue);
		mutex_init(&dev_priv->msvdx_mutex);
		spin_lock_init(&dev_priv->msvdx_lock);
		/*figure out the stepping */
		pci_read_config_byte(dev->pdev, PSB_REVID_OFFSET, &psb_rev_id);
	}

	dev_priv->msvdx_busy = 0;

	/* Enable Clocks */
	PSB_DEBUG_GENERAL("Enabling clocks\n");
	PSB_WMSVDX32(clk_enable_all, MSVDX_MAN_CLK_ENABLE);

	/* Enable MMU by removing all bypass bits */
	PSB_WMSVDX32(0, MSVDX_MMU_CONTROL0);

	/* move firmware loading to the place receiving first command buffer */

	PSB_DEBUG_GENERAL("MSVDX: Setting up RENDEC,allocate CCB 0/1\n");
	/* Allocate device virtual memory as required by rendec.... */
	if (!dev_priv->ccb0) {
		ret = psb_allocate_ccb(dev, &dev_priv->ccb0,
				       &dev_priv->base_addr0,
				       RENDEC_A_SIZE);
		if (ret)
			goto err_exit;
	}

	if (!dev_priv->ccb1) {
		ret = psb_allocate_ccb(dev, &dev_priv->ccb1,
				       &dev_priv->base_addr1,
				       RENDEC_B_SIZE);
		if (ret)
			goto err_exit;
	}


	PSB_DEBUG_GENERAL("MSVDX: RENDEC A: %08x RENDEC B: %08x\n",
			  dev_priv->base_addr0, dev_priv->base_addr1);

	PSB_WMSVDX32(dev_priv->base_addr0, MSVDX_RENDEC_BASE_ADDR0);
	PSB_WMSVDX32(dev_priv->base_addr1, MSVDX_RENDEC_BASE_ADDR1);

	cmd = 0;
	REGIO_WRITE_FIELD(cmd, MSVDX_RENDEC_BUFFER_SIZE,
			  RENDEC_BUFFER_SIZE0, RENDEC_A_SIZE / 4096);
	REGIO_WRITE_FIELD(cmd, MSVDX_RENDEC_BUFFER_SIZE,
			  RENDEC_BUFFER_SIZE1, RENDEC_B_SIZE / 4096);
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_BUFFER_SIZE);

	cmd = 0;
	REGIO_WRITE_FIELD(cmd, MSVDX_RENDEC_CONTROL1,
			  RENDEC_DECODE_START_SIZE, 0);
	REGIO_WRITE_FIELD(cmd, MSVDX_RENDEC_CONTROL1,
			  RENDEC_BURST_SIZE_W, 1);
	REGIO_WRITE_FIELD(cmd, MSVDX_RENDEC_CONTROL1,
			  RENDEC_BURST_SIZE_R, 1);
	REGIO_WRITE_FIELD(cmd, MSVDX_RENDEC_CONTROL1,
			  RENDEC_EXTERNAL_MEMORY, 1);
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_CONTROL1);

	cmd = 0x00101010;
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_CONTEXT0);
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_CONTEXT1);
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_CONTEXT2);
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_CONTEXT3);
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_CONTEXT4);
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_CONTEXT5);

	cmd = 0;
	REGIO_WRITE_FIELD(cmd, MSVDX_RENDEC_CONTROL0, RENDEC_INITIALISE,
			  1);
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_CONTROL0);

	PSB_WMSVDX32(clk_enable_minimal, MSVDX_MAN_CLK_ENABLE);
	PSB_DEBUG_INIT("MSVDX:defer firmware loading to the"
		       " place when receiving user space commands\n");

	dev_priv->msvdx_fw_loaded = 0; /* need to load firware */

	PSB_WMSVDX32(clk_enable_minimal, MSVDX_MAN_CLK_ENABLE);

#if 0
	ret = psb_setup_fw(dev);
	if (ret)
		goto err_exit;
	/* Send Initialisation message to firmware */
	if (0) {
		uint32_t msg_init[FW_VA_INIT_SIZE >> 2];
		MEMIO_WRITE_FIELD(msg_init, FWRK_GENMSG_SIZE,
				  FW_VA_INIT_SIZE);
		MEMIO_WRITE_FIELD(msg_init, FWRK_GENMSG_ID, VA_MSGID_INIT);

		/* Need to set this for all but A0 */
		MEMIO_WRITE_FIELD(msg_init, FW_VA_INIT_GLOBAL_PTD,
				  psb_get_default_pd_addr(dev_priv->mmu));

		ret = psb_mtx_send(dev_priv, msg_init);
		if (ret)
			goto err_exit;

		psb_poll_mtx_irq(dev_priv);
	}
#endif

	return 0;

err_exit:
	DRM_ERROR("MSVDX: initialization failed\n");
	if (dev_priv->ccb0)
		psb_free_ccb(&dev_priv->ccb0);
	if (dev_priv->ccb1)
		psb_free_ccb(&dev_priv->ccb1);

	return 1;
}

int psb_msvdx_uninit(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;

	/* Reset MSVDX chip */
	psb_msvdx_reset(dev_priv);

	/* PSB_WMSVDX32 (clk_enable_minimal, MSVDX_MAN_CLK_ENABLE); */
	PSB_DEBUG_INIT("MSVDX:set the msvdx clock to 0\n");
	PSB_WMSVDX32(0, MSVDX_MAN_CLK_ENABLE);

	if (dev_priv->ccb0)
		psb_free_ccb(&dev_priv->ccb0);
	if (dev_priv->ccb1)
		psb_free_ccb(&dev_priv->ccb1);
	if (dev_priv->msvdx_fw)
		kfree(dev_priv->msvdx_fw);

	return 0;
}
