#define BLC_PWM_CTL		0x61254
#define BLC_PWM_CTL2		0x61250
#define BACKLIGHT_MODULATION_FREQ_SHIFT		(17)
/**
 * This is the most significant 15 bits of the number of backlight cycles in a
 * complete cycle of the modulated backlight control.
 *
 * The actual value is this field multiplied by two.
 */
#define BACKLIGHT_MODULATION_FREQ_MASK		(0x7fff << 17)
#define BLM_LEGACY_MODE				(1 << 16)
/**
 * This is the number of cycles out of the backlight modulation cycle for which
 * the backlight is on.
 *
 * This field must be no greater than the number of cycles in the complete
 * backlight modulation cycle.
 */
#define BACKLIGHT_DUTY_CYCLE_SHIFT		(0)
#define BACKLIGHT_DUTY_CYCLE_MASK		(0xffff)

#define I915_GCFGC			0xf0
#define I915_LOW_FREQUENCY_ENABLE		(1 << 7)
#define I915_DISPLAY_CLOCK_190_200_MHZ		(0 << 4)
#define I915_DISPLAY_CLOCK_333_MHZ		(4 << 4)
#define I915_DISPLAY_CLOCK_MASK			(7 << 4)

#define I855_HPLLCC			0xc0
#define I855_CLOCK_CONTROL_MASK			(3 << 0)
#define I855_CLOCK_133_200			(0 << 0)
#define I855_CLOCK_100_200			(1 << 0)
#define I855_CLOCK_100_133			(2 << 0)
#define I855_CLOCK_166_250			(3 << 0)

/* I830 CRTC registers */
#define HTOTAL_A	0x60000
#define HBLANK_A	0x60004
#define HSYNC_A 	0x60008
#define VTOTAL_A	0x6000c
#define VBLANK_A	0x60010
#define VSYNC_A 	0x60014
#define PIPEASRC	0x6001c
#define BCLRPAT_A	0x60020
#define VSYNCSHIFT_A	0x60028

#define HTOTAL_B	0x61000
#define HBLANK_B	0x61004
#define HSYNC_B 	0x61008
#define VTOTAL_B	0x6100c
#define VBLANK_B	0x61010
#define VSYNC_B 	0x61014
#define PIPEBSRC	0x6101c
#define BCLRPAT_B	0x61020
#define VSYNCSHIFT_B	0x61028

#define PP_STATUS	0x61200
# define PP_ON					(1 << 31)
/**
 * Indicates that all dependencies of the panel are on:
 *
 * - PLL enabled
 * - pipe enabled
 * - LVDS/DVOB/DVOC on
 */
# define PP_READY				(1 << 30)
# define PP_SEQUENCE_NONE			(0 << 28)
# define PP_SEQUENCE_ON				(1 << 28)
# define PP_SEQUENCE_OFF			(2 << 28)
# define PP_SEQUENCE_MASK			0x30000000
#define PP_CONTROL	0x61204
# define POWER_TARGET_ON			(1 << 0)

#define LVDSPP_ON       0x61208
#define LVDSPP_OFF      0x6120c
#define PP_CYCLE        0x61210

#define PFIT_CONTROL	0x61230
# define PFIT_ENABLE				(1 << 31)
# define PFIT_PIPE_MASK				(3 << 29)
# define PFIT_PIPE_SHIFT			29
# define VERT_INTERP_DISABLE			(0 << 10)
# define VERT_INTERP_BILINEAR			(1 << 10)
# define VERT_INTERP_MASK			(3 << 10)
# define VERT_AUTO_SCALE			(1 << 9)
# define HORIZ_INTERP_DISABLE			(0 << 6)
# define HORIZ_INTERP_BILINEAR			(1 << 6)
# define HORIZ_INTERP_MASK			(3 << 6)
# define HORIZ_AUTO_SCALE			(1 << 5)
# define PANEL_8TO6_DITHER_ENABLE		(1 << 3)

#define PFIT_PGM_RATIOS	0x61234
# define PFIT_VERT_SCALE_MASK			0xfff00000
# define PFIT_HORIZ_SCALE_MASK			0x0000fff0

#define PFIT_AUTO_RATIOS	0x61238


#define DPLL_A		0x06014
#define DPLL_B		0x06018
# define DPLL_VCO_ENABLE			(1 << 31)
# define DPLL_DVO_HIGH_SPEED			(1 << 30)
# define DPLL_SYNCLOCK_ENABLE			(1 << 29)
# define DPLL_VGA_MODE_DIS			(1 << 28)
# define DPLLB_MODE_DAC_SERIAL			(1 << 26)	/* i915 */
# define DPLLB_MODE_LVDS			(2 << 26)	/* i915 */
# define DPLL_MODE_MASK				(3 << 26)
# define DPLL_DAC_SERIAL_P2_CLOCK_DIV_10	(0 << 24)	/* i915 */
# define DPLL_DAC_SERIAL_P2_CLOCK_DIV_5		(1 << 24)	/* i915 */
# define DPLLB_LVDS_P2_CLOCK_DIV_14		(0 << 24)	/* i915 */
# define DPLLB_LVDS_P2_CLOCK_DIV_7		(1 << 24)	/* i915 */
# define DPLL_P2_CLOCK_DIV_MASK			0x03000000	/* i915 */
# define DPLL_FPA01_P1_POST_DIV_MASK		0x00ff0000	/* i915 */
/**
 *  The i830 generation, in DAC/serial mode, defines p1 as two plus this
 * bitfield, or just 2 if PLL_P1_DIVIDE_BY_TWO is set.
 */
