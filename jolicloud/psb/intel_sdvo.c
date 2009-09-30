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
 * Authors:
 *	Eric Anholt <eric@anholt.net>
 */
/*
 * Copyright 2006 Dave Airlie <airlied@linux.ie>
 *   Jesse Barnes <jesse.barnes@intel.com>
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include "drm_crtc.h"
#include "intel_sdvo_regs.h"

#include <linux/proc_fs.h>
#include <linux/wait.h>

struct proc_dir_entry *proc_sdvo_dir = NULL;
wait_queue_head_t hotplug_queue;

#define MAX_VAL 1000
#define DPLL_CLOCK_PHASE_9 (1<<9 | 1<<12)

#define PCI_PORT5_REG80_FFUSE				0xD0058000
#define PCI_PORT5_REG80_SDVO_DISABLE		0x0020

#define SII_1392_WA
#ifdef SII_1392_WA
int SII_1392=0;
extern int drm_psb_no_fb;
#endif

typedef struct _EXTVDATA
{
    u32 Value;
    u32 Default;
    u32 Min;
    u32 Max;
    u32 Step;		       // arbitrary unit (e.g. pixel, percent) returned during VP_COMMAND_GET
} EXTVDATA, *PEXTVDATA;

typedef struct _sdvo_display_params
{
    EXTVDATA FlickerFilter;	       /* Flicker Filter : for TV onl */
    EXTVDATA AdaptiveFF;	       /* Adaptive Flicker Filter : for TV onl */
    EXTVDATA TwoD_FlickerFilter;       /* 2D Flicker Filter : for TV onl */
    EXTVDATA Brightness;	       /* Brightness : for TV & CRT onl */
    EXTVDATA Contrast;		       /* Contrast : for TV & CRT onl */
    EXTVDATA PositionX;		       /* Horizontal Position : for all device */
    EXTVDATA PositionY;		       /* Vertical Position : for all device */
    /*EXTVDATA    OverScanX;         Horizontal Overscan : for TV onl */
    EXTVDATA DotCrawl;		       /* Dot crawl value : for TV onl */
    EXTVDATA ChromaFilter;	       /* Chroma Filter : for TV onl */
    /* EXTVDATA    OverScanY;        Vertical Overscan : for TV onl */
    EXTVDATA LumaFilter;	       /* Luma Filter : for TV only */
    EXTVDATA Sharpness;		       /* Sharpness : for TV & CRT onl */
    EXTVDATA Saturation;	       /* Saturation : for TV & CRT onl */
    EXTVDATA Hue;		       /* Hue : for TV & CRT onl */
    EXTVDATA Dither;		       /* Dither : For LVDS onl */
} sdvo_display_params;

typedef enum _SDVO_PICTURE_ASPECT_RATIO_T
{
    UAIM_PAR_NO_DATA = 0x00000000,
    UAIM_PAR_4_3 = 0x00000100,
    UAIM_PAR_16_9 = 0x00000200,
    UAIM_PAR_FUTURE = 0x00000300,
    UAIM_PAR_MASK = 0x00000300,
} SDVO_PICTURE_ASPECT_RATIO_T;

typedef enum _SDVO_FORMAT_ASPECT_RATIO_T
{
    UAIM_FAR_NO_DATA = 0x00000000,
    UAIM_FAR_SAME_AS_PAR = 0x00002000,
    UAIM_FAR_4_BY_3_CENTER = 0x00002400,
    UAIM_FAR_16_BY_9_CENTER = 0x00002800,
    UAIM_FAR_14_BY_9_CENTER = 0x00002C00,
    UAIM_FAR_16_BY_9_LETTERBOX_TOP = 0x00000800,
    UAIM_FAR_14_BY_9_LETTERBOX_TOP = 0x00000C00,
    UAIM_FAR_GT_16_BY_9_LETTERBOX_CENTER = 0x00002000,
    UAIM_FAR_4_BY_3_SNP_14_BY_9_CENTER = 0x00003400,	/* With shoot and protect 14:9 cente */
    UAIM_FAR_16_BY_9_SNP_14_BY_9_CENTER = 0x00003800,	/* With shoot and protect 14:9 cente */
    UAIM_FAR_16_BY_9_SNP_4_BY_3_CENTER = 0x00003C00,	/* With shoot and protect 4:3 cente */
    UAIM_FAR_MASK = 0x00003C00,
} SDVO_FORMAT_ASPECT_RATIO_T;

// TV image aspect ratio
typedef enum _CP_IMAGE_ASPECT_RATIO
{
    CP_ASPECT_RATIO_FF_4_BY_3 = 0,
    CP_ASPECT_RATIO_14_BY_9_CENTER = 1,
    CP_ASPECT_RATIO_14_BY_9_TOP = 2,
    CP_ASPECT_RATIO_16_BY_9_CENTER = 3,
    CP_ASPECT_RATIO_16_BY_9_TOP = 4,
    CP_ASPECT_RATIO_GT_16_BY_9_CENTER = 5,
    CP_ASPECT_RATIO_FF_4_BY_3_PROT_CENTER = 6,
    CP_ASPECT_RATIO_FF_16_BY_9_ANAMORPHIC = 7,
} CP_IMAGE_ASPECT_RATIO;

typedef struct _SDVO_ANCILLARY_INFO_T
{
    CP_IMAGE_ASPECT_RATIO AspectRatio;
    u32 RedistCtrlFlag;	       /* Redistribution control flag (get and set */
} SDVO_ANCILLARY_INFO_T, *PSDVO_ANCILLARY_INFO_T;

struct intel_sdvo_priv {
	struct intel_i2c_chan *i2c_bus;
	int slaveaddr;
	int output_device;

	u16 active_outputs;

	struct intel_sdvo_caps caps;
	int pixel_clock_min, pixel_clock_max;

	int save_sdvo_mult;
	u16 save_active_outputs;
	struct intel_sdvo_dtd save_input_dtd_1, save_input_dtd_2;
	struct intel_sdvo_dtd save_output_dtd[16];
	u32 save_SDVOX;
	/**
	* SDVO TV encoder support
	*/
    u32 ActiveDevice;	       /* CRT, TV, LVDS, TMDS */
    u32 TVStandard;		       /* PAL, NTSC */
    int TVOutput;		       /* S-Video, CVBS,YPbPr,RGB */
    int TVMode;			       /* SDTV/HDTV/SECAM mod */
    u32 TVStdBitmask;
    u32 dwSDVOHDTVBitMask;
    u32 dwSDVOSDTVBitMask;
    u8 byInputWiring;
    bool bGetClk;
    u32 dwMaxDotClk;
    u32 dwMinDotClk;

    u32 dwMaxInDotClk;
    u32 dwMinInDotClk;

    u32 dwMaxOutDotClk;
    u32 dwMinOutDotClk;
    u32 dwSupportedEnhancements;
    EXTVDATA OverScanY;		       /* Vertical Overscan : for TV onl */
    EXTVDATA OverScanX;		       /* Horizontal Overscan : for TV onl */
    sdvo_display_params dispParams;
    SDVO_ANCILLARY_INFO_T AncillaryInfo;	
};

/* Define TV mode type */
/* The full set are defined in xf86str.h*/
#define M_T_TV 0x80

typedef struct _tv_mode_t
{
    /* the following data is detailed mode information as it would be passed to the hardware: */
    struct drm_display_mode mode_entry;
    u32 dwSupportedSDTVvss;
    u32 dwSupportedHDTVvss;
    bool m_preferred;
    bool isTVMode;
} tv_mode_t;

static tv_mode_t tv_modes[] = {
    {
     .mode_entry =
     {DRM_MODE("800x600", DRM_MODE_TYPE_DRIVER | M_T_TV, 0x2625a00 / 1000, 800, 840, 968, 1056, 0,
      600, 601,
      604, 628, 0, V_PHSYNC | V_PVSYNC)},
     .dwSupportedSDTVvss = TVSTANDARD_SDTV_ALL,
     .dwSupportedHDTVvss = TVSTANDARD_HDTV_ALL,
     .m_preferred = TRUE,
     .isTVMode = TRUE,
     },
    {
     .mode_entry =
     {DRM_MODE("1024x768", DRM_MODE_TYPE_DRIVER | M_T_TV, 0x3dfd240 / 1000, 1024, 0x418, 0x49f, 0x540,
      0, 768,
      0x303, 0x308, 0x325, 0, V_PHSYNC | V_PVSYNC)},
     .dwSupportedSDTVvss = TVSTANDARD_SDTV_ALL,
     .dwSupportedHDTVvss = TVSTANDARD_HDTV_ALL,
     .m_preferred = FALSE,
     .isTVMode = TRUE,
     },
    {
     .mode_entry =
     {DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER | M_T_TV, 0x1978ff0 / 1000, 720, 0x2e1, 0x326, 0x380, 0,
      480,
      0x1f0, 0x1e1, 0x1f1, 0, V_PHSYNC | V_PVSYNC)},
     .dwSupportedSDTVvss =
     TVSTANDARD_NTSC_M | TVSTANDARD_NTSC_M_J | TVSTANDARD_NTSC_433,
     .dwSupportedHDTVvss = 0x0,
     .m_preferred = FALSE,
     .isTVMode = TRUE,
     },
    {
     /*Modeline "720x576_SDVO"   0.96 720 756 788 864  576 616 618 700 +vsync  */
     .mode_entry =
     {DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER | M_T_TV, 0x1f25a20 / 1000, 720, 756, 788, 864, 0, 576,
      616,
      618, 700, 0, V_PHSYNC | V_PVSYNC)},
     .dwSupportedSDTVvss =
     (TVSTANDARD_PAL_B | TVSTANDARD_PAL_D | TVSTANDARD_PAL_H |
      TVSTANDARD_PAL_I | TVSTANDARD_PAL_N | TVSTANDARD_SECAM_B |
      TVSTANDARD_SECAM_D | TVSTANDARD_SECAM_G | TVSTANDARD_SECAM_H |
      TVSTANDARD_SECAM_K | TVSTANDARD_SECAM_K1 | TVSTANDARD_SECAM_L |
      TVSTANDARD_PAL_G | TVSTANDARD_SECAM_L1),
     .dwSupportedHDTVvss = 0x0,
     .m_preferred = FALSE,
     .isTVMode = TRUE,
     },
    {
     .mode_entry =
     {DRM_MODE("1280x720@60",DRM_MODE_TYPE_DRIVER | M_T_TV, 74250000 / 1000, 1280, 1390, 1430, 1650, 0,
      720,
      725, 730, 750, 0, V_PHSYNC | V_PVSYNC)},
     .dwSupportedSDTVvss = 0x0,
     .dwSupportedHDTVvss = HDTV_SMPTE_296M_720p60,
     .m_preferred = FALSE,
     .isTVMode = TRUE,
     },
    {
     .mode_entry =
     {DRM_MODE("1280x720@50", DRM_MODE_TYPE_DRIVER | M_T_TV, 74250000 / 1000, 1280, 1720, 1759, 1980, 0,
      720,
      725, 730, 750, 0, V_PHSYNC | V_PVSYNC)},
     .dwSupportedSDTVvss = 0x0,
     .dwSupportedHDTVvss = HDTV_SMPTE_296M_720p50,
     .m_preferred = FALSE,
     .isTVMode = TRUE,
     },
    {
     .mode_entry =
     {DRM_MODE("1920x1080@60", DRM_MODE_TYPE_DRIVER | M_T_TV, 148500000 / 1000, 1920, 2008, 2051, 2200, 0,
      1080,
      1084, 1088, 1124, 0, V_PHSYNC | V_PVSYNC)},
     .dwSupportedSDTVvss = 0x0,
     .dwSupportedHDTVvss = HDTV_SMPTE_274M_1080i60,
     .m_preferred = FALSE,
     .isTVMode = TRUE,
     },
};

#define NUM_TV_MODES sizeof(tv_modes) / sizeof (tv_modes[0])

typedef struct {
    /* given values */    
    int n;
    int m1, m2;
    int p1, p2;
    /* derived values */
    int	dot;
    int	vco;
    int	m;
    int	p;
} ex_intel_clock_t;


/**
 * Writes the SDVOB or SDVOC with the given value, but always writes both
 * SDVOB and SDVOC to work around apparent hardware issues (according to
 * comments in the BIOS).
 */
static void intel_sdvo_write_sdvox(struct drm_output *output, u32 val)
{
	struct drm_device *dev = output->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv   *sdvo_priv = intel_output->dev_priv;
	u32 bval = val, cval = val;
	int i;

	if (sdvo_priv->output_device == SDVOB)
		cval = I915_READ(SDVOC);
	else
		bval = I915_READ(SDVOB);
	/*
	 * Write the registers twice for luck. Sometimes,
	 * writing them only once doesn't appear to 'stick'.
	 * The BIOS does this too. Yay, magic
	 */
	for (i = 0; i < 2; i++)
	{
		I915_WRITE(SDVOB, bval);
		I915_READ(SDVOB);
		I915_WRITE(SDVOC, cval);
		I915_READ(SDVOC);
	}
}

static bool intel_sdvo_read_byte(struct drm_output *output, u8 addr,
				 u8 *ch)
{
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;
	u8 out_buf[2];
	u8 buf[2];
	int ret;

	struct i2c_msg msgs[] = {
		{ 
			.addr = sdvo_priv->i2c_bus->slave_addr,
			.flags = 0,
			.len = 1,
			.buf = out_buf,
		}, 
		{
			.addr = sdvo_priv->i2c_bus->slave_addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = buf,
		}
	};

	out_buf[0] = addr;
	out_buf[1] = 0;

	if ((ret = i2c_transfer(&sdvo_priv->i2c_bus->adapter, msgs, 2)) == 2)
	{
//		DRM_DEBUG("got back from addr %02X = %02x\n", out_buf[0], buf[0]); 
		*ch = buf[0];
		return true;
	}

	DRM_DEBUG("i2c transfer returned %d\n", ret);
	return false;
}


#if 0
static bool intel_sdvo_read_byte_quiet(struct drm_output *output, int addr,
				       u8 *ch)
{
	return true;

}
#endif

static bool intel_sdvo_write_byte(struct drm_output *output, int addr,
				  u8 ch)
{
	struct intel_output *intel_output = output->driver_private;
	u8 out_buf[2];
	struct i2c_msg msgs[] = {
		{ 
			.addr = intel_output->i2c_bus->slave_addr,
			.flags = 0,
			.len = 2,
			.buf = out_buf,
		}
	};

	out_buf[0] = addr;
	out_buf[1] = ch;

	if (i2c_transfer(&intel_output->i2c_bus->adapter, msgs, 1) == 1)
	{
		return true;
	}
	return false;
}

#define SDVO_CMD_NAME_ENTRY(cmd) {cmd, #cmd}
/** Mapping of command numbers to names, for debug output */
const static struct _sdvo_cmd_name {
    u8 cmd;
    char *name;
} sdvo_cmd_names[] = {
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_RESET),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_DEVICE_CAPS),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_FIRMWARE_REV),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_TRAINED_INPUTS),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_ACTIVE_OUTPUTS),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_SET_ACTIVE_OUTPUTS),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_IN_OUT_MAP),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_SET_IN_OUT_MAP),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_ATTACHED_DISPLAYS),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_HOT_PLUG_SUPPORT),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_SET_ACTIVE_HOT_PLUG),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_ACTIVE_HOT_PLUG),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_INTERRUPT_EVENT_SOURCE),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_SET_TARGET_INPUT),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_SET_TARGET_OUTPUT),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_INPUT_TIMINGS_PART1),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_INPUT_TIMINGS_PART2),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_SET_INPUT_TIMINGS_PART1),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_SET_INPUT_TIMINGS_PART2),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_SET_INPUT_TIMINGS_PART1),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_SET_OUTPUT_TIMINGS_PART1),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_SET_OUTPUT_TIMINGS_PART2),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_OUTPUT_TIMINGS_PART1),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_OUTPUT_TIMINGS_PART2),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_CREATE_PREFERRED_INPUT_TIMING),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_PREFERRED_INPUT_TIMING_PART1),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_PREFERRED_INPUT_TIMING_PART2),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_INPUT_PIXEL_CLOCK_RANGE),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_OUTPUT_PIXEL_CLOCK_RANGE),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_SUPPORTED_CLOCK_RATE_MULTS),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_CLOCK_RATE_MULT),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_SET_CLOCK_RATE_MULT),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_SUPPORTED_TV_FORMATS),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_GET_TV_FORMAT),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_SET_TV_FORMAT),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_SET_TV_RESOLUTION_SUPPORT),
    SDVO_CMD_NAME_ENTRY(SDVO_CMD_SET_CONTROL_BUS_SWITCH),
};

#define SDVO_NAME(dev_priv) ((dev_priv)->output_device == SDVOB ? "SDVOB" : "SDVOC")
#define SDVO_PRIV(output)   ((struct intel_sdvo_priv *) (output)->dev_priv)

static void intel_sdvo_write_cmd(struct drm_output *output, u8 cmd,
				 void *args, int args_len)
{
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;
	int i;

        if (drm_debug) {
                DRM_DEBUG("%s: W: %02X ", SDVO_NAME(sdvo_priv), cmd);
                for (i = 0; i < args_len; i++)
                        printk("%02X ", ((u8 *)args)[i]);
                for (; i < 8; i++)
                        printk("   ");
                for (i = 0; i < sizeof(sdvo_cmd_names) / sizeof(sdvo_cmd_names[0]); i++) {
                        if (cmd == sdvo_cmd_names[i].cmd) {
                                printk("(%s)", sdvo_cmd_names[i].name);
                                break;
                        }
                }
                if (i == sizeof(sdvo_cmd_names)/ sizeof(sdvo_cmd_names[0]))
                        printk("(%02X)",cmd);
                printk("\n");
        }
                        
	for (i = 0; i < args_len; i++) {
		intel_sdvo_write_byte(output, SDVO_I2C_ARG_0 - i, ((u8*)args)[i]);
	}

	intel_sdvo_write_byte(output, SDVO_I2C_OPCODE, cmd);
}

static const char *cmd_status_names[] = {
	"Power on",
	"Success",
	"Not supported",
	"Invalid arg",
	"Pending",
	"Target not specified",
	"Scaling not supported"
};

