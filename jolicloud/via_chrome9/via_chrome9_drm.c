/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice
 * (including the next paragraph) shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL VIA, S3 GRAPHICS, AND/OR
 * ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "drmP.h"
#include "via_chrome9_drm.h"
#include "via_chrome9_drv.h"
#include "via_chrome9_mm.h"
#include "via_chrome9_dma.h"
#include "via_chrome9_3d_reg.h"

#define VIA_CHROME9DRM_VIDEO_STARTADDRESS_ALIGNMENT 10
void *via_chrome9_dev_v4l;
void *via_chrome9_filepriv_v4l;

void __via_chrome9ke_udelay(unsigned long usecs)
{
	unsigned long start;
	unsigned long stop;
	unsigned long period;
	unsigned long wait_period;
	struct timespec tval;

#ifdef NDELAY_LIMIT
#define UDELAY_LIMIT    (NDELAY_LIMIT/1000) /* supposed to be 10 msec */
#else
#define UDELAY_LIMIT    (10000)             /* 10 msec */
#endif

	if (usecs > UDELAY_LIMIT) {
		start = jiffies;
		tval.tv_sec = usecs / 1000000;
		tval.tv_nsec = (usecs - tval.tv_sec * 1000000) * 1000;
		wait_period = timespec_to_jiffies(&tval);
		do {
			stop = jiffies;

			if (stop < start)
				period = ((unsigned long)-1 - start) + stop + 1;
			else
				period = stop - start;

		} while (period < wait_period);
	} else
		udelay(usecs);  /* delay value might get checked once again */
}

int via_chrome9_ioctl_process_exit(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	return 0;
}

int via_chrome9_ioctl_restore_primary(struct drm_device *dev,
	void *data, struct drm_file *file_priv)
{
	return 0;
}