# define DPLL_FPA01_P1_POST_DIV_MASK_I830	0x001f0000
/**
 * The i830 generation, in LVDS mode, defines P1 as the bit number set within
 * this field (only one bit may be set).
 */
# define DPLL_FPA01_P1_POST_DIV_MASK_I830_LVDS	0x003f0000
# define DPLL_FPA01_P1_POST_DIV_SHIFT		16
# define PLL_P2_DIVIDE_BY_4		(1 << 23)	/* i830, required
							 * in DVO non-gang */
# define PLL_P1_DIVIDE_BY_TWO			(1 << 21)	/* i830 */
# define PLL_REF_INPUT_DREFCLK			(0 << 13)
# define PLL_REF_INPUT_TVCLKINA			(1 << 13)	/* i830 */
# define PLL_REF_INPUT_TVCLKINBC		(2 << 13)	/* SDVO
								 * TVCLKIN */
# define PLLB_REF_INPUT_SPREADSPECTRUMIN	(3 << 13)
# define PLL_REF_INPUT_MASK			(3 << 13)
# define PLL_LOAD_PULSE_PHASE_SHIFT		9
/*
 * Parallel to Serial Load Pulse phase selection.
 * Selects the phase for the 10X DPLL clock for the PCIe
 * digital display port. The range is 4 to 13; 10 or more
 * is just a flip delay. The default is 6
 */
# define PLL_LOAD_PULSE_PHASE_MASK	(0xf << PLL_LOAD_PULSE_PHASE_SHIFT)
# define DISPLAY_RATE_SELECT_FPA1	(1 << 8)

/**
 * SDVO multiplier for 945G/GM. Not used on 965.
 *
 * \sa DPLL_MD_UDI_MULTIPLIER_MASK
 */
# define SDVO_MULTIPLIER_MASK			0x000000ff
# define SDVO_MULTIPLIER_SHIFT_HIRES		4
# define SDVO_MULTIPLIER_SHIFT_VGA		0

/** @defgroup DPLL_MD
 * @{
 */
/** Pipe A SDVO/UDI clock multiplier/divider register for G965. */
#define DPLL_A_MD		0x0601c
/** Pipe B SDVO/UDI clock multiplier/divider register for G965. */
#define DPLL_B_MD		0x06020
/**
 * UDI pixel divider, controlling how many pixels are stuffed into a packet.
 *
 * Value is pixels minus 1.  Must be set to 1 pixel for SDVO.
 */
# define DPLL_MD_UDI_DIVIDER_MASK		0x3f000000
# define DPLL_MD_UDI_DIVIDER_SHIFT		24
/** UDI pixel divider for VGA, same as DPLL_MD_UDI_DIVIDER_MASK. */
# define DPLL_MD_VGA_UDI_DIVIDER_MASK		0x003f0000
# define DPLL_MD_VGA_UDI_DIVIDER_SHIFT		16
/**
 * SDVO/UDI pixel multiplier.
 *
 * SDVO requires that the bus clock rate be between 1 and 2 Ghz, and the bus
 * clock rate is 10 times the DPLL clock.  At low resolution/refresh rate
 * modes, the bus rate would be below the limits, so SDVO allows for stuffing
 * dummy bytes in the datastream at an increased clock rate, with both sides of
 * the link knowing how many bytes are fill.
 *
 * So, for a mode with a dotclock of 65Mhz, we would want to double the clock
 * rate to 130Mhz to get a bus rate of 1.30Ghz.  The DPLL clock rate would be
 * set to 130Mhz, and the SDVO multiplier set to 2x in this register and
 * through an SDVO command.
 *
 * This register field has values of multiplication factor minus 1, with
 * a maximum multiplier of 5 for SDVO.
 */
# define DPLL_MD_UDI_MULTIPLIER_MASK		0x00003f00
# define DPLL_MD_UDI_MULTIPLIER_SHIFT		8
/** SDVO/UDI pixel multiplier for VGA, same as DPLL_MD_UDI_MULTIPLIER_MASK.
 * This best be set to the default value (3) or the CRT won't work. No,
 * I don't entirely understand what this does...
 */
# define DPLL_MD_VGA_UDI_MULTIPLIER_MASK	0x0000003f
# define DPLL_MD_VGA_UDI_MULTIPLIER_SHIFT	0
/** @} */

#define DPLL_TEST		0x606c
# define DPLLB_TEST_SDVO_DIV_1			(0 << 22)
# define DPLLB_TEST_SDVO_DIV_2			(1 << 22)
# define DPLLB_TEST_SDVO_DIV_4			(2 << 22)
# define DPLLB_TEST_SDVO_DIV_MASK		(3 << 22)
# define DPLLB_TEST_N_BYPASS			(1 << 19)
# define DPLLB_TEST_M_BYPASS			(1 << 18)
# define DPLLB_INPUT_BUFFER_ENABLE		(1 << 16)
# define DPLLA_TEST_N_BYPASS			(1 << 3)
# define DPLLA_TEST_M_BYPASS			(1 << 2)
# define DPLLA_INPUT_BUFFER_ENABLE		(1 << 0)

#define ADPA			0x61100
#define ADPA_DAC_ENABLE 	(1<<31)
#define ADPA_DAC_DISABLE	0
#define ADPA_PIPE_SELECT_MASK	(1<<30)
#define ADPA_PIPE_A_SELECT	0
#define ADPA_PIPE_B_SELECT	(1<<30)
#define ADPA_USE_VGA_HVPOLARITY (1<<15)
#define ADPA_SETS_HVPOLARITY	0
#define ADPA_VSYNC_CNTL_DISABLE (1<<11)
#define ADPA_VSYNC_CNTL_ENABLE	0
#define ADPA_HSYNC_CNTL_DISABLE (1<<10)
#define ADPA_HSYNC_CNTL_ENABLE	0
#define ADPA_VSYNC_ACTIVE_HIGH	(1<<4)
#define ADPA_VSYNC_ACTIVE_LOW	0
#define ADPA_HSYNC_ACTIVE_HIGH	(1<<3)
#define ADPA_HSYNC_ACTIVE_LOW	0

