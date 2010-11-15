/*
 * Copyright ?2006-2007 Intel Corporation
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

/**
 * @file SDVO command definitions and structures.
 */

#define SDVO_OUTPUT_FIRST   (0)
#define SDVO_OUTPUT_TMDS0   (1 << 0)
#define SDVO_OUTPUT_RGB0    (1 << 1)
#define SDVO_OUTPUT_CVBS0   (1 << 2)
#define SDVO_OUTPUT_SVID0   (1 << 3)
#define SDVO_OUTPUT_YPRPB0  (1 << 4)
#define SDVO_OUTPUT_SCART0  (1 << 5)
#define SDVO_OUTPUT_LVDS0   (1 << 6)
#define SDVO_OUTPUT_TMDS1   (1 << 8)
#define SDVO_OUTPUT_RGB1    (1 << 9)
#define SDVO_OUTPUT_CVBS1   (1 << 10)
#define SDVO_OUTPUT_SVID1   (1 << 11)
#define SDVO_OUTPUT_YPRPB1  (1 << 12)
#define SDVO_OUTPUT_SCART1  (1 << 13)
#define SDVO_OUTPUT_LVDS1   (1 << 14)
#define SDVO_OUTPUT_LAST    (14)

struct intel_sdvo_caps {
    u8 vendor_id;
    u8 device_id;
    u8 device_rev_id;
    u8 sdvo_version_major;
    u8 sdvo_version_minor;
    unsigned int sdvo_inputs_mask:2;
    unsigned int smooth_scaling:1;
    unsigned int sharp_scaling:1;
    unsigned int up_scaling:1;
    unsigned int down_scaling:1;
    unsigned int stall_support:1;
    unsigned int pad:1;
    u16 output_flags;
} __attribute__((packed));

/** This matches the EDID DTD structure, more or less */
struct intel_sdvo_dtd {
    struct {
	u16 clock;		/**< pixel clock, in 10kHz units */
	u8 h_active;		/**< lower 8 bits (pixels) */
	u8 h_blank;		/**< lower 8 bits (pixels) */
	u8 h_high;		/**< upper 4 bits each h_active, h_blank */
	u8 v_active;		/**< lower 8 bits (lines) */
	u8 v_blank;		/**< lower 8 bits (lines) */
	u8 v_high;		/**< upper 4 bits each v_active, v_blank */
    } part1;

    struct {
	u8 h_sync_off;	/**< lower 8 bits, from hblank start */
	u8 h_sync_width;	/**< lower 8 bits (pixels) */
	/** lower 4 bits each vsync offset, vsync width */
	u8 v_sync_off_width;
	/**
	 * 2 high bits of hsync offset, 2 high bits of hsync width,
	 * bits 4-5 of vsync offset, and 2 high bits of vsync width.
	 */
	u8 sync_off_width_high;
	u8 dtd_flags;
	u8 sdvo_flags;
	/** bits 6-7 of vsync offset at bits 6-7 */
	u8 v_sync_off_high;
	u8 reserved;
    } part2;
} __attribute__((packed));

struct intel_sdvo_pixel_clock_range {
    u16 min;			/**< pixel clock, in 10kHz units */
    u16 max;			/**< pixel clock, in 10kHz units */
} __attribute__((packed));

struct intel_sdvo_preferred_input_timing_args {
    u16 clock;
    u16 width;
    u16 height;
} __attribute__((packed));

