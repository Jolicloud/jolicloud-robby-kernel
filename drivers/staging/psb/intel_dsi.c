/*
 * Copyright Â© 2006-2007 Intel Corporation
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
 *	jim liu <jim.liu@intel.com>
 */

#include <linux/backlight.h>
#include <drm/drm_crtc.h>
#include <drm/drm_edid.h>

#define DRM_MODE_ENCODER_MIPI  5
#define DRM_MODE_CONNECTOR_MIPI                13

#if DUMP_REGISTER
extern void dump_dsi_registers(struct drm_device *dev);
#endif /* DUMP_REGISTER */

int dsi_backlight;	/* restore backlight to this value */

/**
 * Returns the maximum level of the backlight duty cycle field.
 */
static u32 mrst_dsi_get_max_backlight(struct drm_device *dev)
{
#if PRINT_JLIU7
     DRM_INFO("JLIU7 enter mrst_dsi_get_max_backlight \n");
#endif /* PRINT_JLIU7 */

    return BRIGHTNESS_MAX_LEVEL;

/* FIXME jliu7 need to revisit */
}

/**
 * Sets the backlight level.
 *
 * \param level backlight level, from 0 to intel_dsi_get_max_backlight().
 */
static void mrst_dsi_set_backlight(struct drm_device *dev, int level)
{
	u32 blc_pwm_ctl;
	u32 max_pwm_blc;

#if PRINT_JLIU7
	DRM_INFO("JLIU7 enter mrst_dsi_set_backlight \n");
#endif				/* PRINT_JLIU7 */

#if 1				/* FIXME JLIU7 */
	return;
#endif				/* FIXME JLIU7 */

	/* Provent LVDS going to total black */
	if (level < 20)
		level = 20;

	max_pwm_blc = mrst_lvds_get_PWM_ctrl_freq(dev);

	if (max_pwm_blc ==0)
	{
		return;
	}

	blc_pwm_ctl = level * max_pwm_blc / BRIGHTNESS_MAX_LEVEL;

	if (blc_pol == BLC_POLARITY_INVERSE) {
	    blc_pwm_ctl = max_pwm_blc - blc_pwm_ctl;
	}

	REG_WRITE(BLC_PWM_CTL,
		(max_pwm_blc << MRST_BACKLIGHT_MODULATION_FREQ_SHIFT) |
		blc_pwm_ctl);
}

/**
 * Sets the power state for the panel.
 */
static void mrst_dsi_set_power(struct drm_device *dev,
			struct intel_output *output, bool on)
{
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	u32 pp_status;

#if PRINT_JLIU7
     DRM_INFO("JLIU7 enter mrst_dsi_set_power \n");
#endif /* PRINT_JLIU7 */
	/*
	 * The DIS device must be ready before we can change power state.
	 */
	if (!dev_priv->dsi_device_ready)
	{
		return;
	}

	/*
	 * We don't support dual DSI yet. May be in POR in the future.
	 */
	if (dev_priv->dual_display)
	{
		return;
	}

    if (on) {
        if (dev_priv->dpi & (!dev_priv->dpi_panel_on))
	    {

#if PRINT_JLIU7
            DRM_INFO("JLIU7 mrst_dsi_set_power  dpi = on \n");
#endif /* PRINT_JLIU7 */
		    REG_WRITE(DPI_CONTROL_REG, DPI_TURN_ON);
#if 0 /*FIXME JLIU7 */
            REG_WRITE(DPI_DATA_REG, DPI_BACK_LIGHT_ON_DATA);
            REG_WRITE(DPI_CONTROL_REG, DPI_BACK_LIGHT_ON);
#endif /*FIXME JLIU7 */

            dev_priv->dpi_panel_on = true;

            REG_WRITE(PP_CONTROL, REG_READ(PP_CONTROL) |
                POWER_TARGET_ON);
            do {
                pp_status = REG_READ(PP_STATUS);
            } while ((pp_status & (PP_ON | PP_READY)) == PP_READY);
	}
	else if ((!dev_priv->dpi) & (!dev_priv->dbi_panel_on))
	{
#if PRINT_JLIU7
        DRM_INFO("JLIU7 mrst_dsi_set_power  dbi = on \n");
#endif /* PRINT_JLIU7 */

		dev_priv->DBI_CB_pointer = 0;
		/* exit sleep mode */
		*(dev_priv->p_DBI_commandBuffer + dev_priv->DBI_CB_pointer++) = exit_sleep_mode;

#if 0 /*FIXME JLIU7 */
		/* Check MIPI Adatper command registers */
		while (REG_READ(MIPI_COMMAND_ADDRESS_REG) & BIT0);
#endif /*FIXME JLIU7 */

		/* FIXME_jliu7 mapVitualToPhysical(dev_priv->p_DBI_commandBuffer);*/
		REG_WRITE(MIPI_COMMAND_LENGTH_REG, 1);
		REG_WRITE(MIPI_COMMAND_ADDRESS_REG, (u32)dev_priv->p_DBI_commandBuffer | BIT0);

		/* The host processor must wait five milliseconds after sending exit_sleep_mode command before sending another
		command. This delay allows the supply voltages and clock circuits to stabilize */
		udelay(5000);

		dev_priv->DBI_CB_pointer = 0;

		/* 	set display on */
		*(dev_priv->p_DBI_commandBuffer + dev_priv->DBI_CB_pointer++) = set_display_on ;

#if 0 /*FIXME JLIU7 */
		/* Check MIPI Adatper command registers */
		while (REG_READ(MIPI_COMMAND_ADDRESS_REG) & BIT0);
#endif /*FIXME JLIU7 */

		/* FIXME_jliu7 mapVitualToPhysical(dev_priv->p_DBI_commandBuffer);*/
		REG_WRITE(MIPI_COMMAND_LENGTH_REG, 1);
		REG_WRITE(MIPI_COMMAND_ADDRESS_REG, (u32)dev_priv->p_DBI_commandBuffer | BIT0);

		dev_priv->dbi_panel_on = true;
	}
/*FIXME JLIU7 */
/* Need to figure out how to control the MIPI panel power on sequence*/

	mrst_dsi_set_backlight(dev, dsi_backlight);
	}
	else
	{
	mrst_dsi_set_backlight(dev, 0);
/*FIXME JLIU7 */
/* Need to figure out how to control the MIPI panel power down sequence*/
	/*
	 * Only save the current backlight value if we're going from
	 * on to off.
	 */
	if (dev_priv->dpi & dev_priv->dpi_panel_on)
	{
#if PRINT_JLIU7
     DRM_INFO("JLIU7 mrst_dsi_set_power  dpi = off \n");
#endif /* PRINT_JLIU7 */

		REG_WRITE(PP_CONTROL, REG_READ(PP_CONTROL) &
			   ~POWER_TARGET_ON);
		do {
			pp_status = REG_READ(PP_STATUS);
		} while (pp_status & PP_ON);

#if 0 /*FIXME JLIU7 */
		REG_WRITE(DPI_DATA_REG, DPI_BACK_LIGHT_OFF_DATA);
		REG_WRITE(DPI_CONTROL_REG, DPI_BACK_LIGHT_OFF);
#endif /*FIXME JLIU7 */
		REG_WRITE(DPI_CONTROL_REG, DPI_SHUT_DOWN);
		dev_priv->dpi_panel_on = false;
	}
	else if ((!dev_priv->dpi) & dev_priv->dbi_panel_on)
	{
#if PRINT_JLIU7
     DRM_INFO("JLIU7 mrst_dsi_set_power  dbi = off \n");
#endif /* PRINT_JLIU7 */
		dev_priv->DBI_CB_pointer = 0;
		/* enter sleep mode */
		*(dev_priv->p_DBI_commandBuffer + dev_priv->DBI_CB_pointer++) = enter_sleep_mode;

		/* Check MIPI Adatper command registers */
		while (REG_READ(MIPI_COMMAND_ADDRESS_REG) & BIT0);

		/* FIXME_jliu7 mapVitualToPhysical(dev_priv->p_DBI_commandBuffer);*/
		REG_WRITE(MIPI_COMMAND_LENGTH_REG, 1);
		REG_WRITE(MIPI_COMMAND_ADDRESS_REG, (u32)dev_priv->p_DBI_commandBuffer | BIT0);
		dev_priv->dbi_panel_on = false;
	}
    }
}