#define FPA0		0x06040
#define FPA1		0x06044
#define FPB0		0x06048
#define FPB1		0x0604c
# define FP_N_DIV_MASK				0x003f0000
# define FP_N_DIV_SHIFT				16
# define FP_M1_DIV_MASK				0x00003f00
# define FP_M1_DIV_SHIFT			8
# define FP_M2_DIV_MASK				0x0000003f
# define FP_M2_DIV_SHIFT			0


#define PORT_HOTPLUG_EN		0x61110
# define SDVOB_HOTPLUG_INT_EN			(1 << 26)
# define SDVOC_HOTPLUG_INT_EN			(1 << 25)
# define TV_HOTPLUG_INT_EN			(1 << 18)
# define CRT_HOTPLUG_INT_EN			(1 << 9)
# define CRT_HOTPLUG_FORCE_DETECT		(1 << 3)

#define PORT_HOTPLUG_STAT	0x61114
# define CRT_HOTPLUG_INT_STATUS			(1 << 11)
# define TV_HOTPLUG_INT_STATUS			(1 << 10)
# define CRT_HOTPLUG_MONITOR_MASK		(3 << 8)
# define CRT_HOTPLUG_MONITOR_COLOR		(3 << 8)
# define CRT_HOTPLUG_MONITOR_MONO		(2 << 8)
# define CRT_HOTPLUG_MONITOR_NONE		(0 << 8)
# define SDVOC_HOTPLUG_INT_STATUS		(1 << 7)
# define SDVOB_HOTPLUG_INT_STATUS		(1 << 6)

#define SDVOB			0x61140
#define SDVOC			0x61160
#define SDVO_ENABLE				(1 << 31)
#define SDVO_PIPE_B_SELECT			(1 << 30)
#define SDVO_STALL_SELECT			(1 << 29)
#define SDVO_INTERRUPT_ENABLE			(1 << 26)
/**
 * 915G/GM SDVO pixel multiplier.
 *
 * Programmed value is multiplier - 1, up to 5x.
 *
 * \sa DPLL_MD_UDI_MULTIPLIER_MASK
 */
#define SDVO_PORT_MULTIPLY_MASK			(7 << 23)
#define SDVO_PORT_MULTIPLY_SHIFT		23
#define SDVO_PHASE_SELECT_MASK			(15 << 19)
#define SDVO_PHASE_SELECT_DEFAULT		(6 << 19)
#define SDVO_CLOCK_OUTPUT_INVERT		(1 << 18)
#define SDVOC_GANG_MODE				(1 << 16)
#define SDVO_BORDER_ENABLE			(1 << 7)
#define SDVOB_PCIE_CONCURRENCY			(1 << 3)
#define SDVO_DETECTED				(1 << 2)
/* Bits to be preserved when writing */
#define SDVOB_PRESERVE_MASK		((1 << 17) | (1 << 16) | (1 << 14))
#define SDVOC_PRESERVE_MASK			(1 << 17)

/** @defgroup LVDS
 * @{
 */
/**
 * This register controls the LVDS output enable, pipe selection, and data
 * format selection.
 *
 * All of the clock/data pairs are force powered down by power sequencing.
 */
#define LVDS			0x61180
/**
 * Enables the LVDS port.  This bit must be set before DPLLs are enabled, as
 * the DPLL semantics change when the LVDS is assigned to that pipe.
 */
# define LVDS_PORT_EN			(1 << 31)
/** Selects pipe B for LVDS data.  Must be set on pre-965. */
# define LVDS_PIPEB_SELECT		(1 << 30)

/**
 * Enables the A0-A2 data pairs and CLKA, containing 18 bits of color data per
 * pixel.
 */
# define LVDS_A0A2_CLKA_POWER_MASK	(3 << 8)
# define LVDS_A0A2_CLKA_POWER_DOWN	(0 << 8)
# define LVDS_A0A2_CLKA_POWER_UP	(3 << 8)
/**
 * Controls the A3 data pair, which contains the additional LSBs for 24 bit
 * mode.  Only enabled if LVDS_A0A2_CLKA_POWER_UP also indicates it should be
 * on.
 */
# define LVDS_A3_POWER_MASK		(3 << 6)
# define LVDS_A3_POWER_DOWN		(0 << 6)
# define LVDS_A3_POWER_UP		(3 << 6)
/**
 * Controls the CLKB pair.  This should only be set when LVDS_B0B3_POWER_UP
 * is set.
 */
# define LVDS_CLKB_POWER_MASK		(3 << 4)
# define LVDS_CLKB_POWER_DOWN		(0 << 4)
# define LVDS_CLKB_POWER_UP		(3 << 4)

/**
 * Controls the B0-B3 data pairs.  This must be set to match the DPLL p2
 * setting for whether we are in dual-channel mode.  The B3 pair will
 * additionally only be powered up when LVDS_A3_POWER_UP is set.
 */
# define LVDS_B0B3_POWER_MASK		(3 << 2)
# define LVDS_B0B3_POWER_DOWN		(0 << 2)
# define LVDS_B0B3_POWER_UP		(3 << 2)