/* I2C registers for SDVO */
#define SDVO_I2C_ARG_0				0x07
#define SDVO_I2C_ARG_1				0x06
#define SDVO_I2C_ARG_2				0x05
#define SDVO_I2C_ARG_3				0x04
#define SDVO_I2C_ARG_4				0x03
#define SDVO_I2C_ARG_5				0x02
#define SDVO_I2C_ARG_6				0x01
#define SDVO_I2C_ARG_7				0x00
#define SDVO_I2C_OPCODE				0x08
#define SDVO_I2C_CMD_STATUS			0x09
#define SDVO_I2C_RETURN_0			0x0a
#define SDVO_I2C_RETURN_1			0x0b
#define SDVO_I2C_RETURN_2			0x0c
#define SDVO_I2C_RETURN_3			0x0d
#define SDVO_I2C_RETURN_4			0x0e
#define SDVO_I2C_RETURN_5			0x0f
#define SDVO_I2C_RETURN_6			0x10
#define SDVO_I2C_RETURN_7			0x11
#define SDVO_I2C_VENDOR_BEGIN			0x20

/* Status results */
#define SDVO_CMD_STATUS_POWER_ON		0x0
#define SDVO_CMD_STATUS_SUCCESS			0x1
#define SDVO_CMD_STATUS_NOTSUPP			0x2
#define SDVO_CMD_STATUS_INVALID_ARG		0x3
#define SDVO_CMD_STATUS_PENDING			0x4
#define SDVO_CMD_STATUS_TARGET_NOT_SPECIFIED	0x5
#define SDVO_CMD_STATUS_SCALING_NOT_SUPP	0x6

/* SDVO commands, argument/result registers */

#define SDVO_CMD_RESET					0x01

/** Returns a struct intel_sdvo_caps */
#define SDVO_CMD_GET_DEVICE_CAPS			0x02

#define SDVO_CMD_GET_FIRMWARE_REV			0x86
# define SDVO_DEVICE_FIRMWARE_MINOR			SDVO_I2C_RETURN_0
# define SDVO_DEVICE_FIRMWARE_MAJOR			SDVO_I2C_RETURN_1
# define SDVO_DEVICE_FIRMWARE_PATCH			SDVO_I2C_RETURN_2

/**
 * Reports which inputs are trained (managed to sync).
 *
 * Devices must have trained within 2 vsyncs of a mode change.
 */
#define SDVO_CMD_GET_TRAINED_INPUTS			0x03
struct intel_sdvo_get_trained_inputs_response {
    unsigned int input0_trained:1;
    unsigned int input1_trained:1;
    unsigned int pad:6;
} __attribute__((packed));

/** Returns a struct intel_sdvo_output_flags of active outputs. */
#define SDVO_CMD_GET_ACTIVE_OUTPUTS			0x04

/**
 * Sets the current set of active outputs.
 *
 * Takes a struct intel_sdvo_output_flags.  Must be preceded by a SET_IN_OUT_MAP
 * on multi-output devices.
 */
#define SDVO_CMD_SET_ACTIVE_OUTPUTS			0x05

/**
 * Returns the current mapping of SDVO inputs to outputs on the device.
 *
 * Returns two struct intel_sdvo_output_flags structures.
 */
#define SDVO_CMD_GET_IN_OUT_MAP				0x06

/**
 * Sets the current mapping of SDVO inputs to outputs on the device.
 *
 * Takes two struct i380_sdvo_output_flags structures.
 */
#define SDVO_CMD_SET_IN_OUT_MAP				0x07

/**
 * Returns a struct intel_sdvo_output_flags of attached displays.
 */
#define SDVO_CMD_GET_ATTACHED_DISPLAYS			0x0b

/**
 * Returns a struct intel_sdvo_ouptut_flags of displays supporting hot plugging.
 */
#define SDVO_CMD_GET_HOT_PLUG_SUPPORT			0x0c

/**
 * Takes a struct intel_sdvo_output_flags.
 */
#define SDVO_CMD_SET_ACTIVE_HOT_PLUG			0x0d

/**
 * Returns a struct intel_sdvo_output_flags of displays with hot plug
 * interrupts enabled.
 */
#define SDVO_CMD_GET_ACTIVE_HOT_PLUG			0x0e

#define SDVO_CMD_GET_INTERRUPT_EVENT_SOURCE		0x0f
struct intel_sdvo_get_interrupt_event_source_response {
    u16 interrupt_status;
    unsigned int ambient_light_interrupt:1;
    unsigned int pad:7;
} __attribute__((packed));