void Initialize3DEngine(struct drm_via_chrome9_private *dev_priv)
{
	int i;
	unsigned int StageOfTexture;

	if (dev_priv->chip_sub_index == CHIP_H5 ||
		dev_priv->chip_sub_index == CHIP_H5S1) {
		setmmioregister(dev_priv->mmio->handle, 0x43C,
			0x00010000);

		for (i = 0; i <= 0x8A; i++) {
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(unsigned int) i << 24);
		}

		/* Initial Texture Stage Setting*/
		for (StageOfTexture = 0; StageOfTexture < 0xf;
		StageOfTexture++) {
			setmmioregister(dev_priv->mmio->handle, 0x43C,
				(0x00020000 | 0x00000000 |
				(StageOfTexture & 0xf)<<24));
		/*  *((unsigned int volatile*)(pMapIOPort+HC_REG_TRANS_SET)) =
		(0x00020000 | HC_ParaSubType_Tex0 | (StageOfTexture &
		0xf)<<24);*/
			for (i = 0 ; i <= 0x30 ; i++) {
				setmmioregister(dev_priv->mmio->handle,
				0x440, (unsigned int) i << 24);
			}
		}

		/* Initial Texture Sampler Setting*/
		for (StageOfTexture = 0; StageOfTexture < 0xf;
		StageOfTexture++) {
			setmmioregister(dev_priv->mmio->handle, 0x43C,
				(0x00020000 | 0x00020000 |
				(StageOfTexture & 0xf)<<24));
			/* *((unsigned int volatile*)(pMapIOPort+
			HC_REG_TRANS_SET)) = (0x00020000 | 0x00020000 |
			( StageOfTexture & 0xf)<<24);*/
			for (i = 0 ; i <= 0x30 ; i++) {
				setmmioregister(dev_priv->mmio->handle,
				0x440, (unsigned int) i << 24);
			}
		}

		setmmioregister(dev_priv->mmio->handle, 0x43C,
			(0x00020000 | 0xfe000000));
		/* *((unsigned int volatile*)(pMapIOPort+HC_REG_TRANS_SET)) =
			(0x00020000 | HC_ParaSubType_TexGen);*/
		for (i = 0 ; i <= 0x13 ; i++) {
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(unsigned int) i << 24);
			/* *((unsigned int volatile*)(pMapIOPort+
			HC_REG_Hpara0)) = ((unsigned int) i << 24);*/
		}

		/* Initial Gamma Table Setting*/
		/* Initial Gamma Table Setting*/
		/* 5 + 4 = 9 (12) dwords*/
		/* sRGB texture is not directly support by H3 hardware.
		We have to set the deGamma table for texture sampling.*/

		/* degamma table*/
		setmmioregister(dev_priv->mmio->handle, 0x43C,
			(0x00030000 | 0x15000000));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(0x40000000 | (30 << 20) | (15 << 10) | (5)));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			((119 << 20) | (81 << 10) | (52)));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			((283 << 20) | (219 << 10) | (165)));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			((535 << 20) | (441 << 10) | (357)));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			((119 << 20) | (884 << 20) | (757 << 10) |
			(640)));

		/* gamma table*/
		setmmioregister(dev_priv->mmio->handle, 0x43C,
			(0x00030000 | 0x17000000));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(0x40000000 | (13 << 20) | (13 << 10) | (13)));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(0x40000000 | (26 << 20) | (26 << 10) | (26)));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(0x40000000 | (39 << 20) | (39 << 10) | (39)));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			((51 << 20) | (51 << 10) | (51)));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			((71 << 20) | (71 << 10) | (71)));
		setmmioregister(dev_priv->mmio->handle,
			0x440, (87 << 20) | (87 << 10) | (87));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(113 << 20) | (113 << 10) | (113));
		setmmioregister(dev_priv->mmio->handle,
			0x440, (135 << 20) | (135 << 10) | (135));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(170 << 20) | (170 << 10) | (170));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(199 << 20) | (199 << 10) | (199));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(246 << 20) | (246 << 10) | (246));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(284 << 20) | (284 << 10) | (284));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(317 << 20) | (317 << 10) | (317));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(347 << 20) | (347 << 10) | (347));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(373 << 20) | (373 << 10) | (373));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(398 << 20) | (398 << 10) | (398));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(442 << 20) | (442 << 10) | (442));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(481 << 20) | (481 << 10) | (481));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(517 << 20) | (517 << 10) | (517));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(550 << 20) | (550 << 10) | (550));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(609 << 20) | (609 << 10) | (609));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(662 << 20) | (662 << 10) | (662));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(709 << 20) | (709 << 10) | (709));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(753 << 20) | (753 << 10) | (753));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(794 << 20) | (794 << 10) | (794));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(832 << 20) | (832 << 10) | (832));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(868 << 20) | (868 << 10) | (868));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(902 << 20) | (902 << 10) | (902));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(934 << 20) | (934 << 10) | (934));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(966 << 20) | (966 << 10) | (966));
		setmmioregister(dev_priv->mmio->handle, 0x440,
			(996 << 20) | (996 << 10) | (996));


		/*
		For Interrupt Restore only All types of write through
		regsiters should be write header data to hardware at
		least before it can restore. H/W will automatically
		record the header to write through state buffer for
		resture usage.
		By Jaren:
		HParaType = 8'h03, HParaSubType = 8'h00
						8'h11
						8'h12
						8'h14
						8'h15
						8'h17
		HParaSubType 8'h12, 8'h15 is initialized.
		[HWLimit]
		1. All these write through registers can't be partial
		update.
		2. All these write through must be AGP command
		16 entries : 4 128-bit data */

		 /* Initialize INV_ParaSubType_TexPal  	 */
		setmmioregister(dev_priv->mmio->handle, 0x43C,
			(0x00030000 | 0x00000000));
		for (i = 0; i < 16; i++) {
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x00000000);
		}

		/* Initialize INV_ParaSubType_4X4Cof */
		/* 32 entries : 8 128-bit data */
		setmmioregister(dev_priv->mmio->handle, 0x43C,
			(0x00030000 | 0x11000000));
		for (i = 0; i < 32; i++) {
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x00000000);
		}

		/* Initialize INV_ParaSubType_StipPal */
		/* 5 entries : 2 128-bit data */
		setmmioregister(dev_priv->mmio->handle, 0x43C,
			(0x00030000 | 0x14000000));
		for (i = 0; i < (5+3); i++) {
			setmmioregister(dev_priv->mmio->handle,
				0x440, 0x00000000);
		}

		/* primitive setting & vertex format*/
		setmmioregister(dev_priv->mmio->handle, 0x43C,
			(0x00040000 | 0x14000000));
		for (i = 0; i < 52; i++) {
			setmmioregister(dev_priv->mmio->handle,
			0x440, ((unsigned int) i << 24));
		}
		setmmioregister(dev_priv->mmio->handle, 0x43C,
			0x00fe0000);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x4000840f);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x47000400);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x44000000);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x46000000);

		/* setting Misconfig*/
		setmmioregister(dev_priv->mmio->handle, 0x43C,
			0x00fe0000);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x00001004);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x0800004b);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x0a000049);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x0b0000fb);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x0c000001);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x0d0000cb);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x0e000009);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x10000000);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x110000ff);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x12000000);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x130000db);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x14000000);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x15000000);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x16000000);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x17000000);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x18000000);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x19000000);
		setmmioregister(dev_priv->mmio->handle, 0x440,
			0x20000000);
		} else if (dev_priv->chip_sub_index == CHIP_H6S2) {
			setmmioregister(dev_priv->mmio->handle, 0x43C,
				0x00010000);
			for (i = 0; i <= 0x9A; i++) {
				setmmioregister(dev_priv->mmio->handle, 0x440,
					(unsigned int) i << 24);
			}

			/* Initial Texture Stage Setting*/
			for (StageOfTexture = 0; StageOfTexture <= 0xf;
			StageOfTexture++) {
				setmmioregister(dev_priv->mmio->handle, 0x43C,
					(0x00020000 | 0x00000000 |
					(StageOfTexture & 0xf)<<24));
				for (i = 0 ; i <= 0x30 ; i++) {
					setmmioregister(dev_priv->mmio->handle,
					0x440, (unsigned int) i << 24);
				}
			}

			/* Initial Texture Sampler Setting*/
			for (StageOfTexture = 0; StageOfTexture <= 0xf;
			StageOfTexture++) {
				setmmioregister(dev_priv->mmio->handle, 0x43C,
					(0x00020000 | 0x20000000 |
					(StageOfTexture & 0xf)<<24));
				for (i = 0 ; i <= 0x36 ; i++) {
					setmmioregister(dev_priv->mmio->handle,
						0x440, (unsigned int) i << 24);
				}
			}

			setmmioregister(dev_priv->mmio->handle, 0x43C,
				(0x00020000 | 0xfe000000));
			for (i = 0 ; i <= 0x13 ; i++) {
				setmmioregister(dev_priv->mmio->handle, 0x440,
					(unsigned int) i << 24);
				/* *((unsigned int volatile*)(pMapIOPort+
				HC_REG_Hpara0)) =((unsigned int) i << 24);*/
			}

			/* Initial Gamma Table Setting*/
			/* Initial Gamma Table Setting*/
			/* 5 + 4 = 9 (12) dwords*/
			/* sRGB texture is not directly support by
			H3 hardware.*/
			/* We have to set the deGamma table for texture
			sampling.*/

			/* degamma table*/
			setmmioregister(dev_priv->mmio->handle, 0x43C,
				(0x00030000 | 0x15000000));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(0x40000000 | (30 << 20) | (15 << 10) | (5)));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				((119 << 20) | (81 << 10) | (52)));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				((283 << 20) | (219 << 10) | (165)));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				((535 << 20) | (441 << 10) | (357)));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				((119 << 20) | (884 << 20) | (757 << 10)
				| (640)));

			/* gamma table*/
			setmmioregister(dev_priv->mmio->handle, 0x43C,
				(0x00030000 | 0x17000000));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(0x40000000 | (13 << 20) | (13 << 10) | (13)));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(0x40000000 | (26 << 20) | (26 << 10) | (26)));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(0x40000000 | (39 << 20) | (39 << 10) | (39)));
			setmmioregister(dev_priv->mmio->handle,
				0x440, ((51 << 20) | (51 << 10) | (51)));
			setmmioregister(dev_priv->mmio->handle,
				0x440, ((71 << 20) | (71 << 10) | (71)));
			setmmioregister(dev_priv->mmio->handle,
				0x440, (87 << 20) | (87 << 10) | (87));
			setmmioregister(dev_priv->mmio->handle,
				0x440, (113 << 20) | (113 << 10) | (113));
			setmmioregister(dev_priv->mmio->handle,
				0x440, (135 << 20) | (135 << 10) | (135));
			setmmioregister(dev_priv->mmio->handle,
				0x440, (170 << 20) | (170 << 10) | (170));
			setmmioregister(dev_priv->mmio->handle,
				0x440, (199 << 20) | (199 << 10) | (199));
			setmmioregister(dev_priv->mmio->handle,
				0x440, (246 << 20) | (246 << 10) | (246));
			setmmioregister(dev_priv->mmio->handle,
				0x440, (284 << 20) | (284 << 10) | (284));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(317 << 20) | (317 << 10) | (317));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(347 << 20) | (347 << 10) | (347));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(373 << 20) | (373 << 10) | (373));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(398 << 20) | (398 << 10) | (398));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(442 << 20) | (442 << 10) | (442));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(481 << 20) | (481 << 10) | (481));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(517 << 20) | (517 << 10) | (517));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(550 << 20) | (550 << 10) | (550));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(609 << 20) | (609 << 10) | (609));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(662 << 20) | (662 << 10) | (662));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(709 << 20) | (709 << 10) | (709));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(753 << 20) | (753 << 10) | (753));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(794 << 20) | (794 << 10) | (794));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(832 << 20) | (832 << 10) | (832));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(868 << 20) | (868 << 10) | (868));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(902 << 20) | (902 << 10) | (902));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(934 << 20) | (934 << 10) | (934));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(966 << 20) | (966 << 10) | (966));
			setmmioregister(dev_priv->mmio->handle, 0x440,
				(996 << 20) | (996 << 10) | (996));


			/* For Interrupt Restore only
			All types of write through regsiters should be write
			header data to hardware at least before it can restore.
			H/W will automatically record the header to write
			through state buffer for restureusage.
			By Jaren:
			HParaType = 8'h03, HParaSubType = 8'h00
			     8'h11
			     8'h12
			     8'h14
			     8'h15
			     8'h17
			HParaSubType 8'h12, 8'h15 is initialized.
			[HWLimit]
			1. All these write through registers can't be partial
			update.
			2. All these write through must be AGP command
			16 entries : 4 128-bit data */

			/* Initialize INV_ParaSubType_TexPal  	 */
			setmmioregister(dev_priv->mmio->handle, 0x43C,
				(0x00030000 | 0x00000000));
			for (i = 0; i < 16; i++) {
				setmmioregister(dev_priv->mmio->handle, 0x440,
					0x00000000);
			}

			/* Initialize INV_ParaSubType_4X4Cof */
			/* 32 entries : 8 128-bit data */
			setmmioregister(dev_priv->mmio->handle, 0x43C,
				(0x00030000 | 0x11000000));
			for (i = 0; i < 32; i++) {
				setmmioregister(dev_priv->mmio->handle, 0x440,
					0x00000000);
			}

			/* Initialize INV_ParaSubType_StipPal */
			/* 5 entries : 2 128-bit data */
			setmmioregister(dev_priv->mmio->handle, 0x43C,
				(0x00030000 | 0x14000000));
			for (i = 0; i < (5+3); i++) {
				setmmioregister(dev_priv->mmio->handle, 0x440,
					0x00000000);
			}

			/* primitive setting & vertex format*/
			setmmioregister(dev_priv->mmio->handle, 0x43C,
			(0x00040000));
			for (i = 0; i <= 0x62; i++) {
				setmmioregister(dev_priv->mmio->handle, 0x440,
					((unsigned int) i << 24));
			}

			/*ParaType 0xFE - Configure and Misc Setting*/
			setmmioregister(dev_priv->mmio->handle, 0x43C,
			(0x00fe0000));
			for (i = 0; i <= 0x47; i++) {
				setmmioregister(dev_priv->mmio->handle, 0x440,
					((unsigned int) i << 24));
			}
			/*ParaType 0x11 - Frame Buffer Auto-Swapping and
			Command Regulator Misc*/
			setmmioregister(dev_priv->mmio->handle, 0x43C,
			(0x00110000));
			for (i = 0; i <= 0x20; i++) {
				setmmioregister(dev_priv->mmio->handle, 0x440,
					((unsigned int) i << 24));
			}
			setmmioregister(dev_priv->mmio->handle, 0x43C,
				0x00fe0000);
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x4000840f);
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x47000404);
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x44000000);
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x46000005);

			/* setting Misconfig*/
			setmmioregister(dev_priv->mmio->handle, 0x43C,
			0x00fe0000);
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x00001004);
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x08000249);
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x0a0002c9);
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x0b0002fb);
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x0c000000);
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x0d0002cb);
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x0e000009);
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x10000049);
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x110002ff);
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x12000008);
			setmmioregister(dev_priv->mmio->handle, 0x440,
				0x130002db);
		}
}