#define PIPEACONF 0x70008
#define PIPEACONF_ENABLE	(1<<31)
#define PIPEACONF_DISABLE	0
#define PIPEACONF_DOUBLE_WIDE	(1<<30)
#define I965_PIPECONF_ACTIVE	(1<<30)
#define PIPEACONF_SINGLE_WIDE	0
#define PIPEACONF_PIPE_UNLOCKED 0
#define PIPEACONF_PIPE_LOCKED	(1<<25)
#define PIPEACONF_PALETTE	0
#define PIPEACONF_GAMMA 	(1<<24)
#define PIPECONF_FORCE_BORDER	(1<<25)
#define PIPECONF_PROGRESSIVE	(0 << 21)
#define PIPECONF_INTERLACE_W_FIELD_INDICATION	(6 << 21)
#define PIPECONF_INTERLACE_FIELD_0_ONLY		(7 << 21)

#define PIPEBCONF 0x71008
#define PIPEBCONF_ENABLE	(1<<31)
#define PIPEBCONF_DISABLE	0
#define PIPEBCONF_DOUBLE_WIDE	(1<<30)
#define PIPEBCONF_DISABLE	0
#define PIPEBCONF_GAMMA 	(1<<24)
#define PIPEBCONF_PALETTE	0

#define PIPEBGCMAXRED		0x71010
#define PIPEBGCMAXGREEN		0x71014
#define PIPEBGCMAXBLUE		0x71018
#define PIPEBSTAT		0x71024
#define PIPEBFRAMEHIGH		0x71040
#define PIPEBFRAMEPIXEL		0x71044

#define DSPARB			0x70030
#define DSPFW1			0x70034
#define DSPFW2			0x70038
#define DSPFW3			0x7003c
#define DSPFW4			0x70050
#define DSPFW5			0x70054
#define DSPFW6			0x70058

#define DSPACNTR		0x70180
#define DSPBCNTR		0x71180
#define DISPLAY_PLANE_ENABLE 			(1<<31)
#define DISPLAY_PLANE_DISABLE			0
#define DISPPLANE_GAMMA_ENABLE			(1<<30)
#define DISPPLANE_GAMMA_DISABLE			0
#define DISPPLANE_PIXFORMAT_MASK		(0xf<<26)
#define DISPPLANE_8BPP				(0x2<<26)
#define DISPPLANE_15_16BPP			(0x4<<26)
#define DISPPLANE_16BPP				(0x5<<26)
#define DISPPLANE_32BPP_NO_ALPHA 		(0x6<<26)
#define DISPPLANE_32BPP				(0x7<<26)
#define DISPPLANE_STEREO_ENABLE			(1<<25)
#define DISPPLANE_STEREO_DISABLE		0
#define DISPPLANE_SEL_PIPE_MASK			(1<<24)
#define DISPPLANE_SEL_PIPE_A			0
#define DISPPLANE_SEL_PIPE_B			(1<<24)
#define DISPPLANE_SRC_KEY_ENABLE		(1<<22)
#define DISPPLANE_SRC_KEY_DISABLE		0
#define DISPPLANE_LINE_DOUBLE			(1<<20)
#define DISPPLANE_NO_LINE_DOUBLE		0
#define DISPPLANE_STEREO_POLARITY_FIRST		0
#define DISPPLANE_STEREO_POLARITY_SECOND	(1<<18)
/* plane B only */
#define DISPPLANE_ALPHA_TRANS_ENABLE		(1<<15)
#define DISPPLANE_ALPHA_TRANS_DISABLE		0
#define DISPPLANE_SPRITE_ABOVE_DISPLAYA		0
#define DISPPLANE_SPRITE_ABOVE_OVERLAY		(1)

#define DSPABASE		0x70184
#define DSPASTRIDE		0x70188

#define DSPBBASE		0x71184
#define DSPBADDR		DSPBBASE
#define DSPBSTRIDE		0x71188

#define DSPAKEYVAL		0x70194
#define DSPAKEYMASK		0x70198

#define DSPAPOS			0x7018C	/* reserved */
#define DSPASIZE		0x70190
#define DSPBPOS			0x7118C
#define DSPBSIZE		0x71190

#define DSPASURF		0x7019C
#define DSPATILEOFF		0x701A4

#define DSPBSURF		0x7119C
#define DSPBTILEOFF		0x711A4

#define VGACNTRL		0x71400
# define VGA_DISP_DISABLE			(1 << 31)
# define VGA_2X_MODE				(1 << 30)
# define VGA_PIPE_B_SELECT			(1 << 29)

/*
 * Some BIOS scratch area registers.  The 845 (and 830?) store the amount
 * of video memory available to the BIOS in SWF1.
 */

#define SWF0			0x71410
#define SWF1			0x71414
#define SWF2			0x71418
#define SWF3			0x7141c
#define SWF4			0x71420
#define SWF5			0x71424
#define SWF6			0x71428

/*
 * 855 scratch registers.
 */
#define SWF00			0x70410
#define SWF01			0x70414
#define SWF02			0x70418
#define SWF03			0x7041c
#define SWF04			0x70420
#define SWF05			0x70424
#define SWF06			0x70428

#define SWF10			SWF0
#define SWF11			SWF1
#define SWF12			SWF2
#define SWF13			SWF3
#define SWF14			SWF4
#define SWF15			SWF5
#define SWF16			SWF6

#define SWF30			0x72414
#define SWF31			0x72418
#define SWF32			0x7241c


/*
 * Palette registers
 */
#define PALETTE_A		0x0a000
#define PALETTE_B		0x0a800

#define IS_I830(dev) ((dev)->pci_device == PCI_DEVICE_ID_INTEL_82830_CGC)
#define IS_845G(dev) ((dev)->pci_device == PCI_DEVICE_ID_INTEL_82845G_IG)
#define IS_I85X(dev) ((dev)->pci_device == PCI_DEVICE_ID_INTEL_82855GM_IG)
#define IS_I855(dev) ((dev)->pci_device == PCI_DEVICE_ID_INTEL_82855GM_IG)
#define IS_I865G(dev) ((dev)->pci_device == PCI_DEVICE_ID_INTEL_82865_IG)