/**
 * Selects which input is affected by future input commands.
 *
 * Commands affected include SET_INPUT_TIMINGS_PART[12],
 * GET_INPUT_TIMINGS_PART[12], GET_PREFERRED_INPUT_TIMINGS_PART[12],
 * GET_INPUT_PIXEL_CLOCK_RANGE, and CREATE_PREFERRED_INPUT_TIMINGS.
 */
#define SDVO_CMD_SET_TARGET_INPUT			0x10
struct intel_sdvo_set_target_input_args {
    unsigned int target_1:1;
    unsigned int pad:7;
} __attribute__((packed));

/**
 * Takes a struct intel_sdvo_output_flags of which outputs are targetted by
 * future output commands.
 *
 * Affected commands inclue SET_OUTPUT_TIMINGS_PART[12],
 * GET_OUTPUT_TIMINGS_PART[12], and GET_OUTPUT_PIXEL_CLOCK_RANGE.
 */
#define SDVO_CMD_SET_TARGET_OUTPUT			0x11

#define SDVO_CMD_GET_INPUT_TIMINGS_PART1		0x12
#define SDVO_CMD_GET_INPUT_TIMINGS_PART2		0x13
#define SDVO_CMD_SET_INPUT_TIMINGS_PART1		0x14
#define SDVO_CMD_SET_INPUT_TIMINGS_PART2		0x15
#define SDVO_CMD_SET_OUTPUT_TIMINGS_PART1		0x16
#define SDVO_CMD_SET_OUTPUT_TIMINGS_PART2		0x17
#define SDVO_CMD_GET_OUTPUT_TIMINGS_PART1		0x18
#define SDVO_CMD_GET_OUTPUT_TIMINGS_PART2		0x19
/* Part 1 */
# define SDVO_DTD_CLOCK_LOW				SDVO_I2C_ARG_0
# define SDVO_DTD_CLOCK_HIGH				SDVO_I2C_ARG_1
# define SDVO_DTD_H_ACTIVE				SDVO_I2C_ARG_2
# define SDVO_DTD_H_BLANK				SDVO_I2C_ARG_3
# define SDVO_DTD_H_HIGH				SDVO_I2C_ARG_4
# define SDVO_DTD_V_ACTIVE				SDVO_I2C_ARG_5
# define SDVO_DTD_V_BLANK				SDVO_I2C_ARG_6
# define SDVO_DTD_V_HIGH				SDVO_I2C_ARG_7
/* Part 2 */
# define SDVO_DTD_HSYNC_OFF				SDVO_I2C_ARG_0
# define SDVO_DTD_HSYNC_WIDTH				SDVO_I2C_ARG_1
# define SDVO_DTD_VSYNC_OFF_WIDTH			SDVO_I2C_ARG_2
# define SDVO_DTD_SYNC_OFF_WIDTH_HIGH			SDVO_I2C_ARG_3
# define SDVO_DTD_DTD_FLAGS				SDVO_I2C_ARG_4
# define SDVO_DTD_DTD_FLAG_INTERLACED				(1 << 7)
# define SDVO_DTD_DTD_FLAG_STEREO_MASK				(3 << 5)
# define SDVO_DTD_DTD_FLAG_INPUT_MASK				(3 << 3)
# define SDVO_DTD_DTD_FLAG_SYNC_MASK				(3 << 1)
# define SDVO_DTD_SDVO_FLAS				SDVO_I2C_ARG_5
# define SDVO_DTD_SDVO_FLAG_STALL				(1 << 7)
# define SDVO_DTD_SDVO_FLAG_CENTERED				(0 << 6)
# define SDVO_DTD_SDVO_FLAG_UPPER_LEFT				(1 << 6)
# define SDVO_DTD_SDVO_FLAG_SCALING_MASK			(3 << 4)
# define SDVO_DTD_SDVO_FLAG_SCALING_NONE			(0 << 4)
# define SDVO_DTD_SDVO_FLAG_SCALING_SHARP			(1 << 4)
# define SDVO_DTD_SDVO_FLAG_SCALING_SMOOTH			(2 << 4)
# define SDVO_DTD_VSYNC_OFF_HIGH			SDVO_I2C_ARG_6

