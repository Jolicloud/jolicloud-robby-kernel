/*
 * Copyright © 2006-2007 Intel Corporation
 * Copyright (c) 2006 Dave Airlie <airlied@linux.ie>
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
 *      Dave Airlie <airlied@linux.ie>
 *      Jesse Barnes <jesse.barnes@intel.com>
 */

#include <linux/i2c.h>
#include <drm/drm_crtc.h>
#include <drm/drm_edid.h>
/* MRST defines start */
uint8_t blc_type;
uint8_t blc_pol;
uint8_t blc_freq;
uint8_t blc_minbrightness;
uint8_t blc_i2caddr;
uint8_t blc_brightnesscmd;
int lvds_backlight;		/* restore backlight to this value */

u32 CoreClock;
u32 PWMControlRegFreq;
/* MRST defines end */

/**
 * Sets the backlight level.
 *
 * \param level backlight level, from 0 to intel_lvds_get_max_backlight().
 */
static void intel_lvds_set_backlight(struct drm_device *dev, int level)
{
	u32 blc_pwm_ctl;

	blc_pwm_ctl = REG_READ(BLC_PWM_CTL) & ~BACKLIGHT_DUTY_CYCLE_MASK;
	REG_WRITE(BLC_PWM_CTL, (blc_pwm_ctl |
				(level << BACKLIGHT_DUTY_CYCLE_SHIFT)));
}

/**
 * Returns the maximum level of the backlight duty cycle field.
 */
static u32 intel_lvds_get_max_backlight(struct drm_device *dev)
{
	return ((REG_READ(BLC_PWM_CTL) & BACKLIGHT_MODULATION_FREQ_MASK) >>
		BACKLIGHT_MODULATION_FREQ_SHIFT) * 2;
}

/**
 * Sets the power state for the panel.
 */
static void intel_lvds_set_power(struct drm_device *dev,
				 struct intel_output *output, bool on)
{
	u32 pp_status;

	if (on) {
		REG_WRITE(PP_CONTROL, REG_READ(PP_CONTROL) |
			  POWER_TARGET_ON);
		do {
			pp_status = REG_READ(PP_STATUS);
		} while ((pp_status & PP_ON) == 0);

		intel_lvds_set_backlight(dev,
					 output->
					 mode_dev->backlight_duty_cycle);
	} else {
		intel_lvds_set_backlight(dev, 0);

		REG_WRITE(PP_CONTROL, REG_READ(PP_CONTROL) &
			  ~POWER_TARGET_ON);
		do {
			pp_status = REG_READ(PP_STATUS);
		} while (pp_status & PP_ON);
	}
}

static void intel_lvds_dpms(struct drm_encoder *encoder, int mode)
{
	struct drm_device *dev = encoder->dev;
	struct intel_output *output = enc_to_intel_output(encoder);

	if (mode == DRM_MODE_DPMS_ON)
		intel_lvds_set_power(dev, output, true);
	else
		intel_lvds_set_power(dev, output, false);

	/* XXX: We never power down the LVDS pairs. */
}

static void intel_lvds_save(struct drm_connector *connector)
{
#if 0				/* JB: Disable for drop */
	struct drm_device *dev = connector->dev;

	dev_priv->savePP_ON = REG_READ(PP_ON_DELAYS);
	dev_priv->savePP_OFF = REG_READ(PP_OFF_DELAYS);
	dev_priv->savePP_CONTROL = REG_READ(PP_CONTROL);
	dev_priv->savePP_DIVISOR = REG_READ(PP_DIVISOR);
	dev_priv->saveBLC_PWM_CTL = REG_READ(BLC_PWM_CTL);
	dev_priv->backlight_duty_cycle = (dev_priv->saveBLC_PWM_CTL &
					  BACKLIGHT_DUTY_CYCLE_MASK);

	/*
	 * If the light is off at server startup, just make it full brightness
	 */
	if (dev_priv->backlight_duty_cycle == 0)
		dev_priv->backlight_duty_cycle =
		    intel_lvds_get_max_backlight(dev);
#endif
}