static u8 intel_sdvo_read_response(struct drm_output *output, void *response,
				   int response_len)
{
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;
	int i;
	u8 status;
	u8 retry = 50;

	while (retry--) {
		/* Read the command response */
		for (i = 0; i < response_len; i++) {
			intel_sdvo_read_byte(output, SDVO_I2C_RETURN_0 + i,
				     &((u8 *)response)[i]);
		}

		/* read the return status */
		intel_sdvo_read_byte(output, SDVO_I2C_CMD_STATUS, &status);

	        if (drm_debug) {
			DRM_DEBUG("%s: R: ", SDVO_NAME(sdvo_priv));
       			for (i = 0; i < response_len; i++)
                        	printk("%02X ", ((u8 *)response)[i]);
                	for (; i < 8; i++)
                        	printk("   ");
                	if (status <= SDVO_CMD_STATUS_SCALING_NOT_SUPP)
                        	printk("(%s)", cmd_status_names[status]);
                	else
                        	printk("(??? %d)", status);
                	printk("\n");
        	}

		if (status != SDVO_CMD_STATUS_PENDING)
			return status;

		mdelay(50);
	}

	return status;
}

int intel_sdvo_get_pixel_multiplier(struct drm_display_mode *mode)
{
	if (mode->clock >= 100000)
		return 1;
	else if (mode->clock >= 50000)
		return 2;
	else
		return 4;
}

/**
 * Don't check status code from this as it switches the bus back to the
 * SDVO chips which defeats the purpose of doing a bus switch in the first
 * place.
 */
void intel_sdvo_set_control_bus_switch(struct drm_output *output, u8 target)
{
	intel_sdvo_write_cmd(output, SDVO_CMD_SET_CONTROL_BUS_SWITCH, &target, 1);
}

static bool intel_sdvo_set_target_input(struct drm_output *output, bool target_0, bool target_1)
{
	struct intel_sdvo_set_target_input_args targets = {0};
	u8 status;

	if (target_0 && target_1)
		return SDVO_CMD_STATUS_NOTSUPP;

	if (target_1)
		targets.target_1 = 1;

	intel_sdvo_write_cmd(output, SDVO_CMD_SET_TARGET_INPUT, &targets,
			     sizeof(targets));

	status = intel_sdvo_read_response(output, NULL, 0);

	return (status == SDVO_CMD_STATUS_SUCCESS);
}

/**
 * Return whether each input is trained.
 *
 * This function is making an assumption about the layout of the response,
 * which should be checked against the docs.
 */
static bool intel_sdvo_get_trained_inputs(struct drm_output *output, bool *input_1, bool *input_2)
{
	struct intel_sdvo_get_trained_inputs_response response;
	u8 status;

	intel_sdvo_write_cmd(output, SDVO_CMD_GET_TRAINED_INPUTS, NULL, 0);
	status = intel_sdvo_read_response(output, &response, sizeof(response));
	if (status != SDVO_CMD_STATUS_SUCCESS)
		return false;

	*input_1 = response.input0_trained;
	*input_2 = response.input1_trained;
	return true;
}

static bool intel_sdvo_get_active_outputs(struct drm_output *output,
					  u16 *outputs)
{
	u8 status;

	intel_sdvo_write_cmd(output, SDVO_CMD_GET_ACTIVE_OUTPUTS, NULL, 0);
	status = intel_sdvo_read_response(output, outputs, sizeof(*outputs));

	return (status == SDVO_CMD_STATUS_SUCCESS);
}

static bool intel_sdvo_set_active_outputs(struct drm_output *output,
					  u16 outputs)
{
	u8 status;

	intel_sdvo_write_cmd(output, SDVO_CMD_SET_ACTIVE_OUTPUTS, &outputs,
			     sizeof(outputs));
	status = intel_sdvo_read_response(output, NULL, 0);
	return (status == SDVO_CMD_STATUS_SUCCESS);
}

static bool intel_sdvo_set_encoder_power_state(struct drm_output *output,
					       int mode)
{
	u8 status, state = SDVO_ENCODER_STATE_ON;

	switch (mode) {
	case DPMSModeOn:
		state = SDVO_ENCODER_STATE_ON;
		break;
	case DPMSModeStandby:
		state = SDVO_ENCODER_STATE_STANDBY;
		break;
	case DPMSModeSuspend:
		state = SDVO_ENCODER_STATE_SUSPEND;
		break;
	case DPMSModeOff:
		state = SDVO_ENCODER_STATE_OFF;
		break;
	}
	
	intel_sdvo_write_cmd(output, SDVO_CMD_SET_ENCODER_POWER_STATE, &state,
			     sizeof(state));
	status = intel_sdvo_read_response(output, NULL, 0);

	return (status == SDVO_CMD_STATUS_SUCCESS);
}

static bool intel_sdvo_get_input_pixel_clock_range(struct drm_output *output,
						   int *clock_min,
						   int *clock_max)
{
	struct intel_sdvo_pixel_clock_range clocks;
	u8 status;

	intel_sdvo_write_cmd(output, SDVO_CMD_GET_INPUT_PIXEL_CLOCK_RANGE,
			     NULL, 0);

	status = intel_sdvo_read_response(output, &clocks, sizeof(clocks));

	if (status != SDVO_CMD_STATUS_SUCCESS)
		return false;

	/* Convert the values from units of 10 kHz to kHz. */
	*clock_min = clocks.min * 10;
	*clock_max = clocks.max * 10;

	return true;
}

static bool intel_sdvo_set_target_output(struct drm_output *output,
					 u16 outputs)
{
	u8 status;

	intel_sdvo_write_cmd(output, SDVO_CMD_SET_TARGET_OUTPUT, &outputs,
			     sizeof(outputs));

	status = intel_sdvo_read_response(output, NULL, 0);
	return (status == SDVO_CMD_STATUS_SUCCESS);
}

static bool intel_sdvo_get_timing(struct drm_output *output, u8 cmd,
				  struct intel_sdvo_dtd *dtd)
{
	u8 status;

	intel_sdvo_write_cmd(output, cmd, NULL, 0);
	status = intel_sdvo_read_response(output, &dtd->part1,
					  sizeof(dtd->part1));
	if (status != SDVO_CMD_STATUS_SUCCESS)
		return false;

	intel_sdvo_write_cmd(output, cmd + 1, NULL, 0);
	status = intel_sdvo_read_response(output, &dtd->part2,
					  sizeof(dtd->part2));
	if (status != SDVO_CMD_STATUS_SUCCESS)
		return false;

	return true;
}

static bool intel_sdvo_get_input_timing(struct drm_output *output,
					 struct intel_sdvo_dtd *dtd)
{
	return intel_sdvo_get_timing(output,
				     SDVO_CMD_GET_INPUT_TIMINGS_PART1, dtd);
}

static bool intel_sdvo_get_output_timing(struct drm_output *output,
					 struct intel_sdvo_dtd *dtd)
{
	return intel_sdvo_get_timing(output,
				     SDVO_CMD_GET_OUTPUT_TIMINGS_PART1, dtd);
}

static bool intel_sdvo_set_timing(struct drm_output *output, u8 cmd,
				  struct intel_sdvo_dtd *dtd)
{
	u8 status;

	intel_sdvo_write_cmd(output, cmd, &dtd->part1, sizeof(dtd->part1));
	status = intel_sdvo_read_response(output, NULL, 0);
	if (status != SDVO_CMD_STATUS_SUCCESS)
		return false;

	intel_sdvo_write_cmd(output, cmd + 1, &dtd->part2, sizeof(dtd->part2));
	status = intel_sdvo_read_response(output, NULL, 0);
	if (status != SDVO_CMD_STATUS_SUCCESS)
		return false;

	return true;
}

static bool intel_sdvo_set_input_timing(struct drm_output *output,
					 struct intel_sdvo_dtd *dtd)
{
	return intel_sdvo_set_timing(output,
				     SDVO_CMD_SET_INPUT_TIMINGS_PART1, dtd);
}

static bool intel_sdvo_set_output_timing(struct drm_output *output,
					 struct intel_sdvo_dtd *dtd)
{
	return intel_sdvo_set_timing(output,
				     SDVO_CMD_SET_OUTPUT_TIMINGS_PART1, dtd);
}

#if 0
static bool intel_sdvo_get_preferred_input_timing(struct drm_output *output,
						  struct intel_sdvo_dtd *dtd)
{
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;
	u8 status;

	intel_sdvo_write_cmd(output, SDVO_CMD_GET_PREFERRED_INPUT_TIMING_PART1,
			     NULL, 0);

	status = intel_sdvo_read_response(output, &dtd->part1,
					  sizeof(dtd->part1));
	if (status != SDVO_CMD_STATUS_SUCCESS)
		return false;

	intel_sdvo_write_cmd(output, SDVO_CMD_GET_PREFERRED_INPUT_TIMING_PART2,
			     NULL, 0);
	status = intel_sdvo_read_response(output, &dtd->part2,
					  sizeof(dtd->part2));
	if (status != SDVO_CMD_STATUS_SUCCESS)
		return false;

	return true;
}
#endif

static int intel_sdvo_get_clock_rate_mult(struct drm_output *output)
{
	u8 response, status;

	intel_sdvo_write_cmd(output, SDVO_CMD_GET_CLOCK_RATE_MULT, NULL, 0);
	status = intel_sdvo_read_response(output, &response, 1);

	if (status != SDVO_CMD_STATUS_SUCCESS) {
		DRM_DEBUG("Couldn't get SDVO clock rate multiplier\n");
		return SDVO_CLOCK_RATE_MULT_1X;
	} else {
		DRM_DEBUG("Current clock rate multiplier: %d\n", response);
	}

	return response;
}

static bool intel_sdvo_set_clock_rate_mult(struct drm_output *output, u8 val)
{
	u8 status;

	intel_sdvo_write_cmd(output, SDVO_CMD_SET_CLOCK_RATE_MULT, &val, 1);
	status = intel_sdvo_read_response(output, NULL, 0);
	if (status != SDVO_CMD_STATUS_SUCCESS)
		return false;

	return true;
}

static bool intel_sdvo_mode_fixup(struct drm_output *output,
				  struct drm_display_mode *mode,
				  struct drm_display_mode *adjusted_mode)
{
	/* Make the CRTC code factor in the SDVO pixel multiplier.  The SDVO
	 * device will be told of the multiplier during mode_set.
	 */
	DRM_DEBUG("xxintel_sdvo_fixup\n");
	adjusted_mode->clock *= intel_sdvo_get_pixel_multiplier(mode);
	return true;
}

#if 0
static void i830_sdvo_map_hdtvstd_bitmask(struct drm_output * output)
{
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;

    switch (sdvo_priv->TVStandard) {
    case HDTV_SMPTE_274M_1080i50:
	sdvo_priv->TVStdBitmask = SDVO_HDTV_STD_274M_1080i50;
	break;

    case HDTV_SMPTE_274M_1080i59:
	sdvo_priv->TVStdBitmask = SDVO_HDTV_STD_274M_1080i59;
	break;

    case HDTV_SMPTE_274M_1080i60:
	sdvo_priv->TVStdBitmask = SDVO_HDTV_STD_274M_1080i60;
	break;
    case HDTV_SMPTE_274M_1080p60:
	sdvo_priv->TVStdBitmask = SDVO_HDTV_STD_274M_1080p60;
	break;
    case HDTV_SMPTE_296M_720p59:
	sdvo_priv->TVStdBitmask = SDVO_HDTV_STD_296M_720p59;
	break;

    case HDTV_SMPTE_296M_720p60:
	sdvo_priv->TVStdBitmask = SDVO_HDTV_STD_296M_720p60;
	break;

    case HDTV_SMPTE_296M_720p50:
	sdvo_priv->TVStdBitmask = SDVO_HDTV_STD_296M_720p50;
	break;

    case HDTV_SMPTE_293M_480p59:
	sdvo_priv->TVStdBitmask = SDVO_HDTV_STD_293M_480p59;
	break;

    case HDTV_SMPTE_293M_480p60:
	sdvo_priv->TVStdBitmask = SDVO_HDTV_STD_EIA_7702A_480p60;
	break;

    case HDTV_SMPTE_170M_480i59:
	sdvo_priv->TVStdBitmask = SDVO_HDTV_STD_170M_480i59;
	break;

    case HDTV_ITURBT601_576i50:
	sdvo_priv->TVStdBitmask = SDVO_HDTV_STD_ITURBT601_576i50;
	break;

    case HDTV_ITURBT601_576p50:
	sdvo_priv->TVStdBitmask = SDVO_HDTV_STD_ITURBT601_576p50;
	break;
    default:
	DRM_DEBUG("ERROR: Unknown TV Standard!!!\n");
	/*Invalid return 0 */
	sdvo_priv->TVStdBitmask = 0;
    }

}

static void i830_sdvo_map_sdtvstd_bitmask(struct drm_output * output)
{
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;

    switch (sdvo_priv->TVStandard) {
    case TVSTANDARD_NTSC_M:
	sdvo_priv->TVStdBitmask = SDVO_NTSC_M;
	break;

    case TVSTANDARD_NTSC_M_J:
	sdvo_priv->TVStdBitmask = SDVO_NTSC_M_J;
	break;

    case TVSTANDARD_NTSC_433:
	sdvo_priv->TVStdBitmask = SDVO_NTSC_433;
	break;

    case TVSTANDARD_PAL_B:
	sdvo_priv->TVStdBitmask = SDVO_PAL_B;
	break;

    case TVSTANDARD_PAL_D:
	sdvo_priv->TVStdBitmask = SDVO_PAL_D;
	break;

    case TVSTANDARD_PAL_G:
	sdvo_priv->TVStdBitmask = SDVO_PAL_G;
	break;

    case TVSTANDARD_PAL_H:
	sdvo_priv->TVStdBitmask = SDVO_PAL_H;
	break;

    case TVSTANDARD_PAL_I:
	sdvo_priv->TVStdBitmask = SDVO_PAL_I;
	break;

    case TVSTANDARD_PAL_M:
	sdvo_priv->TVStdBitmask = SDVO_PAL_M;
	break;

    case TVSTANDARD_PAL_N:
	sdvo_priv->TVStdBitmask = SDVO_PAL_N;
	break;

    case TVSTANDARD_PAL_60:
	sdvo_priv->TVStdBitmask = SDVO_PAL_60;
	break;

    case TVSTANDARD_SECAM_B:
	sdvo_priv->TVStdBitmask = SDVO_SECAM_B;
	break;

    case TVSTANDARD_SECAM_D:
	sdvo_priv->TVStdBitmask = SDVO_SECAM_D;
	break;

    case TVSTANDARD_SECAM_G:
	sdvo_priv->TVStdBitmask = SDVO_SECAM_G;
	break;

    case TVSTANDARD_SECAM_K:
	sdvo_priv->TVStdBitmask = SDVO_SECAM_K;
	break;

    case TVSTANDARD_SECAM_K1:
	sdvo_priv->TVStdBitmask = SDVO_SECAM_K1;
	break;

    case TVSTANDARD_SECAM_L:
	sdvo_priv->TVStdBitmask = SDVO_SECAM_L;
	break;

    case TVSTANDARD_SECAM_L1:
	DRM_DEBUG("TVSTANDARD_SECAM_L1 not supported by encoder\n");
	break;

    case TVSTANDARD_SECAM_H:
	DRM_DEBUG("TVSTANDARD_SECAM_H not supported by encoder\n");
	break;

    default:
	DRM_DEBUG("ERROR: Unknown TV Standard\n");
	/*Invalid return 0 */
	sdvo_priv->TVStdBitmask = 0;
	break;
    }
}
#endif 

static bool i830_sdvo_set_tvoutputs_formats(struct drm_output * output)
{
    u8 byArgs[6];
    u8 status;
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;


    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    if (sdvo_priv->TVMode & (TVMODE_SDTV)) {
	/* Fill up the arguement value */
	byArgs[0] = (u8) (sdvo_priv->TVStdBitmask & 0xFF);
	byArgs[1] = (u8) ((sdvo_priv->TVStdBitmask >> 8) & 0xFF);
	byArgs[2] = (u8) ((sdvo_priv->TVStdBitmask >> 16) & 0xFF);
    } else {
	/* Fill up the arguement value */
	byArgs[0] = 0;
	byArgs[1] = 0;
	byArgs[2] = (u8) ((sdvo_priv->TVStdBitmask & 0xFF));
	byArgs[3] = (u8) ((sdvo_priv->TVStdBitmask >> 8) & 0xFF);
	byArgs[4] = (u8) ((sdvo_priv->TVStdBitmask >> 16) & 0xFF);
	byArgs[5] = (u8) ((sdvo_priv->TVStdBitmask >> 24) & 0xFF);
    }

    intel_sdvo_write_cmd(output, SDVO_CMD_SET_TV_FORMATS, byArgs, 6);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;

}

static bool i830_sdvo_create_preferred_input_timing(struct drm_output * output,
					struct drm_display_mode * mode)
{
    u8 byArgs[7];
    u8 status;
    u32 dwClk;
    u32 dwHActive, dwVActive;
    bool bIsInterlaced, bIsScaled;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Fill up the arguement values */
    dwHActive = mode->crtc_hdisplay;
    dwVActive = mode->crtc_vdisplay;

    dwClk = mode->clock * 1000 / 10000;
    byArgs[0] = (u8) (dwClk & 0xFF);
    byArgs[1] = (u8) ((dwClk >> 8) & 0xFF);

    /* HActive & VActive should not exceed 12 bits each. So check it */
    if ((dwHActive > 0xFFF) || (dwVActive > 0xFFF))
	return FALSE;

    byArgs[2] = (u8) (dwHActive & 0xFF);
    byArgs[3] = (u8) ((dwHActive >> 8) & 0xF);
    byArgs[4] = (u8) (dwVActive & 0xFF);
    byArgs[5] = (u8) ((dwVActive >> 8) & 0xF);

    bIsInterlaced = 1;
    bIsScaled = 0;

    byArgs[6] = bIsInterlaced ? 1 : 0;
    byArgs[6] |= bIsScaled ? 2 : 0;

    intel_sdvo_write_cmd(output, SDVO_CMD_CREATE_PREFERRED_INPUT_TIMINGS,
			byArgs, 7);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;

}