int  via_chrome9_drm_resume(struct pci_dev *pci)
{
	struct drm_device *dev = (struct drm_device *)pci_get_drvdata(pci);
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *)dev->dev_private;

	pci_restore_state(pci);

	if (!dev_priv->initialized)
		return 0;

	Initialize3DEngine(dev_priv);

	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_TRANS, 0x00110000);
	if (dev_priv->chip_sub_index == CHIP_H6S2) {
		setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN,
			0x06000000);
		setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN,
			0x07100000);
	} else{
		setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN,
			0x02000000);
		setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN,
			0x03100000);
	}


	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_TRANS,
	INV_ParaType_PreCR);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN,
	INV_SubA_HSetRBGID | INV_HSetRBGID_CR);

	if (dev_priv->chip_sub_index == CHIP_H6S2) {
		unsigned int i;
		/* Here restore SR66~SR6F SR79~SR7B */
		for (i = 0; i < 10; i++) {
			setmmioregisteru8(dev_priv->mmio->handle,
				0x83c4, 0x66 + i);
			setmmioregisteru8(dev_priv->mmio->handle,
				0x83c5, dev_priv->gti_backup[i]);
		}

		for (i = 0; i < 3; i++) {
			setmmioregisteru8(dev_priv->mmio->handle,
				0x83c4, 0x79 + i);
			setmmioregisteru8(dev_priv->mmio->handle,
			 0x83c5, dev_priv->gti_backup[10 + i]);
		}
	}

	via_chrome9_dma_init_inv(dev);