static void mrst_dsi_dpms(struct drm_encoder *encoder, int mode)
{
	struct drm_device *dev = encoder->dev;
	struct intel_output *output = enc_to_intel_output(encoder);

#if PRINT_JLIU7
     DRM_INFO("JLIU7 enter mrst_dsi_dpms \n");
#endif /* PRINT_JLIU7 */

	if (mode == DRM_MODE_DPMS_ON)
		mrst_dsi_set_power(dev, output, true);
	else
		mrst_dsi_set_power(dev, output, false);

	/* XXX: We never power down the DSI pairs. */
}

static void mrst_dsi_save(struct drm_connector *connector)
{
#if 0 /* JB: Disable for drop */
	struct drm_device *dev = connector->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;

#if PRINT_JLIU7
     DRM_INFO("JLIU7 enter  mrst_dsi_save \n");
#endif /* PRINT_JLIU7 */

	dev_priv->savePP_ON = REG_READ(LVDSPP_ON);
	dev_priv->savePP_OFF = REG_READ(LVDSPP_OFF);
	dev_priv->savePP_CONTROL = REG_READ(PP_CONTROL);
	dev_priv->savePP_CYCLE = REG_READ(PP_CYCLE);
	dev_priv->saveBLC_PWM_CTL = REG_READ(BLC_PWM_CTL);
	dev_priv->backlight_duty_cycle = (dev_priv->saveBLC_PWM_CTL &
				       BACKLIGHT_DUTY_CYCLE_MASK);

	/*
	 * make backlight to full brightness
	 */
	dsi_backlight = mrst_dsi_get_max_backlight(dev);
#endif
}

static void mrst_dsi_restore(struct drm_connector *connector)
{
#if 0 /* JB: Disable for drop */
	struct drm_device *dev = connector->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;

#if PRINT_JLIU7
     DRM_INFO("JLIU7 enter  mrst_dsi_restore \n");
#endif /* PRINT_JLIU7 */

	REG_WRITE(BLC_PWM_CTL, dev_priv->saveBLC_PWM_CTL);
	REG_WRITE(LVDSPP_ON, dev_priv->savePP_ON);
	REG_WRITE(LVDSPP_OFF, dev_priv->savePP_OFF);
	REG_WRITE(PP_CYCLE, dev_priv->savePP_CYCLE);
	REG_WRITE(PP_CONTROL, dev_priv->savePP_CONTROL);
	if (dev_priv->savePP_CONTROL & POWER_TARGET_ON)
		mrst_dsi_set_power(dev, true);
	else
		mrst_dsi_set_power(dev, false);
#endif
}

static void mrst_dsi_prepare(struct drm_encoder *encoder)
{
	struct drm_device *dev = encoder->dev;
	struct intel_output *output = enc_to_intel_output(encoder);
	struct intel_mode_device *mode_dev = output->mode_dev;

#if PRINT_JLIU7
     DRM_INFO("JLIU7 enter  mrst_dsi_prepare \n");
#endif /* PRINT_JLIU7 */

	mode_dev->saveBLC_PWM_CTL = REG_READ(BLC_PWM_CTL);
	mode_dev->backlight_duty_cycle = (mode_dev->saveBLC_PWM_CTL &
				       BACKLIGHT_DUTY_CYCLE_MASK);

	mrst_dsi_set_power(dev, output, false);
}

static void mrst_dsi_commit( struct drm_encoder *encoder)
{
	struct drm_device *dev = encoder->dev;
	struct intel_output *output = enc_to_intel_output(encoder);
	struct intel_mode_device *mode_dev = output->mode_dev;

#if PRINT_JLIU7
     DRM_INFO("JLIU7 enter  mrst_dsi_commit \n");
#endif /* PRINT_JLIU7 */

	if (mode_dev->backlight_duty_cycle == 0)
		mode_dev->backlight_duty_cycle =
			mrst_dsi_get_max_backlight(dev);

	mrst_dsi_set_power(dev, output, true);

#if DUMP_REGISTER
	dump_dsi_registers(dev);
#endif /* DUMP_REGISTER */
}

/* ************************************************************************* *\
FUNCTION: GetHS_TX_timeoutCount
																`
DESCRIPTION: In burst mode, value  greater than one DPI line Time in byte clock
	(txbyteclkhs). To timeout this timer 1+ of the above said value is recommended.

	In non-burst mode, Value greater than one DPI frame time in byte clock(txbyteclkhs).
	To timeout this timer 1+ of the above said value is recommended.

\* ************************************************************************* */
static u32 GetHS_TX_timeoutCount(DRM_DRIVER_PRIVATE_T *dev_priv)
{

	u32 timeoutCount = 0, HTOT_count = 0, VTOT_count = 0, HTotalPixel = 0;

	/* Total pixels need to be transfer per line*/
	HTotalPixel = (dev_priv->HsyncWidth + dev_priv->HbackPorch + dev_priv->HfrontPorch) * dev_priv->laneCount + dev_priv->HactiveArea;

	/* byte count = (pixel count *  bits per pixel) / 8 */
	HTOT_count = (HTotalPixel * dev_priv->bpp) / 8;

	if (dev_priv->videoModeFormat == BURST_MODE)
	{
		timeoutCount = HTOT_count + 1;
#if 1 /*FIXME remove it after power-on */
		VTOT_count = dev_priv->VactiveArea + dev_priv->VbackPorch + dev_priv->VfrontPorch
				+ dev_priv->VsyncWidth;
		/* timeoutCount = (HTOT_count * VTOT_count) + 1; */
		timeoutCount = (HTOT_count * VTOT_count) + 1;
#endif
	}
	else
	{
		VTOT_count = dev_priv->VactiveArea + dev_priv->VbackPorch + dev_priv->VfrontPorch
				+ dev_priv->VsyncWidth;
		/* timeoutCount = (HTOT_count * VTOT_count) + 1; */
		timeoutCount = (HTOT_count * VTOT_count) + 1;
	}

	return timeoutCount & 0xFFFF;
}

