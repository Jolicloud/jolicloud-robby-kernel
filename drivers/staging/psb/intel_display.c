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

#include <linux/i2c.h>

#include <drm/drm_crtc_helper.h>
#include "psb_fb.h"
#include "intel_display.h"


struct intel_clock_t {
	/* given values */
	int n;
	int m1, m2;
	int p1, p2;
	/* derived values */
	int dot;
	int vco;
	int m;
	int p;
};

struct intel_range_t {
	int min, max;
};

struct intel_p2_t {
	int dot_limit;
	int p2_slow, p2_fast;
};

#define INTEL_P2_NUM		      2

struct intel_limit_t {
	struct intel_range_t dot, vco, n, m, m1, m2, p, p1;
	struct intel_p2_t p2;
};

#define I8XX_DOT_MIN		  25000
#define I8XX_DOT_MAX		 350000
#define I8XX_VCO_MIN		 930000
#define I8XX_VCO_MAX		1400000
#define I8XX_N_MIN		      3
#define I8XX_N_MAX		     16
#define I8XX_M_MIN		     96
#define I8XX_M_MAX		    140
#define I8XX_M1_MIN		     18
#define I8XX_M1_MAX		     26
#define I8XX_M2_MIN		      6
#define I8XX_M2_MAX		     16
#define I8XX_P_MIN		      4
#define I8XX_P_MAX		    128
#define I8XX_P1_MIN		      2
#define I8XX_P1_MAX		     33
#define I8XX_P1_LVDS_MIN	      1
#define I8XX_P1_LVDS_MAX	      6
#define I8XX_P2_SLOW		      4
#define I8XX_P2_FAST		      2
#define I8XX_P2_LVDS_SLOW	      14
#define I8XX_P2_LVDS_FAST	      14	/* No fast option */
#define I8XX_P2_SLOW_LIMIT	 165000

#define I9XX_DOT_MIN		  20000
#define I9XX_DOT_MAX		 400000
#define I9XX_VCO_MIN		1400000
#define I9XX_VCO_MAX		2800000
#define I9XX_N_MIN		      3
#define I9XX_N_MAX		      8
#define I9XX_M_MIN		     70
#define I9XX_M_MAX		    120
#define I9XX_M1_MIN		     10
#define I9XX_M1_MAX		     20
#define I9XX_M2_MIN		      5
#define I9XX_M2_MAX		      9
#define I9XX_P_SDVO_DAC_MIN	      5
#define I9XX_P_SDVO_DAC_MAX	     80
#define I9XX_P_LVDS_MIN		      7
#define I9XX_P_LVDS_MAX		     98
#define I9XX_P1_MIN		      1
#define I9XX_P1_MAX		      8
#define I9XX_P2_SDVO_DAC_SLOW		     10
#define I9XX_P2_SDVO_DAC_FAST		      5
#define I9XX_P2_SDVO_DAC_SLOW_LIMIT	 200000
#define I9XX_P2_LVDS_SLOW		     14
#define I9XX_P2_LVDS_FAST		      7
#define I9XX_P2_LVDS_SLOW_LIMIT		 112000

#define INTEL_LIMIT_I8XX_DVO_DAC    0
#define INTEL_LIMIT_I8XX_LVDS	    1
#define INTEL_LIMIT_I9XX_SDVO_DAC   2
#define INTEL_LIMIT_I9XX_LVDS	    3

static const struct intel_limit_t intel_limits[] = {
	{			/* INTEL_LIMIT_I8XX_DVO_DAC */
	 .dot = {.min = I8XX_DOT_MIN, .max = I8XX_DOT_MAX},
	 .vco = {.min = I8XX_VCO_MIN, .max = I8XX_VCO_MAX},
	 .n = {.min = I8XX_N_MIN, .max = I8XX_N_MAX},
	 .m = {.min = I8XX_M_MIN, .max = I8XX_M_MAX},
	 .m1 = {.min = I8XX_M1_MIN, .max = I8XX_M1_MAX},
	 .m2 = {.min = I8XX_M2_MIN, .max = I8XX_M2_MAX},
	 .p = {.min = I8XX_P_MIN, .max = I8XX_P_MAX},
	 .p1 = {.min = I8XX_P1_MIN, .max = I8XX_P1_MAX},
	 .p2 = {.dot_limit = I8XX_P2_SLOW_LIMIT,
		.p2_slow = I8XX_P2_SLOW, .p2_fast = I8XX_P2_FAST},
	 },
	{			/* INTEL_LIMIT_I8XX_LVDS */
	 .dot = {.min = I8XX_DOT_MIN, .max = I8XX_DOT_MAX},
	 .vco = {.min = I8XX_VCO_MIN, .max = I8XX_VCO_MAX},
	 .n = {.min = I8XX_N_MIN, .max = I8XX_N_MAX},
	 .m = {.min = I8XX_M_MIN, .max = I8XX_M_MAX},
	 .m1 = {.min = I8XX_M1_MIN, .max = I8XX_M1_MAX},
	 .m2 = {.min = I8XX_M2_MIN, .max = I8XX_M2_MAX},
	 .p = {.min = I8XX_P_MIN, .max = I8XX_P_MAX},
	 .p1 = {.min = I8XX_P1_LVDS_MIN, .max = I8XX_P1_LVDS_MAX},
	 .p2 = {.dot_limit = I8XX_P2_SLOW_LIMIT,
		.p2_slow = I8XX_P2_LVDS_SLOW, .p2_fast = I8XX_P2_LVDS_FAST},
	 },
	{			/* INTEL_LIMIT_I9XX_SDVO_DAC */
	 .dot = {.min = I9XX_DOT_MIN, .max = I9XX_DOT_MAX},
	 .vco = {.min = I9XX_VCO_MIN, .max = I9XX_VCO_MAX},
	 .n = {.min = I9XX_N_MIN, .max = I9XX_N_MAX},
	 .m = {.min = I9XX_M_MIN, .max = I9XX_M_MAX},
	 .m1 = {.min = I9XX_M1_MIN, .max = I9XX_M1_MAX},
	 .m2 = {.min = I9XX_M2_MIN, .max = I9XX_M2_MAX},
	 .p = {.min = I9XX_P_SDVO_DAC_MIN, .max = I9XX_P_SDVO_DAC_MAX},
	 .p1 = {.min = I9XX_P1_MIN, .max = I9XX_P1_MAX},
	 .p2 = {.dot_limit = I9XX_P2_SDVO_DAC_SLOW_LIMIT,
		.p2_slow = I9XX_P2_SDVO_DAC_SLOW, .p2_fast =
		I9XX_P2_SDVO_DAC_FAST},
	 },
	{			/* INTEL_LIMIT_I9XX_LVDS */
	 .dot = {.min = I9XX_DOT_MIN, .max = I9XX_DOT_MAX},
	 .vco = {.min = I9XX_VCO_MIN, .max = I9XX_VCO_MAX},
	 .n = {.min = I9XX_N_MIN, .max = I9XX_N_MAX},
	 .m = {.min = I9XX_M_MIN, .max = I9XX_M_MAX},
	 .m1 = {.min = I9XX_M1_MIN, .max = I9XX_M1_MAX},
	 .m2 = {.min = I9XX_M2_MIN, .max = I9XX_M2_MAX},
	 .p = {.min = I9XX_P_LVDS_MIN, .max = I9XX_P_LVDS_MAX},
	 .p1 = {.min = I9XX_P1_MIN, .max = I9XX_P1_MAX},
	 /* The single-channel range is 25-112Mhz, and dual-channel
	  * is 80-224Mhz.  Prefer single channel as much as possible.
	  */
	 .p2 = {.dot_limit = I9XX_P2_LVDS_SLOW_LIMIT,
		.p2_slow = I9XX_P2_LVDS_SLOW, .p2_fast = I9XX_P2_LVDS_FAST},
	 },
};

static const struct intel_limit_t *intel_limit(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	const struct intel_limit_t *limit;

	if (IS_I9XX(dev)) {
		if (intel_pipe_has_type(crtc, INTEL_OUTPUT_LVDS))
			limit = &intel_limits[INTEL_LIMIT_I9XX_LVDS];
		else
			limit = &intel_limits[INTEL_LIMIT_I9XX_SDVO_DAC];
	} else {
		if (intel_pipe_has_type(crtc, INTEL_OUTPUT_LVDS))
			limit = &intel_limits[INTEL_LIMIT_I8XX_LVDS];
		else
			limit = &intel_limits[INTEL_LIMIT_I8XX_DVO_DAC];
	}
	return limit;
}

/** Derive the pixel clock for the given refclk and divisors for 8xx chips. */

static void i8xx_clock(int refclk, struct intel_clock_t *clock)
{
	clock->m = 5 * (clock->m1 + 2) + (clock->m2 + 2);
	clock->p = clock->p1 * clock->p2;
	clock->vco = refclk * clock->m / (clock->n + 2);
	clock->dot = clock->vco / clock->p;
}

/** Derive the pixel clock for the given refclk and divisors for 9xx chips. */

static void i9xx_clock(int refclk, struct intel_clock_t *clock)
{
	clock->m = 5 * (clock->m1 + 2) + (clock->m2 + 2);
	clock->p = clock->p1 * clock->p2;
	clock->vco = refclk * clock->m / (clock->n + 2);
	clock->dot = clock->vco / clock->p;
}

static void intel_clock(struct drm_device *dev, int refclk,
			struct intel_clock_t *clock)
{
	if (IS_I9XX(dev))
		return i9xx_clock(refclk, clock);
	else
		return i8xx_clock(refclk, clock);
}

/**
 * Returns whether any output on the specified pipe is of the specified type
 */
bool intel_pipe_has_type(struct drm_crtc *crtc, int type)
{
	struct drm_device *dev = crtc->dev;
	struct drm_mode_config *mode_config = &dev->mode_config;
	struct drm_connector *l_entry;

	list_for_each_entry(l_entry, &mode_config->connector_list, head) {
		if (l_entry->encoder && l_entry->encoder->crtc == crtc) {
			struct intel_output *intel_output =
			    to_intel_output(l_entry);
			if (intel_output->type == type)
				return true;
		}
	}
	return false;
}

#define INTELPllInvalid(s)   { /* ErrorF (s) */; return false; }
/**
 * Returns whether the given set of divisors are valid for a given refclk with
 * the given connectors.
 */

static bool intel_PLL_is_valid(struct drm_crtc *crtc,
			       struct intel_clock_t *clock)
{
	const struct intel_limit_t *limit = intel_limit(crtc);

	if (clock->p1 < limit->p1.min || limit->p1.max < clock->p1)
		INTELPllInvalid("p1 out of range\n");
	if (clock->p < limit->p.min || limit->p.max < clock->p)
		INTELPllInvalid("p out of range\n");
	if (clock->m2 < limit->m2.min || limit->m2.max < clock->m2)
		INTELPllInvalid("m2 out of range\n");
	if (clock->m1 < limit->m1.min || limit->m1.max < clock->m1)
		INTELPllInvalid("m1 out of range\n");
	if (clock->m1 <= clock->m2)
		INTELPllInvalid("m1 <= m2\n");
	if (clock->m < limit->m.min || limit->m.max < clock->m)
		INTELPllInvalid("m out of range\n");
	if (clock->n < limit->n.min || limit->n.max < clock->n)
		INTELPllInvalid("n out of range\n");
	if (clock->vco < limit->vco.min || limit->vco.max < clock->vco)
		INTELPllInvalid("vco out of range\n");
	/* XXX: We may need to be checking "Dot clock"
	 * depending on the multiplier, connector, etc.,
	 * rather than just a single range.
	 */
	if (clock->dot < limit->dot.min || limit->dot.max < clock->dot)
		INTELPllInvalid("dot out of range\n");