/**
 * Generates a DTD based on the given width, height, and flags.
 *
 * This will be supported by any device supporting scaling or interlaced
 * modes.
 */
#define SDVO_CMD_CREATE_PREFERRED_INPUT_TIMING		0x1a
# define SDVO_PREFERRED_INPUT_TIMING_CLOCK_LOW		SDVO_I2C_ARG_0
# define SDVO_PREFERRED_INPUT_TIMING_CLOCK_HIGH		SDVO_I2C_ARG_1
# define SDVO_PREFERRED_INPUT_TIMING_WIDTH_LOW		SDVO_I2C_ARG_2
# define SDVO_PREFERRED_INPUT_TIMING_WIDTH_HIGH		SDVO_I2C_ARG_3
# define SDVO_PREFERRED_INPUT_TIMING_HEIGHT_LOW		SDVO_I2C_ARG_4
# define SDVO_PREFERRED_INPUT_TIMING_HEIGHT_HIGH	SDVO_I2C_ARG_5
# define SDVO_PREFERRED_INPUT_TIMING_FLAGS		SDVO_I2C_ARG_6
# define SDVO_PREFERRED_INPUT_TIMING_FLAGS_INTERLACED		(1 << 0)
# define SDVO_PREFERRED_INPUT_TIMING_FLAGS_SCALED		(1 << 1)

#define SDVO_CMD_GET_PREFERRED_INPUT_TIMING_PART1	0x1b
#define SDVO_CMD_GET_PREFERRED_INPUT_TIMING_PART2	0x1c

/** Returns a struct intel_sdvo_pixel_clock_range */
#define SDVO_CMD_GET_INPUT_PIXEL_CLOCK_RANGE		0x1d
/** Returns a struct intel_sdvo_pixel_clock_range */
#define SDVO_CMD_GET_OUTPUT_PIXEL_CLOCK_RANGE		0x1e

/** Returns a byte bitfield containing SDVO_CLOCK_RATE_MULT_* flags */
#define SDVO_CMD_GET_SUPPORTED_CLOCK_RATE_MULTS		0x1f

/** Returns a byte containing a SDVO_CLOCK_RATE_MULT_* flag */
#define SDVO_CMD_GET_CLOCK_RATE_MULT			0x20
/** Takes a byte containing a SDVO_CLOCK_RATE_MULT_* flag */
#define SDVO_CMD_SET_CLOCK_RATE_MULT			0x21
# define SDVO_CLOCK_RATE_MULT_1X				(1 << 0)
# define SDVO_CLOCK_RATE_MULT_2X				(1 << 1)
# define SDVO_CLOCK_RATE_MULT_4X				(1 << 3)

#define SDVO_CMD_GET_SUPPORTED_TV_FORMATS		0x27

#define SDVO_CMD_GET_TV_FORMAT				0x28

#define SDVO_CMD_SET_TV_FORMAT				0x29

#define SDVO_CMD_GET_SUPPORTED_POWER_STATES		0x2a
#define SDVO_CMD_GET_ENCODER_POWER_STATE		0x2b
#define SDVO_CMD_SET_ENCODER_POWER_STATE		0x2c
# define SDVO_ENCODER_STATE_ON					(1 << 0)
# define SDVO_ENCODER_STATE_STANDBY				(1 << 1)
# define SDVO_ENCODER_STATE_SUSPEND				(1 << 2)
# define SDVO_ENCODER_STATE_OFF					(1 << 3)

#define SDVO_CMD_SET_TV_RESOLUTION_SUPPORT		0x93