/* || dev->pci_device == PCI_DEVICE_ID_INTELPCI_CHIP_E7221_G) */
#define IS_I915G(dev) (dev->pci_device == PCI_DEVICE_ID_INTEL_82915G_IG)
#define IS_I915GM(dev) ((dev)->pci_device == PCI_DEVICE_ID_INTEL_82915GM_IG)
#define IS_I945G(dev) ((dev)->pci_device == PCI_DEVICE_ID_INTEL_82945G_IG)
#define IS_I945GM(dev) ((dev)->pci_device == PCI_DEVICE_ID_INTEL_82945GM_IG)

#define IS_I965G(dev) ((dev)->pci_device == 0x2972 || \
		       (dev)->pci_device == 0x2982 || \
		       (dev)->pci_device == 0x2992 || \
		       (dev)->pci_device == 0x29A2 || \
		       (dev)->pci_device == 0x2A02 || \
		       (dev)->pci_device == 0x2A12)

#define IS_I965GM(dev) ((dev)->pci_device == 0x2A02)

#define IS_G33(dev)    ((dev)->pci_device == 0x29C2 ||	\
			(dev)->pci_device == 0x29B2 ||	\
			(dev)->pci_device == 0x29D2)

#define IS_I9XX(dev) (IS_I915G(dev) || IS_I915GM(dev) || IS_I945G(dev) || \
		      IS_I945GM(dev) || IS_I965G(dev) || IS_POULSBO(dev) || \
		      IS_MRST(dev))

#define IS_MOBILE(dev) (IS_I830(dev) || IS_I85X(dev) || IS_I915GM(dev) || \
			IS_I945GM(dev) || IS_I965GM(dev) || \
			IS_POULSBO(dev) || IS_MRST(dev))

/* Cursor A & B regs */
#define CURACNTR		0x70080
#define   CURSOR_MODE_DISABLE   0x00
#define   CURSOR_MODE_64_32B_AX 0x07
#define   CURSOR_MODE_64_ARGB_AX ((1 << 5) | CURSOR_MODE_64_32B_AX)
#define   MCURSOR_GAMMA_ENABLE  (1 << 26)
#define CURABASE		0x70084
#define CURAPOS			0x70088
#define   CURSOR_POS_MASK       0x007FF
#define   CURSOR_POS_SIGN       0x8000
#define   CURSOR_X_SHIFT        0
#define   CURSOR_Y_SHIFT        16
#define CURBCNTR		0x700c0
#define CURBBASE		0x700c4
#define CURBPOS			0x700c8

/*
 * MOORESTOWN delta registers
 */
#define MRST_DPLL_A		0x0f014
#define DPLLA_MODE_LVDS			(2 << 26)	/* mrst */
#define MRST_FPA0		0x0f040
#define MRST_FPA1		0x0f044

/* #define LVDS			0x61180 */
# define MRST_PANEL_8TO6_DITHER_ENABLE		(1 << 25)
# define MRST_PANEL_24_DOT_1_FORMAT		(1 << 24)
# define LVDS_A3_POWER_UP_0_OUTPUT		(1 << 6)

#define MIPI			0x61190
# define MIPI_PORT_EN			(1 << 31)

/* #define PP_CONTROL	0x61204 */
# define POWER_DOWN_ON_RESET		(1 << 1)

/* #define PFIT_CONTROL	0x61230 */
# define PFIT_PIPE_SELECT				(3 << 29)
# define PFIT_PIPE_SELECT_SHIFT			(29)

/* #define BLC_PWM_CTL		0x61254 */
#define MRST_BACKLIGHT_MODULATION_FREQ_SHIFT		(16)
#define MRST_BACKLIGHT_MODULATION_FREQ_MASK		(0xffff << 16)

/* #define PIPEACONF 0x70008 */
#define PIPEACONF_PIPE_STATE	(1<<30)
/* #define DSPACNTR		0x70180 */
#if 0				/*FIXME JLIU7 need to define the following */
1000 = 32 - bit RGBX(10 : 10 : 10 : 2)
pixel format.Ignore alpha.1010 = BGRX 10 : 10 : 10 : 2 1100 = 64 - bit RGBX
(16 : 16 : 16 : 16) 16 bit floating point pixel format.
Ignore alpha.1110 = 32 - bit RGBX(8 : 8 : 8 : 8) pixel format.
    Ignore
    alpha.
#endif				/*FIXME JLIU7 need to define the following */

#define MRST_DSPABASE		0x7019c

/*
 * MOORESTOWN reserved registers
 */
#if 0
#define DSPAPOS			0x7018C	/* reserved */
#define DSPASIZE		0x70190
#endif
/*
 * Moorestown registers.
 */