#if BRANCH_MECHANISM_ENABLE
	if (dev_priv->branch_buf_enabled) {
		unsigned long interrupt_status;
		unsigned int dwreg69;
		interrupt_status = getmmioregister(dev_priv->mmio->handle,
			InterruptControlReg);
		/* if hw gfx's interrupt has not been enabled,
		 * then enable it here */
		if (!(interrupt_status & InterruptEnable)) {
			/* enable hw gfx's interrupt */
			#if 0
			setmmioregister(dev_priv->mmio->handle,
					InterruptControlReg,
					interrupt_status|InterruptEnable);
			#else
			/* temporarily only enable CR's interrupt,
			 * disable all others */
			setmmioregister(dev_priv->mmio->handle,
					InterruptControlReg, InterruptEnable);
			#endif
		}

		/*enable agp branch buffer*/
		if ((dev_priv->chip_sub_index == CHIP_H6S1) ||
			(dev_priv->chip_sub_index == CHIP_H6S2)) {
			dwreg69 = 0x01;
			setmmioregister(dev_priv->mmio->handle,
					INV_REG_CR_TRANS, INV_ParaType_PreCR);
			setmmioregister(dev_priv->mmio->handle,
				INV_REG_CR_BEGIN, dwreg69);
		}
	}
#endif

	return 0;
}