#define SDVO_CMD_SET_CONTROL_BUS_SWITCH			0x7a
# define SDVO_CONTROL_BUS_PROM				0x0
# define SDVO_CONTROL_BUS_DDC1				0x1
# define SDVO_CONTROL_BUS_DDC2				0x2
# define SDVO_CONTROL_BUS_DDC3				0x3

/* xiaolin, to support add-on SDVO TV Encoder */
/* SDVO Bus & SDVO Inputs wiring details*/
/* Bit 0: Is SDVOB connected to In0 (1 = yes, 0 = no*/
/* Bit 1: Is SDVOB connected to In1 (1 = yes, 0 = no*/
/* Bit 2: Is SDVOC connected to In0 (1 = yes, 0 = no*/
/* Bit 3: Is SDVOC connected to In1 (1 = yes, 0 = no*/
#define SDVOB_IN0 0x01
#define SDVOB_IN1 0x02
#define SDVOC_IN0 0x04
#define SDVOC_IN1 0x08

#define SDVO_OUTPUT_TV0     0x003C
#define SDVO_OUTPUT_TV1     0x3C00
#define SDVO_OUTPUT_LAST    (14)

#define SDVO_OUTPUT_CRT     (SDVO_OUTPUT_RGB0 | SDVO_OUTPUT_RGB1 )
#define SDVO_OUTPUT_TV      (SDVO_OUTPUT_TV0 | SDVO_OUTPUT_TV1)
#define SDVO_OUTPUT_LVDS    (SDVO_OUTPUT_LVDS0 | SDVO_OUTPUT_LVDS1)
#define SDVO_OUTPUT_TMDS    (SDVO_OUTPUT_TMDS0 | SDVO_OUTPUT_TMDS1)



#define SDVO_DEVICE_NONE 0x00
#define	SDVO_DEVICE_CRT 0x01
#define	SDVO_DEVICE_TV 0x02
#define	SDVO_DEVICE_LVDS 0x04
#define	SDVO_DEVICE_TMDS 0x08

/* Different TV mode*/
#define	TVMODE_OFF		0x0000
#define	TVMODE_SDTV	0x0001
#define	TVMODE_HDTV	0x0002

#define TVSTANDARD_NONE 0x00
#define TVSTANDARD_NTSC_M   0x0001  //        75 IRE Setup
#define TVSTANDARD_NTSC_M_J 0x0002  // Japan,  0 IRE Setup
#define TVSTANDARD_PAL_B    0x0004
#define TVSTANDARD_PAL_D    0x0008
#define TVSTANDARD_PAL_H    0x0010
#define TVSTANDARD_PAL_I    0x0020
#define TVSTANDARD_PAL_M    0x0040
#define TVSTANDARD_PAL_N    0x0080
#define TVSTANDARD_SECAM_B  0x0100
#define TVSTANDARD_SECAM_D  0x0200
#define TVSTANDARD_SECAM_G  0x0400
#define TVSTANDARD_SECAM_H  0x0800
#define TVSTANDARD_SECAM_K  0x1000
#define TVSTANDARD_SECAM_K1 0x2000
#define TVSTANDARD_SECAM_L  0x4000
#define TVSTANDARD_WIN_VGA  0x8000
/*and the rest*/
#define TVSTANDARD_NTSC_433 0x00010000
#define TVSTANDARD_PAL_G    0x00020000
#define TVSTANDARD_PAL_60   0x00040000
#define TVSTANDARD_SECAM_L1 0x00080000
#define TVSTANDARD_SDTV_ALL 0x000FFFFF