/*===========================================================================
; General Constants
;--------------------------------------------------------------------------*/
#define BIT0  0x00000001
#define BIT1  0x00000002
#define BIT2  0x00000004
#define BIT3  0x00000008
#define BIT4  0x00000010
#define BIT5  0x00000020
#define BIT6  0x00000040
#define BIT7  0x00000080
#define BIT8  0x00000100
#define BIT9  0x00000200
#define BIT10 0x00000400
#define BIT11 0x00000800
#define BIT12 0x00001000
#define BIT13 0x00002000
#define BIT14 0x00004000
#define BIT15 0x00008000
#define BIT16 0x00010000
#define BIT17 0x00020000
#define BIT18 0x00040000
#define BIT19 0x00080000
#define BIT20 0x00100000
#define BIT21 0x00200000
#define BIT22 0x00400000
#define BIT23 0x00800000
#define BIT24 0x01000000
#define BIT25 0x02000000
#define BIT26 0x04000000
#define BIT27 0x08000000
#define BIT28 0x10000000
#define BIT29 0x20000000
#define BIT30 0x40000000
#define BIT31 0x80000000
/*===========================================================================
; MIPI IP registers
;--------------------------------------------------------------------------*/
#define DEVICE_READY_REG             0xb000
#define INTR_STAT_REG                0xb004
#define RX_SOT_ERROR BIT0
#define RX_SOT_SYNC_ERROR BIT1
#define RX_ESCAPE_MODE_ENTRY_ERROR BIT3
#define RX_LP_TX_SYNC_ERROR BIT4
#define RX_HS_RECEIVE_TIMEOUT_ERROR BIT5
#define RX_FALSE_CONTROL_ERROR BIT6
#define RX_ECC_SINGLE_BIT_ERROR BIT7
#define RX_ECC_MULTI_BIT_ERROR BIT8
#define RX_CHECKSUM_ERROR BIT9
#define RX_DSI_DATA_TYPE_NOT_RECOGNIZED BIT10
#define RX_DSI_VC_ID_INVALID BIT11
#define TX_FALSE_CONTROL_ERROR BIT12
#define TX_ECC_SINGLE_BIT_ERROR BIT13
#define TX_ECC_MULTI_BIT_ERROR BIT14
#define TX_CHECKSUM_ERROR BIT15
#define TX_DSI_DATA_TYPE_NOT_RECOGNIZED BIT16
#define TX_DSI_VC_ID_INVALID BIT17
#define HIGH_CONTENTION BIT18
#define LOW_CONTENTION BIT19
#define DPI_FIFO_UNDER_RUN BIT20
#define HS_TX_TIMEOUT BIT21
#define LP_RX_TIMEOUT BIT22
#define TURN_AROUND_ACK_TIMEOUT BIT23
#define ACK_WITH_NO_ERROR BIT24
#define INTR_EN_REG                  0xb008
#define DSI_FUNC_PRG_REG             0xb00c
#define DPI_CHANNEL_NUMBER_POS   0x03
#define DBI_CHANNEL_NUMBER_POS   0x05
#define FMT_DPI_POS              0x07
#define FMT_DBI_POS              0x0A
#define DBI_DATA_WIDTH_POS       0x0D
#define HS_TX_TIMEOUT_REG            0xb010
#define LP_RX_TIMEOUT_REG            0xb014
#define TURN_AROUND_TIMEOUT_REG      0xb018
#define DEVICE_RESET_REG             0xb01C
#define DPI_RESOLUTION_REG           0xb020
#define RES_V_POS                0x10
#define DBI_RESOLUTION_REG           0xb024
#define HORIZ_SYNC_PAD_COUNT_REG     0xb028
#define HORIZ_BACK_PORCH_COUNT_REG   0xb02C
#define HORIZ_FRONT_PORCH_COUNT_REG  0xb030
#define HORIZ_ACTIVE_AREA_COUNT_REG  0xb034
#define VERT_SYNC_PAD_COUNT_REG      0xb038
#define VERT_BACK_PORCH_COUNT_REG    0xb03c
#define VERT_FRONT_PORCH_COUNT_REG   0xb040
#define HIGH_LOW_SWITCH_COUNT_REG    0xb044
#define DPI_CONTROL_REG              0xb048
#define DPI_SHUT_DOWN            BIT0
#define DPI_TURN_ON              BIT1
#define DPI_COLOR_MODE_ON        BIT2
#define DPI_COLOR_MODE_OFF       BIT3
#define DPI_BACK_LIGHT_ON        BIT4
#define DPI_BACK_LIGHT_OFF       BIT5
#define DPI_LP                   BIT6
#define DPI_DATA_REG                 0xb04c
#define DPI_BACK_LIGHT_ON_DATA   0x07
#define DPI_BACK_LIGHT_OFF_DATA  0x17
#define INIT_COUNT_REG               0xb050
#define MAX_RET_PAK_REG              0xb054
#define VIDEO_FMT_REG                0xb058
#define EOT_DISABLE_REG              0xb05c
#define LP_BYTECLK_REG               0xb060
#define LP_GEN_DATA_REG              0xb064
#define HS_GEN_DATA_REG              0xb068
#define LP_GEN_CTRL_REG              0xb06C
#define HS_GEN_CTRL_REG              0xb070
#define GEN_FIFO_STAT_REG            0xb074
#define HS_DATA_FIFO_FULL        BIT0
#define HS_DATA_FIFO_HALF_EMPTY  BIT1
#define HS_DATA_FIFO_EMPTY       BIT2
#define LP_DATA_FIFO_FULL        BIT8
#define LP_DATA_FIFO_HALF_EMPTY  BIT9
#define LP_DATA_FIFO_EMPTY       BIT10
#define HS_CTRL_FIFO_FULL        BIT16
#define HS_CTRL_FIFO_HALF_EMPTY  BIT17
#define HS_CTRL_FIFO_EMPTY       BIT18
#define LP_CTRL_FIFO_FULL        BIT24
#define LP_CTRL_FIFO_HALF_EMPTY  BIT25
#define LP_CTRL_FIFO_EMPTY       BIT26
/*===========================================================================
; MIPI Adapter registers
;--------------------------------------------------------------------------*/
#define MIPI_CONTROL_REG             0xb104
#define MIPI_2X_CLOCK_BITS       (BIT0 | BIT1)
#define MIPI_DATA_ADDRESS_REG        0xb108
#define MIPI_DATA_LENGTH_REG         0xb10C
#define MIPI_COMMAND_ADDRESS_REG     0xb110
#define MIPI_COMMAND_LENGTH_REG      0xb114
#define MIPI_READ_DATA_RETURN_REG0   0xb118
#define MIPI_READ_DATA_RETURN_REG1   0xb11C
#define MIPI_READ_DATA_RETURN_REG2   0xb120
#define MIPI_READ_DATA_RETURN_REG3   0xb124
#define MIPI_READ_DATA_RETURN_REG4   0xb128
#define MIPI_READ_DATA_RETURN_REG5   0xb12C
#define MIPI_READ_DATA_RETURN_REG6   0xb130
#define MIPI_READ_DATA_RETURN_REG7   0xb134
#define MIPI_READ_DATA_VALID_REG     0xb138
/* DBI COMMANDS */
#define soft_reset                   0x01
/* ************************************************************************* *\
The display module performs a software reset.
Registers are written with their SW Reset default values.
\* ************************************************************************* */
#define get_power_mode               0x0a
/* ************************************************************************* *\
The display module returns the current power mode
\* ************************************************************************* */
#define get_address_mode             0x0b
/* ************************************************************************* *\
The display module returns the current status.
\* ************************************************************************* */
#define get_pixel_format             0x0c
/* ************************************************************************* *\
This command gets the pixel format for the RGB image data
used by the interface.
\* ************************************************************************* */
#define get_display_mode             0x0d
/* ************************************************************************* *\
The display module returns the Display Image Mode status.
\* ************************************************************************* */
#define get_signal_mode              0x0e
/* ************************************************************************* *\
The display module returns the Display Signal Mode.
\* ************************************************************************* */
#define get_diagnostic_result        0x0f
/* ************************************************************************* *\
The display module returns the self-diagnostic results following
a Sleep Out command.
\* ************************************************************************* */
#define enter_sleep_mode             0x10
/* ************************************************************************* *\
This command causes the display module to enter the Sleep mode.
In this mode, all unnecessary blocks inside the display module are disabled
except interface communication. This is the lowest power mode
the display module supports.
\* ************************************************************************* */
#define exit_sleep_mode              0x11
/* ************************************************************************* *\
This command causes the display module to exit Sleep mode.
All blocks inside the display module are enabled.
\* ************************************************************************* */
#define enter_partial_mode           0x12
/* ************************************************************************* *\
This command causes the display module to enter the Partial Display Mode.
The Partial Display Mode window is described by the set_partial_area command.
\* ************************************************************************* */
#define enter_normal_mode            0x13
/* ************************************************************************* *\
This command causes the display module to enter the Normal mode.
Normal Mode is defined as Partial Display mode and Scroll mode are off
\* ************************************************************************* */
#define exit_invert_mode             0x20
/* ************************************************************************* *\
This command causes the display module to stop inverting the image data on
the display device. The frame memory contents remain unchanged.
No status bits are changed.
\* ************************************************************************* */
#define enter_invert_mode            0x21
/* ************************************************************************* *\
This command causes the display module to invert the image data only on
the display device. The frame memory contents remain unchanged.
No status bits are changed.
\* ************************************************************************* */
#define set_gamma_curve              0x26
/* ************************************************************************* *\
This command selects the desired gamma curve for the display device.
Four fixed gamma curves are defined in section DCS spec.
\* ************************************************************************* */
#define set_display_off              0x28
/* ************************************************************************* *\
This command causes the display module to stop displaying the image data
on the display device. The frame memory contents remain unchanged.
No status bits are changed.
\* ************************************************************************* */
#define set_display_on               0x29
/* ************************************************************************* *\
This command causes the display module to start displaying the image data
on the display device. The frame memory contents remain unchanged.
No status bits are changed.
\* ************************************************************************* */
#define set_column_address           0x2a
/* ************************************************************************* *\
This command defines the column extent of the frame memory accessed by the
hostprocessor with the read_memory_continue and write_memory_continue commands.
No status bits are changed.
\* ************************************************************************* */
#define set_page_address             0x2b
/* ************************************************************************* *\
This command defines the page extent of the frame memory accessed by the host
processor with the write_memory_continue and read_memory_continue command.
No status bits are changed.
\* ************************************************************************* */
#define write_mem_start              0x2c
/* ************************************************************************* *\
This command transfers image data from the host processor to the display
module s frame memory starting at the pixel location specified by
preceding set_column_address and set_page_address commands.
\* ************************************************************************* */
#define set_partial_area             0x30
/* ************************************************************************* *\
This command defines the Partial Display mode s display area.
There are two parameters associated with
this command, the first defines the Start Row (SR) and the second the End Row
(ER). SR and ER refer to the Frame Memory Line Pointer.
\* ************************************************************************* */
#define set_scroll_area              0x33
/* ************************************************************************* *\
This command defines the display modules Vertical Scrolling Area.
\* ************************************************************************* */
#define set_tear_off                 0x34
/* ************************************************************************* *\
This command turns off the display modules Tearing Effect output signal on
the TE signal line.
\* ************************************************************************* */
#define set_tear_on                  0x35
/* ************************************************************************* *\
This command turns on the display modules Tearing Effect output signal
on the TE signal line.
\* ************************************************************************* */
#define set_address_mode             0x36
/* ************************************************************************* *\
This command sets the data order for transfers from the host processor to
display modules frame memory,bits B[7:5] and B3, and from the display
modules frame memory to the display device, bits B[2:0] and B4.
\* ************************************************************************* */
#define set_scroll_start             0x37
/* ************************************************************************* *\
This command sets the start of the vertical scrolling area in the frame memory.
The vertical scrolling area is fully defined when this command is used with
the set_scroll_area command The set_scroll_start command has one parameter,
the Vertical Scroll Pointer. The VSP defines the line in the frame memory
that is written to the display device as the first line of the vertical
scroll area.
\* ************************************************************************* */
#define exit_idle_mode               0x38
/* ************************************************************************* *\
This command causes the display module to exit Idle mode.
\* ************************************************************************* */
#define enter_idle_mode              0x39
/* ************************************************************************* *\
This command causes the display module to enter Idle Mode.
In Idle Mode, color expression is reduced. Colors are shown on the display
device using the MSB of each of the R, G and B color components in the frame
memory
\* ************************************************************************* */
#define set_pixel_format             0x3a
/* ************************************************************************* *\
This command sets the pixel format for the RGB image data used by the interface.
Bits D[6:4]  DPI Pixel Format Definition
Bits D[2:0]  DBI Pixel Format Definition
Bits D7 and D3 are not used.
\* ************************************************************************* */
#define write_mem_cont               0x3c
/* ************************************************************************* *\
This command transfers image data from the host processor to the display
module's frame memory continuing from the pixel location following the
previous write_memory_continue or write_memory_start command.
\* ************************************************************************* */
#define set_tear_scanline            0x44
/* ************************************************************************* *\
This command turns on the display modules Tearing Effect output signal on the
TE signal line when the display module reaches line N.
\* ************************************************************************* */
#define get_scanline                 0x45
/* ************************************************************************* *\
The display module returns the current scanline, N, used to update the
display device. The total number of scanlines on a display device is
defined as VSYNC + VBP + VACT + VFP.The first scanline is defined as
the first line of V Sync and is denoted as Line 0.
When in Sleep Mode, the value returned by get_scanline is undefined.
\* ************************************************************************* */
/* DCS Interface Pixel Formats */
#define DCS_PIXEL_FORMAT_3BPP         0x1
#define DCS_PIXEL_FORMAT_8BPP         0x2
#define DCS_PIXEL_FORMAT_12BPP        0x3
#define DCS_PIXEL_FORMAT_16BPP        0x5
#define DCS_PIXEL_FORMAT_18BPP        0x6
#define DCS_PIXEL_FORMAT_24BPP        0x7
/* ONE PARAMETER READ DATA */
#define addr_mode_data           0xfc
#define diag_res_data            0x00
#define disp_mode_data           0x23
#define pxl_fmt_data             0x77
#define pwr_mode_data            0x74
#define sig_mode_data            0x00
/* TWO PARAMETERS READ DATA */
#define scanline_data1           0xff
#define scanline_data2           0xff
/* DPI PIXEL FORMATS */
#define RGB_565_FMT		     0x01	/* RGB 565 FORMAT */
#define RGB_666_FMT		     0x02	/* RGB 666 FORMAT */
#define LRGB_666_FMT		     0x03	/* RGB LOOSELY PACKED
						 * 666 FORMAT
						 */