static bool i830_sdvo_get_preferred_input_timing(struct drm_output * output,
				     struct intel_sdvo_dtd *output_dtd)
{
    return intel_sdvo_get_timing(output,
				SDVO_CMD_GET_PREFERRED_INPUT_TIMING_PART1,
				output_dtd);
}

static bool i830_sdvo_set_current_inoutmap(struct drm_output * output, u32 in0outputmask,
			       u32 in1outputmask)
{
    u8 byArgs[4];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Fill up the arguement values; */
    byArgs[0] = (u8) (in0outputmask & 0xFF);
    byArgs[1] = (u8) ((in0outputmask >> 8) & 0xFF);
    byArgs[2] = (u8) (in1outputmask & 0xFF);
    byArgs[3] = (u8) ((in1outputmask >> 8) & 0xFF);
    intel_sdvo_write_cmd(output, SDVO_CMD_SET_IN_OUT_MAP, byArgs, 4);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;

}

void i830_sdvo_set_iomap(struct drm_output * output)
{
    u32 dwCurrentSDVOIn0 = 0;
    u32 dwCurrentSDVOIn1 = 0;
    u32 dwDevMask = 0;

	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;


    /* Please DO NOT change the following code. */
    /* SDVOB_IN0 or SDVOB_IN1 ==> sdvo_in0 */
    /* SDVOC_IN0 or SDVOC_IN1 ==> sdvo_in1 */
    if (sdvo_priv->byInputWiring & (SDVOB_IN0 | SDVOC_IN0)) {
	switch (sdvo_priv->ActiveDevice) {
	case SDVO_DEVICE_LVDS:
	    dwDevMask = SDVO_OUTPUT_LVDS0 | SDVO_OUTPUT_LVDS1;
	    break;

	case SDVO_DEVICE_TMDS:
	    dwDevMask = SDVO_OUTPUT_TMDS0 | SDVO_OUTPUT_TMDS1;
	    break;

	case SDVO_DEVICE_TV:
	    dwDevMask =
		SDVO_OUTPUT_YPRPB0 | SDVO_OUTPUT_SVID0 | SDVO_OUTPUT_CVBS0 |
		SDVO_OUTPUT_YPRPB1 | SDVO_OUTPUT_SVID1 | SDVO_OUTPUT_CVBS1 |
		SDVO_OUTPUT_SCART0 | SDVO_OUTPUT_SCART1;
	    break;

	case SDVO_DEVICE_CRT:
	    dwDevMask = SDVO_OUTPUT_RGB0 | SDVO_OUTPUT_RGB1;
	    break;
	}
	dwCurrentSDVOIn0 = (sdvo_priv->active_outputs & dwDevMask);
    } else if (sdvo_priv->byInputWiring & (SDVOB_IN1 | SDVOC_IN1)) {
	switch (sdvo_priv->ActiveDevice) {
	case SDVO_DEVICE_LVDS:
	    dwDevMask = SDVO_OUTPUT_LVDS0 | SDVO_OUTPUT_LVDS1;
	    break;

	case SDVO_DEVICE_TMDS:
	    dwDevMask = SDVO_OUTPUT_TMDS0 | SDVO_OUTPUT_TMDS1;
	    break;

	case SDVO_DEVICE_TV:
	    dwDevMask =
		SDVO_OUTPUT_YPRPB0 | SDVO_OUTPUT_SVID0 | SDVO_OUTPUT_CVBS0 |
		SDVO_OUTPUT_YPRPB1 | SDVO_OUTPUT_SVID1 | SDVO_OUTPUT_CVBS1 |
		SDVO_OUTPUT_SCART0 | SDVO_OUTPUT_SCART1;
	    break;

	case SDVO_DEVICE_CRT:
	    dwDevMask = SDVO_OUTPUT_RGB0 | SDVO_OUTPUT_RGB1;
	    break;
	}
	dwCurrentSDVOIn1 = (sdvo_priv->active_outputs & dwDevMask);
    }

    i830_sdvo_set_current_inoutmap(output, dwCurrentSDVOIn0,
				   dwCurrentSDVOIn1);
}

static bool i830_sdvo_get_input_output_pixelclock_range(struct drm_output * output,
					    bool direction)
{
    u8 byRets[4];
    u8 status;

	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));
    if (direction)		       /* output pixel clock */
	intel_sdvo_write_cmd(output, SDVO_CMD_GET_OUTPUT_PIXEL_CLOCK_RANGE,
			    NULL, 0);
    else
	intel_sdvo_write_cmd(output, SDVO_CMD_GET_INPUT_PIXEL_CLOCK_RANGE,
			    NULL, 0);
    status = intel_sdvo_read_response(output, byRets, 4);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    if (direction) {
	/* Fill up the return values. */
	sdvo_priv->dwMinOutDotClk =
	    (u32) byRets[0] | ((u32) byRets[1] << 8);
	sdvo_priv->dwMaxOutDotClk =
	    (u32) byRets[2] | ((u32) byRets[3] << 8);

	/* Multiply 10000 with the clocks obtained */
	sdvo_priv->dwMinOutDotClk = (sdvo_priv->dwMinOutDotClk) * 10000;
	sdvo_priv->dwMaxOutDotClk = (sdvo_priv->dwMaxOutDotClk) * 10000;

    } else {
	/* Fill up the return values. */
	sdvo_priv->dwMinInDotClk = (u32) byRets[0] | ((u32) byRets[1] << 8);
	sdvo_priv->dwMaxInDotClk = (u32) byRets[2] | ((u32) byRets[3] << 8);

	/* Multiply 10000 with the clocks obtained */
	sdvo_priv->dwMinInDotClk = (sdvo_priv->dwMinInDotClk) * 10000;
	sdvo_priv->dwMaxInDotClk = (sdvo_priv->dwMaxInDotClk) * 10000;
    }
    DRM_DEBUG("MinDotClk = 0x%x\n", sdvo_priv->dwMinInDotClk);
    DRM_DEBUG("MaxDotClk = 0x%x\n", sdvo_priv->dwMaxInDotClk);

    return TRUE;

}

static bool i830_sdvo_get_supported_tvoutput_formats(struct drm_output * output,
					 u32 * pTVStdMask,
					 u32 * pHDTVStdMask, u32 *pTVStdFormat)
{
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;	

    u8 byRets[6];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_GET_SUPPORTED_TV_FORMATS, NULL, 0);

    status = intel_sdvo_read_response(output, byRets, 6);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    /* Fill up the return values; */
    *pTVStdMask = (((u32) byRets[0]) |
		   ((u32) byRets[1] << 8) |
		   ((u32) (byRets[2] & 0x7) << 16));

    *pHDTVStdMask = (((u32) byRets[2] & 0xF8) |
		     ((u32) byRets[3] << 8) |
		     ((u32) byRets[4] << 16) | ((u32) byRets[5] << 24));

    intel_sdvo_write_cmd(output, SDVO_CMD_GET_TV_FORMATS, NULL, 0);

    status = intel_sdvo_read_response(output, byRets, 6);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    /* Fill up the return values; */
	if(sdvo_priv->TVMode == TVMODE_SDTV)
    *pTVStdFormat = (((u32) byRets[0]) |
		   ((u32) byRets[1] << 8) |
		   ((u32) (byRets[2] & 0x7) << 16));
    else
    *pTVStdFormat = (((u32) byRets[2] & 0xF8) |
		     ((u32) byRets[3] << 8) |
		     ((u32) byRets[4] << 16) | ((u32) byRets[5] << 24));	
	DRM_DEBUG("BIOS TV format is %d\n",*pTVStdFormat);
    return TRUE;

}

static bool i830_sdvo_get_supported_enhancements(struct drm_output * output,
				     u32 * psupported_enhancements)
{

    u8 status;
    u8 byRets[2];
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;


    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_GET_SUPPORTED_ENHANCEMENTS, NULL, 0);

    status = intel_sdvo_read_response(output, byRets, 2);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    sdvo_priv->dwSupportedEnhancements = *psupported_enhancements =
	((u32) byRets[0] | ((u32) byRets[1] << 8));
    return TRUE;

}

static bool i830_sdvo_get_max_horizontal_overscan(struct drm_output * output, u32 * pMaxVal,
				      u32 * pDefaultVal)
{
    u8 byRets[4];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_GET_MAX_HORIZONTAL_OVERSCAN, NULL,
			0);

    status = intel_sdvo_read_response(output, byRets, 4);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;
    /* Fill up the return values. */
    *pMaxVal = (u32) byRets[0] | ((u32) byRets[1] << 8);
    *pDefaultVal = (u32) byRets[2] | ((u32) byRets[3] << 8);
    return TRUE;
}

static bool i830_sdvo_get_max_vertical_overscan(struct drm_output * output, u32 * pMaxVal,
				    u32 * pDefaultVal)
{
    u8 byRets[4];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_GET_MAX_VERTICAL_OVERSCAN, NULL, 0);

    status = intel_sdvo_read_response(output, byRets, 4);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;
    /* Fill up the return values. */
    *pMaxVal = (u32) byRets[0] | ((u32) byRets[1] << 8);
    *pDefaultVal = (u32) byRets[2] | ((u32) byRets[3] << 8);
    return TRUE;
}

static bool i830_sdvo_get_max_horizontal_position(struct drm_output * output, u32 * pMaxVal,
				      u32 * pDefaultVal)
{

    u8 byRets[4];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_GET_MAX_HORIZONTAL_POSITION, NULL,
			0);

    status = intel_sdvo_read_response(output, byRets, 4);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    /* Fill up the return values. */
    *pMaxVal = (u32) byRets[0] | ((u32) byRets[1] << 8);
    *pDefaultVal = (u32) byRets[2] | ((u32) byRets[3] << 8);

    return TRUE;
}

static bool i830_sdvo_get_max_vertical_position(struct drm_output * output,
				    u32 * pMaxVal, u32 * pDefaultVal)
{

    u8 byRets[4];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_GET_MAX_VERTICAL_POSITION, NULL, 0);

    status = intel_sdvo_read_response(output, byRets, 4);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    /* Fill up the return values. */
    *pMaxVal = (u32) byRets[0] | ((u32) byRets[1] << 8);
    *pDefaultVal = (u32) byRets[2] | ((u32) byRets[3] << 8);

    return TRUE;
}

static bool i830_sdvo_get_max_flickerfilter(struct drm_output * output,
				u32 * pMaxVal, u32 * pDefaultVal)
{

    u8 byRets[4];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */

    intel_sdvo_write_cmd(output, SDVO_CMD_GET_MAX_FLICKER_FILTER, NULL, 0);

    status = intel_sdvo_read_response(output, byRets, 4);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;
    /* Fill up the return values. */
    *pMaxVal = (u32) byRets[0] | ((u32) byRets[1] << 8);
    *pDefaultVal = (u32) byRets[2] | ((u32) byRets[3] << 8);

    return TRUE;
}

static bool i830_sdvo_get_max_brightness(struct drm_output * output,
			     u32 * pMaxVal, u32 * pDefaultVal)
{

    u8 byRets[4];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */

    intel_sdvo_write_cmd(output, SDVO_CMD_GET_MAX_BRIGHTNESS, NULL, 0);

    status = intel_sdvo_read_response(output, byRets, 4);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;
    /* Fill up the return values. */
    *pMaxVal = (u32) byRets[0] | ((u32) byRets[1] << 8);
    *pDefaultVal = (u32) byRets[2] | ((u32) byRets[3] << 8);

    return TRUE;
}

static bool i830_sdvo_get_max_contrast(struct drm_output * output,
			   u32 * pMaxVal, u32 * pDefaultVal)
{

    u8 byRets[4];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */

    intel_sdvo_write_cmd(output, SDVO_CMD_GET_MAX_CONTRAST, NULL, 0);

    status = intel_sdvo_read_response(output, byRets, 4);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;
    /* Fill up the return values. */
    *pMaxVal = (u32) byRets[0] | ((u32) byRets[1] << 8);
    *pDefaultVal = (u32) byRets[2] | ((u32) byRets[3] << 8);

    return TRUE;
}

static bool i830_sdvo_get_max_sharpness(struct drm_output * output,
			    u32 * pMaxVal, u32 * pDefaultVal)
{

    u8 byRets[4];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */

    intel_sdvo_write_cmd(output, SDVO_CMD_GET_MAX_SHARPNESS, NULL, 0);

    status = intel_sdvo_read_response(output, byRets, 4);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    /* Fill up the return values. */
    *pMaxVal = (u32) byRets[0] | ((u32) byRets[1] << 8);
    *pDefaultVal = (u32) byRets[2] | ((u32) byRets[3] << 8);

    return TRUE;
}

static bool i830_sdvo_get_max_hue(struct drm_output * output,
		      u32 * pMaxVal, u32 * pDefaultVal)
{
    u8 byRets[4];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_GET_MAX_HUE, NULL, 0);

    status = intel_sdvo_read_response(output, byRets, 4);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    /* Fill up the return values. */
    *pMaxVal = (u32) byRets[0] | ((u32) byRets[1] << 8);
    *pDefaultVal = (u32) byRets[2] | ((u32) byRets[3] << 8);

    return TRUE;
}

static bool i830_sdvo_get_max_saturation(struct drm_output * output,
			     u32 * pMaxVal, u32 * pDefaultVal)
{

    u8 byRets[4];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_GET_MAX_SATURATION, NULL, 0);

    status = intel_sdvo_read_response(output, byRets, 4);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    /* Fill up the return values. */
    *pMaxVal = (u32) byRets[0] | ((u32) byRets[1] << 8);
    *pDefaultVal = (u32) byRets[2] | ((u32) byRets[3] << 8);

    return TRUE;
}

static bool i830_sdvo_get_max_adaptive_flickerfilter(struct drm_output * output,
					 u32 * pMaxVal,
					 u32 * pDefaultVal)
{
    u8 byRets[4];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_GET_MAX_ADAPTIVE_FLICKER_FILTER,
			NULL, 0);
    status = intel_sdvo_read_response(output, byRets, 4);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    /* Fill up the return values. */
    *pMaxVal = (u32) byRets[0] | ((u32) byRets[1] << 8);
    *pDefaultVal = (u32) byRets[2] | ((u32) byRets[3] << 8);

    return TRUE;
}

static bool i830_sdvo_get_max_lumafilter(struct drm_output * output,
			     u32 * pMaxVal, u32 * pDefaultVal)
{

    u8 byRets[4];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_GET_MAX_TV_LUMA_FILTER, NULL, 0);
    status = intel_sdvo_read_response(output, byRets, 4);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    /* Fill up the return values. */
    *pMaxVal = (u32) byRets[0] | ((u32) byRets[1] << 8);
    *pDefaultVal = (u32) byRets[2] | ((u32) byRets[3] << 8);

    return TRUE;
}

static bool i830_sdvo_get_max_chromafilter(struct drm_output * output,
			       u32 * pMaxVal, u32 * pDefaultVal)
{

    u8 byRets[4];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_GET_MAX_TV_CHROMA_FILTER, NULL, 0);
    status = intel_sdvo_read_response(output, byRets, 4);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    /* Fill up the return values. */
    *pMaxVal = (u32) byRets[0] | ((u32) byRets[1] << 8);
    *pDefaultVal = (u32) byRets[2] | ((u32) byRets[3] << 8);

    return TRUE;
}

static bool i830_sdvo_get_dotcrawl(struct drm_output * output,
		       u32 * pCurrentVal, u32 * pDefaultVal)
{

    u8 byRets[2];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */

    intel_sdvo_write_cmd(output, SDVO_CMD_GET_DOT_CRAWL, NULL, 0);
    status = intel_sdvo_read_response(output, byRets, 2);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    /* Tibet issue 1603772: Dot crawl do not persist after reboot/Hibernate */
    /* Details : Bit0 is considered as DotCrawl Max value. But according to EDS, Bit0 */
    /*           represents the Current DotCrawl value. */
    /* Fix     : The current value is updated with Bit0. */

    /* Fill up the return values. */
    *pCurrentVal = (u32) (byRets[0] & 0x1);
    *pDefaultVal = (u32) ((byRets[0] >> 1) & 0x1);
    return TRUE;
}

static bool i830_sdvo_get_max_2D_flickerfilter(struct drm_output * output,
				   u32 * pMaxVal, u32 * pDefaultVal)
{

    u8 byRets[4];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byRets, 0, sizeof(byRets));

    /* Send the arguements & SDVO opcode to the h/w */

    intel_sdvo_write_cmd(output, SDVO_CMD_GET_MAX_2D_FLICKER_FILTER, NULL, 0);
    status = intel_sdvo_read_response(output, byRets, 4);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    /* Fill up the return values. */
    *pMaxVal = (u32) byRets[0] | ((u32) byRets[1] << 8);
    *pDefaultVal = (u32) byRets[2] | ((u32) byRets[3] << 8);

    return TRUE;
}

static bool i830_sdvo_set_horizontal_overscan(struct drm_output * output, u32 dwVal)
{

    u8 byArgs[2];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Fill up the arguement value */
    byArgs[0] = (u8) (dwVal & 0xFF);
    byArgs[1] = (u8) ((dwVal >> 8) & 0xFF);

    /* Send the arguements & SDVO opcode to the h/w */

    intel_sdvo_write_cmd(output, SDVO_CMD_SET_HORIZONTAL_OVERSCAN, byArgs, 2);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;
    return TRUE;
}

static bool i830_sdvo_set_vertical_overscan(struct drm_output * output, u32 dwVal)
{

    u8 byArgs[2];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Fill up the arguement value */
    byArgs[0] = (u8) (dwVal & 0xFF);
    byArgs[1] = (u8) ((dwVal >> 8) & 0xFF);

    /* Send the arguements & SDVO opcode to the h/w */

    intel_sdvo_write_cmd(output, SDVO_CMD_SET_VERTICAL_OVERSCAN, byArgs, 2);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;
    return TRUE;
}

static bool i830_sdvo_set_horizontal_position(struct drm_output * output, u32 dwVal)
{

    u8 byArgs[2];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Fill up the arguement value */
    byArgs[0] = (u8) (dwVal & 0xFF);
    byArgs[1] = (u8) ((dwVal >> 8) & 0xFF);

    /* Send the arguements & SDVO opcode to the h/w */

    intel_sdvo_write_cmd(output, SDVO_CMD_SET_HORIZONTAL_POSITION, byArgs, 2);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;
}