	return true;
}

/**
 * Returns a set of divisors for the desired target clock with the given
 * refclk, or FALSE.  The returned values represent the clock equation:
 * reflck * (5 * (m1 + 2) + (m2 + 2)) / (n + 2) / p1 / p2.
 */
static bool intel_find_best_PLL(struct drm_crtc *crtc, int target,
				int refclk,
				struct intel_clock_t *best_clock)
{
	struct drm_device *dev = crtc->dev;
	struct intel_clock_t clock;
	const struct intel_limit_t *limit = intel_limit(crtc);
	int err = target;

	if (IS_I9XX(dev) && intel_pipe_has_type(crtc, INTEL_OUTPUT_LVDS) &&
	    (REG_READ(LVDS) & LVDS_PORT_EN) != 0) {
		/*
		 * For LVDS, if the panel is on, just rely on its current
		 * settings for dual-channel.  We haven't figured out how to
		 * reliably set up different single/dual channel state, if we
		 * even can.
		 */
		if ((REG_READ(LVDS) & LVDS_CLKB_POWER_MASK) ==
		    LVDS_CLKB_POWER_UP)
			clock.p2 = limit->p2.p2_fast;
		else
			clock.p2 = limit->p2.p2_slow;
	} else {
		if (target < limit->p2.dot_limit)
			clock.p2 = limit->p2.p2_slow;
		else
			clock.p2 = limit->p2.p2_fast;
	}

	memset(best_clock, 0, sizeof(*best_clock));

	for (clock.m1 = limit->m1.min; clock.m1 <= limit->m1.max;
	     clock.m1++) {
		for (clock.m2 = limit->m2.min;
		     clock.m2 < clock.m1 && clock.m2 <= limit->m2.max;
		     clock.m2++) {
			for (clock.n = limit->n.min;
			     clock.n <= limit->n.max; clock.n++) {
				for (clock.p1 = limit->p1.min;
				     clock.p1 <= limit->p1.max;
				     clock.p1++) {
					int this_err;

					intel_clock(dev, refclk, &clock);

					if (!intel_PLL_is_valid
					    (crtc, &clock))
						continue;

					this_err = abs(clock.dot - target);
					if (this_err < err) {
						*best_clock = clock;
						err = this_err;
					}
				}
			}
		}
	}

	return err != target;
}

void intel_wait_for_vblank(struct drm_device *dev)
{
	/* Wait for 20ms, i.e. one cycle at 50hz. */
	udelay(20000);
}

int intel_pipe_set_base(struct drm_crtc *crtc, int x, int y, struct drm_framebuffer *old_fb)
{
	struct drm_device *dev = crtc->dev;
	/* struct drm_i915_master_private *master_priv; */
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	struct psb_framebuffer *psbfb = to_psb_fb(crtc->fb);
	struct intel_mode_device *mode_dev = intel_crtc->mode_dev;
	int pipe = intel_crtc->pipe;
	unsigned long Start, Offset;
	int dspbase = (pipe == 0 ? DSPABASE : DSPBBASE);
	int dspsurf = (pipe == 0 ? DSPASURF : DSPBSURF);
	int dspstride = (pipe == 0) ? DSPASTRIDE : DSPBSTRIDE;
	int dspcntr_reg = (pipe == 0) ? DSPACNTR : DSPBCNTR;
	u32 dspcntr;

	/* no fb bound */
	if (!crtc->fb) {
		DRM_DEBUG("No FB bound\n");
		return 0;
	}

	if (IS_MRST(dev) && (pipe == 0))
		dspbase = MRST_DSPABASE;

	Start = mode_dev->bo_offset(dev, psbfb->bo);
	Offset = y * crtc->fb->pitch + x * (crtc->fb->bits_per_pixel / 8);

	REG_WRITE(dspstride, crtc->fb->pitch);

	dspcntr = REG_READ(dspcntr_reg);
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
	case 24:
	case 32:
		dspcntr |= DISPPLANE_32BPP_NO_ALPHA;
		break;
	default:
		DRM_ERROR("Unknown color depth\n");
		return -EINVAL;
	}
	REG_WRITE(dspcntr_reg, dspcntr);

	DRM_DEBUG("Writing base %08lX %08lX %d %d\n", Start, Offset, x, y);
	if (IS_I965G(dev) || IS_MRST(dev)) {
		REG_WRITE(dspbase, Offset);
		REG_READ(dspbase);
		REG_WRITE(dspsurf, Start);
		REG_READ(dspsurf);
	} else {
		REG_WRITE(dspbase, Start + Offset);
		REG_READ(dspbase);
	}

	if (!dev->primary->master)
		return 0;

#if 0				/* JB: Enable sarea later */
	master_priv = dev->primary->master->driver_priv;
	if (!master_priv->sarea_priv)
		return 0;

	switch (pipe) {
	case 0:
		master_priv->sarea_priv->planeA_x = x;
		master_priv->sarea_priv->planeA_y = y;
		break;
	case 1:
		master_priv->sarea_priv->planeB_x = x;
		master_priv->sarea_priv->planeB_y = y;
		break;
	default:
		DRM_ERROR("Can't update pipe %d in SAREA\n", pipe);
		break;
	}
#endif
}



/**
 * Sets the power management mode of the pipe and plane.
 *
 * This code should probably grow support for turning the cursor off and back
 * on appropriately at the same time as we're turning the pipe off/on.
 */
static void intel_crtc_dpms(struct drm_crtc *crtc, int mode)
{
	struct drm_device *dev = crtc->dev;
	/* struct drm_i915_master_private *master_priv; */
	/* struct drm_i915_private *dev_priv = dev->dev_private; */
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	int pipe = intel_crtc->pipe;
	int dpll_reg = (pipe == 0) ? DPLL_A : DPLL_B;
	int dspcntr_reg = (pipe == 0) ? DSPACNTR : DSPBCNTR;
	int dspbase_reg = (pipe == 0) ? DSPABASE : DSPBBASE;
	int pipeconf_reg = (pipe == 0) ? PIPEACONF : PIPEBCONF;
	u32 temp;
	bool enabled;

	/* XXX: When our outputs are all unaware of DPMS modes other than off
	 * and on, we should map those modes to DRM_MODE_DPMS_OFF in the CRTC.
	 */
	switch (mode) {
	case DRM_MODE_DPMS_ON:
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
		/* Enable the DPLL */
		temp = REG_READ(dpll_reg);
		if ((temp & DPLL_VCO_ENABLE) == 0) {
			REG_WRITE(dpll_reg, temp);
			REG_READ(dpll_reg);
			/* Wait for the clocks to stabilize. */
			udelay(150);
			REG_WRITE(dpll_reg, temp | DPLL_VCO_ENABLE);
			REG_READ(dpll_reg);
			/* Wait for the clocks to stabilize. */
			udelay(150);
			REG_WRITE(dpll_reg, temp | DPLL_VCO_ENABLE);
			REG_READ(dpll_reg);
			/* Wait for the clocks to stabilize. */
			udelay(150);
		}

		/* Enable the pipe */
		temp = REG_READ(pipeconf_reg);
		if ((temp & PIPEACONF_ENABLE) == 0)
			REG_WRITE(pipeconf_reg, temp | PIPEACONF_ENABLE);

		/* Enable the plane */
		temp = REG_READ(dspcntr_reg);
		if ((temp & DISPLAY_PLANE_ENABLE) == 0) {
			REG_WRITE(dspcntr_reg,
				  temp | DISPLAY_PLANE_ENABLE);
			/* Flush the plane changes */
			REG_WRITE(dspbase_reg, REG_READ(dspbase_reg));
		}

		intel_crtc_load_lut(crtc);

		/* Give the overlay scaler a chance to enable
		 * if it's on this pipe */
		/* intel_crtc_dpms_video(crtc, true); TODO */
		break;
	case DRM_MODE_DPMS_OFF:
		/* Give the overlay scaler a chance to disable
		 * if it's on this pipe */
		/* intel_crtc_dpms_video(crtc, FALSE); TODO */

		/* Disable the VGA plane that we never use */
		REG_WRITE(VGACNTRL, VGA_DISP_DISABLE);

		/* Disable display plane */
		temp = REG_READ(dspcntr_reg);
		if ((temp & DISPLAY_PLANE_ENABLE) != 0) {
			REG_WRITE(dspcntr_reg,
				  temp & ~DISPLAY_PLANE_ENABLE);
			/* Flush the plane changes */
			REG_WRITE(dspbase_reg, REG_READ(dspbase_reg));
			REG_READ(dspbase_reg);
		}

		if (!IS_I9XX(dev)) {
			/* Wait for vblank for the disable to take effect */
			intel_wait_for_vblank(dev);
		}

		/* Next, disable display pipes */
		temp = REG_READ(pipeconf_reg);
		if ((temp & PIPEACONF_ENABLE) != 0) {
			REG_WRITE(pipeconf_reg, temp & ~PIPEACONF_ENABLE);
			REG_READ(pipeconf_reg);
		}

		/* Wait for vblank for the disable to take effect. */
		intel_wait_for_vblank(dev);

		temp = REG_READ(dpll_reg);
		if ((temp & DPLL_VCO_ENABLE) != 0) {
			REG_WRITE(dpll_reg, temp & ~DPLL_VCO_ENABLE);
			REG_READ(dpll_reg);
		}

		/* Wait for the clocks to turn off. */
		udelay(150);
		break;
	}

	enabled = crtc->enabled && mode != DRM_MODE_DPMS_OFF;

#if 0				/* JB: Add vblank support later */
	if (enabled)
		dev_priv->vblank_pipe |= (1 << pipe);
	else
		dev_priv->vblank_pipe &= ~(1 << pipe);
#endif

	intel_crtc->dpms_mode = mode;

#if 0				/* JB: Add sarea support later */
	if (!dev->primary->master)
		return 0;

	master_priv = dev->primary->master->driver_priv;
	if (!master_priv->sarea_priv)
		return 0;

	switch (pipe) {
	case 0:
		master_priv->sarea_priv->planeA_w =
		    enabled ? crtc->mode.hdisplay : 0;
		master_priv->sarea_priv->planeA_h =
		    enabled ? crtc->mode.vdisplay : 0;
		break;
	case 1:
		master_priv->sarea_priv->planeB_w =
		    enabled ? crtc->mode.hdisplay : 0;
		master_priv->sarea_priv->planeB_h =
		    enabled ? crtc->mode.vdisplay : 0;
		break;
	default:
		DRM_ERROR("Can't update pipe %d in SAREA\n", pipe);
		break;
	}
#endif
}