#define RGB_888_FMT		     0x04	/* RGB 888 FORMAT */
#define NON_BURST_MODE_SYNC_PULSE	0x01	/* Non Burst Mode
						 * with Sync Pulse
						 */
#define NON_BURST_MODE_SYNC_EVENTS	0x02	/* Non Burst Mode
						 * with Sync events
						 */
#define BURST_MODE			0x03	/* Burst Mode */
#define VIRTUAL_CHANNEL_NUMBER_0	0x00	/* Virtual channel 0 */
#define VIRTUAL_CHANNEL_NUMBER_1	0x01	/* Virtual channel 1 */
#define VIRTUAL_CHANNEL_NUMBER_2	0x02	/* Virtual channel 2 */
#define VIRTUAL_CHANNEL_NUMBER_3	0x03	/* Virtual channel 3 */
#define DBI_NOT_SUPPORTED		0x00	/* command mode
						 * is not supported
						 */
#define DBI_DATA_WIDTH_16BIT		0x01	/* 16 bit data */
#define DBI_DATA_WIDTH_9BIT			0x02	/* 9 bit data */
#define DBI_DATA_WIDTH_8BIT			0x03	/* 8 bit data */
#define DBI_COMMAND_BUFFER_SIZE		0x120	/* Allocate at least
						 * 0x100 Byte with 32
						 * byte alignment
						 */