static bool i830_sdvo_set_vertical_position(struct drm_output * output, u32 dwVal)
{

    u8 byArgs[2];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Fill up the arguement value */
    byArgs[0] = (u8) (dwVal & 0xFF);
    byArgs[1] = (u8) ((dwVal >> 8) & 0xFF);

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_SET_VERTICAL_POSITION, byArgs, 2);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;

}

static bool i830_sdvo_set_flickerilter(struct drm_output * output, u32 dwVal)
{

    u8 byArgs[2];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Fill up the arguement value */
    byArgs[0] = (u8) (dwVal & 0xFF);
    byArgs[1] = (u8) ((dwVal >> 8) & 0xFF);

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_SET_FLICKER_FILTER, byArgs, 2);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;
}

static bool i830_sdvo_set_brightness(struct drm_output * output, u32 dwVal)
{

    u8 byArgs[2];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Fill up the arguement value */
    byArgs[0] = (u8) (dwVal & 0xFF);
    byArgs[1] = (u8) ((dwVal >> 8) & 0xFF);

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_SET_BRIGHTNESS, byArgs, 2);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;
}

static bool i830_sdvo_set_contrast(struct drm_output * output, u32 dwVal)
{

    u8 byArgs[2];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));
    /* Fill up the arguement value */
    byArgs[0] = (u8) (dwVal & 0xFF);
    byArgs[1] = (u8) ((dwVal >> 8) & 0xFF);

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_SET_CONTRAST, byArgs, 2);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;
}

static bool i830_sdvo_set_sharpness(struct drm_output * output, u32 dwVal)
{

    u8 byArgs[2];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Fill up the arguement value */
    byArgs[0] = (u8) (dwVal & 0xFF);
    byArgs[1] = (u8) ((dwVal >> 8) & 0xFF);

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_SET_SHARPNESS, byArgs, 2);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;
}

static bool i830_sdvo_set_hue(struct drm_output * output, u32 dwVal)
{

    u8 byArgs[2];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Fill up the arguement value */
    byArgs[0] = (u8) (dwVal & 0xFF);
    byArgs[1] = (u8) ((dwVal >> 8) & 0xFF);

    /* Send the arguements & SDVO opcode to the h/w */

    intel_sdvo_write_cmd(output, SDVO_CMD_SET_HUE, byArgs, 2);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;
}

static bool i830_sdvo_set_saturation(struct drm_output * output, u32 dwVal)
{

    u8 byArgs[2];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Fill up the arguement value */
    byArgs[0] = (u8) (dwVal & 0xFF);
    byArgs[1] = (u8) ((dwVal >> 8) & 0xFF);

    /* Send the arguements & SDVO opcode to the h/w */
    intel_sdvo_write_cmd(output, SDVO_CMD_SET_SATURATION, byArgs, 2);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;
}

static bool i830_sdvo_set_adaptive_flickerfilter(struct drm_output * output, u32 dwVal)
{
    u8 byArgs[2];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Fill up the arguement value */
    byArgs[0] = (u8) (dwVal & 0xFF);
    byArgs[1] = (u8) ((dwVal >> 8) & 0xFF);

    /* Send the arguements & SDVO opcode to the h/w */

    intel_sdvo_write_cmd(output, SDVO_CMD_SET_ADAPTIVE_FLICKER_FILTER, byArgs,
			2);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;

}

static bool i830_sdvo_set_lumafilter(struct drm_output * output, u32 dwVal)
{
    u8 byArgs[2];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Fill up the arguement value */
    byArgs[0] = (u8) (dwVal & 0xFF);
    byArgs[1] = (u8) ((dwVal >> 8) & 0xFF);

    /* Send the arguements & SDVO opcode to the h/w */

    intel_sdvo_write_cmd(output, SDVO_CMD_SET_TV_LUMA_FILTER, byArgs, 2);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;
}

static bool i830_sdvo_set_chromafilter(struct drm_output * output, u32 dwVal)
{

    u8 byArgs[2];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Fill up the arguement value */
    byArgs[0] = (u8) (dwVal & 0xFF);
    byArgs[1] = (u8) ((dwVal >> 8) & 0xFF);

    /* Send the arguements & SDVO opcode to the h/w */

    intel_sdvo_write_cmd(output, SDVO_CMD_SET_TV_CHROMA_FILTER, byArgs, 2);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;
}

static bool i830_sdvo_set_dotcrawl(struct drm_output * output, u32 dwVal)
{

    u8 byArgs[2];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Fill up the arguement value */
    byArgs[0] = (u8) (dwVal & 0xFF);
    byArgs[1] = (u8) ((dwVal >> 8) & 0xFF);

    /* Send the arguements & SDVO opcode to the h/w */

    intel_sdvo_write_cmd(output, SDVO_CMD_SET_DOT_CRAWL, byArgs, 2);
    status = intel_sdvo_read_response(output, NULL, 0);
    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;
}

static bool i830_sdvo_set_2D_flickerfilter(struct drm_output * output, u32 dwVal)
{

    u8 byArgs[2];
    u8 status;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Fill up the arguement value */
    byArgs[0] = (u8) (dwVal & 0xFF);
    byArgs[1] = (u8) ((dwVal >> 8) & 0xFF);

    /* Send the arguements & SDVO opcode to the h/w */

    intel_sdvo_write_cmd(output, SDVO_CMD_SET_2D_FLICKER_FILTER, byArgs, 2);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;
}

#if 0
static bool i830_sdvo_set_ancillary_video_information(struct drm_output * output)
{

    u8 status;
    u8 byArgs[4];
    u32 dwAncillaryBits = 0;
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;


    PSDVO_ANCILLARY_INFO_T pAncillaryInfo = &sdvo_priv->AncillaryInfo;

    /* Make all fields of the  args/ret to zero */
    memset(byArgs, 0, sizeof(byArgs));

    /* Handle picture aspect ratio (bits 8, 9) and */
    /* active format aspect ratio (bits 10, 13) */
    switch (pAncillaryInfo->AspectRatio) {
    case CP_ASPECT_RATIO_FF_4_BY_3:
	dwAncillaryBits |= UAIM_PAR_4_3;
	dwAncillaryBits |= UAIM_FAR_4_BY_3_CENTER;
	break;
    case CP_ASPECT_RATIO_14_BY_9_CENTER:
	dwAncillaryBits |= UAIM_FAR_14_BY_9_CENTER;
	break;
    case CP_ASPECT_RATIO_14_BY_9_TOP:
	dwAncillaryBits |= UAIM_FAR_14_BY_9_LETTERBOX_TOP;
	break;
    case CP_ASPECT_RATIO_16_BY_9_CENTER:
	dwAncillaryBits |= UAIM_PAR_16_9;
	dwAncillaryBits |= UAIM_FAR_16_BY_9_CENTER;
	break;
    case CP_ASPECT_RATIO_16_BY_9_TOP:
	dwAncillaryBits |= UAIM_PAR_16_9;
	dwAncillaryBits |= UAIM_FAR_16_BY_9_LETTERBOX_TOP;
	break;
    case CP_ASPECT_RATIO_GT_16_BY_9_CENTER:
	dwAncillaryBits |= UAIM_PAR_16_9;
	dwAncillaryBits |= UAIM_FAR_GT_16_BY_9_LETTERBOX_CENTER;
	break;
    case CP_ASPECT_RATIO_FF_4_BY_3_PROT_CENTER:
	dwAncillaryBits |= UAIM_FAR_4_BY_3_SNP_14_BY_9_CENTER;
	break;
    case CP_ASPECT_RATIO_FF_16_BY_9_ANAMORPHIC:
	dwAncillaryBits |= UAIM_PAR_16_9;
	break;
    default:
	DRM_DEBUG("fail to set ancillary video info\n");
	return FALSE;

    }

    /* Fill up the argument value */
    byArgs[0] = (u8) ((dwAncillaryBits >> 0) & 0xFF);
    byArgs[1] = (u8) ((dwAncillaryBits >> 8) & 0xFF);
    byArgs[2] = (u8) ((dwAncillaryBits >> 16) & 0xFF);
    byArgs[3] = (u8) ((dwAncillaryBits >> 24) & 0xFF);

    /* Send the arguements & SDVO opcode to the h/w */

    intel_sdvo_write_cmd(output, SDVO_CMD_SET_ANCILLARY_VIDEO_INFORMATION,
			byArgs, 4);
    status = intel_sdvo_read_response(output, NULL, 0);

    if (status != SDVO_CMD_STATUS_SUCCESS)
	return FALSE;

    return TRUE;

}
#endif
static bool i830_tv_program_display_params(struct drm_output * output)

{
    u8 status;
    u32 dwMaxVal = 0;
    u32 dwDefaultVal = 0;
    u32 dwCurrentVal = 0;

	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;


    /* X & Y Positions */

    /* Horizontal postition */
    if (sdvo_priv->dwSupportedEnhancements & SDVO_HORIZONTAL_POSITION) {
	status =
	    i830_sdvo_get_max_horizontal_position(output, &dwMaxVal,
						  &dwDefaultVal);

	if (status) {
	    /*Tibet issue 1596943: After changing mode from 8x6 to 10x7 open CUI and press Restore Defaults */
	    /*Position changes. */

	    /* Tibet:1629992 : can't keep previous TV setting status if re-boot system after TV setting(screen position & size) of CUI */
	    /* Fix : compare whether current postion is greater than max value and then assign the default value. Earlier the check was */
	    /*       against the pAim->PositionX.Max value to dwMaxVal. When we boot the PositionX.Max value is 0 and so after every reboot, */
	    /*       position is set to default. */

	    if (sdvo_priv->dispParams.PositionX.Value > dwMaxVal)
		sdvo_priv->dispParams.PositionX.Value = dwDefaultVal;

	    status =
		i830_sdvo_set_horizontal_position(output,
						  sdvo_priv->dispParams.PositionX.
						  Value);

	    if (!status)
		return status;

	    sdvo_priv->dispParams.PositionX.Max = dwMaxVal;
	    sdvo_priv->dispParams.PositionX.Min = 0;
	    sdvo_priv->dispParams.PositionX.Default = dwDefaultVal;
	    sdvo_priv->dispParams.PositionX.Step = 1;
	} else {
	    return status;
	}
    }

    /* Vertical position */
    if (sdvo_priv->dwSupportedEnhancements & SDVO_VERTICAL_POSITION) {
	status =
	    i830_sdvo_get_max_vertical_position(output, &dwMaxVal,
						&dwDefaultVal);

	if (status) {

	    /*Tibet issue 1596943: After changing mode from 8x6 to 10x7 open CUI and press Restore Defaults */
	    /*Position changes. */
	    /*currently if we are out of range get back to default */

	    /* Tibet:1629992 : can't keep previous TV setting status if re-boot system after TV setting(screen position & size) of CUI */
	    /* Fix : compare whether current postion is greater than max value and then assign the default value. Earlier the check was */
	    /*       against the pAim->PositionY.Max  value to dwMaxVal. When we boot the PositionX.Max value is 0 and so after every reboot, */
	    /*       position is set to default. */

	    if (sdvo_priv->dispParams.PositionY.Value > dwMaxVal)
		sdvo_priv->dispParams.PositionY.Value = dwDefaultVal;

	    status =
		i830_sdvo_set_vertical_position(output,
						sdvo_priv->dispParams.PositionY.
						Value);
	    if (!status)
		return status;

	    sdvo_priv->dispParams.PositionY.Max = dwMaxVal;
	    sdvo_priv->dispParams.PositionY.Min = 0;
	    sdvo_priv->dispParams.PositionY.Default = dwDefaultVal;
	    sdvo_priv->dispParams.PositionY.Step = 1;
	} else {
	    return status;
	}
    }

    /* Flicker Filter */
    if (sdvo_priv->dwSupportedEnhancements & SDVO_FLICKER_FILTER) {
	status =
	    i830_sdvo_get_max_flickerfilter(output, &dwMaxVal, &dwDefaultVal);

	if (status) {
	    /*currently if we are out of range get back to default */
	    if (sdvo_priv->dispParams.FlickerFilter.Value > dwMaxVal)
		sdvo_priv->dispParams.FlickerFilter.Value = dwDefaultVal;

	    status =
		i830_sdvo_set_flickerilter(output,
					   sdvo_priv->dispParams.FlickerFilter.
					   Value);
	    if (!status)
		return status;

	    sdvo_priv->dispParams.FlickerFilter.Max = dwMaxVal;
	    sdvo_priv->dispParams.FlickerFilter.Min = 0;
	    sdvo_priv->dispParams.FlickerFilter.Default = dwDefaultVal;
	    sdvo_priv->dispParams.FlickerFilter.Step = 1;
	} else {
	    return status;
	}
    }

    /* Brightness */
    if (sdvo_priv->dwSupportedEnhancements & SDVO_BRIGHTNESS) {

	status =
	    i830_sdvo_get_max_brightness(output, &dwMaxVal, &dwDefaultVal);

	if (status) {
	    /*check whether the value is beyond the max value, min value as per EDS is always 0 so */
	    /*no need to check it. */
	    if (sdvo_priv->dispParams.Brightness.Value > dwMaxVal)
		sdvo_priv->dispParams.Brightness.Value = dwDefaultVal;

	    /* Program the device */
	    status =
		i830_sdvo_set_brightness(output,
					 sdvo_priv->dispParams.Brightness.Value);
	    if (!status)
		return status;

	    sdvo_priv->dispParams.Brightness.Max = dwMaxVal;
	    sdvo_priv->dispParams.Brightness.Min = 0;
	    sdvo_priv->dispParams.Brightness.Default = dwDefaultVal;
	    sdvo_priv->dispParams.Brightness.Step = 1;
	} else {
	    return status;
	}

    }

    /* Contrast */
    if (sdvo_priv->dwSupportedEnhancements & SDVO_CONTRAST) {

	status = i830_sdvo_get_max_contrast(output, &dwMaxVal, &dwDefaultVal);

	if (status) {
	    /*check whether the value is beyond the max value, min value as per EDS is always 0 so */
	    /*no need to check it. */
	    if (sdvo_priv->dispParams.Contrast.Value > dwMaxVal)
		sdvo_priv->dispParams.Contrast.Value = dwDefaultVal;

	    /* Program the device */
	    status =
		i830_sdvo_set_contrast(output,
				       sdvo_priv->dispParams.Contrast.Value);
	    if (!status)
		return status;

	    sdvo_priv->dispParams.Contrast.Max = dwMaxVal;
	    sdvo_priv->dispParams.Contrast.Min = 0;
	    sdvo_priv->dispParams.Contrast.Default = dwDefaultVal;

	    sdvo_priv->dispParams.Contrast.Step = 1;

	} else {
	    return status;
	}
    }

    /* Sharpness */
    if (sdvo_priv->dwSupportedEnhancements & SDVO_SHARPNESS) {

	status =
	    i830_sdvo_get_max_sharpness(output, &dwMaxVal, &dwDefaultVal);

	if (status) {
	    /*check whether the value is beyond the max value, min value as per EDS is always 0 so */
	    /*no need to check it. */
	    if (sdvo_priv->dispParams.Sharpness.Value > dwMaxVal)
		sdvo_priv->dispParams.Sharpness.Value = dwDefaultVal;

	    /* Program the device */
	    status =
		i830_sdvo_set_sharpness(output,
					sdvo_priv->dispParams.Sharpness.Value);
	    if (!status)
		return status;
	    sdvo_priv->dispParams.Sharpness.Max = dwMaxVal;
	    sdvo_priv->dispParams.Sharpness.Min = 0;
	    sdvo_priv->dispParams.Sharpness.Default = dwDefaultVal;

	    sdvo_priv->dispParams.Sharpness.Step = 1;
	} else {
	    return status;
	}
    }

    /* Hue */
    if (sdvo_priv->dwSupportedEnhancements & SDVO_HUE) {

	status = i830_sdvo_get_max_hue(output, &dwMaxVal, &dwDefaultVal);

	if (status) {
	    /*check whether the value is beyond the max value, min value as per EDS is always 0 so */
	    /*no need to check it. */
	    if (sdvo_priv->dispParams.Hue.Value > dwMaxVal)
		sdvo_priv->dispParams.Hue.Value = dwDefaultVal;

	    /* Program the device */
	    status = i830_sdvo_set_hue(output, sdvo_priv->dispParams.Hue.Value);
	    if (!status)
		return status;

	    sdvo_priv->dispParams.Hue.Max = dwMaxVal;
	    sdvo_priv->dispParams.Hue.Min = 0;
	    sdvo_priv->dispParams.Hue.Default = dwDefaultVal;

	    sdvo_priv->dispParams.Hue.Step = 1;

	} else {
	    return status;
	}
    }

    /* Saturation */
    if (sdvo_priv->dwSupportedEnhancements & SDVO_SATURATION) {
	status =
	    i830_sdvo_get_max_saturation(output, &dwMaxVal, &dwDefaultVal);

	if (status) {
	    /*check whether the value is beyond the max value, min value as per EDS is always 0 so */
	    /*no need to check it. */
	    if (sdvo_priv->dispParams.Saturation.Value > dwMaxVal)
		sdvo_priv->dispParams.Saturation.Value = dwDefaultVal;

	    /* Program the device */
	    status =
		i830_sdvo_set_saturation(output,
					 sdvo_priv->dispParams.Saturation.Value);
	    if (!status)
		return status;

	    sdvo_priv->dispParams.Saturation.Max = dwMaxVal;
	    sdvo_priv->dispParams.Saturation.Min = 0;
	    sdvo_priv->dispParams.Saturation.Default = dwDefaultVal;
	    sdvo_priv->dispParams.Saturation.Step = 1;
	} else {
	    return status;
	}

    }

    /* Adaptive Flicker filter */
    if (sdvo_priv->dwSupportedEnhancements & SDVO_ADAPTIVE_FLICKER_FILTER) {
	status =
	    i830_sdvo_get_max_adaptive_flickerfilter(output, &dwMaxVal,
						     &dwDefaultVal);

	if (status) {
	    /*check whether the value is beyond the max value, min value as per EDS is always 0 so */
	    /*no need to check it. */
	    if (sdvo_priv->dispParams.AdaptiveFF.Value > dwMaxVal)
		sdvo_priv->dispParams.AdaptiveFF.Value = dwDefaultVal;

	    status =
		i830_sdvo_set_adaptive_flickerfilter(output,
						     sdvo_priv->dispParams.
						     AdaptiveFF.Value);
	    if (!status)
		return status;

	    sdvo_priv->dispParams.AdaptiveFF.Max = dwMaxVal;
	    sdvo_priv->dispParams.AdaptiveFF.Min = 0;
	    sdvo_priv->dispParams.AdaptiveFF.Default = dwDefaultVal;
	    sdvo_priv->dispParams.AdaptiveFF.Step = 1;
	} else {
	    return status;
	}
    }

    /* 2D Flicker filter */
    if (sdvo_priv->dwSupportedEnhancements & SDVO_2D_FLICKER_FILTER) {

	status =
	    i830_sdvo_get_max_2D_flickerfilter(output, &dwMaxVal,
					       &dwDefaultVal);

	if (status) {
	    /*check whether the value is beyond the max value, min value as per EDS is always 0 so */
	    /*no need to check it. */
	    if (sdvo_priv->dispParams.TwoD_FlickerFilter.Value > dwMaxVal)
		sdvo_priv->dispParams.TwoD_FlickerFilter.Value = dwDefaultVal;

	    status =
		i830_sdvo_set_2D_flickerfilter(output,
					       sdvo_priv->dispParams.
					       TwoD_FlickerFilter.Value);
	    if (!status)
		return status;

	    sdvo_priv->dispParams.TwoD_FlickerFilter.Max = dwMaxVal;
	    sdvo_priv->dispParams.TwoD_FlickerFilter.Min = 0;
	    sdvo_priv->dispParams.TwoD_FlickerFilter.Default = dwDefaultVal;
	    sdvo_priv->dispParams.TwoD_FlickerFilter.Step = 1;
	} else {
	    return status;
	}
    }

    /* Luma Filter */
    if (sdvo_priv->dwSupportedEnhancements & SDVO_TV_MAX_LUMA_FILTER) {
	status =
	    i830_sdvo_get_max_lumafilter(output, &dwMaxVal, &dwDefaultVal);

	if (status) {
	    /*check whether the value is beyond the max value, min value as per EDS is always 0 so */
	    /*no need to check it. */
	    if (sdvo_priv->dispParams.LumaFilter.Value > dwMaxVal)
		sdvo_priv->dispParams.LumaFilter.Value = dwDefaultVal;

	    /* Program the device */
	    status =
		i830_sdvo_set_lumafilter(output,
					 sdvo_priv->dispParams.LumaFilter.Value);
	    if (!status)
		return status;

	    sdvo_priv->dispParams.LumaFilter.Max = dwMaxVal;
	    sdvo_priv->dispParams.LumaFilter.Min = 0;
	    sdvo_priv->dispParams.LumaFilter.Default = dwDefaultVal;
	    sdvo_priv->dispParams.LumaFilter.Step = 1;

	} else {
	    return status;
	}

    }

    /* Chroma Filter */
    if (sdvo_priv->dwSupportedEnhancements & SDVO_MAX_TV_CHROMA_FILTER) {

	status =
	    i830_sdvo_get_max_chromafilter(output, &dwMaxVal, &dwDefaultVal);

	if (status) {
	    /*check whether the value is beyond the max value, min value as per EDS is always 0 so */
	    /*no need to check it. */
	    if (sdvo_priv->dispParams.ChromaFilter.Value > dwMaxVal)
		sdvo_priv->dispParams.ChromaFilter.Value = dwDefaultVal;

	    /* Program the device */
	    status =
		i830_sdvo_set_chromafilter(output,
					   sdvo_priv->dispParams.ChromaFilter.
					   Value);
	    if (!status)
		return status;

	    sdvo_priv->dispParams.ChromaFilter.Max = dwMaxVal;
	    sdvo_priv->dispParams.ChromaFilter.Min = 0;
	    sdvo_priv->dispParams.ChromaFilter.Default = dwDefaultVal;
	    sdvo_priv->dispParams.ChromaFilter.Step = 1;
	} else {
	    return status;
	}

    }

    /* Dot Crawl */
    if (sdvo_priv->dwSupportedEnhancements & SDVO_DOT_CRAWL) {
	status = i830_sdvo_get_dotcrawl(output, &dwCurrentVal, &dwDefaultVal);

	if (status) {

	    dwMaxVal = 1;
	    /*check whether the value is beyond the max value, min value as per EDS is always 0 so */
	    /*no need to check it. */

	    /* Tibet issue 1603772: Dot crawl do not persist after reboot/Hibernate */
	    /* Details : "Dotcrawl.value" is compared with "dwDefaultVal". Since */
	    /*            dwDefaultVal is always 0, dotCrawl value is always set to 0. */
	    /* Fix     : Compare the current dotCrawl value with dwMaxValue. */

	    if (sdvo_priv->dispParams.DotCrawl.Value > dwMaxVal)

		sdvo_priv->dispParams.DotCrawl.Value = dwMaxVal;

	    status =
		i830_sdvo_set_dotcrawl(output,
				       sdvo_priv->dispParams.DotCrawl.Value);
	    if (!status)
		return status;

	    sdvo_priv->dispParams.DotCrawl.Max = dwMaxVal;
	    sdvo_priv->dispParams.DotCrawl.Min = 0;
	    sdvo_priv->dispParams.DotCrawl.Default = dwMaxVal;
	    sdvo_priv->dispParams.DotCrawl.Step = 1;
	} else {
	    return status;
	}
    }

    return TRUE;
}