/*HDTV standard defination added using the unused upper 12 bits of dwTVStandard*/
#define HDTV_SMPTE_170M_480i59		0x00100000
#define HDTV_SMPTE_293M_480p60		0x00200000
#define HDTV_SMPTE_293M_480p59		0x00400000
#define HDTV_ITURBT601_576i50		0x00800000
#define HDTV_ITURBT601_576p50		0x01000000
#define HDTV_SMPTE_296M_720p50		0x02000000
#define HDTV_SMPTE_296M_720p59		0x04000000
#define HDTV_SMPTE_296M_720p60		0x08000000
#define HDTV_SMPTE_274M_1080i50		0x10000000
#define HDTV_SMPTE_274M_1080i59		0x20000000
#define HDTV_SMPTE_274M_1080i60		0x40000000
#define HDTV_SMPTE_274M_1080p60		0x80000000
#define TVSTANDARD_HDTV_ALL			0xFFF00000


#define TVSTANDARD_NTSC 0x01
#define TVSTANDARD_PAL 0x02

#define TVOUTPUT_NONE 0x00
#define TVOUTPUT_COMPOSITE 0x01
#define TVOUTPUT_SVIDEO 0x02
#define TVOUTPUT_RGB 0x04
#define TVOUTPUT_YCBCR 0x08
#define TVOUTPUT_SC 0x16

/* Encoder supported TV standard bit mask per SDVO ED*/
#define SDVO_NTSC_M                                     0x00000001
#define SDVO_NTSC_M_J                                   0x00000002
#define SDVO_NTSC_433                                   0x00000004
#define SDVO_PAL_B                                      0x00000008
#define SDVO_PAL_D                                      0x00000010
#define SDVO_PAL_G                                      0x00000020
#define SDVO_PAL_H                                      0x00000040
#define SDVO_PAL_I                                      0x00000080
#define SDVO_PAL_M                                      0x00000100
#define SDVO_PAL_N                                      0x00000200
#define SDVO_PAL_NC                                     0x00000400
#define SDVO_PAL_60                                     0x00000800
#define SDVO_SECAM_B                                    0x00001000
#define SDVO_SECAM_D                                    0x00002000
#define SDVO_SECAM_G                                    0x00004000
#define SDVO_SECAM_K                                    0x00008000
#define SDVO_SECAM_K1                                   0x00010000
#define SDVO_SECAM_L                                    0x00020000
#define SDVO_SECAM_60									0x00040000

/* Number of SDTV format*/
#define SDTV_NUM_STANDARDS                              19

/* Encoder supported HDTV standard bit mask per SDVO ED*/
#define SDVO_HDTV_STD_240M_1080i59                      0x00000008
#define SDVO_HDTV_STD_240M_1080i60                      0x00000010
#define SDVO_HDTV_STD_260M_1080i59                      0x00000020
#define SDVO_HDTV_STD_260M_1080i60                      0x00000040
#define SDVO_HDTV_STD_274M_1080i50                      0x00000080
#define SDVO_HDTV_STD_274M_1080i59                      0x00000100
#define SDVO_HDTV_STD_274M_1080i60                      0x00000200
#define SDVO_HDTV_STD_274M_1080p23                      0x00000400
#define SDVO_HDTV_STD_274M_1080p24                      0x00000800
#define SDVO_HDTV_STD_274M_1080p25                      0x00001000
#define SDVO_HDTV_STD_274M_1080p29                      0x00002000
#define SDVO_HDTV_STD_274M_1080p30                      0x00004000
#define SDVO_HDTV_STD_274M_1080p50                      0x00008000
#define SDVO_HDTV_STD_274M_1080p59                      0x00010000
#define SDVO_HDTV_STD_274M_1080p60                      0x00020000
#define SDVO_HDTV_STD_295M_1080i50                      0x00040000
#define SDVO_HDTV_STD_295M_1080p50                      0x00080000
#define SDVO_HDTV_STD_296M_720p59                       0x00100000
#define SDVO_HDTV_STD_296M_720p60                       0x00200000
#define SDVO_HDTV_STD_296M_720p50	                    0x00400000
#define SDVO_HDTV_STD_293M_480p59                       0x00800000
#define SDVO_HDTV_STD_170M_480i59                       0x01000000
#define SDVO_HDTV_STD_ITURBT601_576i50			        0x02000000
#define SDVO_HDTV_STD_ITURBT601_576p50			        0x04000000
#define SDVO_HDTV_STD_EIA_7702A_480i60                  0x08000000
#define SDVO_HDTV_STD_EIA_7702A_480p60                  0x10000000