static void intel_crtc_prepare(struct drm_crtc *crtc)
{
	struct drm_crtc_helper_funcs *crtc_funcs = crtc->helper_private;
	crtc_funcs->dpms(crtc, DRM_MODE_DPMS_OFF);
}

static void intel_crtc_commit(struct drm_crtc *crtc)
{
	struct drm_crtc_helper_funcs *crtc_funcs = crtc->helper_private;
	crtc_funcs->dpms(crtc, DRM_MODE_DPMS_ON);
}

void intel_encoder_prepare(struct drm_encoder *encoder)
{
	struct drm_encoder_helper_funcs *encoder_funcs =
	    encoder->helper_private;
	/* lvds has its own version of prepare see intel_lvds_prepare */
	encoder_funcs->dpms(encoder, DRM_MODE_DPMS_OFF);
}

void intel_encoder_commit(struct drm_encoder *encoder)
{
	struct drm_encoder_helper_funcs *encoder_funcs =
	    encoder->helper_private;
	/* lvds has its own version of commit see intel_lvds_commit */
	encoder_funcs->dpms(encoder, DRM_MODE_DPMS_ON);
}

static bool intel_crtc_mode_fixup(struct drm_crtc *crtc,
				  struct drm_display_mode *mode,
				  struct drm_display_mode *adjusted_mode)
{
	return true;
}


/** Returns the core display clock speed for i830 - i945 */
static int intel_get_core_clock_speed(struct drm_device *dev)
{
#if 0				/* JB: Look into this more */
	/* Core clock values taken from the published datasheets.
	 * The 830 may go up to 166 Mhz, which we should check.
	 */
	if (IS_I945G(dev))
		return 400000;
	else if (IS_I915G(dev))
		return 333000;
	else if (IS_I945GM(dev) || IS_845G(dev))
		return 200000;
	else if (IS_I915GM(dev)) {
		u16 gcfgc = 0;

		pci_read_config_word(dev->pdev, GCFGC, &gcfgc);

		if (gcfgc & GC_LOW_FREQUENCY_ENABLE)
			return 133000;
		else {
			switch (gcfgc & GC_DISPLAY_CLOCK_MASK) {
			case GC_DISPLAY_CLOCK_333_MHZ:
				return 333000;
			default:
			case GC_DISPLAY_CLOCK_190_200_MHZ:
				return 190000;
			}
		}
	} else if (IS_I865G(dev))
		return 266000;
	else if (IS_I855(dev)) {
#if 0
		PCITAG bridge = pciTag(0, 0, 0);
					/* This is always the host bridge */
		u16 hpllcc = pciReadWord(bridge, HPLLCC);

#endif
		u16 hpllcc = 0;
		/* Assume that the hardware is in the high speed state.  This
		 * should be the default.
		 */
		switch (hpllcc & GC_CLOCK_CONTROL_MASK) {
		case GC_CLOCK_133_200:
		case GC_CLOCK_100_200:
			return 200000;
		case GC_CLOCK_166_250:
			return 250000;
		case GC_CLOCK_100_133:
			return 133000;
		}
	} else			/* 852, 830 */
		return 133000;
#endif
	return 0;		/* Silence gcc warning */
}


/**
 * Return the pipe currently connected to the panel fitter,
 * or -1 if the panel fitter is not present or not in use
 */
static int intel_panel_fitter_pipe(struct drm_device *dev)
{
	u32 pfit_control;

	/* i830 doesn't have a panel fitter */
	if (IS_I830(dev))
		return -1;

	pfit_control = REG_READ(PFIT_CONTROL);

	/* See if the panel fitter is in use */
	if ((pfit_control & PFIT_ENABLE) == 0)
		return -1;

	/* 965 can place panel fitter on either pipe */
	if (IS_I965G(dev) || IS_MRST(dev))
		return (pfit_control >> 29) & 0x3;

	/* older chips can only use pipe 1 */
	return 1;
}

static int intel_crtc_mode_set(struct drm_crtc *crtc,
			       struct drm_display_mode *mode,
			       struct drm_display_mode *adjusted_mode,
			       int x, int y,
			       struct drm_framebuffer *old_fb)
{
	struct drm_device *dev = crtc->dev;
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	int pipe = intel_crtc->pipe;
	int fp_reg = (pipe == 0) ? FPA0 : FPB0;
	int dpll_reg = (pipe == 0) ? DPLL_A : DPLL_B;
	int dpll_md_reg = (intel_crtc->pipe == 0) ? DPLL_A_MD : DPLL_B_MD;
	int dspcntr_reg = (pipe == 0) ? DSPACNTR : DSPBCNTR;
	int pipeconf_reg = (pipe == 0) ? PIPEACONF : PIPEBCONF;
	int htot_reg = (pipe == 0) ? HTOTAL_A : HTOTAL_B;
	int hblank_reg = (pipe == 0) ? HBLANK_A : HBLANK_B;
	int hsync_reg = (pipe == 0) ? HSYNC_A : HSYNC_B;
	int vtot_reg = (pipe == 0) ? VTOTAL_A : VTOTAL_B;
	int vblank_reg = (pipe == 0) ? VBLANK_A : VBLANK_B;
	int vsync_reg = (pipe == 0) ? VSYNC_A : VSYNC_B;
	int dspsize_reg = (pipe == 0) ? DSPASIZE : DSPBSIZE;
	int dsppos_reg = (pipe == 0) ? DSPAPOS : DSPBPOS;
	int pipesrc_reg = (pipe == 0) ? PIPEASRC : PIPEBSRC;
	int refclk;
	struct intel_clock_t clock;
	u32 dpll = 0, fp = 0, dspcntr, pipeconf;
	bool ok, is_sdvo = false, is_dvo = false;
	bool is_crt = false, is_lvds = false, is_tv = false;
	struct drm_mode_config *mode_config = &dev->mode_config;
	struct drm_connector *connector;

	list_for_each_entry(connector, &mode_config->connector_list, head) {
		struct intel_output *intel_output =
		    to_intel_output(connector);

		if (!connector->encoder
		    || connector->encoder->crtc != crtc)
			continue;

		switch (intel_output->type) {
		case INTEL_OUTPUT_LVDS:
			is_lvds = true;
			break;
		case INTEL_OUTPUT_SDVO:
			is_sdvo = true;
			break;
		case INTEL_OUTPUT_DVO:
			is_dvo = true;
			break;
		case INTEL_OUTPUT_TVOUT:
			is_tv = true;
			break;
		case INTEL_OUTPUT_ANALOG:
			is_crt = true;
			break;
		}
	}

	if (IS_I9XX(dev))
		refclk = 96000;
	else
		refclk = 48000;

	ok = intel_find_best_PLL(crtc, adjusted_mode->clock, refclk,
				 &clock);
	if (!ok) {
		DRM_ERROR("Couldn't find PLL settings for mode!\n");
		return 0;
	}

	fp = clock.n << 16 | clock.m1 << 8 | clock.m2;

	dpll = DPLL_VGA_MODE_DIS;
	if (IS_I9XX(dev)) {
		if (is_lvds) {
			dpll |= DPLLB_MODE_LVDS;
			if (IS_POULSBO(dev))
				dpll |= DPLL_DVO_HIGH_SPEED;
		} else
			dpll |= DPLLB_MODE_DAC_SERIAL;
		if (is_sdvo) {
			dpll |= DPLL_DVO_HIGH_SPEED;
			if (IS_I945G(dev) || IS_I945GM(dev)) {
				int sdvo_pixel_multiply =
				    adjusted_mode->clock / mode->clock;
				dpll |=
				    (sdvo_pixel_multiply -
				     1) << SDVO_MULTIPLIER_SHIFT_HIRES;
			}
		}

		/* compute bitmask from p1 value */
		dpll |= (1 << (clock.p1 - 1)) << 16;
		switch (clock.p2) {
		case 5:
			dpll |= DPLL_DAC_SERIAL_P2_CLOCK_DIV_5;
			break;
		case 7:
			dpll |= DPLLB_LVDS_P2_CLOCK_DIV_7;
			break;
		case 10:
			dpll |= DPLL_DAC_SERIAL_P2_CLOCK_DIV_10;
			break;
		case 14:
			dpll |= DPLLB_LVDS_P2_CLOCK_DIV_14;
			break;
		}
		if (IS_I965G(dev))
			dpll |= (6 << PLL_LOAD_PULSE_PHASE_SHIFT);
	} else {
		if (is_lvds) {
			dpll |=
			    (1 << (clock.p1 - 1)) <<
			    DPLL_FPA01_P1_POST_DIV_SHIFT;
		} else {
			if (clock.p1 == 2)
				dpll |= PLL_P1_DIVIDE_BY_TWO;
			else
				dpll |=
				    (clock.p1 -
				     2) << DPLL_FPA01_P1_POST_DIV_SHIFT;
			if (clock.p2 == 4)
				dpll |= PLL_P2_DIVIDE_BY_4;
		}
	}

	if (is_tv) {
		/* XXX: just matching BIOS for now */
/*	dpll |= PLL_REF_INPUT_TVCLKINBC; */
		dpll |= 3;
	}
#if 0
	else if (is_lvds)
		dpll |= PLLB_REF_INPUT_SPREADSPECTRUMIN;
#endif
	else
		dpll |= PLL_REF_INPUT_DREFCLK;

	/* setup pipeconf */
	pipeconf = REG_READ(pipeconf_reg);

	/* Set up the display plane register */
	dspcntr = DISPPLANE_GAMMA_ENABLE;

	if (pipe == 0)
		dspcntr |= DISPPLANE_SEL_PIPE_A;
	else
		dspcntr |= DISPPLANE_SEL_PIPE_B;

	if (pipe == 0 && !IS_I965G(dev)) {
		/* Enable pixel doubling when the dot clock is > 90%
		 * of the (display) core speed.
		 *
		 * XXX: No double-wide on 915GM pipe B.
		 * Is that the only reason for the
		 * pipe == 0 check?
		 */
		if (mode->clock > intel_get_core_clock_speed(dev) * 9 / 10)
			pipeconf |= PIPEACONF_DOUBLE_WIDE;
		else
			pipeconf &= ~PIPEACONF_DOUBLE_WIDE;
	}

	dspcntr |= DISPLAY_PLANE_ENABLE;
	pipeconf |= PIPEACONF_ENABLE;
	dpll |= DPLL_VCO_ENABLE;


	/* Disable the panel fitter if it was on our pipe */
	if (intel_panel_fitter_pipe(dev) == pipe)
		REG_WRITE(PFIT_CONTROL, 0);

	DRM_DEBUG("Mode for pipe %c:\n", pipe == 0 ? 'A' : 'B');
	drm_mode_debug_printmodeline(mode);

#if 0
	if (!xf86ModesEqual(mode, adjusted_mode)) {
		xf86DrvMsg(pScrn->scrnIndex, X_INFO,
			   "Adjusted mode for pipe %c:\n",
			   pipe == 0 ? 'A' : 'B');
		xf86PrintModeline(pScrn->scrnIndex, mode);
	}
	i830PrintPll("chosen", &clock);
#endif

	if (dpll & DPLL_VCO_ENABLE) {
		REG_WRITE(fp_reg, fp);
		REG_WRITE(dpll_reg, dpll & ~DPLL_VCO_ENABLE);
		REG_READ(dpll_reg);
		udelay(150);
	}