static void intel_lvds_restore(struct drm_connector *connector)
{
#if 0				/* JB: Disable for drop */
	struct drm_device *dev = connector->dev;

	REG_WRITE(BLC_PWM_CTL, dev_priv->saveBLC_PWM_CTL);
	REG_WRITE(PP_ON_DELAYS, dev_priv->savePP_ON);
	REG_WRITE(PP_OFF_DELAYS, dev_priv->savePP_OFF);
	REG_WRITE(PP_DIVISOR, dev_priv->savePP_DIVISOR);
	REG_WRITE(PP_CONTROL, dev_priv->savePP_CONTROL);
	if (dev_priv->savePP_CONTROL & POWER_TARGET_ON)
		intel_lvds_set_power(dev, true);
	else
		intel_lvds_set_power(dev, false);
#endif
}

static int intel_lvds_mode_valid(struct drm_connector *connector,
				 struct drm_display_mode *mode)
{
	struct intel_output *intel_output = to_intel_output(connector);
	struct drm_display_mode *fixed_mode =
	    intel_output->mode_dev->panel_fixed_mode;

#if PRINT_JLIU7
	DRM_INFO("JLIU7 enter  intel_lvds_mode_valid \n");
#endif				/* PRINT_JLIU7 */

	if (fixed_mode) {
		if (mode->hdisplay > fixed_mode->hdisplay)
			return MODE_PANEL;
		if (mode->vdisplay > fixed_mode->vdisplay)
			return MODE_PANEL;
	}
	return MODE_OK;
}

static bool intel_lvds_mode_fixup(struct drm_encoder *encoder,
				  struct drm_display_mode *mode,
				  struct drm_display_mode *adjusted_mode)
{
	struct intel_mode_device *mode_dev =
	    enc_to_intel_output(encoder)->mode_dev;
	struct drm_device *dev = encoder->dev;
	struct intel_crtc *intel_crtc = to_intel_crtc(encoder->crtc);
	struct drm_encoder *tmp_encoder;

#if PRINT_JLIU7
	DRM_INFO("JLIU7 enter  intel_lvds_mode_fixup \n");
#endif				/* PRINT_JLIU7 */

	/* Should never happen!! */
	if (IS_MRST(dev) && intel_crtc->pipe != 0) {
		printk(KERN_ERR
		       "Can't support LVDS/MIPI on pipe B on MRST\n");
		return false;
	} else if (!IS_MRST(dev) && !IS_I965G(dev)
		   && intel_crtc->pipe == 0) {
		printk(KERN_ERR "Can't support LVDS on pipe A\n");
		return false;
	}
	/* Should never happen!! */
	list_for_each_entry(tmp_encoder, &dev->mode_config.encoder_list,
			    head) {
		if (tmp_encoder != encoder
		    && tmp_encoder->crtc == encoder->crtc) {
			printk(KERN_ERR "Can't enable LVDS and another "
			       "encoder on the same pipe\n");
			return false;
		}
	}

	/*
	 * If we have timings from the BIOS for the panel, put them in
	 * to the adjusted mode.  The CRTC will be set up for this mode,
	 * with the panel scaling set up to source from the H/VDisplay
	 * of the original mode.
	 */
	if (mode_dev->panel_fixed_mode != NULL) {
		adjusted_mode->hdisplay =
		    mode_dev->panel_fixed_mode->hdisplay;
		adjusted_mode->hsync_start =
		    mode_dev->panel_fixed_mode->hsync_start;
		adjusted_mode->hsync_end =
		    mode_dev->panel_fixed_mode->hsync_end;
		adjusted_mode->htotal = mode_dev->panel_fixed_mode->htotal;
		adjusted_mode->vdisplay =
		    mode_dev->panel_fixed_mode->vdisplay;
		adjusted_mode->vsync_start =
		    mode_dev->panel_fixed_mode->vsync_start;
		adjusted_mode->vsync_end =
		    mode_dev->panel_fixed_mode->vsync_end;
		adjusted_mode->vtotal = mode_dev->panel_fixed_mode->vtotal;
		adjusted_mode->clock = mode_dev->panel_fixed_mode->clock;
		drm_mode_set_crtcinfo(adjusted_mode,
				      CRTC_INTERLACE_HALVE_V);
	}