/* SDTV resolution*/
#define SDVO_SDTV_320x200                               0x00000001
#define SDVO_SDTV_320x240                               0x00000002
#define SDVO_SDTV_400x300                               0x00000004
#define SDVO_SDTV_640x350                               0x00000008
#define SDVO_SDTV_640x400                               0x00000010
#define SDVO_SDTV_640x480                               0x00000020
#define SDVO_SDTV_704x480                               0x00000040
#define SDVO_SDTV_704x576                               0x00000080
#define SDVO_SDTV_720x350                               0x00000100
#define SDVO_SDTV_720x400                               0x00000200
#define SDVO_SDTV_720x480                               0x00000400
#define SDVO_SDTV_720x540                               0x00000800
#define SDVO_SDTV_720x576                               0x00001000
#define SDVO_SDTV_768x576                               0x00002000
#define SDVO_SDTV_800x600                               0x00004000
#define SDVO_SDTV_832x624                               0x00008000
#define SDVO_SDTV_920x766                               0x00010000
#define SDVO_SDTV_1024x768                              0x00020000
#define SDVO_SDTV_1280x1024								0x00040000


#define SDVO_HDTV_640x480                               0x00000001
#define SDVO_HDTV_800x600                               0x00000002
#define SDVO_HDTV_1024x768                              0x00000004
#define SDVO_HDTV_1064x600                              0x00020000
#define SDVO_HDTV_1280x720                              0x00040000
#define SDVO_HDTV_1704x960                              0x00100000
#define SDVO_HDTV_1864x1050                             0x00200000
#define SDVO_HDTV_1920x1080                             0x00400000
#define SDVO_HDTV_640x400                               0x02000000

/* Number of SDTV mode*/
#define SDTV_NUM_MODES                                  19

/* sdvo cmd for sdvo tv */
#define SDVO_CMD_CREATE_PREFERRED_INPUT_TIMINGS 0x1A
#define SDVO_CMD_GET_SUPPORTED_TV_FORMATS	0x27
#define SDVO_CMD_GET_TV_FORMATS				0x28
#define SDVO_CMD_SET_TV_FORMATS				0x29

#define SDVO_CMD_GET_SUPPORTED_POWER_STATES		0x2a
#define SDVO_CMD_GET_ENCODER_POWER_STATE		0x2b
#define SDVO_CMD_SET_ENCODER_POWER_STATE		0x2c
#define SDVO_ENCODER_STATE_ON					(1 << 0)
#define SDVO_ENCODER_STATE_STANDBY				(1 << 1)
#define SDVO_ENCODER_STATE_SUSPEND				(1 << 2)
#define SDVO_ENCODER_STATE_OFF					(1 << 3)

/* Bit mask of picture enhancement*/
#define SDVO_FLICKER_FILTER                             0x00000001
#define SDVO_ADAPTIVE_FLICKER_FILTER                    0x00000002
#define SDVO_2D_FLICKER_FILTER                          0x00000004
#define SDVO_SATURATION                                 0x00000008
#define SDVO_HUE                                        0x00000010
#define SDVO_BRIGHTNESS                                 0x00000020
#define SDVO_CONTRAST                                   0x00000040
#define SDVO_HORIZONTAL_OVERSCAN                        0x00000080
#define SDVO_VERTICAL_OVERSCAN                          0x00000100
#define SDVO_HORIZONTAL_POSITION                        0x00000200
#define SDVO_VERTICAL_POSITION                          0x00000400
#define SDVO_SHARPNESS                                  0x00000800
#define SDVO_DOT_CRAWL                                  0x00001000
#define SDVO_DITHER                                     0x00002000
#define SDVO_MAX_TV_CHROMA_FILTER                       0x00004000
#define SDVO_TV_MAX_LUMA_FILTER                         0x00008000