/* ************************************************************************* *\
FUNCTION: GetLP_RX_timeoutCount

DESCRIPTION: The timeout value is protocol specific. Time out value is calculated
	from txclkesc(50ns).

	Minimum value =
		Time to send one Trigger message =   4 X txclkesc [Escape mode entry sequence)
		+ 8-bit trigger message (2x8xtxclkesc)
		+1 txclksesc [stop_state]
	= 21 X txclkesc [ 15h]

	Maximum Value =
		Time to send a long packet with maximum payload data
			= 4 X txclkesc [Escape mode entry sequence)
		+ 8-bit Low power data transmission Command (2x8xtxclkesc)
		+ packet header [ 4X8X2X txclkesc]
		+payload [ nX8X2Xtxclkesc]
		+CRC[2X8X2txclkesc]
		+1 txclksesc [stop_state]
	= 117 txclkesc +n[payload in terms of bytes]X16txclkesc.

\* ************************************************************************* */
static u32 GetLP_RX_timeoutCount(DRM_DRIVER_PRIVATE_T *dev_priv)
{

	u32 timeoutCount = 0;

	if (dev_priv->config_phase)
	{
		/* Assuming 256 byte DDB data.*/
		timeoutCount = 117 + 256 * 16;
	}
	else
	{
		/* For DPI video only mode use the minimum value.*/
		timeoutCount = 0x15;
#if 1 /*FIXME remove it after power-on */
		/* Assuming 256 byte DDB data.*/
		timeoutCount = 117 + 256 * 16;
#endif
	}

	return timeoutCount;
}

/* ************************************************************************* *\
FUNCTION: GetHSA_Count

DESCRIPTION: Shows the horizontal sync value in terms of byte clock
			(txbyteclkhs)
	Minimum HSA period should be sufficient to transmit a hsync start short
		packet(4 bytes)
		i) For Non-burst Mode with sync pulse, Min value – 4 in decimal [plus
			an optional 6 bytes for a zero payload blanking packet]. But if
			the value is less than 10 but more than 4, then this count will
			be added to the HBP’s count for one lane.
		ii) For Non-Burst Sync Event & Burst Mode, there is no HSA, so you
			can program this to zero. If you program this register, these
			byte values will be added to HBP.
		iii) For Burst mode of operation, normally the values programmed in
			terms of byte clock are based on the principle - time for transfering
			HSA in Burst mode is the same as in non-bust mode.
\* ************************************************************************* */
static u32 GetHSA_Count(DRM_DRIVER_PRIVATE_T *dev_priv)
{
	u32 HSA_count;
	u32 HSA_countX8;

	/* byte clock count = (pixel clock count *  bits per pixel) /8 */
	HSA_countX8 = dev_priv->HsyncWidth * dev_priv->bpp;

	if (dev_priv->videoModeFormat == BURST_MODE)
	{
		HSA_countX8 *= dev_priv->DDR_Clock / dev_priv->DDR_Clock_Calculated;
	}

	HSA_count = HSA_countX8 / 8;

	return HSA_count;
}

/* ************************************************************************* *\
FUNCTION: GetHBP_Count

DESCRIPTION: Shows the horizontal back porch value in terms of txbyteclkhs.
	Minimum HBP period should be sufficient to transmit a “hsync end short
		packet(4 bytes) + Blanking packet overhead(6 bytes) + RGB packet header(4 bytes)”
	For Burst mode of operation, normally the values programmed in terms of
		byte clock are based on the principle - time for transfering HBP
		in Burst mode is the same as in non-bust mode.

	Min value – 14 in decimal [ accounted with zero payload for blanking packet] for one lane.
	Max value – any value greater than 14 based on DPI resolution
\* ************************************************************************* */
static u32 GetHBP_Count(DRM_DRIVER_PRIVATE_T *dev_priv)
{
	u32 HBP_count;
	u32 HBP_countX8;

	/* byte clock count = (pixel clock count *  bits per pixel) /8 */
	HBP_countX8 = dev_priv->HbackPorch * dev_priv->bpp;

	if (dev_priv->videoModeFormat == BURST_MODE)
	{
		HBP_countX8 *= dev_priv->DDR_Clock / dev_priv->DDR_Clock_Calculated;
	}

	HBP_count = HBP_countX8 / 8;

	return HBP_count;
}

/* ************************************************************************* *\
FUNCTION: GetHFP_Count

DESCRIPTION: Shows the horizontal front porch value in terms of txbyteclkhs.
	Minimum HFP period should be sufficient to transmit “RGB Data packet
	footer(2 bytes) + Blanking packet overhead(6 bytes)” for non burst mode.

	For burst mode, Minimum HFP period should be sufficient to transmit
	Blanking packet overhead(6 bytes)”

	For Burst mode of operation, normally the values programmed in terms of
		byte clock are based on the principle - time for transfering HFP
		in Burst mode is the same as in non-bust mode.

	Min value – 8 in decimal  for non-burst mode [accounted with zero payload
		for blanking packet] for one lane.
	Min value – 6 in decimal for burst mode for one lane.

	Max value – any value greater than the minimum vaue based on DPI resolution
\* ************************************************************************* */
static u32 GetHFP_Count(DRM_DRIVER_PRIVATE_T *dev_priv)
{
	u32 HFP_count;
	u32 HFP_countX8;

	/* byte clock count = (pixel clock count *  bits per pixel) /8 */
	HFP_countX8 = dev_priv->HfrontPorch * dev_priv->bpp;

	if (dev_priv->videoModeFormat == BURST_MODE)
	{
		HFP_countX8 *= dev_priv->DDR_Clock / dev_priv->DDR_Clock_Calculated;
	}

	HFP_count = HFP_countX8 / 8;

	return HFP_count;
}

/* ************************************************************************* *\
FUNCTION: GetHAdr_Count

DESCRIPTION: Shows the horizontal active area value in terms of txbyteclkhs.
	In Non Burst Mode, Count equal to RGB word count value

	In Burst Mode, RGB pixel packets are time-compressed, leaving more time
		during a scan line for LP mode (saving power) or for multiplexing
		other transmissions onto the DSI link. Hence, the count equals the
		time in txbyteclkhs for sending  time compressed RGB pixels plus
		the time needed for moving to power save mode or the time needed
		for secondary channel to use the DSI link.

	But if the left out time for moving to low power mode is less than
		8 txbyteclkhs [2txbyteclkhs for RGB data packet footer and
		6txbyteclkhs for a blanking packet with zero payload],  then
		this count will be added to the HFP's count for one lane.

	Min value – 8 in decimal  for non-burst mode [accounted with zero payload
		for blanking packet] for one lane.
	Min value – 6 in decimal for burst mode for one lane.

	Max value – any value greater than the minimum vaue based on DPI resolution
\* ************************************************************************* */
static u32 GetHAdr_Count(DRM_DRIVER_PRIVATE_T *dev_priv)
{
	u32 HAdr_count;
	u32 HAdr_countX8;

	/* byte clock count = (pixel clock count *  bits per pixel) /8 */
	HAdr_countX8 = dev_priv->HactiveArea * dev_priv->bpp;

	if (dev_priv->videoModeFormat == BURST_MODE)
	{
		HAdr_countX8 *= dev_priv->DDR_Clock / dev_priv->DDR_Clock_Calculated;
	}

	HAdr_count = HAdr_countX8 / 8;

	return HAdr_count;
}