static bool i830_tv_set_overscan_parameters(struct drm_output * output)
{
    u8 status;

    u32 dwDefaultVal = 0;
    u32 dwMaxVal = 0;
    u32 dwPercentageValue = 0;
    u32 dwDefOverscanXValue = 0;
    u32 dwDefOverscanYValue = 0;
    u32 dwOverscanValue = 0;
    u32 dwSupportedEnhancements;
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;


    /* Get supported picture enhancements */
    status =
	i830_sdvo_get_supported_enhancements(output,
					     &dwSupportedEnhancements);
    if (!status)
	return status;

    /* Horizontal Overscan */
    if (dwSupportedEnhancements & SDVO_HORIZONTAL_OVERSCAN) {
	status =
	    i830_sdvo_get_max_horizontal_overscan(output, &dwMaxVal,
						  &dwDefaultVal);
	if (!status)
	    return status;

	/*Calculate the default value in terms of percentage */
	dwDefOverscanXValue = ((dwDefaultVal * 100) / dwMaxVal);

	/*Calculate the default value in 0-1000 range */
	dwDefOverscanXValue = (dwDefOverscanXValue * 10);

	/*Overscan is in the range of 0 to 10000 as per MS spec */
	if (sdvo_priv->OverScanX.Value > MAX_VAL)
	    sdvo_priv->OverScanX.Value = dwDefOverscanXValue;

	/*Calculate the percentage(0-100%) of the overscan value */
	dwPercentageValue = (sdvo_priv->OverScanX.Value * 100) / 1000;

	/* Now map the % value to absolute value to be programed to the encoder */
	dwOverscanValue = (dwMaxVal * dwPercentageValue) / 100;

	status = i830_sdvo_set_horizontal_overscan(output, dwOverscanValue);
	if (!status)
	    return status;

	sdvo_priv->OverScanX.Max = 1000;
	sdvo_priv->OverScanX.Min = 0;
	sdvo_priv->OverScanX.Default = dwDefOverscanXValue;
	sdvo_priv->OverScanX.Step = 20;
    }

    /* Horizontal Overscan */
    /* vertical Overscan */
    if (dwSupportedEnhancements & SDVO_VERTICAL_OVERSCAN) {
	status =
	    i830_sdvo_get_max_vertical_overscan(output, &dwMaxVal,
						&dwDefaultVal);
	if (!status)
	    return status;

	/*Calculate the default value in terms of percentage */
	dwDefOverscanYValue = ((dwDefaultVal * 100) / dwMaxVal);

	/*Calculate the default value in 0-1000 range */
	dwDefOverscanYValue = (dwDefOverscanYValue * 10);

	/*Overscan is in the range of 0 to 10000 as per MS spec */
	if (sdvo_priv->OverScanY.Value > MAX_VAL)
	    sdvo_priv->OverScanY.Value = dwDefOverscanYValue;

	/*Calculate the percentage(0-100%) of the overscan value */
	dwPercentageValue = (sdvo_priv->OverScanY.Value * 100) / 1000;

	/* Now map the % value to absolute value to be programed to the encoder */
	dwOverscanValue = (dwMaxVal * dwPercentageValue) / 100;

	status = i830_sdvo_set_vertical_overscan(output, dwOverscanValue);
	if (!status)
	    return status;

	sdvo_priv->OverScanY.Max = 1000;
	sdvo_priv->OverScanY.Min = 0;
	sdvo_priv->OverScanY.Default = dwDefOverscanYValue;
	sdvo_priv->OverScanY.Step = 20;

    }
    /* vertical Overscan */
    return TRUE;
}

static bool i830_translate_dtd2timing(struct drm_display_mode * pTimingInfo,
			  struct intel_sdvo_dtd *pDTD)
{

    u32 dwHBLHigh = 0;
    u32 dwVBLHigh = 0;
    u32 dwHSHigh1 = 0;
    u32 dwHSHigh2 = 0;
    u32 dwVSHigh1 = 0;
    u32 dwVSHigh2 = 0;
    u32 dwVPWLow = 0;
    bool status = FALSE;

    if ((pDTD == NULL) || (pTimingInfo == NULL)) {
	return status;
    }

    pTimingInfo->clock= pDTD->part1.clock * 10000 / 1000;	/*fix me if i am wrong */

    pTimingInfo->hdisplay = pTimingInfo->crtc_hdisplay =
	(u32) pDTD->part1.
	h_active | ((u32) (pDTD->part1.h_high & 0xF0) << 4);

    pTimingInfo->vdisplay = pTimingInfo->crtc_vdisplay =
	(u32) pDTD->part1.
	v_active | ((u32) (pDTD->part1.v_high & 0xF0) << 4);

    pTimingInfo->crtc_hblank_start = pTimingInfo->crtc_hdisplay;

    /* Horizontal Total = Horizontal Active + Horizontal Blanking */
    dwHBLHigh = (u32) (pDTD->part1.h_high & 0x0F);
    pTimingInfo->htotal = pTimingInfo->crtc_htotal =
	pTimingInfo->crtc_hdisplay + (u32) pDTD->part1.h_blank +
	(dwHBLHigh << 8);

    pTimingInfo->crtc_hblank_end = pTimingInfo->crtc_htotal - 1;

    /* Vertical Total = Vertical Active + Vertical Blanking */
    dwVBLHigh = (u32) (pDTD->part1.v_high & 0x0F);
    pTimingInfo->vtotal = pTimingInfo->crtc_vtotal =
	pTimingInfo->crtc_vdisplay + (u32) pDTD->part1.v_blank +
	(dwVBLHigh << 8);
    pTimingInfo->crtc_vblank_start = pTimingInfo->crtc_vdisplay;
    pTimingInfo->crtc_vblank_end = pTimingInfo->crtc_vtotal - 1;

    /* Horz Sync Start = Horz Blank Start + Horz Sync Offset */
    dwHSHigh1 = (u32) (pDTD->part2.sync_off_width_high & 0xC0);
    pTimingInfo->hsync_start = pTimingInfo->crtc_hsync_start =
	pTimingInfo->crtc_hblank_start + (u32) pDTD->part2.h_sync_off +
	(dwHSHigh1 << 2);

    /* Horz Sync End = Horz Sync Start + Horz Sync Pulse Width */
    dwHSHigh2 = (u32) (pDTD->part2.sync_off_width_high & 0x30);
    pTimingInfo->hsync_end = pTimingInfo->crtc_hsync_end =
	pTimingInfo->crtc_hsync_start + (u32) pDTD->part2.h_sync_width +
	(dwHSHigh2 << 4) - 1;

    /* Vert Sync Start = Vert Blank Start + Vert Sync Offset */
    dwVSHigh1 = (u32) (pDTD->part2.sync_off_width_high & 0x0C);
    dwVPWLow = (u32) (pDTD->part2.v_sync_off_width & 0xF0);

    pTimingInfo->vsync_start = pTimingInfo->crtc_vsync_start =
	pTimingInfo->crtc_vblank_start + (dwVPWLow >> 4) + (dwVSHigh1 << 2);

    /* Vert Sync End = Vert Sync Start + Vert Sync Pulse Width */
    dwVSHigh2 = (u32) (pDTD->part2.sync_off_width_high & 0x03);
    pTimingInfo->vsync_end = pTimingInfo->crtc_vsync_end =
	pTimingInfo->crtc_vsync_start +
	(u32) (pDTD->part2.v_sync_off_width & 0x0F) + (dwVSHigh2 << 4) - 1;

    /* Fillup flags */
    status = TRUE;

    return status;
}

static void i830_translate_timing2dtd(struct drm_display_mode * mode, struct intel_sdvo_dtd *dtd)
{
    u16 width, height;
    u16 h_blank_len, h_sync_len, v_blank_len, v_sync_len;
    u16 h_sync_offset, v_sync_offset;

    width = mode->crtc_hdisplay;
    height = mode->crtc_vdisplay;

    /* do some mode translations */
    h_blank_len = mode->crtc_hblank_end - mode->crtc_hblank_start;
    h_sync_len = mode->crtc_hsync_end - mode->crtc_hsync_start;

    v_blank_len = mode->crtc_vblank_end - mode->crtc_vblank_start;
    v_sync_len = mode->crtc_vsync_end - mode->crtc_vsync_start;

    h_sync_offset = mode->crtc_hsync_start - mode->crtc_hblank_start;
    v_sync_offset = mode->crtc_vsync_start - mode->crtc_vblank_start;

    dtd->part1.clock = mode->clock * 1000 / 10000;	/*xiaolin, fixme, do i need to by 1k hz */
    dtd->part1.h_active = width & 0xff;
    dtd->part1.h_blank = h_blank_len & 0xff;
    dtd->part1.h_high = (((width >> 8) & 0xf) << 4) |
	((h_blank_len >> 8) & 0xf);
    dtd->part1.v_active = height & 0xff;
    dtd->part1.v_blank = v_blank_len & 0xff;
    dtd->part1.v_high = (((height >> 8) & 0xf) << 4) |
	((v_blank_len >> 8) & 0xf);

    dtd->part2.h_sync_off = h_sync_offset;
    dtd->part2.h_sync_width = h_sync_len & 0xff;
    dtd->part2.v_sync_off_width = ((v_sync_offset & 0xf) << 4 |
				   (v_sync_len & 0xf)) + 1;
    dtd->part2.sync_off_width_high = ((h_sync_offset & 0x300) >> 2) |
	((h_sync_len & 0x300) >> 4) | ((v_sync_offset & 0x30) >> 2) |
	((v_sync_len & 0x30) >> 4);

    dtd->part2.dtd_flags = 0x18;
    if (mode->flags & V_PHSYNC)
	dtd->part2.dtd_flags |= 0x2;
    if (mode->flags & V_PVSYNC)
	dtd->part2.dtd_flags |= 0x4;

    dtd->part2.sdvo_flags = 0;
    dtd->part2.v_sync_off_high = v_sync_offset & 0xc0;
    dtd->part2.reserved = 0;

}

static bool i830_tv_set_target_io(struct drm_output* output)
{
    bool status;
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;

    status = intel_sdvo_set_target_input(output, TRUE, FALSE);
    if (status)
	status = intel_sdvo_set_target_output(output, sdvo_priv->active_outputs);

    return status;
}

static bool i830_tv_get_max_min_dotclock(struct drm_output* output)
{
    u32 dwMaxClkRateMul = 1;
    u32 dwMinClkRateMul = 1;
    u8 status;

	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;


    /* Set Target Input/Outputs */
    status = i830_tv_set_target_io(output);
    if (!status) {
	DRM_DEBUG("SetTargetIO function FAILED!!! \n");
	return status;
    }

    /* Get the clock rate multiplies supported by the encoder */
    dwMinClkRateMul = 1;
#if 0
    /* why we need do this, some time, tv can't bring up for the wrong setting in the last time */
    dwClkRateMulMask = i830_sdvo_get_clock_rate_mult(output);

    /* Find the minimum clock rate multiplier supported */

    if (dwClkRateMulMask & SDVO_CLOCK_RATE_MULT_1X)
	dwMinClkRateMul = 1;
    else if (dwClkRateMulMask & SDVO_CLOCK_RATE_MULT_2X)
	dwMinClkRateMul = 2;
    else if (dwClkRateMulMask & SDVO_CLOCK_RATE_MULT_3X)
	dwMinClkRateMul = 3;
    else if (dwClkRateMulMask & SDVO_CLOCK_RATE_MULT_4X)
	dwMinClkRateMul = 4;
    else if (dwClkRateMulMask & SDVO_CLOCK_RATE_MULT_5X)
	dwMinClkRateMul = 5;
    else
	return FALSE;
#endif
    /* Get the min and max input Dot Clock supported by the encoder */
    status = i830_sdvo_get_input_output_pixelclock_range(output, FALSE);	/* input */

    if (!status) {
	DRM_DEBUG("SDVOGetInputPixelClockRange() FAILED!!! \n");
	return status;
    }

    /* Get the min and max output Dot Clock supported by the encoder */
    status = i830_sdvo_get_input_output_pixelclock_range(output, TRUE);	/* output */

    if (!status) {
	DRM_DEBUG("SDVOGetOutputPixelClockRange() FAILED!!! \n");
	return status;
    }

    /* Maximum Dot Clock supported should be the minimum of the maximum */
    /* dot clock supported by the encoder & the SDVO bus clock rate */
    sdvo_priv->dwMaxDotClk =
	((sdvo_priv->dwMaxInDotClk * dwMaxClkRateMul) <
	 (sdvo_priv->dwMaxOutDotClk)) ? (sdvo_priv->dwMaxInDotClk *
				     dwMaxClkRateMul) : (sdvo_priv->dwMaxOutDotClk);

    /* Minimum Dot Clock supported should be the maximum of the minimum */
    /* dot clocks supported by the input & output */
    sdvo_priv->dwMinDotClk =
	((sdvo_priv->dwMinInDotClk * dwMinClkRateMul) >
	 (sdvo_priv->dwMinOutDotClk)) ? (sdvo_priv->dwMinInDotClk *
				     dwMinClkRateMul) : (sdvo_priv->dwMinOutDotClk);

    DRM_DEBUG("leave, i830_tv_get_max_min_dotclock() !!! \n");

    return TRUE;

}