int  via_chrome9_drm_suspend(struct pci_dev *pci,
	pm_message_t state)
{
	int i;
	unsigned char reg_tmp;
	struct drm_device *dev = (struct drm_device *)pci_get_drvdata(pci);
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *)dev->dev_private;

	pci_save_state(pci);

	if (!dev_priv->initialized)
		return 0;

	/* Save registers from SR66~SR6F */
	for (i = 0; i < 10; i++) {
		setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x66 + i);
		dev_priv->gti_backup[i] =
			getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
	}

	/* Save registers from SR79~SR7B */
	for (i = 0; i < 3; i++) {
		setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x79 + i);
		dev_priv->gti_backup[10 + i] =
			getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
	}

	/* Patch for VX800 S3 hang issue
	 * VX800 may load hardware hang while doing suspend
	 * need to close IGA and screen
	 * */
	if( dev_priv->chip_sub_index == CHIP_H6S2 ) {
		setmmioregisteru8(dev_priv->mmio->handle, 0x83d4, 0x17);
		reg_tmp = getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
		setmmioregisteru8(dev_priv->mmio->handle, 0x83d5, reg_tmp & 0x7f);
	
		setmmioregisteru8(dev_priv->mmio->handle, 0x83d4, 0x6A);
		reg_tmp = getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
		setmmioregisteru8(dev_priv->mmio->handle, 0x83d5, reg_tmp & 0x77);
	
		setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x01);
		reg_tmp = getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
		setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, reg_tmp & 0x20);
	
		setmmioregisteru8(dev_priv->mmio->handle, 0x83d4, 0x6B);
		reg_tmp = getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
		setmmioregisteru8(dev_priv->mmio->handle, 0x83d5, reg_tmp & 0x06);
	}

	return 0;
}