	/*
	 * XXX: It would be nice to support lower refresh rates on the
	 * panels to reduce power consumption, and perhaps match the
	 * user's requested refresh rate.
	 */

	return true;
}

static void intel_lvds_prepare(struct drm_encoder *encoder)
{
	struct drm_device *dev = encoder->dev;
	struct intel_output *output = enc_to_intel_output(encoder);
	struct intel_mode_device *mode_dev = output->mode_dev;

#if PRINT_JLIU7
	DRM_INFO("JLIU7 enter  intel_lvds_prepare \n");
#endif				/* PRINT_JLIU7 */

	mode_dev->saveBLC_PWM_CTL = REG_READ(BLC_PWM_CTL);
	mode_dev->backlight_duty_cycle = (mode_dev->saveBLC_PWM_CTL &
					  BACKLIGHT_DUTY_CYCLE_MASK);

	intel_lvds_set_power(dev, output, false);
}

static void intel_lvds_commit(struct drm_encoder *encoder)
{
	struct drm_device *dev = encoder->dev;
	struct intel_output *output = enc_to_intel_output(encoder);
	struct intel_mode_device *mode_dev = output->mode_dev;

#if PRINT_JLIU7
	DRM_INFO("JLIU7 enter  intel_lvds_commit \n");
#endif				/* PRINT_JLIU7 */

	if (mode_dev->backlight_duty_cycle == 0)
		mode_dev->backlight_duty_cycle =
		    intel_lvds_get_max_backlight(dev);

	intel_lvds_set_power(dev, output, true);
}

static void intel_lvds_mode_set(struct drm_encoder *encoder,
				struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode)
{
	struct intel_mode_device *mode_dev =
	    enc_to_intel_output(encoder)->mode_dev;
	struct drm_device *dev = encoder->dev;
	struct intel_crtc *intel_crtc = to_intel_crtc(encoder->crtc);
	u32 pfit_control;

	/*
	 * The LVDS pin pair will already have been turned on in the
	 * intel_crtc_mode_set since it has a large impact on the DPLL
	 * settings.
	 */

	/*
	 * Enable automatic panel scaling so that non-native modes fill the
	 * screen.  Should be enabled before the pipe is enabled, according to
	 * register description and PRM.
	 */
	if (mode->hdisplay != adjusted_mode->hdisplay ||
	    mode->vdisplay != adjusted_mode->vdisplay)
		pfit_control = (PFIT_ENABLE | VERT_AUTO_SCALE |
				HORIZ_AUTO_SCALE | VERT_INTERP_BILINEAR |
				HORIZ_INTERP_BILINEAR);
	else
		pfit_control = 0;

	if (!IS_I965G(dev)) {
		if (mode_dev->panel_wants_dither)
			pfit_control |= PANEL_8TO6_DITHER_ENABLE;
	} else
		pfit_control |= intel_crtc->pipe << PFIT_PIPE_SHIFT;

	REG_WRITE(PFIT_CONTROL, pfit_control);
}

/**
 * Detect the LVDS connection.
 *
 * This always returns CONNECTOR_STATUS_CONNECTED.
 * This connector should only have
 * been set up if the LVDS was actually connected anyway.
 */
static enum drm_connector_status intel_lvds_detect(struct drm_connector
						   *connector)
{
	return connector_status_connected;
}

/**
 * Return the list of DDC modes if available, or the BIOS fixed mode otherwise.
 */