bool i830_tv_mode_check_support(struct drm_output* output, struct drm_display_mode* pMode)
{
    u32 dwDotClk = 0;
    bool status;
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;


    dwDotClk = pMode->clock * 1000;

    /*TODO:  Need to fix this from SoftBios side........ */
    if (sdvo_priv->TVMode == TVMODE_HDTV) {
	if (((pMode->hdisplay == 1920) && (pMode->vdisplay== 1080)) ||
	    ((pMode->hdisplay== 1864) && (pMode->vdisplay== 1050)) ||
	    ((pMode->hdisplay== 1704) && (pMode->vdisplay== 960)) ||
	    ((pMode->hdisplay== 640) && (pMode->vdisplay== 448)))
	    return true;
    }

    if (sdvo_priv->bGetClk) {
	status = i830_tv_get_max_min_dotclock(output);
	if (!status) {
	    DRM_DEBUG("get max min dotclok failed\n");
	    return status;
	}
	sdvo_priv->bGetClk = false;
    }

    /* Check the Dot clock first. If the requested Dot Clock should fall */
    /* in the supported range for the mode to be supported */
    if ((dwDotClk <= sdvo_priv->dwMinDotClk) || (dwDotClk >= sdvo_priv->dwMaxDotClk)) {
	DRM_DEBUG("dwDotClk value is out of range\n");
	/*TODO: now consider VBT add and Remove mode. */
	/* This mode can't be supported */
	return false;
    }
    DRM_DEBUG("i830_tv_mode_check_support leave\n");
    return true;

}

void print_Pll(char *prefix, ex_intel_clock_t * clock)
{
    DRM_DEBUG("%s: dotclock %d vco %d ((m %d, m1 %d, m2 %d), n %d, (p %d, p1 %d, p2 %d))\n",
	      prefix, clock->dot, clock->vco, clock->m, clock->m1, clock->m2,
	      clock->n, clock->p, clock->p1, clock->p2);
}

extern int intel_panel_fitter_pipe (struct drm_device *dev);
extern int intel_get_core_clock_speed(struct drm_device *dev);

void i830_sdvo_tv_settiming(struct drm_crtc *crtc, struct drm_display_mode * mode,
		       struct drm_display_mode * adjusted_mode)
{

	struct drm_device *dev = crtc->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;

    int pipe = 0;
    int fp_reg = (pipe == 0) ? FPA0 : FPB0;
    int dpll_reg = (pipe == 0) ? DPLL_A : DPLL_B;
    int dspcntr_reg = (pipe == 0) ? DSPACNTR : DSPBCNTR;
    int pipeconf_reg = (pipe == 0) ? PIPEACONF : PIPEBCONF;
    int htot_reg = (pipe == 0) ? HTOTAL_A : HTOTAL_B;
    int hblank_reg = (pipe == 0) ? HBLANK_A : HBLANK_B;
    int hsync_reg = (pipe == 0) ? HSYNC_A : HSYNC_B;
    int vtot_reg = (pipe == 0) ? VTOTAL_A : VTOTAL_B;
    int vblank_reg = (pipe == 0) ? VBLANK_A : VBLANK_B;
    int vsync_reg = (pipe == 0) ? VSYNC_A : VSYNC_B;
    int dspsize_reg = (pipe == 0) ? DSPASIZE : DSPBSIZE;
	int dspstride_reg = (pipe == 0) ? DSPASTRIDE : DSPBSTRIDE;
    int dsppos_reg = (pipe == 0) ? DSPAPOS : DSPBPOS;
    int pipesrc_reg = (pipe == 0) ? PIPEASRC : PIPEBSRC;
    ex_intel_clock_t clock;
    u32 dpll = 0, fp = 0, dspcntr, pipeconf;
    bool ok, is_sdvo = FALSE;
    int centerX = 0, centerY = 0;
    u32 ulPortMultiplier, ulTemp, ulDotClock;
    int sdvo_pixel_multiply;
	u32 dotclock;

    /* Set up some convenient bools for what outputs are connected to
     * our pipe, used in DPLL setup.
     */
	if (!crtc->fb) {
		DRM_ERROR("Can't set mode without attached fb\n");
		return;
	}     
    is_sdvo = TRUE;
    ok = TRUE;
    ulDotClock = mode->clock * 1000 / 1000;	/*xiaolin, fixme, do i need to by 1k hz */
    for (ulPortMultiplier = 1; ulPortMultiplier <= 5; ulPortMultiplier++) {
	ulTemp = ulDotClock * ulPortMultiplier;
	if ((ulTemp >= 100000) && (ulTemp <= 200000)) {
	    if ((ulPortMultiplier == 3) || (ulPortMultiplier == 5))
		continue;
	    else
		break;
	}
    }
    /* ulPortMultiplier is 2, dotclok is 1babc, fall into the first one case */
    /* add two to each m and n value -- optimizes (slightly) the search algo. */
    dotclock = ulPortMultiplier * (mode->clock * 1000) / 1000;
	DRM_DEBUG("mode->clock is %x, dotclock is %x,!\n", mode->clock,dotclock);

    if ((dotclock >= 100000) && (dotclock < 140500)) {
	DRM_DEBUG("dotclock is between 10000 and 140500!\n");
	clock.p1 = 0x2;
	clock.p2 = 0x00;
	clock.n = 0x3;
	clock.m1 = 0x10;
	clock.m2 = 0x8;
    } else if ((dotclock >= 140500) && (dotclock <= 200000)) {
    
	DRM_DEBUG("dotclock is between 140500 and 200000!\n");
	clock.p1 = 0x1;
	/*CG was using 0x10 from spreadsheet it should be 0 */
	/*pClock_Data->Clk_P2 = 0x10; */
	clock.p2 = 0x00;
	clock.n = 0x6;
	clock.m1 = 0xC;
	clock.m2 = 0x8;
    } else
	ok = FALSE;

    if (!ok)
	DRM_DEBUG("Couldn't find PLL settings for mode!\n");

    fp = clock.n << 16 | clock.m1 << 8 | clock.m2;

    dpll = DPLL_VGA_MODE_DIS | DPLL_CLOCK_PHASE_9;

    dpll |= DPLLB_MODE_DAC_SERIAL;

    sdvo_pixel_multiply = ulPortMultiplier;
    dpll |= DPLL_DVO_HIGH_SPEED;
    dpll |= (sdvo_pixel_multiply - 1) << SDVO_MULTIPLIER_SHIFT_HIRES;

    /* compute bitmask from p1 value */
    dpll |= (clock.p1 << 16);
    dpll |= (clock.p2 << 24);

    dpll |= PLL_REF_INPUT_TVCLKINBC;

    /* Set up the display plane register */
    dspcntr = DISPPLANE_GAMMA_ENABLE;
    switch (crtc->fb->bits_per_pixel) {
    case 8:
	dspcntr |= DISPPLANE_8BPP;
	break;
    case 16:
	if (crtc->fb->depth == 15)
	    dspcntr |= DISPPLANE_15_16BPP;
	else
	    dspcntr |= DISPPLANE_16BPP;
	break;
    case 32:
	dspcntr |= DISPPLANE_32BPP_NO_ALPHA;
	break;
    default:
	DRM_DEBUG("unknown display bpp\n");
    }

    if (pipe == 0)
	dspcntr |= DISPPLANE_SEL_PIPE_A;
    else
	dspcntr |= DISPPLANE_SEL_PIPE_B;

    pipeconf = I915_READ(pipeconf_reg);
    if (pipe == 0) {
	/* Enable pixel doubling when the dot clock is > 90% of the (display)
	 * core speed.
	 *
	 * XXX: No double-wide on 915GM pipe B. Is that the only reason for the
	 * pipe == 0 check?
	 */
	if (mode->clock * 1000 > (intel_get_core_clock_speed(dev)) * 9 / 10)	/*xiaolin, fixme, do i need to by 1k hz */
	   { pipeconf |= PIPEACONF_DOUBLE_WIDE; DRM_DEBUG("PIPEACONF_DOUBLE_WIDE\n");}
	else
	   { pipeconf &= ~PIPEACONF_DOUBLE_WIDE; DRM_DEBUG("non PIPEACONF_DOUBLE_WIDE\n");}
    }
	
	dspcntr |= DISPLAY_PLANE_ENABLE;
    pipeconf |= PIPEACONF_ENABLE;
    dpll |= DPLL_VCO_ENABLE;

    /* Disable the panel fitter if it was on our pipe */
    if (intel_panel_fitter_pipe(dev) == pipe)
	I915_WRITE(PFIT_CONTROL, 0);

    print_Pll("chosen", &clock);
	DRM_DEBUG("Mode for pipe %c:\n", pipe == 0 ? 'A' : 'B');
	drm_mode_debug_printmodeline(dev, mode);	
	DRM_DEBUG("Modeline %d:\"%s\" %d %d %d %d %d %d %d %d\n",
		  mode->mode_id, mode->name, mode->crtc_htotal, mode->crtc_hdisplay,
		  mode->crtc_hblank_end, mode->crtc_hblank_start,
		  mode->crtc_vtotal, mode->crtc_vdisplay,
		  mode->crtc_vblank_end, mode->crtc_vblank_start);	
    DRM_DEBUG("clock regs: 0x%08x, 0x%08x,dspntr is 0x%8x, pipeconf is 0x%8x\n", (int)dpll,
	      (int)fp,(int)dspcntr,(int)pipeconf);

    if (dpll & DPLL_VCO_ENABLE) {
	I915_WRITE(fp_reg, fp);
	I915_WRITE(dpll_reg, dpll & ~DPLL_VCO_ENABLE);
	(void)I915_READ(dpll_reg);
	udelay(150);
    }
    I915_WRITE(fp_reg, fp);
    I915_WRITE(dpll_reg, dpll);
    (void)I915_READ(dpll_reg);
    /* Wait for the clocks to stabilize. */
    udelay(150);

	/* write it again -- the BIOS does, after all */
	I915_WRITE(dpll_reg, dpll);
	I915_READ(dpll_reg);
	/* Wait for the clocks to stabilize. */
	udelay(150);

    I915_WRITE(htot_reg, (mode->crtc_hdisplay - 1) |
		((mode->crtc_htotal - 1) << 16));
    I915_WRITE(hblank_reg, (mode->crtc_hblank_start - 1) |
		((mode->crtc_hblank_end - 1) << 16));
    I915_WRITE(hsync_reg, (mode->crtc_hsync_start - 1) |
		((mode->crtc_hsync_end - 1) << 16));
    I915_WRITE(vtot_reg, (mode->crtc_vdisplay - 1) |
		((mode->crtc_vtotal - 1) << 16));
    I915_WRITE(vblank_reg, (mode->crtc_vblank_start - 1) |
		((mode->crtc_vblank_end - 1) << 16));
    I915_WRITE(vsync_reg, (mode->crtc_vsync_start - 1) |
		((mode->crtc_vsync_end - 1) << 16));
	I915_WRITE(dspstride_reg, crtc->fb->pitch);

    if (0) {

	centerX = (adjusted_mode->crtc_hdisplay - mode->hdisplay) / 2;
	centerY = (adjusted_mode->crtc_vdisplay - mode->vdisplay) / 2;
	I915_WRITE(dspsize_reg,
		    ((mode->vdisplay - 1) << 16) | (mode->hdisplay - 1));

	I915_WRITE(dsppos_reg, centerY << 16 | centerX);
	I915_WRITE(pipesrc_reg,
		    ((adjusted_mode->crtc_hdisplay -
		      1) << 16) | (adjusted_mode->crtc_vdisplay - 1));
    } else {
	/* pipesrc and dspsize control the size that is scaled from, which should
	 * always be the user's requested size.
	 */
	I915_WRITE(dspsize_reg,
		    ((mode->vdisplay - 1) << 16) | (mode->hdisplay - 1));
	I915_WRITE(dsppos_reg, 0);
	I915_WRITE(pipesrc_reg,
		    ((mode->hdisplay - 1) << 16) | (mode->vdisplay - 1));

    }
    I915_WRITE(pipeconf_reg, pipeconf);
    I915_READ(pipeconf_reg);
	
    intel_wait_for_vblank(dev);

    I915_WRITE(dspcntr_reg, dspcntr);
	/* Flush the plane changes */
	//intel_pipe_set_base(crtc, 0, 0);
	/* Disable the VGA plane that we never use */
	//I915_WRITE(VGACNTRL, VGA_DISP_DISABLE);	
    //intel_wait_for_vblank(dev);

}

static void intel_sdvo_mode_set(struct drm_output *output,
				struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode)
{
	struct drm_device *dev = output->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	struct drm_crtc *crtc = output->crtc;
	struct intel_crtc *intel_crtc = crtc->driver_private;
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;

	u32 sdvox;
	struct intel_sdvo_dtd output_dtd;
	int sdvo_pixel_multiply;
	bool success;
	struct drm_display_mode * save_mode;
	DRM_DEBUG("xxintel_sdvo_mode_set\n");

	if (!mode)
		return;

    if (sdvo_priv->ActiveDevice == SDVO_DEVICE_TV) {
	if (!i830_tv_mode_check_support(output, mode)) {
	    DRM_DEBUG("mode setting failed, use the forced mode\n");
	    mode = &tv_modes[0].mode_entry;
		drm_mode_set_crtcinfo(mode, 0);
	}
    }	
    save_mode = mode;
#if 0
	width = mode->crtc_hdisplay;
	height = mode->crtc_vdisplay;

	/* do some mode translations */
	h_blank_len = mode->crtc_hblank_end - mode->crtc_hblank_start;
	h_sync_len = mode->crtc_hsync_end - mode->crtc_hsync_start;

	v_blank_len = mode->crtc_vblank_end - mode->crtc_vblank_start;
	v_sync_len = mode->crtc_vsync_end - mode->crtc_vsync_start;

	h_sync_offset = mode->crtc_hsync_start - mode->crtc_hblank_start;
	v_sync_offset = mode->crtc_vsync_start - mode->crtc_vblank_start;

	output_dtd.part1.clock = mode->clock / 10;
	output_dtd.part1.h_active = width & 0xff;
	output_dtd.part1.h_blank = h_blank_len & 0xff;
	output_dtd.part1.h_high = (((width >> 8) & 0xf) << 4) |
		((h_blank_len >> 8) & 0xf);
	output_dtd.part1.v_active = height & 0xff;
	output_dtd.part1.v_blank = v_blank_len & 0xff;
	output_dtd.part1.v_high = (((height >> 8) & 0xf) << 4) |
		((v_blank_len >> 8) & 0xf);
	
	output_dtd.part2.h_sync_off = h_sync_offset;
	output_dtd.part2.h_sync_width = h_sync_len & 0xff;
	output_dtd.part2.v_sync_off_width = (v_sync_offset & 0xf) << 4 |
		(v_sync_len & 0xf);
	output_dtd.part2.sync_off_width_high = ((h_sync_offset & 0x300) >> 2) |
		((h_sync_len & 0x300) >> 4) | ((v_sync_offset & 0x30) >> 2) |
		((v_sync_len & 0x30) >> 4);
	
	output_dtd.part2.dtd_flags = 0x18;
	if (mode->flags & V_PHSYNC)
		output_dtd.part2.dtd_flags |= 0x2;
	if (mode->flags & V_PVSYNC)
		output_dtd.part2.dtd_flags |= 0x4;

	output_dtd.part2.sdvo_flags = 0;
	output_dtd.part2.v_sync_off_high = v_sync_offset & 0xc0;
	output_dtd.part2.reserved = 0;
#else
    /* disable and enable the display output */
    intel_sdvo_set_target_output(output, 0);

    //intel_sdvo_set_active_outputs(output, sdvo_priv->active_outputs);
    memset(&output_dtd, 0, sizeof(struct intel_sdvo_dtd));
    /* check if this mode can be supported or not */
	
    i830_translate_timing2dtd(mode, &output_dtd);
#endif	
    intel_sdvo_set_target_output(output, 0);
    /* set the target input & output first */
	/* Set the input timing to the screen. Assume always input 0. */
	intel_sdvo_set_target_output(output, sdvo_priv->active_outputs);
	intel_sdvo_set_output_timing(output, &output_dtd);
	intel_sdvo_set_target_input(output, true, false);

    if (sdvo_priv->ActiveDevice == SDVO_DEVICE_TV) {
	i830_tv_set_overscan_parameters(output);
	/* Set TV standard */
	#if 0
	if (sdvo_priv->TVMode == TVMODE_HDTV)
	    i830_sdvo_map_hdtvstd_bitmask(output);
	else
	    i830_sdvo_map_sdtvstd_bitmask(output);
	#endif
	/* Set TV format */
	i830_sdvo_set_tvoutputs_formats(output);
	/* We would like to use i830_sdvo_create_preferred_input_timing() to
	 * provide the device with a timing it can support, if it supports that
	 * feature.  However, presumably we would need to adjust the CRTC to output
	 * the preferred timing, and we don't support that currently.
	 */
	success = i830_sdvo_create_preferred_input_timing(output, mode);
	if (success) {
	    i830_sdvo_get_preferred_input_timing(output, &output_dtd);
	}
	/* Set the overscan values now as input timing is dependent on overscan values */

    }
	

	/* We would like to use i830_sdvo_create_preferred_input_timing() to
	 * provide the device with a timing it can support, if it supports that
	 * feature.  However, presumably we would need to adjust the CRTC to
	 * output the preferred timing, and we don't support that currently.
	 */
#if 0
	success = intel_sdvo_create_preferred_input_timing(output, clock,
							   width, height);
	if (success) {
		struct intel_sdvo_dtd *input_dtd;
		
		intel_sdvo_get_preferred_input_timing(output, &input_dtd);
		intel_sdvo_set_input_timing(output, &input_dtd);
	}
#else
    /* Set input timing (in DTD) */
	intel_sdvo_set_input_timing(output, &output_dtd);
#endif	
    if (sdvo_priv->ActiveDevice == SDVO_DEVICE_TV) {
	
	DRM_DEBUG("xxintel_sdvo_mode_set tv path\n");
	i830_tv_program_display_params(output);
	/* translate dtd 2 timing */
	i830_translate_dtd2timing(mode, &output_dtd);
	/* Program clock rate multiplier, 2x,clock is = 0x360b730 */
	if ((mode->clock * 1000 >= 24000000)
	    && (mode->clock * 1000 < 50000000)) {
	    intel_sdvo_set_clock_rate_mult(output, SDVO_CLOCK_RATE_MULT_4X);
	} else if ((mode->clock * 1000 >= 50000000)
		   && (mode->clock * 1000 < 100000000)) {
	    intel_sdvo_set_clock_rate_mult(output, SDVO_CLOCK_RATE_MULT_2X);
	} else if ((mode->clock * 1000 >= 100000000)
		   && (mode->clock * 1000 < 200000000)) {
	    intel_sdvo_set_clock_rate_mult(output, SDVO_CLOCK_RATE_MULT_1X);
	} else
	    DRM_DEBUG("i830_sdvo_set_clock_rate is failed\n");

	i830_sdvo_tv_settiming(output->crtc, mode, adjusted_mode);
	//intel_crtc_mode_set(output->crtc, mode,adjusted_mode,0,0);
	mode = save_mode;
    } else {
	DRM_DEBUG("xxintel_sdvo_mode_set - non tv path\n");
	switch (intel_sdvo_get_pixel_multiplier(mode)) {
	case 1:
		intel_sdvo_set_clock_rate_mult(output,
					       SDVO_CLOCK_RATE_MULT_1X);
		break;
	case 2:
		intel_sdvo_set_clock_rate_mult(output,
					       SDVO_CLOCK_RATE_MULT_2X);
		break;
	case 4:
		intel_sdvo_set_clock_rate_mult(output,
					       SDVO_CLOCK_RATE_MULT_4X);
		break;
	}	
    }
	/* Set the SDVO control regs. */
        if (0/*IS_I965GM(dev)*/) {
                sdvox = SDVO_BORDER_ENABLE;
        } else {
                sdvox = I915_READ(sdvo_priv->output_device);
                switch (sdvo_priv->output_device) {
                case SDVOB:
                        sdvox &= SDVOB_PRESERVE_MASK;
                        break;
                case SDVOC:
                        sdvox &= SDVOC_PRESERVE_MASK;
                        break;
                }
                sdvox |= (9 << 19) | SDVO_BORDER_ENABLE;
        }
	if (intel_crtc->pipe == 1)
		sdvox |= SDVO_PIPE_B_SELECT;

	sdvo_pixel_multiply = intel_sdvo_get_pixel_multiplier(mode);
	if (IS_I965G(dev)) {
		/* done in crtc_mode_set as the dpll_md reg must be written 
		   early */
	} else if (IS_POULSBO(dev) || IS_I945G(dev) || IS_I945GM(dev)) {
		/* done in crtc_mode_set as it lives inside the 
		   dpll register */
	} else {
		sdvox |= (sdvo_pixel_multiply - 1) << SDVO_PORT_MULTIPLY_SHIFT;
	}

	intel_sdvo_write_sdvox(output, sdvox);
	i830_sdvo_set_iomap(output);	
}