/* ************************************************************************* *\
FUNCTION: GetHighLowSwitchCount

DESCRIPTION: High speed to low power or Low power to high speed switching time
				in terms byte clock (txbyteclkhs). This value is based on the
				byte clock (txbyteclkhs) and low power clock frequency (txclkesc)

	Typical value - Number of byte clocks required to switch from low power mode
		to high speed mode after "txrequesths" is asserted.

	The worst count value among the low to high or high to low switching time
		in terms of txbyteclkhs  has to be programmed in this register.

	Usefull Formulae:
		DDR clock period = 2 times UI
		txbyteclkhs clock = 8 times UI
		Tlpx = 1 / txclkesc
		CALCULATION OF LOW POWER TO HIGH SPEED SWITCH COUNT VALUE (from Standard D-PHY spec)
			LP01 + LP00 + HS0 = 1Tlpx + 1Tlpx + 3Tlpx [Approx] + 1DDR clock [2UI] + 1txbyteclkhs clock [8UI]
		CALCULATION OF  HIGH SPEED  TO LOW POWER SWITCH COUNT VALUE (from Standard D-PHY spec)
			Ths-trail = 1txbyteclkhs clock [8UI] + 5DDR clock [10UI] + 4 Tlpx [Approx]
\* ************************************************************************* */
static u32 GetHighLowSwitchCount(DRM_DRIVER_PRIVATE_T *dev_priv)
{
	u32 HighLowSwitchCount, HighToLowSwitchCount, LowToHighSwitchCount;

/* ************************************************************************* *\
	CALCULATION OF  HIGH SPEED  TO LOW POWER SWITCH COUNT VALUE (from Standard D-PHY spec)
	Ths-trail = 1txbyteclkhs clock [8UI] + 5DDR clock [10UI] + 4 Tlpx [Approx]

	Tlpx = 50 ns, Using max txclkesc (20MHz)

	txbyteclkhs_period = 4000 / dev_priv->DDR_Clock; in ns
	UI_period = 500 / dev_priv->DDR_Clock; in ns

	HS_to_LP = Ths-trail = 18 * UI_period  + 4 * Tlpx
			= 9000 / dev_priv->DDR_Clock + 200;

	HighToLowSwitchCount = HS_to_LP / txbyteclkhs_period
			= (9000 / dev_priv->DDR_Clock + 200) / (4000 / dev_priv->DDR_Clock)
			= (9000 + (200 *  dev_priv->DDR_Clock)) / 4000

\* ************************************************************************* */
	HighToLowSwitchCount = (9000 + (200 *  dev_priv->DDR_Clock)) / 4000	+ 1;

/* ************************************************************************* *\
	CALCULATION OF LOW POWER TO HIGH SPEED SWITCH COUNT VALUE (from Standard D-PHY spec)
	LP01 + LP00 + HS0 = 1Tlpx + 1Tlpx + 3Tlpx [Approx] + 1DDR clock [2UI] + 1txbyteclkhs clock [8UI]

	LP_to_HS = 10 * UI_period + 5 * Tlpx =
			= 5000 / dev_priv->DDR_Clock + 250;

	LowToHighSwitchCount = LP_to_HS / txbyteclkhs_period
			= (5000 / dev_priv->DDR_Clock + 250) / (4000 / dev_priv->DDR_Clock)
			= (5000 + (250 *  dev_priv->DDR_Clock)) / 4000

\* ************************************************************************* */
	LowToHighSwitchCount = (5000 + (250 *  dev_priv->DDR_Clock)) / 4000	+ 1;

	if (HighToLowSwitchCount > LowToHighSwitchCount)
	{
		HighLowSwitchCount = HighToLowSwitchCount;
	}
	else
	{
		HighLowSwitchCount = LowToHighSwitchCount;
	}


	/* FIXME jliu need to fine tune the above formulae and remove the following after power on */
	if (HighLowSwitchCount < 0x1f)
		HighLowSwitchCount = 0x1f;

	return HighLowSwitchCount;
}

/* ************************************************************************* *\
FUNCTION: mrst_gen_long_write
																`
DESCRIPTION:

\* ************************************************************************* */
static void	mrst_gen_long_write(struct drm_device *dev, u32 *data, u16 wc,u8 vc)
{
	u32 gen_data_reg = HS_GEN_DATA_REG;
	u32 gen_ctrl_reg = HS_GEN_CTRL_REG;
	u32 date_full_bit = HS_DATA_FIFO_FULL;
	u32 control_full_bit = HS_CTRL_FIFO_FULL;
	u16 wc_saved = wc;

#if PRINT_JLIU7
     DRM_INFO("JLIU7 enter mrst_gen_long_write \n");
#endif /* PRINT_JLIU7 */

	/* sanity check */
	if (vc > 4)
	{
		DRM_ERROR(KERN_ERR "MIPI Virtual channel Can't greater than 4. \n");
		return;
	}


	if (0) /* FIXME JLIU7 check if it is in LP*/
	{
		gen_data_reg = LP_GEN_DATA_REG;
		gen_ctrl_reg = LP_GEN_CTRL_REG;
		date_full_bit = LP_DATA_FIFO_FULL;
		control_full_bit = LP_CTRL_FIFO_FULL;
	}

	while (wc >= 4)
	{
		/* Check if MIPI IP generic data fifo is not full */
		while ((REG_READ(GEN_FIFO_STAT_REG) & date_full_bit) == date_full_bit);

	 	/* write to data buffer */
	    REG_WRITE(gen_data_reg, *data);

		wc -= 4;
		data ++;
	}

	switch (wc)
	{
	case 1:
		REG_WRITE8(gen_data_reg, *((u8 *)data));
		break;
	case 2:
		REG_WRITE16(gen_data_reg, *((u16 *)data));
		break;
	case 3:
		REG_WRITE16(gen_data_reg, *((u16 *)data));
		data = (u32*)((u8*) data + 2);
		REG_WRITE8(gen_data_reg, *((u8 *)data));
		break;
	}

	/* Check if MIPI IP generic control fifo is not full */
	while ((REG_READ(GEN_FIFO_STAT_REG) & control_full_bit) == control_full_bit);
	/* write to control buffer */
    REG_WRITE(gen_ctrl_reg, 0x29 | (wc_saved << 8) | (vc << 6));
}