#define DBI_DATA_BUFFER_SIZE		0x120	/* Allocate at least
						 * 0x100 Byte with 32
						 * byte alignment
						 */
#define ALIGNMENT_32BYTE_MASK		(~(BIT0|BIT1|BIT2|BIT3|BIT4))
#define SKU_83 						0x01
#define SKU_100 					0x02
#define SKU_100L 					0x04
#define SKU_BYPASS 					0x08
#if 0
/* ************************************************************************* *\
DSI command data structure
\* ************************************************************************* */
union DSI_LONG_PACKET_HEADER {
	u32 DSI_longPacketHeader;
	struct {
		u8 dataID;
		u16 wordCount;
		u8 ECC;
	};
#if 0				/*FIXME JLIU7 */
	struct {
		u8 DT:6;
		u8 VC:2;
	};
#endif				/*FIXME JLIU7 */
};

union MIPI_ADPT_CMD_LNG_REG {
	u32 commnadLengthReg;
	struct {
		u8 command0;
		u8 command1;
		u8 command2;
		u8 command3;
	};
};

struct SET_COLUMN_ADDRESS_DATA {
	u8 command;
	u16 SC;			/* Start Column */
	u16 EC;			/* End Column */
};

struct SET_PAGE_ADDRESS_DATA {
	u8 command;
	u16 SP;			/* Start Page */
	u16 EP;			/* End Page */
};
#endif