	/* The LVDS pin pair needs to be on before the DPLLs are enabled.
	 * This is an exception to the general rule that mode_set doesn't turn
	 * things on.
	 */
	if (is_lvds) {
		u32 lvds = REG_READ(LVDS);

		lvds |=
		    LVDS_PORT_EN | LVDS_A0A2_CLKA_POWER_UP |
		    LVDS_PIPEB_SELECT;
		/* Set the B0-B3 data pairs corresponding to
		 * whether we're going to
		 * set the DPLLs for dual-channel mode or not.
		 */
		if (clock.p2 == 7)
			lvds |= LVDS_B0B3_POWER_UP | LVDS_CLKB_POWER_UP;
		else
			lvds &= ~(LVDS_B0B3_POWER_UP | LVDS_CLKB_POWER_UP);

		/* It would be nice to set 24 vs 18-bit mode (LVDS_A3_POWER_UP)
		 * appropriately here, but we need to look more
		 * thoroughly into how panels behave in the two modes.
		 */

		REG_WRITE(LVDS, lvds);
		REG_READ(LVDS);
	}

	REG_WRITE(fp_reg, fp);
	REG_WRITE(dpll_reg, dpll);
	REG_READ(dpll_reg);
	/* Wait for the clocks to stabilize. */
	udelay(150);

	if (IS_I965G(dev)) {
		int sdvo_pixel_multiply =
		    adjusted_mode->clock / mode->clock;
		REG_WRITE(dpll_md_reg,
			  (0 << DPLL_MD_UDI_DIVIDER_SHIFT) |
			  ((sdvo_pixel_multiply -
			    1) << DPLL_MD_UDI_MULTIPLIER_SHIFT));
	} else {
		/* write it again -- the BIOS does, after all */
		REG_WRITE(dpll_reg, dpll);
	}
	REG_READ(dpll_reg);
	/* Wait for the clocks to stabilize. */
	udelay(150);

	REG_WRITE(htot_reg, (adjusted_mode->crtc_hdisplay - 1) |
		  ((adjusted_mode->crtc_htotal - 1) << 16));
	REG_WRITE(hblank_reg, (adjusted_mode->crtc_hblank_start - 1) |
		  ((adjusted_mode->crtc_hblank_end - 1) << 16));
	REG_WRITE(hsync_reg, (adjusted_mode->crtc_hsync_start - 1) |
		  ((adjusted_mode->crtc_hsync_end - 1) << 16));
	REG_WRITE(vtot_reg, (adjusted_mode->crtc_vdisplay - 1) |
		  ((adjusted_mode->crtc_vtotal - 1) << 16));
	REG_WRITE(vblank_reg, (adjusted_mode->crtc_vblank_start - 1) |
		  ((adjusted_mode->crtc_vblank_end - 1) << 16));
	REG_WRITE(vsync_reg, (adjusted_mode->crtc_vsync_start - 1) |
		  ((adjusted_mode->crtc_vsync_end - 1) << 16));
	/* pipesrc and dspsize control the size that is scaled from,
	 * which should always be the user's requested size.
	 */
	REG_WRITE(dspsize_reg,
		  ((mode->vdisplay - 1) << 16) | (mode->hdisplay - 1));
	REG_WRITE(dsppos_reg, 0);
	REG_WRITE(pipesrc_reg,
		  ((mode->hdisplay - 1) << 16) | (mode->vdisplay - 1));
	REG_WRITE(pipeconf_reg, pipeconf);
	REG_READ(pipeconf_reg);

	intel_wait_for_vblank(dev);

	REG_WRITE(dspcntr_reg, dspcntr);

	/* Flush the plane changes */
	{
		struct drm_crtc_helper_funcs *crtc_funcs =
		    crtc->helper_private;
		crtc_funcs->mode_set_base(crtc, x, y, old_fb);
	}

	intel_wait_for_vblank(dev);

	return 0;
}

/** Loads the palette/gamma unit for the CRTC with the prepared values */
void intel_crtc_load_lut(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	int palreg = (intel_crtc->pipe == 0) ? PALETTE_A : PALETTE_B;
	int i;

	/* The clocks have to be on to load the palette. */
	if (!crtc->enabled)
		return;

	for (i = 0; i < 256; i++) {
		REG_WRITE(palreg + 4 * i,
			  (intel_crtc->lut_r[i] << 16) |
			  (intel_crtc->lut_g[i] << 8) |
			  intel_crtc->lut_b[i]);
	}
}

static int intel_crtc_cursor_set(struct drm_crtc *crtc,
				 struct drm_file *file_priv,
				 uint32_t handle,
				 uint32_t width, uint32_t height)
{
	struct drm_device *dev = crtc->dev;
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	struct intel_mode_device *mode_dev = intel_crtc->mode_dev;
	int pipe = intel_crtc->pipe;
	uint32_t control = (pipe == 0) ? CURACNTR : CURBCNTR;
	uint32_t base = (pipe == 0) ? CURABASE : CURBBASE;
	uint32_t temp;
	size_t addr = 0;
	size_t size;
	void *bo;
	int ret;

	DRM_DEBUG("\n");

	/* if we want to turn of the cursor ignore width and height */
	if (!handle) {
		DRM_DEBUG("cursor off\n");
		/* turn of the cursor */
		temp = 0;
		temp |= CURSOR_MODE_DISABLE;

		REG_WRITE(control, temp);
		REG_WRITE(base, 0);

		/* unpin the old bo */
		if (intel_crtc->cursor_bo) {
			mode_dev->bo_unpin_for_scanout(dev,
						       intel_crtc->
						       cursor_bo);
			intel_crtc->cursor_bo = NULL;
		}

		return 0;
	}

	/* Currently we only support 64x64 cursors */
	if (width != 64 || height != 64) {
		DRM_ERROR("we currently only support 64x64 cursors\n");
		return -EINVAL;
	}

	bo = mode_dev->bo_from_handle(dev, file_priv, handle);
	if (!bo)
		return -ENOENT;

	ret = mode_dev->bo_pin_for_scanout(dev, bo);
	if (ret)
		return ret;

	size = mode_dev->bo_size(dev, bo);
	if (size < width * height * 4) {
		DRM_ERROR("buffer is to small\n");
		return -ENOMEM;
	}

	addr = mode_dev->bo_size(dev, bo);
	if (mode_dev->cursor_needs_physical)
		addr = dev->agp->base + addr;

	intel_crtc->cursor_addr = addr;
	temp = 0;
	/* set the pipe for the cursor */
	temp |= (pipe << 28);
	temp |= CURSOR_MODE_64_ARGB_AX | MCURSOR_GAMMA_ENABLE;

	REG_WRITE(control, temp);
	REG_WRITE(base, addr);

	/* unpin the old bo */
	if (intel_crtc->cursor_bo && intel_crtc->cursor_bo != bo) {
		mode_dev->bo_unpin_for_scanout(dev, intel_crtc->cursor_bo);
		intel_crtc->cursor_bo = bo;
	}

	return 0;
}

static int intel_crtc_cursor_move(struct drm_crtc *crtc, int x, int y)
{
	struct drm_device *dev = crtc->dev;
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	int pipe = intel_crtc->pipe;
	uint32_t temp = 0;
	uint32_t adder;

	if (x < 0) {
		temp |= (CURSOR_POS_SIGN << CURSOR_X_SHIFT);
		x = -x;
	}
	if (y < 0) {
		temp |= (CURSOR_POS_SIGN << CURSOR_Y_SHIFT);
		y = -y;
	}

	temp |= ((x & CURSOR_POS_MASK) << CURSOR_X_SHIFT);
	temp |= ((y & CURSOR_POS_MASK) << CURSOR_Y_SHIFT);

	adder = intel_crtc->cursor_addr;
	REG_WRITE((pipe == 0) ? CURAPOS : CURBPOS, temp);
	REG_WRITE((pipe == 0) ? CURABASE : CURBBASE, adder);

	return 0;
}

/** Sets the color ramps on behalf of RandR */
void intel_crtc_fb_gamma_set(struct drm_crtc *crtc, u16 red, u16 green,
			     u16 blue, int regno)
{
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);

	intel_crtc->lut_r[regno] = red >> 8;
	intel_crtc->lut_g[regno] = green >> 8;
	intel_crtc->lut_b[regno] = blue >> 8;
}

static void intel_crtc_gamma_set(struct drm_crtc *crtc, u16 *red,
				 u16 *green, u16 *blue, uint32_t size)
{
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	int i;

	if (size != 256)
		return;

	for (i = 0; i < 256; i++) {
		intel_crtc->lut_r[i] = red[i] >> 8;
		intel_crtc->lut_g[i] = green[i] >> 8;
		intel_crtc->lut_b[i] = blue[i] >> 8;
	}

	intel_crtc_load_lut(crtc);
}

/**
 * Get a pipe with a simple mode set on it for doing load-based monitor
 * detection.
 *
 * It will be up to the load-detect code to adjust the pipe as appropriate for
 * its requirements.  The pipe will be connected to no other outputs.
 *
 * Currently this code will only succeed if there is a pipe with no outputs
 * configured for it.  In the future, it could choose to temporarily disable
 * some outputs to free up a pipe for its use.
 *
 * \return crtc, or NULL if no pipes are available.
 */