static void intel_sdvo_dpms(struct drm_output *output, int mode)
{
	struct drm_device *dev = output->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;
	u32 temp;

	DRM_DEBUG("xxintel_sdvo_dpms, dpms mode is %d, active output is %d\n",mode,sdvo_priv->active_outputs);

#ifdef SII_1392_WA
	if((SII_1392==1) && (drm_psb_no_fb ==1)) {
		DRM_DEBUG("don't touch 1392 card when no_fb=1\n");
		return;
	}
#endif

	if (mode != DPMSModeOn) {
		intel_sdvo_set_active_outputs(output, sdvo_priv->output_device);
		if (0)
			intel_sdvo_set_encoder_power_state(output, mode);

		if (mode == DPMSModeOff) {
			temp = I915_READ(sdvo_priv->output_device);
			if ((temp & SDVO_ENABLE) != 0) {
				intel_sdvo_write_sdvox(output, temp & ~SDVO_ENABLE);
			}
		}
	} else {
		bool input1, input2;
		int i;
		u8 status;
		
		temp = I915_READ(sdvo_priv->output_device);
		if ((temp & SDVO_ENABLE) == 0)
			intel_sdvo_write_sdvox(output, temp | SDVO_ENABLE);
		for (i = 0; i < 2; i++)
		  intel_wait_for_vblank(dev);
		
		status = intel_sdvo_get_trained_inputs(output, &input1,
						       &input2);

		
		/* Warn if the device reported failure to sync. 
		 * A lot of SDVO devices fail to notify of sync, but it's
		 * a given it the status is a success, we succeeded.
		 */
		if (status == SDVO_CMD_STATUS_SUCCESS && !input1) {
			DRM_DEBUG("First %s output reported failure to sync\n",
				   SDVO_NAME(sdvo_priv));
		}
		
		if (0)
			intel_sdvo_set_encoder_power_state(output, mode);
		
		DRM_DEBUG("xiaolin active output is %d\n",sdvo_priv->active_outputs);
		intel_sdvo_set_active_outputs(output, sdvo_priv->active_outputs);
	}	
	return;
}

static void intel_sdvo_save(struct drm_output *output)
{
	struct drm_device *dev = output->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;

	DRM_DEBUG("xxintel_sdvo_save\n");

	sdvo_priv->save_sdvo_mult = intel_sdvo_get_clock_rate_mult(output);
	intel_sdvo_get_active_outputs(output, &sdvo_priv->save_active_outputs);

	if (sdvo_priv->caps.sdvo_inputs_mask & 0x1) {
		intel_sdvo_set_target_input(output, true, false);
		intel_sdvo_get_input_timing(output,
					    &sdvo_priv->save_input_dtd_1);
	}

	if (sdvo_priv->caps.sdvo_inputs_mask & 0x2) {
		intel_sdvo_set_target_input(output, false, true);
		intel_sdvo_get_input_timing(output,
					    &sdvo_priv->save_input_dtd_2);
	}

    intel_sdvo_set_target_output(output, sdvo_priv->active_outputs);
    intel_sdvo_get_output_timing(output,
				&sdvo_priv->save_output_dtd[sdvo_priv->active_outputs]);	
	sdvo_priv->save_SDVOX = I915_READ(sdvo_priv->output_device);
}

static void intel_sdvo_restore(struct drm_output *output)
{
	struct drm_device *dev = output->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;
	int i;
	bool input1, input2;
	u8 status;
	DRM_DEBUG("xxintel_sdvo_restore\n");

	intel_sdvo_set_active_outputs(output, 0);

    intel_sdvo_set_target_output(output, sdvo_priv->save_active_outputs);
    intel_sdvo_set_output_timing(output,
				&sdvo_priv->save_output_dtd[sdvo_priv->save_active_outputs]);
	if (sdvo_priv->caps.sdvo_inputs_mask & 0x1) {
		intel_sdvo_set_target_input(output, true, false);
		intel_sdvo_set_input_timing(output, &sdvo_priv->save_input_dtd_1);
	}

	if (sdvo_priv->caps.sdvo_inputs_mask & 0x2) {
		intel_sdvo_set_target_input(output, false, true);
		intel_sdvo_set_input_timing(output, &sdvo_priv->save_input_dtd_2);
	}
	
	intel_sdvo_set_clock_rate_mult(output, sdvo_priv->save_sdvo_mult);
	
	I915_WRITE(sdvo_priv->output_device, sdvo_priv->save_SDVOX);
	
	if (sdvo_priv->save_SDVOX & SDVO_ENABLE)
	{
		for (i = 0; i < 2; i++)
			intel_wait_for_vblank(dev);
		status = intel_sdvo_get_trained_inputs(output, &input1, &input2);
		if (status == SDVO_CMD_STATUS_SUCCESS && !input1)
			DRM_DEBUG("First %s output reported failure to sync\n",
				   SDVO_NAME(sdvo_priv));
	}
	
    i830_sdvo_set_iomap(output);	
	intel_sdvo_set_active_outputs(output, sdvo_priv->save_active_outputs);
}

static bool i830_tv_mode_find(struct drm_output * output,struct drm_display_mode * pMode)
{
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;

    bool find = FALSE;
    int i;
	
    DRM_DEBUG("i830_tv_mode_find,0x%x\n", sdvo_priv->TVStandard);
	
    for (i = 0; i < NUM_TV_MODES; i++) 
    {
       const tv_mode_t *tv_mode = &tv_modes[i];
       if (strcmp (tv_mode->mode_entry.name, pMode->name) == 0
                           && (pMode->type & M_T_TV)) {
           find = TRUE;
           break;
       }
    }
    return find;
}


static int intel_sdvo_mode_valid(struct drm_output *output,
				 struct drm_display_mode *mode)
{
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;
	
	bool status = TRUE;
	DRM_DEBUG("xxintel_sdvo_mode_valid\n");

	if (sdvo_priv->ActiveDevice == SDVO_DEVICE_TV) {
	   status = i830_tv_mode_check_support(output, mode);
	   if (status) {
		   if(i830_tv_mode_find(output,mode)) {
			   DRM_DEBUG("%s is ok\n", mode->name);
			   return MODE_OK;
		   }
		   else
			   return MODE_CLOCK_RANGE;
	   } else {
		   DRM_DEBUG("%s is failed\n",
				 mode->name);
		   return MODE_CLOCK_RANGE;
	   }
	}

	if (mode->flags & V_DBLSCAN)
		return MODE_NO_DBLESCAN;

	if (sdvo_priv->pixel_clock_min > mode->clock)
		return MODE_CLOCK_LOW;

	if (sdvo_priv->pixel_clock_max < mode->clock)
		return MODE_CLOCK_HIGH;

	return MODE_OK;
}

static bool intel_sdvo_get_capabilities(struct drm_output *output, struct intel_sdvo_caps *caps)
{
	u8 status;

	intel_sdvo_write_cmd(output, SDVO_CMD_GET_DEVICE_CAPS, NULL, 0);
	status = intel_sdvo_read_response(output, caps, sizeof(*caps));
	if (status != SDVO_CMD_STATUS_SUCCESS)
		return false;

	return true;
}

void i830_tv_get_default_params(struct drm_output * output)
{
    u32 dwSupportedSDTVBitMask = 0;
    u32 dwSupportedHDTVBitMask = 0;
	u32 dwTVStdBitmask = 0;

	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;	


    /* Get supported TV Standard */
    i830_sdvo_get_supported_tvoutput_formats(output, &dwSupportedSDTVBitMask,
					     &dwSupportedHDTVBitMask,&dwTVStdBitmask);

    sdvo_priv->dwSDVOSDTVBitMask = dwSupportedSDTVBitMask;
    sdvo_priv->dwSDVOHDTVBitMask = dwSupportedHDTVBitMask;
	sdvo_priv->TVStdBitmask = dwTVStdBitmask;

}

static enum drm_output_status intel_sdvo_detect(struct drm_output *output)
{
	u8 response[2];
	u8 status;
	u8 count = 5;

    char deviceName[256];
    char *name_suffix;
	char *name_prefix;
	unsigned char bytes[2];
        
	struct drm_device *dev = output->dev;
	
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;	
	
	DRM_DEBUG("xxintel_sdvo_detect\n");
    intel_sdvo_dpms(output, DPMSModeOn);
 
    if (!intel_sdvo_get_capabilities(output, &sdvo_priv->caps)) {
        /*No SDVO support, power down the pipe */
        intel_sdvo_dpms(output, DPMSModeOff);
        return output_status_disconnected;
    }

#ifdef SII_1392_WA
	if ((sdvo_priv->caps.vendor_id == 0x04) && (sdvo_priv->caps.device_id==0xAE)){
	    /*Leave the control of 1392 to X server*/
		SII_1392=1;
		printk("%s: detect 1392 card, leave the setting to up level\n", __FUNCTION__);
		if (drm_psb_no_fb == 0)
			intel_sdvo_dpms(output, DPMSModeOff);
		return output_status_disconnected;
	}
#endif
    while (count--) {
	intel_sdvo_write_cmd(output, SDVO_CMD_GET_ATTACHED_DISPLAYS, NULL, 0);
	status = intel_sdvo_read_response(output, &response, 2);

	if(count >3 && status == SDVO_CMD_STATUS_PENDING) {
		intel_sdvo_write_cmd(output,SDVO_CMD_RESET,NULL,0);
		intel_sdvo_read_response(output, &response, 2);
		continue;
	}

	if ((status != SDVO_CMD_STATUS_SUCCESS) || (response[0] == 0 && response[1] == 0)) {
	    udelay(500);
	    continue;		
    } else
     	break;
    }	
    if (response[0] != 0 || response[1] != 0) {
	/*Check what device types are connected to the hardware CRT/HDTV/S-Video/Composite */
	/*in case of CRT and multiple TV's attached give preference in the order mentioned below */
	/* 1. RGB */
	/* 2. HDTV */
	/* 3. S-Video */
	/* 4. composite */
	if (sdvo_priv->caps.output_flags & SDVO_OUTPUT_TMDS0) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_TMDS0;
	    output->subpixel_order = SubPixelHorizontalRGB;
	    name_prefix = "TMDS";
	    sdvo_priv->ActiveDevice = SDVO_DEVICE_TMDS;
	} else if (sdvo_priv->caps.output_flags & SDVO_OUTPUT_TMDS1) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_TMDS1;
	    output->subpixel_order = SubPixelHorizontalRGB;
	    name_prefix = "TMDS";
	    sdvo_priv->ActiveDevice = SDVO_DEVICE_TMDS;
	} else if (response[0] & SDVO_OUTPUT_RGB0) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_RGB0;
	    output->subpixel_order = SubPixelHorizontalRGB;
	    name_prefix = "RGB0";
	    sdvo_priv->ActiveDevice = SDVO_DEVICE_CRT;
	} else if ((response[1] << 8 | response[0]) & SDVO_OUTPUT_RGB1) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_RGB1;
	    output->subpixel_order = SubPixelHorizontalRGB;
	    name_prefix = "RGB1";
	    sdvo_priv->ActiveDevice = SDVO_DEVICE_CRT;
	} else if (response[0] & SDVO_OUTPUT_YPRPB0) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_YPRPB0;
	} else if ((response[1] << 8 | response[0]) & SDVO_OUTPUT_YPRPB1) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_YPRPB1;
	}
	/* SCART is given Second preference */
	else if (response[0] & SDVO_OUTPUT_SCART0) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_SCART0;

	} else if ((response[1] << 8 | response[0]) & SDVO_OUTPUT_SCART1) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_SCART1;
	}
	/* if S-Video type TV is connected along with Composite type TV give preference to S-Video */
	else if (response[0] & SDVO_OUTPUT_SVID0) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_SVID0;

	} else if ((response[1] << 8 | response[0]) & SDVO_OUTPUT_SVID1) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_SVID1;
	}
	/* Composite is given least preference */
	else if (response[0] & SDVO_OUTPUT_CVBS0) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_CVBS0;
	} else if ((response[1] << 8 | response[0]) & SDVO_OUTPUT_CVBS1) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_CVBS1;
	} else {
	    DRM_DEBUG("no display attached\n");

	    memcpy(bytes, &sdvo_priv->caps.output_flags, 2);
	    DRM_DEBUG("%s: No active TMDS or RGB outputs (0x%02x%02x) 0x%08x\n",
		       SDVO_NAME(sdvo_priv), bytes[0], bytes[1],
		       sdvo_priv->caps.output_flags);
	    name_prefix = "Unknown";
	}

	/* init para for TV connector */
	if (sdvo_priv->active_outputs & SDVO_OUTPUT_TV0) {
		DRM_INFO("TV is attaced\n");
	    output->subpixel_order = SubPixelHorizontalRGB;
	    name_prefix = "TV0";
	    /* Init TV mode setting para */
	    sdvo_priv->ActiveDevice = SDVO_DEVICE_TV;
	    sdvo_priv->bGetClk = TRUE;
            if (sdvo_priv->active_outputs == SDVO_OUTPUT_YPRPB0 ||
                                         sdvo_priv->active_outputs == SDVO_OUTPUT_YPRPB1) {
            /*sdvo_priv->TVStandard = HDTV_SMPTE_274M_1080i60;*/
                sdvo_priv->TVMode = TVMODE_HDTV;
            } else {
            /*sdvo_priv->TVStandard = TVSTANDARD_NTSC_M;*/
                sdvo_priv->TVMode = TVMODE_SDTV;
            }
			
	    /*intel_output->pDevice->TVEnabled = TRUE;*/
		
		i830_tv_get_default_params(output);
	    /*Init Display parameter for TV */
	    sdvo_priv->OverScanX.Value = 0xffffffff;
	    sdvo_priv->OverScanY.Value = 0xffffffff;
	    sdvo_priv->dispParams.Brightness.Value = 0x80;
	    sdvo_priv->dispParams.FlickerFilter.Value = 0xffffffff;
	    sdvo_priv->dispParams.AdaptiveFF.Value = 7;
	    sdvo_priv->dispParams.TwoD_FlickerFilter.Value = 0xffffffff;
	    sdvo_priv->dispParams.Contrast.Value = 0x40;
	    sdvo_priv->dispParams.PositionX.Value = 0x200;
	    sdvo_priv->dispParams.PositionY.Value = 0x200;
	    sdvo_priv->dispParams.DotCrawl.Value = 1;
	    sdvo_priv->dispParams.ChromaFilter.Value = 1;
	    sdvo_priv->dispParams.LumaFilter.Value = 2;
	    sdvo_priv->dispParams.Sharpness.Value = 4;
	    sdvo_priv->dispParams.Saturation.Value = 0x45;
	    sdvo_priv->dispParams.Hue.Value = 0x40;
	    sdvo_priv->dispParams.Dither.Value = 0;
		
	}
	else {
            name_prefix = "RGB0";
	    DRM_INFO("non TV is attaced\n");
	}
        if (sdvo_priv->output_device == SDVOB) {
	    name_suffix = "-1";
	} else {
	    name_suffix = "-2";
	}

	strcpy(deviceName, name_prefix);
	strcat(deviceName, name_suffix);

	if(output->name && (strcmp(output->name,deviceName) != 0)){
	    DRM_DEBUG("change the output name to %s\n", deviceName);
	    if (!drm_output_rename(output, deviceName)) {
		drm_output_destroy(output);
		return output_status_disconnected;
	    }

	}
	i830_sdvo_set_iomap(output);

	DRM_INFO("get attached displays=0x%x,0x%x,connectedouputs=0x%x\n",
		  response[0], response[1], sdvo_priv->active_outputs);
		return output_status_connected;
    } else {
        /*No SDVO display device attached */
        intel_sdvo_dpms(output, DPMSModeOff);
		sdvo_priv->ActiveDevice = SDVO_DEVICE_NONE;
		return output_status_disconnected;
    }	
}