/* ************************************************************************* *\
FUNCTION: mrst_init_HIMAX_MIPI_bridge
																`
DESCRIPTION:

\* ************************************************************************* */
static void	mrst_init_HIMAX_MIPI_bridge(struct drm_device *dev)
{
	u32 gen_data[2];
	u16 wc = 0;
	u8 vc =0;
	u32 gen_data_intel = 0x200105;

#if PRINT_JLIU7
     DRM_INFO("JLIU7 enter mrst_init_HIMAX_MIPI_bridge \n");
#endif /* PRINT_JLIU7 */

	/* exit sleep mode */
	wc = 0x5;
	gen_data[0] = gen_data_intel | (0x11 << 24);
	gen_data[1] = 0;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* set_pixel_format */
	gen_data[0] = gen_data_intel | (0x3A << 24);
	gen_data[1] = 0x77;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* Set resolution for (800X480) */
	wc = 0x8;
	gen_data[0] = gen_data_intel | (0x2A << 24);
	gen_data[1] = 0x1F030000;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[0] = gen_data_intel | (0x2B << 24);
	gen_data[1] = 0xDF010000;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* System control */
	wc = 0x6;
	gen_data[0] = gen_data_intel | (0xEE << 24);
	gen_data[1] = 0x10FA;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* INPUT TIMING FOR TEST PATTERN(800X480) */
	/* H-size */
	gen_data[1] = 0x2000;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x0301;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* V-size */
	gen_data[1] = 0xE002;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x0103;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* H-total */
	gen_data[1] = 0x2004;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x0405;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* V-total */
	gen_data[1] = 0x0d06;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x0207;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* H-blank */
	gen_data[1] = 0x0308;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x0009;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* H-blank */
	gen_data[1] = 0x030A;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x000B;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* H-start */
	gen_data[1] = 0xD80C;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x000D;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* V-start */
	gen_data[1] = 0x230E;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x000F;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* RGB domain */
	gen_data[1] = 0x0027;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* INP_FORM Setting */
	/* set_1 */
	gen_data[1] = 0x1C10;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* set_2 */
	gen_data[1] = 0x0711;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* set_3 */
	gen_data[1] = 0x0012;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* set_4 */
	gen_data[1] = 0x0013;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* set_5 */
	gen_data[1] = 0x2314;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* set_6 */
	gen_data[1] = 0x0015;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* set_7 */
	gen_data[1] = 0x2316;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* set_8 */
	gen_data[1] = 0x0017;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* set_1 */
	gen_data[1] = 0x0330;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* FRC Setting */
	/* FRC_set_2 */
	gen_data[1] = 0x237A;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* FRC_set_3 */
	gen_data[1] = 0x4C7B;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* FRC_set_4 */
	gen_data[1] = 0x037C;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* FRC_set_5 */
	gen_data[1] = 0x3482;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* FRC_set_7 */
	gen_data[1] = 0x1785;
	mrst_gen_long_write(dev, gen_data, wc, vc);

#if 0
	/* FRC_set_8 */
	gen_data[1] = 0xD08F;
	mrst_gen_long_write(dev, gen_data, wc, vc);
#endif

	/* OUTPUT TIMING FOR TEST PATTERN (800X480) */
	/* out_htotal */
	gen_data[1] = 0x2090;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x0491;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* out_hsync */
	gen_data[1] = 0x0392;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x0093;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* out_hstart */
	gen_data[1] = 0xD894;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x0095;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* out_hsize */
	gen_data[1] = 0x2096;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x0397;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* out_vtotal */
	gen_data[1] = 0x0D98;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x0299;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* out_vsync */
	gen_data[1] = 0x039A;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x009B;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* out_vstart */
	gen_data[1] = 0x239C;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x009D;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* out_vsize */
	gen_data[1] = 0xE09E;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x019F;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* FRC_set_6 */
	gen_data[1] = 0x9084;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* Other setting */
	gen_data[1] = 0x0526;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* RBG domain */
	gen_data[1] = 0x1177;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* rgbw */
	/* set_1 */
	gen_data[1] = 0xD28F;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* set_2 */
	gen_data[1] = 0x02D0;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* set_3 */
	gen_data[1] = 0x08D1;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* set_4 */
	gen_data[1] = 0x05D2;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* set_5 */
	gen_data[1] = 0x24D4;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* set_6 */
	gen_data[1] = 0x00D5;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x02D7;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x00D8;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	gen_data[1] = 0x48F3;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0xD4F2;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x3D8E;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x60FD;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x00B5;
	mrst_gen_long_write(dev, gen_data, wc, vc);
	gen_data[1] = 0x48F4;
	mrst_gen_long_write(dev, gen_data, wc, vc);

	/* inside patten */
	gen_data[1] = 0x0060;
	mrst_gen_long_write(dev, gen_data, wc, vc);
}

/* ************************************************************************* *\
FUNCTION: mrst_init_NSC_MIPI_bridge
																`
DESCRIPTION:

\* ************************************************************************* */
static void	mrst_init_NSC_MIPI_bridge(struct drm_device *dev)
{

	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
#if PRINT_JLIU7
     DRM_INFO("JLIU7 enter mrst_init_NSC_MIPI_bridge.\n");
#endif /* PRINT_JLIU7 */
	/* Program MIPI IP to 50MHz DSI, Non-Burst mode with sync event,
		1 or 2 Data Lanes */

	udelay(DELAY_TIME1);
	/* enable RGB24*/
	REG_WRITE(LP_GEN_CTRL_REG, 0x003205e3);

	udelay(DELAY_TIME1);
	/* enable all error reporting*/
	REG_WRITE(LP_GEN_CTRL_REG, 0x000040e3);
	udelay(DELAY_TIME1);
	REG_WRITE(LP_GEN_CTRL_REG, 0x000041e3);

	udelay(DELAY_TIME1);
	/* enable 2 data lane; video shaping & error reporting */
	REG_WRITE(LP_GEN_CTRL_REG, 0x00a842e3); /* 0x006842e3 for 1 data lane */

	udelay(DELAY_TIME1);
	/* HS timeout */
	REG_WRITE(LP_GEN_CTRL_REG, 0x009243e3);

	udelay(DELAY_TIME1);
	/* setle = 6h; low power timeout = ((2^21)-1)*4TX_esc_clks. */
	REG_WRITE(LP_GEN_CTRL_REG, 0x00e645e3);

	/* enable all virtual channels */
	REG_WRITE(LP_GEN_CTRL_REG, 0x000f46e3);

	/* set output strength to low-drive */
	REG_WRITE(LP_GEN_CTRL_REG, 0x00007de3);

	if (dev_priv->sku_83)
	{
		/* set escape clock to divede by 8 */
		REG_WRITE(LP_GEN_CTRL_REG, 0x000044e3);
	}
	else if(dev_priv->sku_100L)
	{
		/* set escape clock to divede by 16 */
		REG_WRITE(LP_GEN_CTRL_REG, 0x001044e3);
	}
	else if(dev_priv->sku_100)
	{
		/* set escape clock to divede by 32*/
		REG_WRITE(LP_GEN_CTRL_REG, 0x003044e3);

		/* setle = 6h; low power timeout = ((2^21)-1)*4TX_esc_clks. */
		REG_WRITE(LP_GEN_CTRL_REG, 0x00ec45e3);
	}

	/* CFG_VALID=1; RGB_CLK_EN=1. */
	REG_WRITE(LP_GEN_CTRL_REG, 0x00057fe3);

}