int via_chrome9_driver_load(struct drm_device *dev,
	unsigned long chipset)
{
	struct drm_via_chrome9_private *dev_priv;
	int ret = 0;
	static int associate;

	if (!associate) {
		pci_set_drvdata(dev->pdev, dev);
		dev->pdev->driver = &dev->driver->pci_driver;
		associate = 1;
	}

	dev->counters += 4;
	dev->types[6] = _DRM_STAT_IRQ;
	dev->types[7] = _DRM_STAT_PRIMARY;
	dev->types[8] = _DRM_STAT_SECONDARY;
	dev->types[9] = _DRM_STAT_DMA;

	dev_priv = drm_calloc_large(1, sizeof(struct drm_via_chrome9_private));
	if (dev_priv == NULL)
		return -ENOMEM;

	/* Clear */
	memset(dev_priv, 0, sizeof(struct drm_via_chrome9_private));

	dev_priv->dev = dev;
	dev->dev_private = (void *)dev_priv;

	ret = drm_sman_init(&dev_priv->sman, 2, 12, 8);
	if (ret)
		drm_free_large(dev_priv);
	return ret;
}

int via_chrome9_driver_unload(struct drm_device *dev)
{
	struct drm_via_chrome9_private *dev_priv = dev->dev_private;

	drm_sman_takedown(&dev_priv->sman);

	drm_free_large(dev_priv);

	dev->dev_private = 0;

	return 0;
}

static int via_chrome9_initialize(struct drm_device *dev,
	struct drm_via_chrome9_init *init, struct drm_file *file_priv)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *)dev->dev_private;

	if (!dev_priv)
		return -EINVAL;

	if (dev_priv->initialized)
		return 0;

	dev_priv->chip_agp = init->chip_agp;
	dev_priv->chip_sub_index = init->chip_sub_index;

	dev_priv->usec_timeout = init->usec_timeout;
	dev_priv->front_offset = init->front_offset;
	dev_priv->back_offset = init->back_offset >>
		VIA_CHROME9DRM_VIDEO_STARTADDRESS_ALIGNMENT <<
		VIA_CHROME9DRM_VIDEO_STARTADDRESS_ALIGNMENT;
	dev_priv->available_fb_size = init->available_fb_size -
		(init->available_fb_size %
		(1 << VIA_CHROME9DRM_VIDEO_STARTADDRESS_ALIGNMENT));
	dev_priv->depth_offset = init->depth_offset;

	/* Find all the map added first, doing this is necessary to
	intialize hw */
	if (via_chrome9_map_init(dev, init)) {
		DRM_ERROR("function via_chrome9_map_init ERROR !\n");
		goto error;
	}

	/* Necessary information has been gathered for initialize hw */
	if (via_chrome9_hw_init(dev, init)) {
		DRM_ERROR("function via_chrome9_hw_init ERROR !\n");
		goto error;
	}

	/* After hw intialization, we have kown whether to use agp
	or to use pcie for texture */
	if (via_chrome9_heap_management_init(dev, init)) {
		DRM_ERROR("function \
			via_chrome9_heap_management_init ERROR !\n");
		goto error;
	}

	/* the agp branch buffer mechanism need further initialization of HW and
	   allocate reserved dma buffer for it works
	  */
#if BRANCH_MECHANISM_ENABLE
	if (via_chrome9_branch_buf_init(dev, init, file_priv)) {
		DRM_ERROR("function \
			via_chrome9_branch_buf_init ERROR !\n");
	} else
		DRM_INFO("Branch Buffer mechanism enabled !\n");
