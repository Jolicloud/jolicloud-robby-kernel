/*
 * Copyright © 2006-2007 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file lvds definitions and structures.
 */

#define BLC_I2C_TYPE 0x01
#define BLC_PWM_TYPE 0x02
#define BRIGHTNESS_MASK 0xff
#define BRIGHTNESS_MAX_LEVEL 100
#define BLC_POLARITY_NORMAL 0
#define BLC_POLARITY_INVERSE 1
#define BACKLIGHT_PWM_POLARITY_BIT_CLEAR (0xfffe)
#define BACKLIGHT_PWM_CTL_SHIFT (16)
#define BLC_MAX_PWM_REG_FREQ 0xfffe
#define BLC_MIN_PWM_REG_FREQ 0x2
#define BLC_PWM_LEGACY_MODE_ENABLE 0x0001
#define BLC_PWM_PRECISION_FACTOR 10//10000000 
#define BLC_PWM_FREQ_CALC_CONSTANT 32 
#define MHz 1000000 
#define OFFSET_OPREGION_VBT	0x400	

typedef struct OpRegion_Header
{
	char sign[16];
	u32 size;
	u32 over;
	char sver[32];
	char vver[16];
	char gver[16];
	u32 mbox;
	char rhd1[164];
} OpRegionRec, *OpRegionPtr;

struct vbt_header
{
	char signature[20];		/**< Always starts with 'VBT$' */
	u16 version;			/**< decimal */
	u16 header_size;		/**< in bytes */
	u16 vbt_size;			/**< in bytes */
	u8 vbt_checksum;
	u8 reserved0;
	u32 bdb_offset;			/**< from beginning of VBT */
	u32 aim1_offset;		/**< from beginning of VBT */
	u32 aim2_offset;		/**< from beginning of VBT */
	u32 aim3_offset;		/**< from beginning of VBT */
	u32 aim4_offset;		/**< from beginning of VBT */
} __attribute__ ((packed));

struct bdb_header
{
	char signature[16];		/**< Always 'BIOS_DATA_BLOCK' */
	u16 version;			/**< decimal */
	u16 header_size;		/**< in bytes */
	u16 bdb_size;			/**< in bytes */
} __attribute__ ((packed));	

#define LVDS_CAP_EDID			(1 << 6)
#define LVDS_CAP_DITHER			(1 << 5)
#define LVDS_CAP_PFIT_AUTO_RATIO	(1 << 4)
#define LVDS_CAP_PFIT_GRAPHICS_MODE	(1 << 3)
#define LVDS_CAP_PFIT_TEXT_MODE		(1 << 2)
#define LVDS_CAP_PFIT_GRAPHICS		(1 << 1)
#define LVDS_CAP_PFIT_TEXT		(1 << 0)
struct lvds_bdb_1
{
	u8 id;				/**< 40 */
	u16 size;
	u8 panel_type;
	u8 reserved0;
	u16 caps;
} __attribute__ ((packed));

struct lvds_bdb_2_fp_params
{
	u16 x_res;
	u16 y_res;
	u32 lvds_reg;
	u32 lvds_reg_val;
	u32 pp_on_reg;
	u32 pp_on_reg_val;
	u32 pp_off_reg;
	u32 pp_off_reg_val;
	u32 pp_cycle_reg;
	u32 pp_cycle_reg_val;
	u32 pfit_reg;
	u32 pfit_reg_val;
	u16 terminator;
} __attribute__ ((packed));

struct lvds_bdb_2_fp_edid_dtd
{
	u16 dclk;		/**< In 10khz */
	u8 hactive;
	u8 hblank;
	u8 high_h;		/**< 7:4 = hactive 11:8, 3:0 = hblank 11:8 */
	u8 vactive;
	u8 vblank;
	u8 high_v;		/**< 7:4 = vactive 11:8, 3:0 = vblank 11:8 */
	u8 hsync_off;
	u8 hsync_pulse_width;
	u8 vsync_off;
	u8 high_hsync_off;	/**< 7:6 = hsync off 9:8 */
	u8 h_image;
	u8 v_image;
	u8 max_hv;
	u8 h_border;
	u8 v_border;
	u8 flags;
#define FP_EDID_FLAG_VSYNC_POSITIVE	(1 << 2)
#define FP_EDID_FLAG_HSYNC_POSITIVE	(1 << 1)
} __attribute__ ((packed));

struct lvds_bdb_2_entry
{
	u16 fp_params_offset;		/**< From beginning of BDB */
	u8 fp_params_size;
	u16 fp_edid_dtd_offset;
	u8 fp_edid_dtd_size;
	u16 fp_edid_pid_offset;
	u8 fp_edid_pid_size;
} __attribute__ ((packed));

struct lvds_bdb_2
{
	u8 id;				/**< 41 */
	u16 size;
	u8 table_size;		       /* not sure on this one */
	struct lvds_bdb_2_entry panels[16];
} __attribute__ ((packed));


struct lvds_bdb_blc
{
	u8 id;				/**< 43 */
	u16 size;
	u8 table_size;
} __attribute__ ((packed));

struct lvds_blc
{
	u8 type:2;
	u8 pol:1;
	u8 gpio:3;
	u8 gmbus:2;
	u16 freq;
	u8 minbrightness;
	u8 i2caddr;
	u8 brightnesscmd;
	/* more... */
} __attribute__ ((packed));