static void mrst_dsi_mode_set(struct drm_encoder *encoder,
				struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode)
{
	struct drm_device *dev = encoder->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	u32 pfit_control;
	u32 dsiFuncPrgValue = 0;
	u32 SupportedFormat = 0;
	u32 channelNumber = 0;
	u32 DBI_dataWidth = 0;
	u32 resolution = 0;
	u32 mipiport = 0;

#if PRINT_JLIU7
     DRM_INFO("JLIU7 enter  mrst_dsi_mode_set \n");
#endif /* PRINT_JLIU7 */

	switch (dev_priv->bpp)
	{
	case 16:
		SupportedFormat = RGB_565_FMT;
		break;
	case 18:
		SupportedFormat = RGB_666_FMT;
		break;
	case 24:
		SupportedFormat = RGB_888_FMT;
		break;
	default:
		DRM_INFO("mrst_dsi_mode_set,  invalid bpp \n");
		break;
	}

	resolution = dev_priv->HactiveArea | (dev_priv->VactiveArea << RES_V_POS);

	if (dev_priv->dpi)
	{
    /* Enable automatic panel scaling for non-native modes so that they fill
     * the screen.  Should be enabled before the pipe is enabled, according to
     * register description and PRM.
     */
	/*FIXME JLIU7, enable Auto-scale only */
	/*
	 * Enable automatic panel scaling so that non-native modes fill the
	 * screen.  Should be enabled before the pipe is enabled, according to
	 * register description and PRM.
	 */
#if 0 /*JLIU7_PO */
		if (mode->hdisplay != adjusted_mode->hdisplay ||
			    mode->vdisplay != adjusted_mode->vdisplay)
	    {
			pfit_control = PFIT_ENABLE;
	    }
    	else
#endif /*JLIU7_PO */
	    {
			pfit_control = 0;
	    }
	    REG_WRITE(PFIT_CONTROL, pfit_control);

		/* Enable MIPI Port */
		mipiport = MIPI_PORT_EN;
		REG_WRITE(MIPI, mipiport);

		/* JLIU7_FIXME set MIPI clock ratio to 1:1 for NSC init */
		REG_WRITE(MIPI_CONTROL_REG, 0x00000018);

		/* Enable all the error interrupt */
		REG_WRITE(INTR_EN_REG, 0xffffffff);
		REG_WRITE(TURN_AROUND_TIMEOUT_REG, 0x0000000F);
		REG_WRITE(DEVICE_RESET_REG, 0x000000ff); /* old value = 0x00000015 may depends on the DSI RX device*/
		REG_WRITE(INIT_COUNT_REG, 0x00000fff); /* Minimum value = 0x000007d0 */

		SupportedFormat <<= FMT_DPI_POS;
		dsiFuncPrgValue = dev_priv->laneCount | SupportedFormat;
		REG_WRITE(DSI_FUNC_PRG_REG, dsiFuncPrgValue);

		REG_WRITE(DPI_RESOLUTION_REG, resolution);
		REG_WRITE(DBI_RESOLUTION_REG, 0x00000000);

		REG_WRITE(VERT_SYNC_PAD_COUNT_REG, dev_priv->VsyncWidth);
		REG_WRITE(VERT_BACK_PORCH_COUNT_REG, dev_priv->VbackPorch);
		REG_WRITE(VERT_FRONT_PORCH_COUNT_REG, dev_priv->VfrontPorch);

#if 1 /*JLIU7_PO hard coded for NSC PO */
		REG_WRITE(HORIZ_SYNC_PAD_COUNT_REG, 0x1e);
		REG_WRITE(HORIZ_BACK_PORCH_COUNT_REG, 0x18);
		REG_WRITE(HORIZ_FRONT_PORCH_COUNT_REG, 0x8);
		REG_WRITE(HORIZ_ACTIVE_AREA_COUNT_REG, 0x4b0);
#else /*JLIU7_PO hard coded for NSC PO */
		REG_WRITE(HORIZ_SYNC_PAD_COUNT_REG, GetHSA_Count(dev_priv));
		REG_WRITE(HORIZ_BACK_PORCH_COUNT_REG, GetHBP_Count(dev_priv));
		REG_WRITE(HORIZ_FRONT_PORCH_COUNT_REG, GetHFP_Count(dev_priv));
		REG_WRITE(HORIZ_ACTIVE_AREA_COUNT_REG, GetHAdr_Count(dev_priv));
#endif  /*JLIU7_PO hard coded for NSC PO */
		REG_WRITE(VIDEO_FMT_REG, dev_priv->videoModeFormat);
	}
	else
	{
		/* JLIU7 FIXME VIRTUAL_CHANNEL_NUMBER_1 or VIRTUAL_CHANNEL_NUMBER_0*/
		channelNumber = VIRTUAL_CHANNEL_NUMBER_1 << DBI_CHANNEL_NUMBER_POS;
		DBI_dataWidth = DBI_DATA_WIDTH_16BIT << DBI_DATA_WIDTH_POS;
		dsiFuncPrgValue = dev_priv->laneCount | channelNumber | DBI_dataWidth;
		/* JLIU7 FIXME */
		SupportedFormat <<= FMT_DBI_POS;
		dsiFuncPrgValue |= SupportedFormat;
		REG_WRITE(DSI_FUNC_PRG_REG, dsiFuncPrgValue);

		REG_WRITE(DPI_RESOLUTION_REG, 0x00000000);
		REG_WRITE(DBI_RESOLUTION_REG, resolution);
	}

#if 1 /*JLIU7_PO hard code for NSC PO */
	REG_WRITE(HS_TX_TIMEOUT_REG, 0xffff);
	REG_WRITE(LP_RX_TIMEOUT_REG, 0xffff);

	REG_WRITE(HIGH_LOW_SWITCH_COUNT_REG, 0x46);
#else /*JLIU7_PO hard code for NSC PO */
	REG_WRITE(HS_TX_TIMEOUT_REG, GetHS_TX_timeoutCount(dev_priv));
	REG_WRITE(LP_RX_TIMEOUT_REG, GetLP_RX_timeoutCount(dev_priv));

	REG_WRITE(HIGH_LOW_SWITCH_COUNT_REG, GetHighLowSwitchCount(dev_priv));
#endif /*JLIU7_PO hard code for NSC PO */


	REG_WRITE(EOT_DISABLE_REG, 0x00000000);

	/* FIXME JLIU7 for NSC PO */
	REG_WRITE(LP_BYTECLK_REG, 0x00000004);

	REG_WRITE(DEVICE_READY_REG, 0x00000001);
	REG_WRITE(DPI_CONTROL_REG, 0x00000002); /* Turn On */

	dev_priv->dsi_device_ready = true;

#if 0 /*JLIU7_PO */
	mrst_init_HIMAX_MIPI_bridge(dev);
#endif /*JLIU7_PO */
	mrst_init_NSC_MIPI_bridge(dev);

        if (dev_priv->sku_100L)
		/* Set DSI link to 100MHz; 2:1 clock ratio */
		REG_WRITE(MIPI_CONTROL_REG, 0x00000009);

	REG_WRITE(PIPEACONF, dev_priv->pipeconf);
	REG_READ(PIPEACONF);

	/* Wait for 20ms for the pipe enable to take effect. */
	udelay(20000);

        /* JLIU7_PO hard code for NSC PO Program the display FIFO watermarks */
	REG_WRITE(DSPARB, 0x00001d9c);
	REG_WRITE(DSPFW1, 0xfc0f0f18);
	REG_WRITE(DSPFW5, 0x04140404);
	REG_WRITE(DSPFW6, 0x000001f0);

	REG_WRITE(DSPACNTR, dev_priv->dspcntr);

	/* Wait for 20ms for the plane enable to take effect. */
	udelay(20000);
}

/**
 * Detect the MIPI connection.
 *
 * This always returns CONNECTOR_STATUS_CONNECTED.
 * This connector should only have
 * been set up if the MIPI was actually connected anyway.
 */
static enum drm_connector_status mrst_dsi_detect(struct drm_connector
						   *connector)
{
#if PRINT_JLIU7
     DRM_INFO("JLIU7 enter  mrst_dsi_detect \n");
#endif /* PRINT_JLIU7 */

	return connector_status_connected;
}

/**
 * Return the list of MIPI DDB modes if available.
 */
static int mrst_dsi_get_modes(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct intel_output *intel_output = to_intel_output(connector);
	struct intel_mode_device *mode_dev = intel_output->mode_dev;

/* FIXME get the MIPI DDB modes */

	/* Didn't get an DDB, so
	 * Set wide sync ranges so we get all modes
	 * handed to valid_mode for checking
	 */
	connector->display_info.min_vfreq = 0;
	connector->display_info.max_vfreq = 200;
	connector->display_info.min_hfreq = 0;
	connector->display_info.max_hfreq = 200;

	if (mode_dev->panel_fixed_mode != NULL) {
		struct drm_display_mode *mode =
		    drm_mode_duplicate(dev, mode_dev->panel_fixed_mode);
		drm_mode_probed_add(connector, mode);
		return 1;
	}

	return 0;
}

static const struct drm_encoder_helper_funcs mrst_dsi_helper_funcs = {
	.dpms = mrst_dsi_dpms,
	.mode_fixup = intel_lvds_mode_fixup,
	.prepare = mrst_dsi_prepare,
	.mode_set = mrst_dsi_mode_set,
	.commit = mrst_dsi_commit,
};

static const struct drm_connector_helper_funcs
    mrst_dsi_connector_helper_funcs = {
	.get_modes = mrst_dsi_get_modes,
	.mode_valid = intel_lvds_mode_valid,
	.best_encoder = intel_best_encoder,
};