#define SDVO_CMD_GET_ANCILLARY_VIDEO_INFORMATION        0x3A
#define SDVO_CMD_SET_ANCILLARY_VIDEO_INFORMATION        0x3B

#define SDVO_CMD_GET_SUPPORTED_ENHANCEMENTS              0x84
#define SDVO_CMD_GET_MAX_FLICKER_FILTER                  0x4D
#define SDVO_CMD_GET_FLICKER_FILTER                      0x4E
#define SDVO_CMD_SET_FLICKER_FILTER                      0x4F
#define SDVO_CMD_GET_ADAPTIVE_FLICKER_FILTER             0x50
#define SDVO_CMD_SET_ADAPTIVE_FLICKER_FILTER             0x51
#define SDVO_CMD_GET_MAX_2D_FLICKER_FILTER               0x52
#define SDVO_CMD_GET_2D_FLICKER_FILTER                   0x53
#define SDVO_CMD_SET_2D_FLICKER_FILTER                   0x54
#define SDVO_CMD_GET_MAX_SATURATION                      0x55
#define SDVO_CMD_GET_SATURATION                          0x56
#define SDVO_CMD_SET_SATURATION                          0x57
#define SDVO_CMD_GET_MAX_HUE                             0x58
#define SDVO_CMD_GET_HUE                                 0x59
#define SDVO_CMD_SET_HUE                                 0x5A
#define SDVO_CMD_GET_MAX_BRIGHTNESS                      0x5B
#define SDVO_CMD_GET_BRIGHTNESS                          0x5C
#define SDVO_CMD_SET_BRIGHTNESS                          0x5D
#define SDVO_CMD_GET_MAX_CONTRAST                        0x5E
#define SDVO_CMD_GET_CONTRAST                            0x5F
#define SDVO_CMD_SET_CONTRAST                            0x60

#define SDVO_CMD_GET_MAX_HORIZONTAL_OVERSCAN             0x61
#define SDVO_CMD_GET_HORIZONTAL_OVERSCAN                 0x62
#define SDVO_CMD_SET_HORIZONTAL_OVERSCAN                 0x63
#define SDVO_CMD_GET_MAX_VERTICAL_OVERSCAN               0x64
#define SDVO_CMD_GET_VERTICAL_OVERSCAN                   0x65
#define SDVO_CMD_SET_VERTICAL_OVERSCAN                   0x66
#define SDVO_CMD_GET_MAX_HORIZONTAL_POSITION             0x67
#define SDVO_CMD_GET_HORIZONTAL_POSITION                 0x68
#define SDVO_CMD_SET_HORIZONTAL_POSITION                 0x69
#define SDVO_CMD_GET_MAX_VERTICAL_POSITION               0x6A
#define SDVO_CMD_GET_VERTICAL_POSITION                   0x6B
#define SDVO_CMD_SET_VERTICAL_POSITION                   0x6C
#define SDVO_CMD_GET_MAX_SHARPNESS                       0x6D
#define SDVO_CMD_GET_SHARPNESS                           0x6E
#define SDVO_CMD_SET_SHARPNESS                           0x6F
#define SDVO_CMD_GET_DOT_CRAWL                           0x70
#define SDVO_CMD_SET_DOT_CRAWL                           0x71
#define SDVO_CMD_GET_MAX_TV_CHROMA_FILTER                0x74
#define SDVO_CMD_GET_TV_CHROMA_FILTER                    0x75
#define SDVO_CMD_SET_TV_CHROMA_FILTER                    0x76
#define SDVO_CMD_GET_MAX_TV_LUMA_FILTER                  0x77
#define SDVO_CMD_GET_TV_LUMA_FILTER                      0x78
#define SDVO_CMD_SET_TV_LUMA_FILTER                      0x79
#define SDVO_CMD_GET_MAX_ADAPTIVE_FLICKER_FILTER         0x7B