/* VESA 640x480x72Hz mode to set on the pipe */
static struct drm_display_mode load_detect_mode = {
	DRM_MODE("640x480", DRM_MODE_TYPE_DEFAULT, 31500, 640, 664,
		 704, 832, 0, 480, 489, 491, 520, 0,
		 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
};

struct drm_crtc *intel_get_load_detect_pipe(struct intel_output
					    *intel_output,
					    struct drm_display_mode *mode,
					    int *dpms_mode)
{
	struct intel_crtc *intel_crtc;
	struct drm_crtc *possible_crtc;
	struct drm_crtc *supported_crtc = NULL;
	struct drm_encoder *encoder = &intel_output->enc;
	struct drm_crtc *crtc = NULL;
	struct drm_device *dev = encoder->dev;
	struct drm_encoder_helper_funcs *encoder_funcs =
	    encoder->helper_private;
	struct drm_crtc_helper_funcs *crtc_funcs;
	int i = -1;

	/*
	 * Algorithm gets a little messy:
	 *   - if the connector already has an assigned crtc, use it (but make
	 *     sure it's on first)
	 *   - try to find the first unused crtc that can drive this connector,
	 *     and use that if we find one
	 *   - if there are no unused crtcs available, try to use the first
	 *     one we found that supports the connector
	 */

	/* See if we already have a CRTC for this connector */
	if (encoder->crtc) {
		crtc = encoder->crtc;
		/* Make sure the crtc and connector are running */
		intel_crtc = to_intel_crtc(crtc);
		*dpms_mode = intel_crtc->dpms_mode;
		if (intel_crtc->dpms_mode != DRM_MODE_DPMS_ON) {
			crtc_funcs = crtc->helper_private;
			crtc_funcs->dpms(crtc, DRM_MODE_DPMS_ON);
			encoder_funcs->dpms(encoder, DRM_MODE_DPMS_ON);
		}
		return crtc;
	}

	/* Find an unused one (if possible) */
	list_for_each_entry(possible_crtc, &dev->mode_config.crtc_list,
			    head) {
		i++;
		if (!(encoder->possible_crtcs & (1 << i)))
			continue;
		if (!possible_crtc->enabled) {
			crtc = possible_crtc;
			break;
		}
		if (!supported_crtc)
			supported_crtc = possible_crtc;
	}

	/*
	 * If we didn't find an unused CRTC, don't use any.
	 */
	if (!crtc)
		return NULL;

	encoder->crtc = crtc;
	intel_output->load_detect_temp = true;

	intel_crtc = to_intel_crtc(crtc);
	*dpms_mode = intel_crtc->dpms_mode;

	if (!crtc->enabled) {
		if (!mode)
			mode = &load_detect_mode;
		drm_crtc_helper_set_mode(crtc, mode, 0, 0, crtc->fb);
	} else {
		if (intel_crtc->dpms_mode != DRM_MODE_DPMS_ON) {
			crtc_funcs = crtc->helper_private;
			crtc_funcs->dpms(crtc, DRM_MODE_DPMS_ON);
		}

		/* Add this connector to the crtc */
		encoder_funcs->mode_set(encoder, &crtc->mode, &crtc->mode);
		encoder_funcs->commit(encoder);
	}
	/* let the connector get through one full cycle before testing */
	intel_wait_for_vblank(dev);

	return crtc;
}

void intel_release_load_detect_pipe(struct intel_output *intel_output,
				    int dpms_mode)
{
	struct drm_encoder *encoder = &intel_output->enc;
	struct drm_device *dev = encoder->dev;
	struct drm_crtc *crtc = encoder->crtc;
	struct drm_encoder_helper_funcs *encoder_funcs =
	    encoder->helper_private;
	struct drm_crtc_helper_funcs *crtc_funcs = crtc->helper_private;

	if (intel_output->load_detect_temp) {
		encoder->crtc = NULL;
		intel_output->load_detect_temp = false;
		crtc->enabled = drm_helper_crtc_in_use(crtc);
		drm_helper_disable_unused_functions(dev);
	}

	/* Switch crtc and output back off if necessary */
	if (crtc->enabled && dpms_mode != DRM_MODE_DPMS_ON) {
		if (encoder->crtc == crtc)
			encoder_funcs->dpms(encoder, dpms_mode);
		crtc_funcs->dpms(crtc, dpms_mode);
	}
}

/* Returns the clock of the currently programmed mode of the given pipe. */
static int intel_crtc_clock_get(struct drm_device *dev,
				struct drm_crtc *crtc)
{
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	int pipe = intel_crtc->pipe;
	u32 dpll = REG_READ((pipe == 0) ? DPLL_A : DPLL_B);
	u32 fp;
	struct intel_clock_t clock;

	if ((dpll & DISPLAY_RATE_SELECT_FPA1) == 0)
		fp = REG_READ((pipe == 0) ? FPA0 : FPB0);
	else
		fp = REG_READ((pipe == 0) ? FPA1 : FPB1);

	clock.m1 = (fp & FP_M1_DIV_MASK) >> FP_M1_DIV_SHIFT;
	clock.m2 = (fp & FP_M2_DIV_MASK) >> FP_M2_DIV_SHIFT;
	clock.n = (fp & FP_N_DIV_MASK) >> FP_N_DIV_SHIFT;
	if (IS_I9XX(dev)) {
		clock.p1 = ffs((dpll & DPLL_FPA01_P1_POST_DIV_MASK) >>
			       DPLL_FPA01_P1_POST_DIV_SHIFT);

		switch (dpll & DPLL_MODE_MASK) {
		case DPLLB_MODE_DAC_SERIAL:
			clock.p2 = dpll & DPLL_DAC_SERIAL_P2_CLOCK_DIV_5 ?
			    5 : 10;
			break;
		case DPLLB_MODE_LVDS:
			clock.p2 = dpll & DPLLB_LVDS_P2_CLOCK_DIV_7 ?
			    7 : 14;
			break;
		default:
			DRM_DEBUG("Unknown DPLL mode %08x in programmed "
				  "mode\n", (int) (dpll & DPLL_MODE_MASK));
			return 0;
		}

		/* XXX: Handle the 100Mhz refclk */
		i9xx_clock(96000, &clock);
	} else {
		bool is_lvds = (pipe == 1)
		    && (REG_READ(LVDS) & LVDS_PORT_EN);

		if (is_lvds) {
			clock.p1 =
			    ffs((dpll &
				 DPLL_FPA01_P1_POST_DIV_MASK_I830_LVDS) >>
				DPLL_FPA01_P1_POST_DIV_SHIFT);
			clock.p2 = 14;

			if ((dpll & PLL_REF_INPUT_MASK) ==
			    PLLB_REF_INPUT_SPREADSPECTRUMIN) {
				/* XXX: might not be 66MHz */
				i8xx_clock(66000, &clock);
			} else
				i8xx_clock(48000, &clock);
		} else {
			if (dpll & PLL_P1_DIVIDE_BY_TWO)
				clock.p1 = 2;
			else {
				clock.p1 =
				    ((dpll &
				      DPLL_FPA01_P1_POST_DIV_MASK_I830) >>
				     DPLL_FPA01_P1_POST_DIV_SHIFT) + 2;
			}
			if (dpll & PLL_P2_DIVIDE_BY_4)
				clock.p2 = 4;
			else
				clock.p2 = 2;

			i8xx_clock(48000, &clock);
		}
	}

	/* XXX: It would be nice to validate the clocks, but we can't reuse
	 * i830PllIsValid() because it relies on the xf86_config connector
	 * configuration being accurate, which it isn't necessarily.
	 */

	return clock.dot;
}

/** Returns the currently programmed mode of the given pipe. */
struct drm_display_mode *intel_crtc_mode_get(struct drm_device *dev,
					     struct drm_crtc *crtc)
{
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	int pipe = intel_crtc->pipe;
	struct drm_display_mode *mode;
	int htot = REG_READ((pipe == 0) ? HTOTAL_A : HTOTAL_B);
	int hsync = REG_READ((pipe == 0) ? HSYNC_A : HSYNC_B);
	int vtot = REG_READ((pipe == 0) ? VTOTAL_A : VTOTAL_B);
	int vsync = REG_READ((pipe == 0) ? VSYNC_A : VSYNC_B);

	mode = kzalloc(sizeof(*mode), GFP_KERNEL);
	if (!mode)
		return NULL;

	mode->clock = intel_crtc_clock_get(dev, crtc);
	mode->hdisplay = (htot & 0xffff) + 1;
	mode->htotal = ((htot & 0xffff0000) >> 16) + 1;
	mode->hsync_start = (hsync & 0xffff) + 1;
	mode->hsync_end = ((hsync & 0xffff0000) >> 16) + 1;
	mode->vdisplay = (vtot & 0xffff) + 1;
	mode->vtotal = ((vtot & 0xffff0000) >> 16) + 1;
	mode->vsync_start = (vsync & 0xffff) + 1;
	mode->vsync_end = ((vsync & 0xffff0000) >> 16) + 1;

	drm_mode_set_name(mode);
	drm_mode_set_crtcinfo(mode, 0);

	return mode;
}

static void intel_crtc_destroy(struct drm_crtc *crtc)
{
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);

	drm_crtc_cleanup(crtc);
	kfree(intel_crtc);
}

static const struct drm_crtc_helper_funcs intel_helper_funcs = {
	.dpms = intel_crtc_dpms,
	.mode_fixup = intel_crtc_mode_fixup,
	.mode_set = intel_crtc_mode_set,
	.mode_set_base = intel_pipe_set_base,
	.prepare = intel_crtc_prepare,
	.commit = intel_crtc_commit,
};

static const struct drm_crtc_helper_funcs mrst_helper_funcs;

const struct drm_crtc_funcs intel_crtc_funcs = {
	.cursor_set = intel_crtc_cursor_set,
	.cursor_move = intel_crtc_cursor_move,
	.gamma_set = intel_crtc_gamma_set,
	.set_config = drm_crtc_helper_set_config,
	.destroy = intel_crtc_destroy,
};


void intel_crtc_init(struct drm_device *dev, int pipe,
		     struct intel_mode_device *mode_dev)
{
	struct intel_crtc *intel_crtc;
	int i;

#if PRINT_JLIU7
	DRM_INFO("JLIU7 enter intel_crtc_init \n");
#endif				/* PRINT_JLIU7 */

	/* We allocate a extra array of drm_connector pointers
	 * for fbdev after the crtc */
	intel_crtc =
	    kzalloc(sizeof(struct intel_crtc) +
		    (INTELFB_CONN_LIMIT * sizeof(struct drm_connector *)),
		    GFP_KERNEL);
	if (intel_crtc == NULL)
		return;

	drm_crtc_init(dev, &intel_crtc->base, &intel_crtc_funcs);

	drm_mode_crtc_set_gamma_size(&intel_crtc->base, 256);
	intel_crtc->pipe = pipe;
	for (i = 0; i < 256; i++) {
		intel_crtc->lut_r[i] = i;
		intel_crtc->lut_g[i] = i;
		intel_crtc->lut_b[i] = i;
	}

	intel_crtc->mode_dev = mode_dev;
	intel_crtc->cursor_addr = 0;
	intel_crtc->dpms_mode = DRM_MODE_DPMS_OFF;

	if (IS_MRST(dev)) {
		drm_crtc_helper_add(&intel_crtc->base, &mrst_helper_funcs);
	} else {
		drm_crtc_helper_add(&intel_crtc->base,
				    &intel_helper_funcs);
	}

	/* Setup the array of drm_connector pointer array */
	intel_crtc->mode_set.crtc = &intel_crtc->base;
	intel_crtc->mode_set.connectors =
	    (struct drm_connector **) (intel_crtc + 1);
	intel_crtc->mode_set.num_connectors = 0;

#if 0				/* JB: not drop, What should go in here? */
	if (i915_fbpercrtc)
#endif
}

struct drm_crtc *intel_get_crtc_from_pipe(struct drm_device *dev, int pipe)
{
	struct drm_crtc *crtc = NULL;

	list_for_each_entry(crtc, &dev->mode_config.crtc_list, head) {
		struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
		if (intel_crtc->pipe == pipe)
			break;
	}
	return crtc;
}

int intel_connector_clones(struct drm_device *dev, int type_mask)
{
	int index_mask = 0;
	struct drm_connector *connector;
	int entry = 0;

	list_for_each_entry(connector, &dev->mode_config.connector_list,
			    head) {
		struct intel_output *intel_output =
		    to_intel_output(connector);
		if (type_mask & (1 << intel_output->type))
			index_mask |= (1 << entry);
		entry++;
	}
	return index_mask;
}