static const struct drm_connector_funcs mrst_dsi_connector_funcs = {
	.save = mrst_dsi_save,
	.restore = mrst_dsi_restore,
	.detect = mrst_dsi_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = intel_lvds_destroy,
};

/** Returns the panel fixed mode from configuration. */
/** FIXME JLIU7 need to revist it. */
struct drm_display_mode *mrst_dsi_get_configuration_mode(struct drm_device *dev)
{
	struct drm_display_mode *mode;

	mode = kzalloc(sizeof(*mode), GFP_KERNEL);
	if (!mode)
		return NULL;

#if 1 /*FIXME jliu7 remove it later */
	    /* copy from SV - hard coded fixed mode for DSI TPO TD043MTEA2 LCD panel */
	mode->hdisplay = 800;
	mode->vdisplay = 480;
	mode->hsync_start = 808;
	mode->hsync_end = 848;
	mode->htotal = 880;
	mode->vsync_start = 482;
	mode->vsync_end = 483;
	mode->vtotal = 486;
	mode->clock = 33264;
#endif  /*FIXME jliu7 remove it later */

#if 0 /*FIXME jliu7 remove it later */
	    /* hard coded fixed mode for DSI TPO TD043MTEA2 LCD panel */
	mode->hdisplay = 800;
	mode->vdisplay = 480;
	mode->hsync_start = 836;
	mode->hsync_end = 846;
	mode->htotal = 1056;
	mode->vsync_start = 489;
	mode->vsync_end = 491;
	mode->vtotal = 525;
	mode->clock = 33264;
#endif  /*FIXME jliu7 remove it later */

#if 0 /*FIXME jliu7 remove it later */
	    /* hard coded fixed mode for LVDS 800x480 */
	mode->hdisplay = 800;
	mode->vdisplay = 480;
	mode->hsync_start = 801;
	mode->hsync_end = 802;
	mode->htotal = 1024;
	mode->vsync_start = 481;
	mode->vsync_end = 482;
	mode->vtotal = 525;
	mode->clock = 30994;
#endif  /*FIXME jliu7 remove it later */

#if 0				/*FIXME jliu7 remove it later, jliu7 modify it according to the spec */
	/* hard coded fixed mode for Samsung 480wsvga LVDS 1024x600@75 */
	mode->hdisplay = 1024;
	mode->vdisplay = 600;
	mode->hsync_start = 1072;
	mode->hsync_end = 1104;
	mode->htotal = 1184;
	mode->vsync_start = 603;
	mode->vsync_end = 604;
	mode->vtotal = 608;
	mode->clock = 53990;
#endif				/*FIXME jliu7 remove it later */

#if 0				/*FIXME jliu7 remove it, it is copied from SBIOS */
	/* hard coded fixed mode for Samsung 480wsvga LVDS 1024x600@75 */
	mode->hdisplay = 1024;
	mode->vdisplay = 600;
	mode->hsync_start = 1104;
	mode->hsync_end = 1136;
	mode->htotal = 1184;
	mode->vsync_start = 603;
	mode->vsync_end = 604;
	mode->vtotal = 608;
	mode->clock = 53990;
#endif				/*FIXME jliu7 remove it later */

#if 0				/*FIXME jliu7 remove it later */
	/* hard coded fixed mode for Sharp wsvga LVDS 1024x600 */
	mode->hdisplay = 1024;
	mode->vdisplay = 600;
	mode->hsync_start = 1124;
	mode->hsync_end = 1204;
	mode->htotal = 1312;
	mode->vsync_start = 607;
	mode->vsync_end = 610;
	mode->vtotal = 621;
	mode->clock = 48885;
#endif				/*FIXME jliu7 remove it later */

#if 0				/*FIXME jliu7 remove it later */
	/* hard coded fixed mode for LVDS 1024x768 */
	mode->hdisplay = 1024;
	mode->vdisplay = 768;
	mode->hsync_start = 1048;
	mode->hsync_end = 1184;
	mode->htotal = 1344;
	mode->vsync_start = 771;
	mode->vsync_end = 777;
	mode->vtotal = 806;
	mode->clock = 65000;
#endif				/*FIXME jliu7 remove it later */

#if 0				/*FIXME jliu7 remove it later */
	/* hard coded fixed mode for LVDS 1366x768 */
	mode->hdisplay = 1366;
	mode->vdisplay = 768;
	mode->hsync_start = 1430;
	mode->hsync_end = 1558;
	mode->htotal = 1664;
	mode->vsync_start = 769;
	mode->vsync_end = 770;
	mode->vtotal = 776;
	mode->clock = 77500;
#endif				/*FIXME jliu7 remove it later */

	drm_mode_set_name(mode);
	drm_mode_set_crtcinfo(mode, 0);

	return mode;
}

/* ************************************************************************* *\
FUNCTION: mrstDSI_clockInit
																`
DESCRIPTION:

\* ************************************************************************* */
static u32 sku_83_mipi_2xclk[4] = {166667, 333333, 444444, 666667};
static u32 sku_100_mipi_2xclk[4] = {200000, 400000, 533333, 800000};
static u32 sku_100L_mipi_2xclk[4] = {100000, 200000, 266667, 400000};
#define MIPI_2XCLK_COUNT			0x04