static int intel_lvds_get_modes(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct intel_output *intel_output = to_intel_output(connector);
	struct intel_mode_device *mode_dev = intel_output->mode_dev;
	int ret = 0;

	if (!IS_MRST(dev))
		ret = intel_ddc_get_modes(intel_output);

	if (ret)
		return ret;

	/* Didn't get an EDID, so
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

/**
 * intel_lvds_destroy - unregister and free LVDS structures
 * @connector: connector to free
 *
 * Unregister the DDC bus for this connector then free the driver private
 * structure.
 */
static void intel_lvds_destroy(struct drm_connector *connector)
{
	struct intel_output *intel_output = to_intel_output(connector);

	if (intel_output->ddc_bus)
		intel_i2c_destroy(intel_output->ddc_bus);
	drm_sysfs_connector_remove(connector);
	drm_connector_cleanup(connector);
	kfree(connector);
}

static const struct drm_encoder_helper_funcs intel_lvds_helper_funcs = {
	.dpms = intel_lvds_dpms,
	.mode_fixup = intel_lvds_mode_fixup,
	.prepare = intel_lvds_prepare,
	.mode_set = intel_lvds_mode_set,
	.commit = intel_lvds_commit,
};

static const struct drm_connector_helper_funcs
    intel_lvds_connector_helper_funcs = {
	.get_modes = intel_lvds_get_modes,
	.mode_valid = intel_lvds_mode_valid,
	.best_encoder = intel_best_encoder,
};

static const struct drm_connector_funcs intel_lvds_connector_funcs = {
	.save = intel_lvds_save,
	.restore = intel_lvds_restore,
	.detect = intel_lvds_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = intel_lvds_destroy,
};


static void intel_lvds_enc_destroy(struct drm_encoder *encoder)
{
	drm_encoder_cleanup(encoder);
}

static const struct drm_encoder_funcs intel_lvds_enc_funcs = {
	.destroy = intel_lvds_enc_destroy,
};



/**
 * intel_lvds_init - setup LVDS connectors on this device
 * @dev: drm device
 *
 * Create the connector, register the LVDS DDC bus, and try to figure out what
 * modes we can display on the LVDS panel (if present).
 */
void intel_lvds_init(struct drm_device *dev,
		     struct intel_mode_device *mode_dev)
{
	struct intel_output *intel_output;
	struct drm_connector *connector;
	struct drm_encoder *encoder;
	struct drm_display_mode *scan;	/* *modes, *bios_mode; */
	struct drm_crtc *crtc;
	u32 lvds;
	int pipe;

	intel_output = kzalloc(sizeof(struct intel_output), GFP_KERNEL);
	if (!intel_output)
		return;

	intel_output->mode_dev = mode_dev;
	connector = &intel_output->base;
	encoder = &intel_output->enc;
	drm_connector_init(dev, &intel_output->base,
			   &intel_lvds_connector_funcs,
			   DRM_MODE_CONNECTOR_LVDS);

	drm_encoder_init(dev, &intel_output->enc, &intel_lvds_enc_funcs,
			 DRM_MODE_ENCODER_LVDS);

	drm_mode_connector_attach_encoder(&intel_output->base,
					  &intel_output->enc);
	intel_output->type = INTEL_OUTPUT_LVDS;

	drm_encoder_helper_add(encoder, &intel_lvds_helper_funcs);
	drm_connector_helper_add(connector,
				 &intel_lvds_connector_helper_funcs);
	connector->display_info.subpixel_order = SubPixelHorizontalRGB;
	connector->interlace_allowed = false;
	connector->doublescan_allowed = false;


	/*
	 * LVDS discovery:
	 * 1) check for EDID on DDC
	 * 2) check for VBT data
	 * 3) check to see if LVDS is already on
	 *    if none of the above, no panel
	 * 4) make sure lid is open
	 *    if closed, act like it's not there for now
	 */

	/* Set up the DDC bus. */
	intel_output->ddc_bus = intel_i2c_create(dev, GPIOC, "LVDSDDC_C");
	if (!intel_output->ddc_bus) {
		dev_printk(KERN_ERR, &dev->pdev->dev,
			   "DDC bus registration " "failed.\n");
		goto failed_ddc;
	}

	/*
	 * Attempt to get the fixed panel mode from DDC.  Assume that the
	 * preferred mode is the right one.
	 */
	intel_ddc_get_modes(intel_output);
	list_for_each_entry(scan, &connector->probed_modes, head) {
		if (scan->type & DRM_MODE_TYPE_PREFERRED) {
			mode_dev->panel_fixed_mode =
			    drm_mode_duplicate(dev, scan);
			goto out;	/* FIXME: check for quirks */
		}
	}

	/* Failed to get EDID, what about VBT? */
	if (mode_dev->vbt_mode)
		mode_dev->panel_fixed_mode =
		    drm_mode_duplicate(dev, mode_dev->vbt_mode);

	/*
	 * If we didn't get EDID, try checking if the panel is already turned
	 * on.  If so, assume that whatever is currently programmed is the
	 * correct mode.
	 */
	lvds = REG_READ(LVDS);
	pipe = (lvds & LVDS_PIPEB_SELECT) ? 1 : 0;
	crtc = intel_get_crtc_from_pipe(dev, pipe);

	if (crtc && (lvds & LVDS_PORT_EN)) {
		mode_dev->panel_fixed_mode =
		    intel_crtc_mode_get(dev, crtc);
		if (mode_dev->panel_fixed_mode) {
			mode_dev->panel_fixed_mode->type |=
			    DRM_MODE_TYPE_PREFERRED;
			goto out;	/* FIXME: check for quirks */
		}
	}

	/* If we still don't have a mode after all that, give up. */
	if (!mode_dev->panel_fixed_mode) {
		DRM_DEBUG
		    ("Found no modes on the lvds, ignoring the LVDS\n");
		goto failed_find;
	}

	/* FIXME: detect aopen & mac mini type stuff automatically? */
	/*
	 * Blacklist machines with BIOSes that list an LVDS panel without
	 * actually having one.
	 */
	if (IS_I945GM(dev)) {
		/* aopen mini pc */
		if (dev->pdev->subsystem_vendor == 0xa0a0) {
			DRM_DEBUG
			    ("Suspected AOpen Mini PC, ignoring the LVDS\n");
			goto failed_find;
		}

		if ((dev->pdev->subsystem_vendor == 0x8086) &&
		    (dev->pdev->subsystem_device == 0x7270)) {
			/* It's a Mac Mini or Macbook Pro.
			 *
			 * Apple hardware is out to get us.  The macbook pro
			 * has a real LVDS panel, but the mac mini does not,
			 * and they have the same device IDs.  We'll
			 * distinguish by panel size, on the assumption
			 * that Apple isn't about to make any machines with an
			 * 800x600 display.
			 */

			if (mode_dev->panel_fixed_mode != NULL &&
			    mode_dev->panel_fixed_mode->hdisplay == 800 &&
			    mode_dev->panel_fixed_mode->vdisplay == 600) {
				DRM_DEBUG
				    ("Suspected Mac Mini, ignoring the LVDS\n");
				goto failed_find;
			}
		}
	}

out:
	drm_sysfs_connector_add(connector);

#if PRINT_JLIU7
	DRM_INFO("PRINT_JLIU7 hdisplay = %d\n",
		 mode_dev->panel_fixed_mode->hdisplay);
	DRM_INFO("PRINT_JLIU7 vdisplay = %d\n",
		 mode_dev->panel_fixed_mode->vdisplay);
	DRM_INFO("PRINT_JLIU7 hsync_start = %d\n",
		 mode_dev->panel_fixed_mode->hsync_start);
	DRM_INFO("PRINT_JLIU7 hsync_end = %d\n",
		 mode_dev->panel_fixed_mode->hsync_end);
	DRM_INFO("PRINT_JLIU7 htotal = %d\n",
		 mode_dev->panel_fixed_mode->htotal);
	DRM_INFO("PRINT_JLIU7 vsync_start = %d\n",
		 mode_dev->panel_fixed_mode->vsync_start);
	DRM_INFO("PRINT_JLIU7 vsync_end = %d\n",
		 mode_dev->panel_fixed_mode->vsync_end);
	DRM_INFO("PRINT_JLIU7 vtotal = %d\n",
		 mode_dev->panel_fixed_mode->vtotal);
	DRM_INFO("PRINT_JLIU7 clock = %d\n",
		 mode_dev->panel_fixed_mode->clock);
#endif				/* PRINT_JLIU7 */
	return;

failed_find:
	if (intel_output->ddc_bus)
		intel_i2c_destroy(intel_output->ddc_bus);
failed_ddc:
	drm_encoder_cleanup(encoder);
	drm_connector_cleanup(connector);
	kfree(connector);
}

/* MRST platform start */

/*
 * FIXME need to move to register define head file
 */
#define MRST_BACKLIGHT_MODULATION_FREQ_SHIFT		(16)
#define MRST_BACKLIGHT_MODULATION_FREQ_MASK		(0xffff << 16)

/* The max/min PWM frequency in BPCR[31:17] - */
/* The smallest number is 1 (not 0) that can fit in the
 * 15-bit field of the and then*/
/* shifts to the left by one bit to get the actual 16-bit
 * value that the 15-bits correspond to.*/
#define MRST_BLC_MAX_PWM_REG_FREQ	    0xFFFF

#define BRIGHTNESS_MAX_LEVEL 100
#define BLC_PWM_PRECISION_FACTOR 10	/* 10000000 */
#define BLC_PWM_FREQ_CALC_CONSTANT 32
#define MHz 1000000
#define BLC_POLARITY_NORMAL 0
#define BLC_POLARITY_INVERSE 1

/**
 * Calculate PWM control register value.
 */
static bool mrstLVDSCalculatePWMCtrlRegFreq(struct drm_device *dev)
{
	unsigned long value = 0;
	if (blc_freq == 0) {
		/* DRM_ERROR(KERN_ERR "mrstLVDSCalculatePWMCtrlRegFreq:
		 * Frequency Requested is 0.\n"); */
		return false;
	}

	value = (CoreClock * MHz);
	value = (value / BLC_PWM_FREQ_CALC_CONSTANT);
	value = (value * BLC_PWM_PRECISION_FACTOR);
	value = (value / blc_freq);
	value = (value / BLC_PWM_PRECISION_FACTOR);

	if (value > (unsigned long) MRST_BLC_MAX_PWM_REG_FREQ) {
		return 0;
	} else {
		PWMControlRegFreq = (u32) value;
		return 1;
	}
}

/**
 * Returns the maximum level of the backlight duty cycle field.
 */
static u32 mrst_lvds_get_PWM_ctrl_freq(struct drm_device *dev)
{
	u32 max_pwm_blc = 0;

#if PRINT_JLIU7
	DRM_INFO("JLIU7 enter mrst_lvds_get_PWM_ctrl_freq \n");
#endif				/* PRINT_JLIU7 */

/*FIXME JLIU7 get the PWM frequency from configuration */

	max_pwm_blc =
	    (REG_READ(BLC_PWM_CTL) & MRST_BACKLIGHT_MODULATION_FREQ_MASK)
	    >> MRST_BACKLIGHT_MODULATION_FREQ_SHIFT;


	if (!max_pwm_blc) {
		if (mrstLVDSCalculatePWMCtrlRegFreq(dev))
			max_pwm_blc = PWMControlRegFreq;
	}

	return max_pwm_blc;
}

/**
 * Sets the backlight level.
 *
 * \param level backlight level, from 0 to intel_lvds_get_max_backlight().
 */
static void mrst_lvds_set_backlight(struct drm_device *dev, int level)
{
	u32 blc_pwm_ctl;
	u32 max_pwm_blc;
#if PRINT_JLIU7
	DRM_INFO("JLIU7 enter mrst_lvds_set_backlight \n");
#endif				/* PRINT_JLIU7 */

#if 1				/* FIXME JLIU7 */
	return;
#endif				/* FIXME JLIU7 */

	/* Provent LVDS going to total black */
	if (level < 20)
		level = 20;

	max_pwm_blc = mrst_lvds_get_PWM_ctrl_freq(dev);

	if (max_pwm_blc == 0)
		return;

	blc_pwm_ctl = level * max_pwm_blc / BRIGHTNESS_MAX_LEVEL;

	if (blc_pol == BLC_POLARITY_INVERSE)
		blc_pwm_ctl = max_pwm_blc - blc_pwm_ctl;

	REG_WRITE(BLC_PWM_CTL,
		  (max_pwm_blc << MRST_BACKLIGHT_MODULATION_FREQ_SHIFT) |
		  blc_pwm_ctl);
}

/**
 * Sets the power state for the panel.
 */
static void mrst_lvds_set_power(struct drm_device *dev,
				struct intel_output *output, bool on)
{
	u32 pp_status;

#if PRINT_JLIU7
	DRM_INFO("JLIU7 enter mrst_lvds_set_power \n");
#endif				/* PRINT_JLIU7 */

	if (on) {
		REG_WRITE(PP_CONTROL, REG_READ(PP_CONTROL) |
			  POWER_TARGET_ON);
		do {
			pp_status = REG_READ(PP_STATUS);
		} while ((pp_status & (PP_ON | PP_READY)) == PP_READY);

		mrst_lvds_set_backlight(dev, lvds_backlight);
	} else {
		mrst_lvds_set_backlight(dev, 0);

		REG_WRITE(PP_CONTROL, REG_READ(PP_CONTROL) &
			  ~POWER_TARGET_ON);
		do {
			pp_status = REG_READ(PP_STATUS);
		} while (pp_status & PP_ON);
	}
}

static void mrst_lvds_dpms(struct drm_encoder *encoder, int mode)
{
	struct drm_device *dev = encoder->dev;
	struct intel_output *output = enc_to_intel_output(encoder);

#if PRINT_JLIU7
	DRM_INFO("JLIU7 enter mrst_lvds_dpms \n");
#endif				/* PRINT_JLIU7 */

	if (mode == DRM_MODE_DPMS_ON)
		mrst_lvds_set_power(dev, output, true);
	else
		mrst_lvds_set_power(dev, output, false);

	/* XXX: We never power down the LVDS pairs. */
}

static void mrst_lvds_mode_set(struct drm_encoder *encoder,
			       struct drm_display_mode *mode,
			       struct drm_display_mode *adjusted_mode)
{
	struct intel_mode_device *mode_dev =
	    enc_to_intel_output(encoder)->mode_dev;
	struct drm_device *dev = encoder->dev;
	u32 pfit_control;
	u32 lvds_port;

#if PRINT_JLIU7
	DRM_INFO("JLIU7 enter  mrst_lvds_mode_set \n");
#endif				/* PRINT_JLIU7 */

	/*
	 * The LVDS pin pair will already have been turned on in the
	 * intel_crtc_mode_set since it has a large impact on the DPLL
	 * settings.
	 */
	/*FIXME JLIU7 Get panel power delay parameters from config data */
	REG_WRITE(0x61208, 0x25807d0);
	REG_WRITE(0x6120c, 0x1f407d0);
	REG_WRITE(0x61210, 0x270f04);

	lvds_port = (REG_READ(LVDS) & (~LVDS_PIPEB_SELECT)) | LVDS_PORT_EN;

	if (mode_dev->panel_wants_dither)
		lvds_port |= MRST_PANEL_8TO6_DITHER_ENABLE;

	REG_WRITE(LVDS, lvds_port);

	/*
	 * Enable automatic panel scaling so that non-native modes fill the
	 * screen.  Should be enabled before the pipe is enabled, according to
	 * register description and PRM.
	 */
	if (mode->hdisplay != adjusted_mode->hdisplay ||
	    mode->vdisplay != adjusted_mode->vdisplay)
		pfit_control = PFIT_ENABLE;
	else
		pfit_control = 0;

	REG_WRITE(PFIT_CONTROL, pfit_control);
}


static const struct drm_encoder_helper_funcs mrst_lvds_helper_funcs = {
	.dpms = mrst_lvds_dpms,
	.mode_fixup = intel_lvds_mode_fixup,
	.prepare = intel_lvds_prepare,
	.mode_set = mrst_lvds_mode_set,
	.commit = intel_lvds_commit,
};

/** Returns the panel fixed mode from configuration. */
/** FIXME JLIU7 need to revist it. */
struct drm_display_mode *mrst_lvds_get_configuration_mode(struct drm_device
							  *dev)
{
	struct drm_display_mode *mode;

	mode = kzalloc(sizeof(*mode), GFP_KERNEL);
	if (!mode)
		return NULL;

#if 0				/*FIXME jliu7 remove it later */
	/* hard coded fixed mode for TPO LTPS LPJ040K001A */
	mode->hdisplay = 800;
	mode->vdisplay = 480;
	mode->hsync_start = 836;
	mode->hsync_end = 846;
	mode->htotal = 1056;
	mode->vsync_start = 489;
	mode->vsync_end = 491;
	mode->vtotal = 525;
	mode->clock = 33264;
#endif				/*FIXME jliu7 remove it later */

#if 0				/*FIXME jliu7 remove it later */
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
#endif				/*FIXME jliu7 remove it later */

#if 1				/*FIXME jliu7 remove it later, jliu7 modify it according to the spec */
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

/**
 * mrst_lvds_init - setup LVDS connectors on this device
 * @dev: drm device
 *
 * Create the connector, register the LVDS DDC bus, and try to figure out what
 * modes we can display on the LVDS panel (if present).
 */
void mrst_lvds_init(struct drm_device *dev,
		    struct intel_mode_device *mode_dev)
{
	struct intel_output *intel_output;
	struct drm_connector *connector;
	struct drm_encoder *encoder;
#if MRST_I2C
	struct drm_display_mode *scan;	/* *modes, *bios_mode; */
#endif
#if PRINT_JLIU7
	DRM_INFO("JLIU7 enter mrst_lvds_init \n");
#endif				/* PRINT_JLIU7 */

	intel_output = kzalloc(sizeof(struct intel_output), GFP_KERNEL);
	if (!intel_output)
		return;

	intel_output->mode_dev = mode_dev;
	connector = &intel_output->base;
	encoder = &intel_output->enc;
	drm_connector_init(dev, &intel_output->base,
			   &intel_lvds_connector_funcs,
			   DRM_MODE_CONNECTOR_LVDS);

	drm_encoder_init(dev, &intel_output->enc, &intel_lvds_enc_funcs,
			 DRM_MODE_ENCODER_LVDS);

	drm_mode_connector_attach_encoder(&intel_output->base,
					  &intel_output->enc);
	intel_output->type = INTEL_OUTPUT_LVDS;

	drm_encoder_helper_add(encoder, &mrst_lvds_helper_funcs);
	drm_connector_helper_add(connector,
				 &intel_lvds_connector_helper_funcs);
	connector->display_info.subpixel_order = SubPixelHorizontalRGB;
	connector->interlace_allowed = false;
	connector->doublescan_allowed = false;

	lvds_backlight = BRIGHTNESS_MAX_LEVEL;

	/*
	 * LVDS discovery:
	 * 1) check for EDID on DDC
	 * 2) check for VBT data
	 * 3) check to see if LVDS is already on
	 *    if none of the above, no panel
	 * 4) make sure lid is open
	 *    if closed, act like it's not there for now
	 */

#if MRST_I2C
	/* Set up the DDC bus. */
	intel_output->ddc_bus = intel_i2c_create(dev, GPIOC, "LVDSDDC_C");
	if (!intel_output->ddc_bus) {
		dev_printk(KERN_ERR, &dev->pdev->dev,
			   "DDC bus registration " "failed.\n");
		goto failed_ddc;
	}

	/*
	 * Attempt to get the fixed panel mode from DDC.  Assume that the
	 * preferred mode is the right one.
	 */
	intel_ddc_get_modes(intel_output);
	list_for_each_entry(scan, &connector->probed_modes, head) {
		if (scan->type & DRM_MODE_TYPE_PREFERRED) {
			mode_dev->panel_fixed_mode =
			    drm_mode_duplicate(dev, scan);
			goto out;	/* FIXME: check for quirks */
		}
	}
#endif				/* MRST_I2C */

	/*
	 * If we didn't get EDID, try geting panel timing
	 * from configuration data
	 */
	mode_dev->panel_fixed_mode = mrst_lvds_get_configuration_mode(dev);

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
	DRM_DEBUG("No LVDS modes found, disabling.\n");
	if (intel_output->ddc_bus)
		intel_i2c_destroy(intel_output->ddc_bus);
#if MRST_I2C
failed_ddc:
#endif
	drm_encoder_cleanup(encoder);
	drm_connector_cleanup(connector);
	kfree(connector);
}

/* MRST platform end */