static int i830_sdvo_get_tvmode_from_table(struct drm_output *output)
{
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;
	struct drm_device *dev = output->dev;

	int i, modes = 0;

	for (i = 0; i < NUM_TV_MODES; i++)
		if (((sdvo_priv->TVMode == TVMODE_HDTV) && /*hdtv mode list */
		(tv_modes[i].dwSupportedHDTVvss & TVSTANDARD_HDTV_ALL)) ||
		((sdvo_priv->TVMode == TVMODE_SDTV) && /*sdtv mode list */
		(tv_modes[i].dwSupportedSDTVvss & TVSTANDARD_SDTV_ALL))) {
			struct drm_display_mode *newmode;
			newmode = drm_mode_duplicate(dev, &tv_modes[i].mode_entry);		
			drm_mode_set_crtcinfo(newmode,0);
			drm_mode_probed_add(output, newmode);
			modes++;
		}

	return modes;	

}

static int intel_sdvo_get_modes(struct drm_output *output)
{
	struct intel_output *intel_output = output->driver_private;
	struct intel_sdvo_priv *sdvo_priv = intel_output->dev_priv;

	DRM_DEBUG("xxintel_sdvo_get_modes\n");

	if (sdvo_priv->ActiveDevice == SDVO_DEVICE_TV) {
		DRM_DEBUG("SDVO_DEVICE_TV\n");
		i830_sdvo_get_tvmode_from_table(output);
		if (list_empty(&output->probed_modes))
			return 0;
		return 1;

	} else {
	/* set the bus switch and get the modes */
	intel_sdvo_set_control_bus_switch(output, SDVO_CONTROL_BUS_DDC2);
	intel_ddc_get_modes(output);

	if (list_empty(&output->probed_modes))
		return 0;
	return 1;
	}
#if 0
	/* Mac mini hack.  On this device, I get DDC through the analog, which
	 * load-detects as disconnected.  I fail to DDC through the SDVO DDC,
	 * but it does load-detect as connected.  So, just steal the DDC bits 
	 * from analog when we fail at finding it the right way.
	 */
	/* TODO */
	return NULL;

	return NULL;
#endif
}

static void intel_sdvo_destroy(struct drm_output *output)
{
	struct intel_output *intel_output = output->driver_private;
	DRM_DEBUG("xxintel_sdvo_destroy\n");

	if (intel_output->i2c_bus)
		intel_i2c_destroy(intel_output->i2c_bus);

	if (intel_output) {
		kfree(intel_output);
		output->driver_private = NULL;
	}
}

static const struct drm_output_funcs intel_sdvo_output_funcs = {
	.dpms = intel_sdvo_dpms,
	.save = intel_sdvo_save,
	.restore = intel_sdvo_restore,
	.mode_valid = intel_sdvo_mode_valid,
	.mode_fixup = intel_sdvo_mode_fixup,
	.prepare = intel_output_prepare,
	.mode_set = intel_sdvo_mode_set,
	.commit = intel_output_commit,
	.detect = intel_sdvo_detect,
	.get_modes = intel_sdvo_get_modes,
	.cleanup = intel_sdvo_destroy
};

extern char hotplug_env;
static int intel_sdvo_proc_read_hotplug(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
        memset(buf, 0, count);
        
        if (count < 1)
           return 0;
   
        wait_event_interruptible(hotplug_queue, hotplug_env == '1');
        buf[0] = hotplug_env;

        return count; 
}

static int intel_sdvo_proc_write_hotplug(struct file *file, const char * user_buffer, unsigned long count, void *data)
{
        hotplug_env = '0';
	return count;
}

void intel_sdvo_init(struct drm_device *dev, int output_device)
{
	struct drm_output *output;
	struct intel_output *intel_output;
	struct intel_sdvo_priv *sdvo_priv;
	struct intel_i2c_chan *i2cbus = NULL;
	u8 ch[0x40];
	int i;
	char name[DRM_OUTPUT_LEN];
	char *name_prefix;
	char *name_suffix;

	int count = 3;
	u8 response[2];
	u8 status;	
	unsigned char bytes[2];

	struct proc_dir_entry *ent;
	char name_hotplug[64] = "dri/sdvo";
	char name_file[64] = "hotplug";

	DRM_DEBUG("xxintel_sdvo_init\n");
	
	init_waitqueue_head(&hotplug_queue);

	if (IS_POULSBO(dev)) {
		struct pci_dev * pci_root = pci_get_bus_and_slot(0, 0);
		u32 sku_value = 0;
		bool sku_bSDVOEnable = true;
		if(pci_root)
		{
			pci_write_config_dword(pci_root, 0xD0, PCI_PORT5_REG80_FFUSE);
			pci_read_config_dword(pci_root, 0xD4, &sku_value);
			sku_bSDVOEnable = (sku_value & PCI_PORT5_REG80_SDVO_DISABLE)?false : true;
			DRM_INFO("intel_sdvo_init: sku_value is 0x%08x\n", sku_value);
			DRM_INFO("intel_sdvo_init: sku_bSDVOEnable is %d\n", sku_bSDVOEnable);
			if (sku_bSDVOEnable == false)
				return;
		}
	}

	output = drm_output_create(dev, &intel_sdvo_output_funcs, NULL);
	if (!output)
		return;

	intel_output = kcalloc(sizeof(struct intel_output)+sizeof(struct intel_sdvo_priv), 1, GFP_KERNEL);
	if (!intel_output) {
		drm_output_destroy(output);
		return;
	}

	sdvo_priv = (struct intel_sdvo_priv *)(intel_output + 1);
	intel_output->type = INTEL_OUTPUT_SDVO;
	output->driver_private = intel_output;
	output->interlace_allowed = 0;
	output->doublescan_allowed = 0;

	/* setup the DDC bus. */
	if (output_device == SDVOB)
		i2cbus = intel_i2c_create(dev, GPIOE, "SDVOCTRL_E for SDVOB");
	else
		i2cbus = intel_i2c_create(dev, GPIOE, "SDVOCTRL_E for SDVOC");

	if (i2cbus == NULL) {
		drm_output_destroy(output);
		return;
	}

	sdvo_priv->i2c_bus = i2cbus;

	if (output_device == SDVOB) {
		name_suffix = "-1";
		sdvo_priv->i2c_bus->slave_addr = 0x38;
		sdvo_priv->byInputWiring = SDVOB_IN0;
	} else {
		name_suffix = "-2";
		sdvo_priv->i2c_bus->slave_addr = 0x39;
	}

	sdvo_priv->output_device = output_device;
	intel_output->i2c_bus = i2cbus;
	intel_output->dev_priv = sdvo_priv;


	/* Read the regs to test if we can talk to the device */
	for (i = 0; i < 0x40; i++) {
		if (!intel_sdvo_read_byte(output, i, &ch[i])) {
			DRM_DEBUG("No SDVO device found on SDVO%c\n",
				  output_device == SDVOB ? 'B' : 'C');
			drm_output_destroy(output);
			return;
		}
	} 
 

	proc_sdvo_dir = proc_mkdir(name_hotplug, NULL);
	if (!proc_sdvo_dir) {
		printk("create /proc/dri/sdvo folder error\n");
	}

	ent = create_proc_entry(name_file,
			S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, proc_sdvo_dir);
	if (!ent) {
		printk("create /proc/dri/sdvo/hotplug error\n");
	}

	ent->read_proc = intel_sdvo_proc_read_hotplug;
	ent->write_proc = intel_sdvo_proc_write_hotplug;
	ent->data = dev;

	intel_sdvo_get_capabilities(output, &sdvo_priv->caps);

	// Set active hot-plug OpCode.
	uint8_t  state_orig;
	uint8_t  state_set;
	uint8_t  byArgs_orig[2];
	uint8_t  byArgs_set[2];
	uint32_t value;

	intel_sdvo_write_cmd(output, SDVO_CMD_GET_ACTIVE_HOT_PLUG, NULL, 0);
	state_orig = intel_sdvo_read_response(output, byArgs_orig, 2);

	value = (uint32_t)byArgs_orig[1];
	value = (value << 8);
	value |= (uint32_t)byArgs_orig[0];

	value = value | (0x1);

	byArgs_orig[0] = (uint8_t)(value & 0xFF);
	byArgs_orig[1] = (uint8_t)((value >> 8) & 0xFF);
	intel_sdvo_write_cmd(output, SDVO_CMD_SET_ACTIVE_HOT_PLUG, byArgs_orig, 2);
	intel_sdvo_write_cmd(output, SDVO_CMD_GET_ACTIVE_HOT_PLUG, NULL, 0);
	state_set = intel_sdvo_read_response(output, byArgs_set, 2);

#ifdef SII_1392_WA
	if ((sdvo_priv->caps.vendor_id == 0x04) && (sdvo_priv->caps.device_id==0xAE)){
	    /*Leave the control of 1392 to X server*/
		SII_1392=1;
		printk("%s: detect 1392 card, leave the setting to up level\n", __FUNCTION__);
		if (drm_psb_no_fb == 0)
			intel_sdvo_dpms(output, DPMSModeOff);
		sdvo_priv->active_outputs = 0;
		output->subpixel_order = SubPixelHorizontalRGB;
		name_prefix = "SDVO";
		sdvo_priv->ActiveDevice = SDVO_DEVICE_NONE;
		strcpy(name, name_prefix);
		strcat(name, name_suffix);
		if (!drm_output_rename(output, name)) {
			drm_output_destroy(output);
			return;
        	}
		return;
	}
#endif
	memset(&sdvo_priv->active_outputs, 0, sizeof(sdvo_priv->active_outputs));

    while (count--) {
	intel_sdvo_write_cmd(output, SDVO_CMD_GET_ATTACHED_DISPLAYS, NULL, 0);
	status = intel_sdvo_read_response(output, &response, 2);

	if (status != SDVO_CMD_STATUS_SUCCESS) {
	    udelay(1000);
	    continue;
	}
	if (status == SDVO_CMD_STATUS_SUCCESS)
		break;
    }
    if (response[0] != 0 || response[1] != 0) {
	/*Check what device types are connected to the hardware CRT/HDTV/S-Video/Composite */
	/*in case of CRT and multiple TV's attached give preference in the order mentioned below */
	/* 1. RGB */
	/* 2. HDTV */
	/* 3. S-Video */
	/* 4. composite */
	if (sdvo_priv->caps.output_flags & SDVO_OUTPUT_TMDS0) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_TMDS0;
	    output->subpixel_order = SubPixelHorizontalRGB;
	    name_prefix = "TMDS";
	    sdvo_priv->ActiveDevice = SDVO_DEVICE_TMDS;
	} else if (sdvo_priv->caps.output_flags & SDVO_OUTPUT_TMDS1) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_TMDS1;
	    output->subpixel_order = SubPixelHorizontalRGB;
	    name_prefix = "TMDS";
	    sdvo_priv->ActiveDevice = SDVO_DEVICE_TMDS;
	} else if (response[0] & SDVO_OUTPUT_RGB0) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_RGB0;
	    output->subpixel_order = SubPixelHorizontalRGB;
	    name_prefix = "RGB0";
	    sdvo_priv->ActiveDevice = SDVO_DEVICE_CRT;
	} else if ((response[1] << 8 | response[0]) & SDVO_OUTPUT_RGB1) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_RGB1;
	    output->subpixel_order = SubPixelHorizontalRGB;
	    name_prefix = "RGB1";
	    sdvo_priv->ActiveDevice = SDVO_DEVICE_CRT;
	} else if (response[0] & SDVO_OUTPUT_YPRPB0) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_YPRPB0;
	} else if ((response[1] << 8 | response[0]) & SDVO_OUTPUT_YPRPB1) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_YPRPB1;
	}
	/* SCART is given Second preference */
	else if (response[0] & SDVO_OUTPUT_SCART0) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_SCART0;

	} else if ((response[1] << 8 | response[0]) & SDVO_OUTPUT_SCART1) {
	    sdvo_priv->active_outputs = SDVO_OUTPUT_SCART1;
	}
        /* if S-Video type TV is connected along with Composite type TV give preference to S-Video */
        else if (response[0] & SDVO_OUTPUT_SVID0) {
            sdvo_priv->active_outputs = SDVO_OUTPUT_SVID0;

        } else if ((response[1] << 8 | response[0]) & SDVO_OUTPUT_SVID1) {
            sdvo_priv->active_outputs = SDVO_OUTPUT_SVID1;
        }
        /* Composite is given least preference */
        else if (response[0] & SDVO_OUTPUT_CVBS0) {
            sdvo_priv->active_outputs = SDVO_OUTPUT_CVBS0;
        } else if ((response[1] << 8 | response[0]) & SDVO_OUTPUT_CVBS1) {
            sdvo_priv->active_outputs = SDVO_OUTPUT_CVBS1;
        } else {
            DRM_DEBUG("no display attached\n");

            memcpy(bytes, &sdvo_priv->caps.output_flags, 2);
            DRM_INFO("%s: No active TMDS or RGB outputs (0x%02x%02x) 0x%08x\n",
                       SDVO_NAME(sdvo_priv), bytes[0], bytes[1],
                       sdvo_priv->caps.output_flags);
            name_prefix = "Unknown";
        }

         /* init para for TV connector */
        if (sdvo_priv->active_outputs & SDVO_OUTPUT_TV0) {
			DRM_INFO("TV is attaced\n");
            output->subpixel_order = SubPixelHorizontalRGB;
            name_prefix = "TV0";
            /* Init TV mode setting para */
            sdvo_priv->ActiveDevice = SDVO_DEVICE_TV;
            sdvo_priv->bGetClk = TRUE;
            if (sdvo_priv->active_outputs == SDVO_OUTPUT_YPRPB0 ||
                                         sdvo_priv->active_outputs == SDVO_OUTPUT_YPRPB1) {
                sdvo_priv->TVStandard = HDTV_SMPTE_274M_1080i60;
                sdvo_priv->TVMode = TVMODE_HDTV;
            } else {
                sdvo_priv->TVStandard = TVSTANDARD_NTSC_M;
                sdvo_priv->TVMode = TVMODE_SDTV;
            }
            /*intel_output->pDevice->TVEnabled = TRUE;*/
             /*Init Display parameter for TV */
            sdvo_priv->OverScanX.Value = 0xffffffff;
            sdvo_priv->OverScanY.Value = 0xffffffff;
            sdvo_priv->dispParams.Brightness.Value = 0x80;
            sdvo_priv->dispParams.FlickerFilter.Value = 0xffffffff;
            sdvo_priv->dispParams.AdaptiveFF.Value = 7;
            sdvo_priv->dispParams.TwoD_FlickerFilter.Value = 0xffffffff;
            sdvo_priv->dispParams.Contrast.Value = 0x40;
            sdvo_priv->dispParams.PositionX.Value = 0x200;
            sdvo_priv->dispParams.PositionY.Value = 0x200;
            sdvo_priv->dispParams.DotCrawl.Value = 1;
            sdvo_priv->dispParams.ChromaFilter.Value = 1;
            sdvo_priv->dispParams.LumaFilter.Value = 2;
            sdvo_priv->dispParams.Sharpness.Value = 4;
            sdvo_priv->dispParams.Saturation.Value = 0x45;
            sdvo_priv->dispParams.Hue.Value = 0x40;
            sdvo_priv->dispParams.Dither.Value = 0;
        }
        else {
            name_prefix = "RGB0";
	    DRM_INFO("non TV is attaced\n");
        }
        
        strcpy(name, name_prefix);
        strcat(name, name_suffix);
        if (!drm_output_rename(output, name)) {
            drm_output_destroy(output);
            return;
        }
    } else {
        /*No SDVO display device attached */
        intel_sdvo_dpms(output, DPMSModeOff);
        sdvo_priv->active_outputs = 0;
        output->subpixel_order = SubPixelHorizontalRGB;
        name_prefix = "SDVO";
        sdvo_priv->ActiveDevice = SDVO_DEVICE_NONE;
        strcpy(name, name_prefix);
        strcat(name, name_suffix);
        if (!drm_output_rename(output, name)) {
            drm_output_destroy(output);
            return;
        }

    }

	/*(void)intel_sdvo_set_active_outputs(output, sdvo_priv->active_outputs);*/

	/* Set the input timing to the screen. Assume always input 0. */
	intel_sdvo_set_target_input(output, true, false);
	
	intel_sdvo_get_input_pixel_clock_range(output,
					       &sdvo_priv->pixel_clock_min,
					       &sdvo_priv->pixel_clock_max);


	DRM_DEBUG("%s device VID/DID: %02X:%02X.%02X, "
		  "clock range %dMHz - %dMHz, "
		  "input 1: %c, input 2: %c, "
		  "output 1: %c, output 2: %c\n",
		  SDVO_NAME(sdvo_priv),
		  sdvo_priv->caps.vendor_id, sdvo_priv->caps.device_id,
		  sdvo_priv->caps.device_rev_id,
		  sdvo_priv->pixel_clock_min / 1000,
		  sdvo_priv->pixel_clock_max / 1000,
		  (sdvo_priv->caps.sdvo_inputs_mask & 0x1) ? 'Y' : 'N',
		  (sdvo_priv->caps.sdvo_inputs_mask & 0x2) ? 'Y' : 'N',
		  /* check currently supported outputs */
		  sdvo_priv->caps.output_flags & 
		  	(SDVO_OUTPUT_TMDS0 | SDVO_OUTPUT_RGB0) ? 'Y' : 'N',
		  sdvo_priv->caps.output_flags & 
		  	(SDVO_OUTPUT_TMDS1 | SDVO_OUTPUT_RGB1) ? 'Y' : 'N');

	intel_output->ddc_bus = i2cbus;	
}