static bool mrstDSI_clockInit(DRM_DRIVER_PRIVATE_T *dev_priv)
{
	u32 Htotal = 0, Vtotal = 0, RRate = 0, mipi_2xclk = 0;
	u32 i = 0;
	u32 *p_mipi_2xclk = NULL;

        (void)GetHS_TX_timeoutCount;
        (void)GetLP_RX_timeoutCount;
        (void)GetHSA_Count;
        (void)GetHBP_Count;
        (void)GetHFP_Count;
        (void)GetHAdr_Count;
        (void)GetHighLowSwitchCount;
        (void)mrst_init_HIMAX_MIPI_bridge;

#if 0 /* JLIU7_PO old values */
	/* FIXME jliu7 DPI hard coded for TPO TD043MTEA2 LCD panel */
    dev_priv->pixelClock = 33264; /*KHz*/
    dev_priv->HsyncWidth = 10;
    dev_priv->HbackPorch = 210;
    dev_priv->HfrontPorch = 36;
    dev_priv->HactiveArea = 800;
    dev_priv->VsyncWidth = 2;
    dev_priv->VbackPorch = 34;
    dev_priv->VfrontPorch = 9;
    dev_priv->VactiveArea = 480;
    dev_priv->bpp = 24;

	/* FIXME jliu7 DBI hard coded for TPO TD043MTEA2 LCD panel */
    dev_priv->dbi_pixelClock = 33264; /*KHz*/
    dev_priv->dbi_HsyncWidth = 10;
    dev_priv->dbi_HbackPorch = 210;
    dev_priv->dbi_HfrontPorch = 36;
    dev_priv->dbi_HactiveArea = 800;
    dev_priv->dbi_VsyncWidth = 2;
    dev_priv->dbi_VbackPorch = 34;
    dev_priv->dbi_VfrontPorch = 9;
    dev_priv->dbi_VactiveArea = 480;
    dev_priv->dbi_bpp = 24;
#else /* JLIU7_PO old values */
    /* FIXME jliu7 DPI hard coded for TPO TD043MTEA2 LCD panel */
    /* FIXME Pre-Si value, 1 or 2 lanes; 50MHz; Non-Burst w/ sync event */
    dev_priv->pixelClock = 33264; /*KHz*/
    dev_priv->HsyncWidth = 10;
    dev_priv->HbackPorch = 8;
    dev_priv->HfrontPorch = 3;
    dev_priv->HactiveArea = 800;
    dev_priv->VsyncWidth = 2;
    dev_priv->VbackPorch = 3;
    dev_priv->VfrontPorch = 2;
    dev_priv->VactiveArea = 480;
    dev_priv->bpp = 24;

	/* FIXME jliu7 DBI hard coded for TPO TD043MTEA2 LCD panel */
    dev_priv->dbi_pixelClock = 33264; /*KHz*/
    dev_priv->dbi_HsyncWidth = 10;
    dev_priv->dbi_HbackPorch = 8;
    dev_priv->dbi_HfrontPorch = 3;
    dev_priv->dbi_HactiveArea = 800;
    dev_priv->dbi_VsyncWidth = 2;
    dev_priv->dbi_VbackPorch = 3;
    dev_priv->dbi_VfrontPorch = 2;
    dev_priv->dbi_VactiveArea = 480;
    dev_priv->dbi_bpp = 24;
#endif  /* JLIU7_PO old values */

	Htotal = dev_priv->HsyncWidth + dev_priv->HbackPorch + dev_priv->HfrontPorch + dev_priv->HactiveArea;
	Vtotal = dev_priv->VsyncWidth + dev_priv->VbackPorch + dev_priv->VfrontPorch + dev_priv->VactiveArea;

	RRate = ((dev_priv->pixelClock * 1000) / (Htotal * Vtotal)) + 1;

	dev_priv->RRate = RRate;

	/* ddr clock frequence = (pixel clock frequence *  bits per pixel)/2*/
	mipi_2xclk = (dev_priv->pixelClock * dev_priv->bpp) / dev_priv->laneCount; /* KHz */
    dev_priv->DDR_Clock_Calculated = mipi_2xclk / 2; /* KHz */

	DRM_DEBUG("mrstDSI_clockInit RRate = %d, mipi_2xclk = %d. \n", RRate, mipi_2xclk);

	if (dev_priv->sku_100)
	{
		p_mipi_2xclk = sku_100_mipi_2xclk;
	}
	else if (dev_priv->sku_100L)
	{
		p_mipi_2xclk = sku_100L_mipi_2xclk;
	}
	else
	{
		p_mipi_2xclk = sku_83_mipi_2xclk;
	}

	for (; i < MIPI_2XCLK_COUNT; i++)
	{
		if ((dev_priv->DDR_Clock_Calculated * 2) < p_mipi_2xclk[i])
			break;
	}

	if (i == MIPI_2XCLK_COUNT)
	{
		DRM_DEBUG("mrstDSI_clockInit the DDR clock is too big, DDR_Clock_Calculated is = %d\n", dev_priv->DDR_Clock_Calculated);
		return false;
	}

	dev_priv->DDR_Clock = p_mipi_2xclk[i] / 2;
	dev_priv->ClockBits = i;

#if 0 /*JLIU7_PO */
#if 0 /* FIXME remove it after power on*/
	mipiControlReg = REG_READ(MIPI_CONTROL_REG) & (~MIPI_2X_CLOCK_BITS);
	mipiControlReg |= i;
    REG_WRITE(MIPI_CONTROL_REG, mipiControlReg);
#else  /* FIXME remove it after power on*/
	mipiControlReg |= i;
    REG_WRITE(MIPI_CONTROL_REG, mipiControlReg);
#endif  /* FIXME remove it after power on*/
#endif /*JLIU7_PO */

#if 1 /* FIXME remove it after power on*/
	DRM_DEBUG("mrstDSI_clockInit, mipi_2x_clock_divider = 0x%x, DDR_Clock_Calculated is = %d\n", i, dev_priv->DDR_Clock_Calculated);
#endif  /* FIXME remove it after power on*/

	return true;
}

/**
 * mrst_dsi_init - setup MIPI connectors on this device
 * @dev: drm device
 *
 * Create the connector, try to figure out what
 * modes we can display on the MIPI panel (if present).
 */
void mrst_dsi_init(struct drm_device *dev,
		    struct intel_mode_device *mode_dev)
{
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	struct intel_output *intel_output;
	struct drm_connector *connector;
	struct drm_encoder *encoder;

#if PRINT_JLIU7
     DRM_INFO("JLIU7 enter mrst_dsi_init \n");
#endif /* PRINT_JLIU7 */

	intel_output = kzalloc(sizeof(struct intel_output), GFP_KERNEL);
	if (!intel_output)
		return;

	intel_output->mode_dev = mode_dev;
	connector = &intel_output->base;
	encoder = &intel_output->enc;
	drm_connector_init(dev, &intel_output->base,
			   &mrst_dsi_connector_funcs,
			   DRM_MODE_CONNECTOR_MIPI);

	drm_encoder_init(dev, &intel_output->enc, &intel_lvds_enc_funcs,
			 DRM_MODE_ENCODER_MIPI);

	drm_mode_connector_attach_encoder(&intel_output->base,
					  &intel_output->enc);
	intel_output->type = INTEL_OUTPUT_MIPI;

	drm_encoder_helper_add(encoder, &mrst_dsi_helper_funcs);
	drm_connector_helper_add(connector,
				 &mrst_dsi_connector_helper_funcs);
	connector->display_info.subpixel_order = SubPixelHorizontalRGB;
	connector->interlace_allowed = false;
	connector->doublescan_allowed = false;

	dsi_backlight = BRIGHTNESS_MAX_LEVEL;
	blc_pol = BLC_POLARITY_INVERSE;
	blc_freq = 0xc8;

	/*
	 * MIPI	discovery:
	 * 1) check for DDB data
	 * 2) check for VBT data
	 * 4) make sure lid is open
	 *    if closed, act like it's not there for now
	 */

	/* FIXME jliu7 we only support DPI */
    dev_priv->dpi = true;

	/* FIXME hard coded 4 lanes for Himax HX8858-A, 2 lanes for NSC LM2550 */
    dev_priv->laneCount = 2;

	/* FIXME hard coded for NSC PO. */
	/* We only support BUST_MODE */
    dev_priv->videoModeFormat =  NON_BURST_MODE_SYNC_EVENTS; /*	BURST_MODE */
	/* FIXME change it to true if GET_DDB works */
    dev_priv->config_phase = false;

	if (!mrstDSI_clockInit(dev_priv))
	{
		DRM_DEBUG("Can't iniitialize MRST DSI clock.\n");
#if 0 /* FIXME JLIU7 */
		goto failed_find;
#endif /* FIXME JLIU7 */
	}

	/*
	 * If we didn't get DDB data, try geting panel timing
	 * from configuration data
	 */
	mode_dev->panel_fixed_mode = mrst_dsi_get_configuration_mode(dev);

	if (mode_dev->panel_fixed_mode) {
		mode_dev->panel_fixed_mode->type |=
		    DRM_MODE_TYPE_PREFERRED;
		goto out;	/* FIXME: check for quirks */
	}

	/* If we still don't have a mode after all that, give up. */
	if (!mode_dev->panel_fixed_mode) {
		DRM_DEBUG
		    ("Found no modes on the lvds, ignoring the LVDS\n");
		goto failed_find;
	}

out:
	drm_sysfs_connector_add(connector);
	return;

failed_find:
	DRM_DEBUG("No MIIP modes found, disabling.\n");
	drm_encoder_cleanup(encoder);
	drm_connector_cleanup(connector);
	kfree(connector);
}