#if 0				/* JB: Should be per device */
static void intel_setup_outputs(struct drm_device *dev)
{
	struct drm_connector *connector;

	intel_crt_init(dev);

	/* Set up integrated LVDS */
	if (IS_MOBILE(dev) && !IS_I830(dev))
		intel_lvds_init(dev);

	if (IS_I9XX(dev)) {
		intel_sdvo_init(dev, SDVOB);
		intel_sdvo_init(dev, SDVOC);
	} else
		intel_dvo_init(dev);

	if (IS_I9XX(dev) && !IS_I915G(dev))
		intel_tv_init(dev);

	list_for_each_entry(connector, &dev->mode_config.connector_list,
			    head) {
		struct intel_output *intel_output =
		    to_intel_output(connector);
		struct drm_encoder *encoder = &intel_output->enc;
		int crtc_mask = 0, clone_mask = 0;

		/* valid crtcs */
		switch (intel_output->type) {
		case INTEL_OUTPUT_DVO:
		case INTEL_OUTPUT_SDVO:
			crtc_mask = ((1 << 0) | (1 << 1));
			clone_mask = ((1 << INTEL_OUTPUT_ANALOG) |
				      (1 << INTEL_OUTPUT_DVO) |
				      (1 << INTEL_OUTPUT_SDVO));
			break;
		case INTEL_OUTPUT_ANALOG:
			crtc_mask = ((1 << 0) | (1 << 1));
			clone_mask = ((1 << INTEL_OUTPUT_ANALOG) |
				      (1 << INTEL_OUTPUT_DVO) |
				      (1 << INTEL_OUTPUT_SDVO));
			break;
		case INTEL_OUTPUT_LVDS:
			crtc_mask = (1 << 1);
			clone_mask = (1 << INTEL_OUTPUT_LVDS);
			break;
		case INTEL_OUTPUT_TVOUT:
			crtc_mask = ((1 << 0) | (1 << 1));
			clone_mask = (1 << INTEL_OUTPUT_TVOUT);
			break;
		}
		encoder->possible_crtcs = crtc_mask;
		encoder->possible_clones =
		    intel_connector_clones(dev, clone_mask);
	}
}
#endif

#if 0	/* JB: Rework framebuffer code into something none device specific */
static void intel_user_framebuffer_destroy(struct drm_framebuffer *fb)
{
	struct intel_framebuffer *intel_fb = to_intel_framebuffer(fb);
	struct drm_device *dev = fb->dev;

	if (fb->fbdev)
		intelfb_remove(dev, fb);

	drm_framebuffer_cleanup(fb);
	drm_gem_object_unreference(fb->mm_private);

	kfree(intel_fb);
}

static int intel_user_framebuffer_create_handle(struct drm_framebuffer *fb,
						struct drm_file *file_priv,
						unsigned int *handle)
{
	struct drm_gem_object *object = fb->mm_private;

	return drm_gem_handle_create(file_priv, object, handle);
}

static const struct drm_framebuffer_funcs intel_fb_funcs = {
	.destroy = intel_user_framebuffer_destroy,
	.create_handle = intel_user_framebuffer_create_handle,
};

struct drm_framebuffer *intel_framebuffer_create(struct drm_device *dev,
						 struct drm_mode_fb_cmd
						 *mode_cmd,
						 void *mm_private)
{
	struct intel_framebuffer *intel_fb;

	intel_fb = kzalloc(sizeof(*intel_fb), GFP_KERNEL);
	if (!intel_fb)
		return NULL;

	if (!drm_framebuffer_init(dev, &intel_fb->base, &intel_fb_funcs))
		return NULL;

	drm_helper_mode_fill_fb_struct(&intel_fb->base, mode_cmd);

	return &intel_fb->base;
}


static struct drm_framebuffer *intel_user_framebuffer_create(struct
							     drm_device
							     *dev,
							     struct
							     drm_file
							     *filp,
							     struct
							     drm_mode_fb_cmd
							     *mode_cmd)
{
	struct drm_gem_object *obj;

	obj = drm_gem_object_lookup(dev, filp, mode_cmd->handle);
	if (!obj)
		return NULL;

	return intel_framebuffer_create(dev, mode_cmd, obj);
}

static int intel_insert_new_fb(struct drm_device *dev,
			       struct drm_file *file_priv,
			       struct drm_framebuffer *fb,
			       struct drm_mode_fb_cmd *mode_cmd)
{
	struct intel_framebuffer *intel_fb;
	struct drm_gem_object *obj;
	struct drm_crtc *crtc;

	intel_fb = to_intel_framebuffer(fb);

	mutex_lock(&dev->struct_mutex);
	obj = drm_gem_object_lookup(dev, file_priv, mode_cmd->handle);

	if (!obj) {
		mutex_unlock(&dev->struct_mutex);
		return -EINVAL;
	}
	drm_gem_object_unreference(intel_fb->base.mm_private);
	drm_helper_mode_fill_fb_struct(fb, mode_cmd, obj);
	mutex_unlock(&dev->struct_mutex);

	list_for_each_entry(crtc, &dev->mode_config.crtc_list, head) {
		if (crtc->fb == fb) {
			struct drm_crtc_helper_funcs *crtc_funcs =
			    crtc->helper_private;
			crtc_funcs->mode_set_base(crtc, crtc->x, crtc->y);
		}
	}
	return 0;
}

static const struct drm_mode_config_funcs intel_mode_funcs = {
	.resize_fb = intel_insert_new_fb,
	.fb_create = intel_user_framebuffer_create,
	.fb_changed = intelfb_probe,
};
#endif

#if 0				/* Should be per device */
void intel_modeset_init(struct drm_device *dev)
{
	int num_pipe;
	int i;

	drm_mode_config_init(dev);

	dev->mode_config.min_width = 0;
	dev->mode_config.min_height = 0;

	dev->mode_config.funcs = (void *) &intel_mode_funcs;

	if (IS_I965G(dev)) {
		dev->mode_config.max_width = 8192;
		dev->mode_config.max_height = 8192;
	} else {
		dev->mode_config.max_width = 2048;
		dev->mode_config.max_height = 2048;
	}

	/* set memory base */
	if (IS_I9XX(dev))
		dev->mode_config.fb_base =
		    pci_resource_start(dev->pdev, 2);
	else
		dev->mode_config.fb_base =
		    pci_resource_start(dev->pdev, 0);

	if (IS_MOBILE(dev) || IS_I9XX(dev))
		num_pipe = 2;
	else
		num_pipe = 1;
	DRM_DEBUG("%d display pipe%s available.\n",
		  num_pipe, num_pipe > 1 ? "s" : "");

	for (i = 0; i < num_pipe; i++)
		intel_crtc_init(dev, i);

	intel_setup_outputs(dev);

	/* setup fbs */
	/* drm_initial_config(dev, false); */
}
#endif

void intel_modeset_cleanup(struct drm_device *dev)
{
	drm_mode_config_cleanup(dev);
}


/* current intel driver doesn't take advantage of encoders
   always give back the encoder for the connector
*/
struct drm_encoder *intel_best_encoder(struct drm_connector *connector)
{
	struct intel_output *intel_output = to_intel_output(connector);

	return &intel_output->enc;
}

/* MRST_PLATFORM start */

#if DUMP_REGISTER
void dump_dc_registers(struct drm_device *dev)
{
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	unsigned int i = 0;

	DRM_INFO("jliu7 dump_dc_registers\n");


	if (0x80000000 & REG_READ(0x70008)) {
		for (i = 0x20a0; i < 0x20af; i += 4) {
			DRM_INFO("jliu7 interrupt register=0x%x, value=%x\n", i,		 (unsigned int) REG_READ(i));
		}

		for (i = 0xf014; i < 0xf047; i += 4) {
			DRM_INFO
			    ("jliu7 pipe A dpll register=0x%x, value=%x\n",
			     i, (unsigned int) REG_READ(i));
		}

		for (i = 0x60000; i < 0x6005f; i += 4) {
			DRM_INFO
			    ("jliu7 pipe A timing register=0x%x, value=%x\n",
			     i, (unsigned int) REG_READ(i));
		}

		for (i = 0x61140; i < 0x61143; i += 4) {
			DRM_INFO("jliu7 SDBOB register=0x%x, value=%x\n",
				 i, (unsigned int) REG_READ(i));
		}

		for (i = 0x61180; i < 0x6123F; i += 4) {
			DRM_INFO
			    ("jliu7 LVDS PORT register=0x%x, value=%x\n",
			     i, (unsigned int) REG_READ(i));
		}

		for (i = 0x61254; i < 0x612AB; i += 4) {
			DRM_INFO("jliu7 BLC register=0x%x, value=%x\n",
				 i, (unsigned int) REG_READ(i));
		}

		for (i = 0x70000; i < 0x70047; i += 4) {
			DRM_INFO
			    ("jliu7 PIPE A control register=0x%x, value=%x\n",
			     i, (unsigned int) REG_READ(i));
		}

		for (i = 0x70180; i < 0x7020b; i += 4) {
			DRM_INFO("jliu7 display A control register=0x%x,"
				 "value=%x\n", i,
				 (unsigned int) REG_READ(i));
		}

		for (i = 0x71400; i < 0x71403; i += 4) {
			DRM_INFO
			    ("jliu7 VGA Display Plane Control register=0x%x,"
			     "value=%x\n", i, (unsigned int) REG_READ(i));
		}
	}

	if (0x80000000 & REG_READ(0x71008)) {
		for (i = 0x61000; i < 0x6105f; i += 4) {
			DRM_INFO
			    ("jliu7 pipe B timing register=0x%x, value=%x\n",
			     i, (unsigned int) REG_READ(i));
		}

		for (i = 0x71000; i < 0x71047; i += 4) {
			DRM_INFO
			    ("jliu7 PIPE B control register=0x%x, value=%x\n",
			     i, (unsigned int) REG_READ(i));
		}

		for (i = 0x71180; i < 0x7120b; i += 4) {
			DRM_INFO("jliu7 display B control register=0x%x,"
				 "value=%x\n", i,
				 (unsigned int) REG_READ(i));
		}
	}
#if 0
	for (i = 0x70080; i < 0x700df; i += 4) {
		DRM_INFO("jliu7 cursor A & B register=0x%x, value=%x\n",
			 i, (unsigned int) REG_READ(i));
	}
#endif

}

void dump_dsi_registers(struct drm_device *dev)
{
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	unsigned int i = 0;

	DRM_INFO("jliu7 dump_dsi_registers\n");

	for (i = 0xb000; i < 0xb064; i += 4) {
		DRM_INFO("jliu7 MIPI IP register=0x%x, value=%x\n", i,
			 (unsigned int) REG_READ(i));
	}

	i = 0xb104;
	DRM_INFO("jliu7 MIPI control register=0x%x, value=%x\n",
		 i, (unsigned int) REG_READ(i));
}
#endif				/* DUMP_REGISTER */


struct mrst_limit_t {
	struct intel_range_t dot, m, p1;
};

struct mrst_clock_t {
	/* derived values */
	int dot;
	int m;
	int p1;
};

#define MRST_LIMIT_LVDS_100L	    0
#define MRST_LIMIT_LVDS_83	    1
#define MRST_LIMIT_LVDS_100	    2

#define MRST_DOT_MIN		  19750
#define MRST_DOT_MAX		  120000
#define MRST_M_MIN_100L		    20
#define MRST_M_MIN_100		    10
#define MRST_M_MIN_83		    12
#define MRST_M_MAX_100L		    34
#define MRST_M_MAX_100		    17
#define MRST_M_MAX_83		    20
#define MRST_P1_MIN		    2
#define MRST_P1_MAX_0		    7
#define MRST_P1_MAX_1		    8