#endif

	dev_priv->initialized = 1;

	return 0;

error:
	/* all the error recover has been processed in relevant function,
	so here just return error */
	return -EINVAL;
}

static void via_chrome9_cleanup(struct drm_device *dev,
	struct drm_via_chrome9_init *init, struct drm_file *file_priv)
{
	struct drm_via_chrome9_dma_manager *lpcmdmamanager = NULL;
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *)dev->dev_private;
	DRM_DEBUG("function via_chrome9_cleanup run!\n");

	if (!dev_priv || (!dev_priv->initialized))
		return ;

#if BRANCH_MECHANISM_ENABLE
	if (via_chrome9_branch_buf_cleanup(dev, init, file_priv)) {
		DRM_ERROR("function \
			via_chrome9_branch_buf_cleanup ERROR !\n");
	}
#endif
	lpcmdmamanager =
		(struct drm_via_chrome9_dma_manager *)dev_priv->dma_manager;
	if (dev_priv->pcie_vmalloc_nocache) {
		dev_priv->pcie_vmalloc_nocache = 0;
		if (lpcmdmamanager)
			lpcmdmamanager->addr_linear = NULL;
	}

	if (dev->sg) {
		via_chrome9_drm_sg_cleanup(dev->sg);
		dev->sg = NULL;
	}

	if (dev_priv->pagetable_map.pagetable_handle) {
		iounmap(dev_priv->pagetable_map.pagetable_handle);
		dev_priv->pagetable_map.pagetable_handle = NULL;
	}

	if (lpcmdmamanager && lpcmdmamanager->addr_linear) {
		iounmap(lpcmdmamanager->addr_linear);
		lpcmdmamanager->addr_linear = NULL;
	}

	kfree(lpcmdmamanager);
	dev_priv->dma_manager = NULL;

	if (dev_priv->event_tag_info) {
		vfree(dev_priv->event_tag_info);
		dev_priv->event_tag_info = NULL;
	}

	if (dev_priv->bci_buffer) {
		vfree(dev_priv->bci_buffer);
		dev_priv->bci_buffer = NULL;
	}

	via_chrome9_memory_destroy_heap(dev, dev_priv);

	/* After cleanup, it should to set the value to null */
	dev_priv->sarea = dev_priv->mmio = dev_priv->hostBlt =
	dev_priv->fb = dev_priv->shadow_map.shadow = 0;
	dev_priv->sarea_priv = 0;
	dev_priv->initialized = 0;
}

/*
Do almost everything intialize here,include:
1.intialize all addmaps in private data structure
2.intialize memory heap management for video agp/pcie
3.intialize hw for dma(pcie/agp) function

Note:all this function will dispatch into relevant function
*/
int via_chrome9_ioctl_init(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	struct drm_via_chrome9_init *init = (struct drm_via_chrome9_init *)data;

	switch (init->func) {
	case VIA_CHROME9_INIT:
		if (via_chrome9_initialize(dev, init, file_priv)) {
			DRM_ERROR("function via_chrome9_initialize error\n");
			return -1;
		}
		via_chrome9_filepriv_v4l = (void *)file_priv;
		via_chrome9_dev_v4l = (void *)dev;
		break;

	case VIA_CHROME9_CLEANUP:
		via_chrome9_cleanup(dev, init, file_priv);
		via_chrome9_filepriv_v4l = 0;
		via_chrome9_dev_v4l = 0;
		break;

	default:
		return -1;
	}

	return 0;
}

int via_chrome9_ioctl_allocate_event_tag(struct drm_device *dev,
	void *data, struct drm_file *file_priv)
{
	struct drm_via_chrome9_event_tag *event_tag = data;
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *)dev->dev_private;
	struct drm_clb_event_tag_info *event_tag_info =
		dev_priv->event_tag_info;
	unsigned int *event_addr = 0, i = 0;

	for (i = 0; i < NUMBER_OF_EVENT_TAGS; i++) {
		if (!event_tag_info->usage[i])
			break;
	}

	if (i < NUMBER_OF_EVENT_TAGS) {
		event_tag_info->usage[i] = 1;
		event_tag->event_offset = i;
		event_tag->last_sent_event_value.event_low = 0;
		event_tag->current_event_value.event_low = 0;
		event_addr = event_tag_info->linear_address +
		event_tag->event_offset * 4;
		*event_addr = 0;
		return 0;
	} else {
		return -7;
	}

	return 0;
}