static const struct mrst_limit_t mrst_limits[] = {
	{			/* MRST_LIMIT_LVDS_100L */
	 .dot = {.min = MRST_DOT_MIN, .max = MRST_DOT_MAX},
	 .m = {.min = MRST_M_MIN_100L, .max = MRST_M_MAX_100L},
	 .p1 = {.min = MRST_P1_MIN, .max = MRST_P1_MAX_1},
	 },
	{			/* MRST_LIMIT_LVDS_83L */
	 .dot = {.min = MRST_DOT_MIN, .max = MRST_DOT_MAX},
	 .m = {.min = MRST_M_MIN_83, .max = MRST_M_MAX_83},
	 .p1 = {.min = MRST_P1_MIN, .max = MRST_P1_MAX_0},
	 },
	{			/* MRST_LIMIT_LVDS_100 */
	 .dot = {.min = MRST_DOT_MIN, .max = MRST_DOT_MAX},
	 .m = {.min = MRST_M_MIN_100, .max = MRST_M_MAX_100},
	 .p1 = {.min = MRST_P1_MIN, .max = MRST_P1_MAX_1},
	 },
};

#define MRST_M_MIN	    10
static const u32 mrst_m_converts[] = {
	0x2B, 0x15, 0x2A, 0x35, 0x1A, 0x0D, 0x26, 0x33, 0x19, 0x2C,
	0x36, 0x3B, 0x1D, 0x2E, 0x37, 0x1B, 0x2D, 0x16, 0x0B, 0x25,
	0x12, 0x09, 0x24, 0x32, 0x39, 0x1c,
};

#define COUNT_MAX 0x10000000
void mrstWaitForPipeDisable(struct drm_device *dev)
{
	int count, temp;

	/* FIXME JLIU7_PO */
	intel_wait_for_vblank(dev);
	return;

	/* Wait for for the pipe disable to take effect. */
	for (count = 0; count < COUNT_MAX; count++) {
		temp = REG_READ(PIPEACONF);
		if ((temp & PIPEACONF_PIPE_STATE) == 0)
			break;
	}

	if (count == COUNT_MAX) {
#if PRINT_JLIU7
		DRM_INFO("JLIU7 mrstWaitForPipeDisable time out. \n");
#endif				/* PRINT_JLIU7 */
	} else {
#if PRINT_JLIU7
		DRM_INFO("JLIU7 mrstWaitForPipeDisable cout = %d. \n",
			 count);
#endif				/* PRINT_JLIU7 */
	}
}

void mrstWaitForPipeEnable(struct drm_device *dev)
{
	int count, temp;

	/* FIXME JLIU7_PO */
	intel_wait_for_vblank(dev);
	return;

	/* Wait for for the pipe disable to take effect. */
	for (count = 0; count < COUNT_MAX; count++) {
		temp = REG_READ(PIPEACONF);
		if ((temp & PIPEACONF_PIPE_STATE) == 1)
			break;
	}

	if (count == COUNT_MAX) {
#if PRINT_JLIU7
		DRM_INFO("JLIU7 mrstWaitForPipeEnable time out. \n");
#endif				/* PRINT_JLIU7 */
	} else {
#if PRINT_JLIU7
		DRM_INFO("JLIU7 mrstWaitForPipeEnable cout = %d. \n",
			 count);
#endif				/* PRINT_JLIU7 */
	}
}

static const struct mrst_limit_t *mrst_limit(struct drm_crtc *crtc)
{
	const struct mrst_limit_t *limit;
	struct drm_device *dev = crtc->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;

	if (intel_pipe_has_type(crtc, INTEL_OUTPUT_LVDS)
	    || intel_pipe_has_type(crtc, INTEL_OUTPUT_MIPI)) {
		if (dev_priv->sku_100L)
			limit = &mrst_limits[MRST_LIMIT_LVDS_100L];
		if (dev_priv->sku_83)
			limit = &mrst_limits[MRST_LIMIT_LVDS_83];
		if (dev_priv->sku_100)
			limit = &mrst_limits[MRST_LIMIT_LVDS_100];
	} else {
		limit = NULL;
#if PRINT_JLIU7
		DRM_INFO("JLIU7 jliu7 mrst_limit Wrong display type. \n");
#endif				/* PRINT_JLIU7 */
	}

	return limit;
}

/** Derive the pixel clock for the given refclk and divisors for 8xx chips. */
static void mrst_clock(int refclk, struct mrst_clock_t *clock)
{
	clock->dot = (refclk * clock->m) / (14 * clock->p1);
}

void mrstPrintPll(char *prefix, struct mrst_clock_t *clock)
{
#if PRINT_JLIU7
	DRM_INFO
	    ("JLIU7 mrstPrintPll %s: dotclock = %d,  m = %d, p1 = %d. \n",
	     prefix, clock->dot, clock->m, clock->p1);
#endif				/* PRINT_JLIU7 */
}

/**
 * Returns a set of divisors for the desired target clock with the given refclk,
 * or FALSE.  Divisor values are the actual divisors for
 */
static bool
mrstFindBestPLL(struct drm_crtc *crtc, int target, int refclk,
		struct mrst_clock_t *best_clock)
{
	struct mrst_clock_t clock;
	const struct mrst_limit_t *limit = mrst_limit(crtc);
	int err = target;

	memset(best_clock, 0, sizeof(*best_clock));

	for (clock.m = limit->m.min; clock.m <= limit->m.max; clock.m++) {
		for (clock.p1 = limit->p1.min; clock.p1 <= limit->p1.max;
		     clock.p1++) {
			int this_err;

			mrst_clock(refclk, &clock);

			this_err = abs(clock.dot - target);
			if (this_err < err) {
				*best_clock = clock;
				err = this_err;
			}
		}
	}
	DRM_DEBUG("mrstFindBestPLL err = %d.\n", err);

	return err != target;
}

/**
 * Sets the power management mode of the pipe and plane.
 *
 * This code should probably grow support for turning the cursor off and back
 * on appropriately at the same time as we're turning the pipe off/on.
 */
static void mrst_crtc_dpms(struct drm_crtc *crtc, int mode)
{
	struct drm_device *dev = crtc->dev;
	/* struct drm_i915_master_private *master_priv; */
	/* struct drm_i915_private *dev_priv = dev->dev_private; */
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	int pipe = intel_crtc->pipe;
	int dpll_reg = (pipe == 0) ? MRST_DPLL_A : DPLL_B;
	int dspcntr_reg = (pipe == 0) ? DSPACNTR : DSPBCNTR;
	int dspbase_reg = (pipe == 0) ? MRST_DSPABASE : DSPBBASE;
	int pipeconf_reg = (pipe == 0) ? PIPEACONF : PIPEBCONF;
	u32 temp;
	bool enabled;

#if PRINT_JLIU7
	DRM_INFO("JLIU7 enter mrst_crtc_dpms, mode = %d, pipe = %d \n",
		 mode, pipe);
#endif				/* PRINT_JLIU7 */

	/* XXX: When our outputs are all unaware of DPMS modes other than off
	 * and on, we should map those modes to DRM_MODE_DPMS_OFF in the CRTC.
	 */
	switch (mode) {
	case DRM_MODE_DPMS_ON:
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
		/* Enable the DPLL */
		temp = REG_READ(dpll_reg);
		if ((temp & DPLL_VCO_ENABLE) == 0) {
			REG_WRITE(dpll_reg, temp);
			REG_READ(dpll_reg);
			/* Wait for the clocks to stabilize. */
			udelay(150);
			REG_WRITE(dpll_reg, temp | DPLL_VCO_ENABLE);
			REG_READ(dpll_reg);
			/* Wait for the clocks to stabilize. */
			udelay(150);
			REG_WRITE(dpll_reg, temp | DPLL_VCO_ENABLE);
			REG_READ(dpll_reg);
			/* Wait for the clocks to stabilize. */
			udelay(150);
		}

		/* Enable the pipe */
		temp = REG_READ(pipeconf_reg);
		if ((temp & PIPEACONF_ENABLE) == 0)
			REG_WRITE(pipeconf_reg, temp | PIPEACONF_ENABLE);

		/* Enable the plane */
		temp = REG_READ(dspcntr_reg);
		if ((temp & DISPLAY_PLANE_ENABLE) == 0) {
			REG_WRITE(dspcntr_reg,
				  temp | DISPLAY_PLANE_ENABLE);
			/* Flush the plane changes */
			REG_WRITE(dspbase_reg, REG_READ(dspbase_reg));
		}

		intel_crtc_load_lut(crtc);

		/* Give the overlay scaler a chance to enable
		   if it's on this pipe */
		/* intel_crtc_dpms_video(crtc, true); TODO */
		break;
	case DRM_MODE_DPMS_OFF:
		/* Give the overlay scaler a chance to disable
		 * if it's on this pipe */
		/* intel_crtc_dpms_video(crtc, FALSE); TODO */

		/* Disable the VGA plane that we never use */
		REG_WRITE(VGACNTRL, VGA_DISP_DISABLE);

		/* Disable display plane */
		temp = REG_READ(dspcntr_reg);
		if ((temp & DISPLAY_PLANE_ENABLE) != 0) {
			REG_WRITE(dspcntr_reg,
				  temp & ~DISPLAY_PLANE_ENABLE);
			/* Flush the plane changes */
			REG_WRITE(dspbase_reg, REG_READ(dspbase_reg));
			REG_READ(dspbase_reg);
		}

		if (!IS_I9XX(dev)) {
			/* Wait for vblank for the disable to take effect */
			intel_wait_for_vblank(dev);
		}

		/* Next, disable display pipes */
		temp = REG_READ(pipeconf_reg);
		if ((temp & PIPEACONF_ENABLE) != 0) {
			REG_WRITE(pipeconf_reg, temp & ~PIPEACONF_ENABLE);
			REG_READ(pipeconf_reg);
		}

		/* Wait for for the pipe disable to take effect. */
		mrstWaitForPipeDisable(dev);

		temp = REG_READ(dpll_reg);
		if ((temp & DPLL_VCO_ENABLE) != 0) {
			REG_WRITE(dpll_reg, temp & ~DPLL_VCO_ENABLE);
			REG_READ(dpll_reg);
		}

		/* Wait for the clocks to turn off. */
		udelay(150);
		break;
	}

#if DUMP_REGISTER
	dump_dc_registers(dev);
#endif				/* DUMP_REGISTER */

	enabled = crtc->enabled && mode != DRM_MODE_DPMS_OFF;

#if 0				/* JB: Add vblank support later */
	if (enabled)
		dev_priv->vblank_pipe |= (1 << pipe);
	else
		dev_priv->vblank_pipe &= ~(1 << pipe);
#endif

	intel_crtc->dpms_mode = mode;

#if 0				/* JB: Add sarea support later */
	if (!dev->primary->master)
		return;

	master_priv = dev->primary->master->driver_priv;
	if (!master_priv->sarea_priv)
		return;

	switch (pipe) {
	case 0:
		master_priv->sarea_priv->planeA_w =
		    enabled ? crtc->mode.hdisplay : 0;
		master_priv->sarea_priv->planeA_h =
		    enabled ? crtc->mode.vdisplay : 0;
		break;
	case 1:
		master_priv->sarea_priv->planeB_w =
		    enabled ? crtc->mode.hdisplay : 0;
		master_priv->sarea_priv->planeB_h =
		    enabled ? crtc->mode.vdisplay : 0;
		break;
	default:
		DRM_ERROR("Can't update pipe %d in SAREA\n", pipe);
		break;
	}
#endif
}

static int mrst_crtc_mode_set(struct drm_crtc *crtc,
			      struct drm_display_mode *mode,
			      struct drm_display_mode *adjusted_mode,
			      int x, int y,
			      struct drm_framebuffer *old_fb)
{
	struct drm_device *dev = crtc->dev;
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	int pipe = intel_crtc->pipe;
	int fp_reg = (pipe == 0) ? MRST_FPA0 : FPB0;
	int dpll_reg = (pipe == 0) ? MRST_DPLL_A : DPLL_B;
	int dspcntr_reg = (pipe == 0) ? DSPACNTR : DSPBCNTR;
	int pipeconf_reg = (pipe == 0) ? PIPEACONF : PIPEBCONF;
	int htot_reg = (pipe == 0) ? HTOTAL_A : HTOTAL_B;
	int hblank_reg = (pipe == 0) ? HBLANK_A : HBLANK_B;
	int hsync_reg = (pipe == 0) ? HSYNC_A : HSYNC_B;
	int vtot_reg = (pipe == 0) ? VTOTAL_A : VTOTAL_B;
	int vblank_reg = (pipe == 0) ? VBLANK_A : VBLANK_B;
	int vsync_reg = (pipe == 0) ? VSYNC_A : VSYNC_B;
	int dspsize_reg = (pipe == 0) ? DSPASIZE : DSPBSIZE;
	int pipesrc_reg = (pipe == 0) ? PIPEASRC : PIPEBSRC;
	int refclk = 0;
	struct mrst_clock_t clock;
	u32 dpll = 0, fp = 0, dspcntr, pipeconf, lvdsport;
	bool ok, is_sdvo = false;
	bool is_crt = false, is_lvds = false, is_tv = false;
	bool is_mipi = false;
	struct drm_mode_config *mode_config = &dev->mode_config;
	struct drm_connector *connector;
	struct intel_output *intel_output;

#if PRINT_JLIU7
	DRM_INFO("JLIU7 enter mrst_crtc_mode_set \n");
#endif				/* PRINT_JLIU7 */

	list_for_each_entry(connector, &mode_config->connector_list, head) {
		intel_output = to_intel_output(connector);

		if (!connector->encoder
		    || connector->encoder->crtc != crtc)
			continue;

		switch (intel_output->type) {
		case INTEL_OUTPUT_LVDS:
			is_lvds = true;
			break;
		case INTEL_OUTPUT_SDVO:
			is_sdvo = true;
			break;
		case INTEL_OUTPUT_TVOUT:
			is_tv = true;
			break;
		case INTEL_OUTPUT_ANALOG:
			is_crt = true;
			break;
		case INTEL_OUTPUT_MIPI:
			is_mipi = true;
			break;
		}
	}

	if (is_lvds | is_mipi) {
		/*FIXME JLIU7 Get panel power delay parameters from
		  config data */
		REG_WRITE(0x61208, 0x25807d0);
		REG_WRITE(0x6120c, 0x1f407d0);
		REG_WRITE(0x61210, 0x270f04);
	}

	/* Disable the VGA plane that we never use */
	REG_WRITE(VGACNTRL, VGA_DISP_DISABLE);

	/* Disable the panel fitter if it was on our pipe */
	if (intel_panel_fitter_pipe(dev) == pipe)
		REG_WRITE(PFIT_CONTROL, 0);

	DRM_DEBUG("Mode for pipe %c:\n", pipe == 0 ? 'A' : 'B');
	drm_mode_debug_printmodeline(mode);

	REG_WRITE(htot_reg, (adjusted_mode->crtc_hdisplay - 1) |
		  ((adjusted_mode->crtc_htotal - 1) << 16));
	REG_WRITE(hblank_reg, (adjusted_mode->crtc_hblank_start - 1) |
		  ((adjusted_mode->crtc_hblank_end - 1) << 16));
	REG_WRITE(hsync_reg, (adjusted_mode->crtc_hsync_start - 1) |
		  ((adjusted_mode->crtc_hsync_end - 1) << 16));
	REG_WRITE(vtot_reg, (adjusted_mode->crtc_vdisplay - 1) |
		  ((adjusted_mode->crtc_vtotal - 1) << 16));
	REG_WRITE(vblank_reg, (adjusted_mode->crtc_vblank_start - 1) |
		  ((adjusted_mode->crtc_vblank_end - 1) << 16));
	REG_WRITE(vsync_reg, (adjusted_mode->crtc_vsync_start - 1) |
		  ((adjusted_mode->crtc_vsync_end - 1) << 16));
	/* pipesrc and dspsize control the size that is scaled from,
	 * which should always be the user's requested size.
	 */
	REG_WRITE(dspsize_reg,
		  ((mode->vdisplay - 1) << 16) | (mode->hdisplay - 1));
	REG_WRITE(pipesrc_reg,
		  ((mode->hdisplay - 1) << 16) | (mode->vdisplay - 1));

	/* Flush the plane changes */
	{
		struct drm_crtc_helper_funcs *crtc_funcs =
		    crtc->helper_private;
		crtc_funcs->mode_set_base(crtc, x, y, old_fb);
	}

	/* setup pipeconf */
	pipeconf = REG_READ(pipeconf_reg);

	/* Set up the display plane register */
 	dspcntr = REG_READ(dspcntr_reg);
	dspcntr |= DISPPLANE_GAMMA_ENABLE;

	if (pipe == 0)
		dspcntr |= DISPPLANE_SEL_PIPE_A;
	else
		dspcntr |= DISPPLANE_SEL_PIPE_B;

	dev_priv->dspcntr = dspcntr |= DISPLAY_PLANE_ENABLE;
	dev_priv->pipeconf = pipeconf |= PIPEACONF_ENABLE;

	if (is_mipi)
		return 0;

	if (dev_priv->sku_100L)
		refclk = 100000;
	else if (dev_priv->sku_83)
		refclk = 166000;
	else if (dev_priv->sku_100)
		refclk = 200000;

	dpll = 0;		/*BIT16 = 0 for 100MHz reference */

	ok = mrstFindBestPLL(crtc, adjusted_mode->clock, refclk, &clock);

	if (!ok) {
#if 0				/* FIXME JLIU7 */
		DRM_ERROR("Couldn't find PLL settings for mode!\n");
		return;
#endif				/* FIXME JLIU7 */
#if PRINT_JLIU7
		DRM_INFO
		    ("JLIU7 mrstFindBestPLL fail in mrst_crtc_mode_set. \n");
#endif				/* PRINT_JLIU7 */
	} else {
#if PRINT_JLIU7
		DRM_INFO("JLIU7 mrst_crtc_mode_set pixel clock = %d,"
			 "m = %x, p1 = %x. \n", clock.dot, clock.m,
			 clock.p1);
#endif				/* PRINT_JLIU7 */
	}

	fp = mrst_m_converts[(clock.m - MRST_M_MIN)] << 8;

	dpll |= DPLL_VGA_MODE_DIS;


	dpll |= DPLL_VCO_ENABLE;

	if (is_lvds)
		dpll |= DPLLA_MODE_LVDS;
	else
		dpll |= DPLLB_MODE_DAC_SERIAL;

	if (is_sdvo) {
		int sdvo_pixel_multiply =
		    adjusted_mode->clock / mode->clock;

		dpll |= DPLL_DVO_HIGH_SPEED;
		dpll |=
		    (sdvo_pixel_multiply -
		     1) << SDVO_MULTIPLIER_SHIFT_HIRES;
	}


	/* compute bitmask from p1 value */
	dpll |= (1 << (clock.p1 - 2)) << 17;

	dpll |= DPLL_VCO_ENABLE;

#if PRINT_JLIU7
	mrstPrintPll("chosen", &clock);
#endif				/* PRINT_JLIU7 */

#if 0
	if (!xf86ModesEqual(mode, adjusted_mode)) {
		xf86DrvMsg(pScrn->scrnIndex, X_INFO,
			   "Adjusted mode for pipe %c:\n",
			   pipe == 0 ? 'A' : 'B');
		xf86PrintModeline(pScrn->scrnIndex, mode);
	}
	i830PrintPll("chosen", &clock);
#endif

	if (dpll & DPLL_VCO_ENABLE) {
		REG_WRITE(fp_reg, fp);
		REG_WRITE(dpll_reg, dpll & ~DPLL_VCO_ENABLE);
		REG_READ(dpll_reg);
/* FIXME jliu7 check the DPLLA lock bit PIPEACONF[29] */
		udelay(150);
	}

	/* The LVDS pin pair needs to be on before the DPLLs are enabled.
	 * This is an exception to the general rule that mode_set doesn't turn
	 * things on.
	 */
	if (is_lvds) {

		/* FIXME JLIU7 need to support 24bit panel */
#if MRST_24BIT_LVDS
		lvdsport =
		    (REG_READ(LVDS) & (~LVDS_PIPEB_SELECT)) | LVDS_PORT_EN
		    | LVDS_A3_POWER_UP | LVDS_A0A2_CLKA_POWER_UP;

#if MRST_24BIT_DOT_1
		lvdsport |= MRST_PANEL_24_DOT_1_FORMAT;
#endif				/* MRST_24BIT_DOT_1 */

#else				/* MRST_24BIT_LVDS */
		lvdsport =
		    (REG_READ(LVDS) & (~LVDS_PIPEB_SELECT)) | LVDS_PORT_EN;
#endif				/* MRST_24BIT_LVDS */

#if MRST_24BIT_WA
		lvdsport = 0x80300340;
#else /* MRST_24BIT_DOT_WA */
		lvdsport = 0x82300300;
#endif				/* MRST_24BIT_DOT_WA */

		REG_WRITE(LVDS, lvdsport);
		REG_READ(LVDS);
	}

	REG_WRITE(fp_reg, fp);
	REG_WRITE(dpll_reg, dpll);
	REG_READ(dpll_reg);
	/* Wait for the clocks to stabilize. */
	udelay(150);

	/* write it again -- the BIOS does, after all */
	REG_WRITE(dpll_reg, dpll);
	REG_READ(dpll_reg);
	/* Wait for the clocks to stabilize. */
	udelay(150);

	REG_WRITE(pipeconf_reg, pipeconf);
	REG_READ(pipeconf_reg);

	/* Wait for for the pipe enable to take effect. */
	mrstWaitForPipeEnable(dev);

	REG_WRITE(dspcntr_reg, dspcntr);
	intel_wait_for_vblank(dev);

	return 0;
}


static const struct drm_crtc_helper_funcs mrst_helper_funcs = {
	.dpms = mrst_crtc_dpms,
	.mode_fixup = intel_crtc_mode_fixup,
	.mode_set = mrst_crtc_mode_set,
	.mode_set_base = intel_pipe_set_base,
	.prepare = intel_crtc_prepare,
	.commit = intel_crtc_commit,
};

/* MRST_PLATFORM end */