int via_chrome9_ioctl_free_event_tag(struct drm_device *dev,
	void *data, struct drm_file *file_priv)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *)dev->dev_private;
	struct drm_clb_event_tag_info *event_tag_info =
		dev_priv->event_tag_info;
	struct drm_via_chrome9_event_tag *event_tag = data;

	event_tag_info->usage[event_tag->event_offset] = 0;
	return 0;
}

void via_chrome9_lastclose(struct drm_device *dev)
{
	via_chrome9_cleanup(dev, 0, 0);
	return ;
}

static int via_chrome9_do_wait_vblank(struct drm_via_chrome9_private
		*dev_priv)
{
	int i;

	for (i = 0; i < dev_priv->usec_timeout; i++) {
		VIA_CHROME9_WRITE8(0x83d4, 0x34);
		if ((VIA_CHROME9_READ8(0x83d5)) & 0x8)
			return 0;
		__via_chrome9ke_udelay(1);
	}

	return -1;
}

void via_chrome9_preclose(struct drm_device *dev, struct drm_file *file_priv)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_sarea *sarea_priv = NULL;

	if (!dev_priv)
		return ;

	sarea_priv = dev_priv->sarea_priv;
	if (!sarea_priv)
		return ;

	if ((sarea_priv->page_flip == 1) &&
		(sarea_priv->current_page != VIA_CHROME9_FRONT)) {
		__volatile__ unsigned long *bci_base;
		if (via_chrome9_do_wait_vblank(dev_priv))
			return;

		bci_base = (__volatile__ unsigned long *)(dev_priv->bci);

		bci_set_stream_register(bci_base, 0x81c4, 0xc0000000);
		bci_set_stream_register(bci_base, 0x81c0,
			dev_priv->front_offset);
		bci_send(bci_base, 0x64000000);/* wait vsync */

		sarea_priv->current_page = VIA_CHROME9_FRONT;
	}
}

int via_chrome9_is_agp(struct drm_device *dev)
{
	/* filter out pcie group which has no AGP device */
	if (dev->pci_device == 0x1122 || dev->pci_device == 0x5122) {
		dev->driver->driver_features &=
		~(DRIVER_USE_AGP | DRIVER_USE_MTRR | DRIVER_REQUIRE_AGP);
		return 0;
	}
	return 1;
}

irqreturn_t via_chrome9_interrupt(int irq, void *dev_instance)
{
	struct drm_device *dev = dev_instance;
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	unsigned long interrupt_status2;
	unsigned int fence_id_readback;

	interrupt_status2 = getmmioregister(dev_priv->mmio->handle,
		InterruptCtlReg2);
	if (interrupt_status2 & CR_Int_Status) {
		if ((dev_priv->chip_sub_index == CHIP_H5) ||
			(dev_priv->chip_sub_index == CHIP_H5S1))
			/* clear the CR interrupt status flag,
			 * write 1 to clear ! */
			setmmioregister(dev_priv->mmio->handle,
				InterruptCtlReg2, interrupt_status2 |
				CR_Int_Status);

		if ((dev_priv->chip_sub_index == CHIP_H6S1) ||
			(dev_priv->chip_sub_index == CHIP_H6S2)) {
			/* Hardware depend on address
			* switch to clean interrupt */
			fence_id_readback =
				getmmioregister(dev_priv->mmio->handle,
				dev_priv->fenceid_readback_register);
			fence_id_readback =
				getmmioregister(dev_priv->mmio->handle,
				dev_priv->fenceid_readback_register + 8);
		}

#if BRANCH_AGP_DEBUG
		dev_priv->branch_irqcount++;
#endif
		/* schedule workqueue for bottom half process */
#if BRANCH_PRIVATE_WORKQUEUE
		if (dev_priv->branch_buf_wq) {
			queue_work(dev_priv->branch_buf_wq,
				&dev_priv->branch_buf_work);
		} else {
			schedule_work(&dev_priv->branch_buf_work);
		}
#else
		schedule_work(&dev_priv->branch_buf_work);
#endif
	}

	return (interrupt_status2 & CR_Int_Status) ? IRQ_HANDLED : IRQ_NONE;
}


