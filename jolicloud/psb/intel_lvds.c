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
#include <linux/backlight.h>
#include "drm_crtc.h"
#include "drm_edid.h"
#include "intel_lvds.h"

#include <acpi/acpi_drivers.h>

int drm_intel_ignore_acpi = 0;
MODULE_PARM_DESC(ignore_acpi, "Ignore ACPI");
module_param_named(ignore_acpi, drm_intel_ignore_acpi, int, 0600);

uint8_t blc_type;
uint8_t blc_pol;
uint8_t blc_freq;
uint8_t blc_minbrightness;
uint8_t blc_i2caddr;
uint8_t blc_brightnesscmd;
int lvds_backlight;	/* restore backlight to this value */

struct intel_i2c_chan *lvds_i2c_bus; 
u32 CoreClock;
u32 PWMControlRegFreq;

unsigned char * dev_OpRegion = NULL;
unsigned int dev_OpRegionSize;

#define PCI_PORT5_REG80_FFUSE				0xD0058000
#define PCI_PORT5_REG80_MAXRES_INT_EN		0x0040
#define MAX_HDISPLAY 800
#define MAX_VDISPLAY 480
bool sku_bMaxResEnableInt = false;

/** Set BLC through I2C*/
static int
LVDSI2CSetBacklight(struct drm_device *dev, unsigned char ch)
{
	u8 out_buf[2];
	struct i2c_msg msgs[] = {
		{ 
			.addr = lvds_i2c_bus->slave_addr,
			.flags = 0,
			.len = 2,
			.buf = out_buf,
		}
	};

	DRM_INFO("LVDSI2CSetBacklight: the slave_addr is 0x%x, the backlight value is %d\n", lvds_i2c_bus->slave_addr, ch);

	out_buf[0] = blc_brightnesscmd;
	out_buf[1] = ch;

	if (i2c_transfer(&lvds_i2c_bus->adapter, msgs, 1) == 1)
	{
		DRM_INFO("LVDSI2CSetBacklight: i2c_transfer done\n");
		return true;
	}

	DRM_ERROR("msg: i2c_transfer error\n");
	return false;
}

/**
 * Calculate PWM control register value.
 */
static int 
LVDSCalculatePWMCtrlRegFreq(struct drm_device *dev)
{
	unsigned long value = 0;

	DRM_INFO("Enter LVDSCalculatePWMCtrlRegFreq.\n");
	if (blc_freq == 0) {
		DRM_ERROR("LVDSCalculatePWMCtrlRegFreq:  Frequency Requested is 0.\n");
		return FALSE;
	}
	value = (CoreClock * MHz);
	value = (value / BLC_PWM_FREQ_CALC_CONSTANT);
	value = (value * BLC_PWM_PRECISION_FACTOR);
	value = (value / blc_freq);
	value = (value / BLC_PWM_PRECISION_FACTOR);

	if (value > (unsigned long)BLC_MAX_PWM_REG_FREQ ||
			value < (unsigned long)BLC_MIN_PWM_REG_FREQ) {
		return FALSE;
	} else {
		PWMControlRegFreq = ((u32)value & ~BLC_PWM_LEGACY_MODE_ENABLE);
		return TRUE;
	}
}

/**
 * Returns the maximum level of the backlight duty cycle field.
 */
static u32
LVDSGetPWMMaxBacklight(struct drm_device *dev)
{
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	u32 max_pwm_blc = 0;

	max_pwm_blc = ((I915_READ(BLC_PWM_CTL) & BACKLIGHT_MODULATION_FREQ_MASK) >> \
			BACKLIGHT_MODULATION_FREQ_SHIFT) * 2;

	if (!(max_pwm_blc & BLC_MAX_PWM_REG_FREQ)) {
		if (LVDSCalculatePWMCtrlRegFreq(dev)) {
			max_pwm_blc = PWMControlRegFreq;
		}
	}

	DRM_INFO("LVDSGetPWMMaxBacklight: the max_pwm_blc is %d.\n", max_pwm_blc);
	return max_pwm_blc;
}


/**
 * Sets the backlight level.
 *
 * \param level backlight level, from 0 to intel_lvds_get_max_backlight().
 */
static void intel_lvds_set_backlight(struct drm_device *dev, int level)
{
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	//u32 blc_pwm_ctl;

	/*           
     	blc_pwm_ctl = I915_READ(BLC_PWM_CTL) & ~BACKLIGHT_DUTY_CYCLE_MASK;
	I915_WRITE(BLC_PWM_CTL, (blc_pwm_ctl |
		(level << BACKLIGHT_DUTY_CYCLE_SHIFT)));
	 */
	u32 newbacklight = 0;

	DRM_INFO("intel_lvds_set_backlight: the level is %d\n", level);

	if(blc_type == BLC_I2C_TYPE){
		newbacklight = BRIGHTNESS_MASK & ((unsigned long)level * \
				BRIGHTNESS_MASK /BRIGHTNESS_MAX_LEVEL);

		if (blc_pol == BLC_POLARITY_INVERSE) {
			newbacklight = BRIGHTNESS_MASK - newbacklight;
		}

		LVDSI2CSetBacklight(dev, newbacklight);

	} else if (blc_type == BLC_PWM_TYPE) {
		u32 max_pwm_blc = LVDSGetPWMMaxBacklight(dev);

		u32 blc_pwm_duty_cycle;

		/* Provent LVDS going to total black */
		if ( level < 20) {
			level = 20;
		}
		blc_pwm_duty_cycle = level * max_pwm_blc/BRIGHTNESS_MAX_LEVEL;

		if (blc_pol == BLC_POLARITY_INVERSE) {
			blc_pwm_duty_cycle = max_pwm_blc - blc_pwm_duty_cycle;
		}

		blc_pwm_duty_cycle &= BACKLIGHT_PWM_POLARITY_BIT_CLEAR;

		I915_WRITE(BLC_PWM_CTL,
				(max_pwm_blc << BACKLIGHT_PWM_CTL_SHIFT)| (blc_pwm_duty_cycle));
	}
}

/**
 * Returns the maximum level of the backlight duty cycle field.
 */
static u32 intel_lvds_get_max_backlight(struct drm_device *dev)
{
	return BRIGHTNESS_MAX_LEVEL;
	/*
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
    
	return ((I915_READ(BLC_PWM_CTL) & BACKLIGHT_MODULATION_FREQ_MASK) >>
		BACKLIGHT_MODULATION_FREQ_SHIFT) * 2;
	*/
}

/**
 * Sets the power state for the panel.
 */
static void intel_lvds_set_power(struct drm_device *dev, bool on)
{
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	u32 pp_status;

	DRM_INFO("intel_lvds_set_power: %d\n", on);
	if (on) {
		I915_WRITE(PP_CONTROL, I915_READ(PP_CONTROL) |
				POWER_TARGET_ON);
		do {
			pp_status = I915_READ(PP_STATUS);
		} while ((pp_status & PP_ON) == 0);

		intel_lvds_set_backlight(dev, lvds_backlight);
	} else {
		intel_lvds_set_backlight(dev, 0);

		I915_WRITE(PP_CONTROL, I915_READ(PP_CONTROL) &
				~POWER_TARGET_ON);
		do {
			pp_status = I915_READ(PP_STATUS);
		} while (pp_status & PP_ON);
	}
}

static void intel_lvds_dpms(struct drm_output *output, int mode)
{
	struct drm_device *dev = output->dev;

	DRM_INFO("intel_lvds_dpms: the mode is %d\n", mode);
	if (mode == DPMSModeOn)
		intel_lvds_set_power(dev, true);
	else
		intel_lvds_set_power(dev, false);

	/* XXX: We never power down the LVDS pairs. */
}

static void intel_lvds_save(struct drm_output *output)
{
	struct drm_device *dev = output->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;

	dev_priv->savePP_ON = I915_READ(LVDSPP_ON);
	dev_priv->savePP_OFF = I915_READ(LVDSPP_OFF);
	dev_priv->savePP_CONTROL = I915_READ(PP_CONTROL);
	dev_priv->savePP_CYCLE = I915_READ(PP_CYCLE);
	dev_priv->saveBLC_PWM_CTL = I915_READ(BLC_PWM_CTL);
	dev_priv->backlight_duty_cycle = (dev_priv->saveBLC_PWM_CTL &
				       BACKLIGHT_DUTY_CYCLE_MASK);

	/*
	 * If the light is off at server startup, just make it full brightness
	 */
	if (dev_priv->backlight_duty_cycle == 0)
		lvds_backlight=
			intel_lvds_get_max_backlight(dev);
}

static void intel_lvds_restore(struct drm_output *output)
{
	struct drm_device *dev = output->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;

	I915_WRITE(BLC_PWM_CTL, dev_priv->saveBLC_PWM_CTL);
	I915_WRITE(LVDSPP_ON, dev_priv->savePP_ON);
	I915_WRITE(LVDSPP_OFF, dev_priv->savePP_OFF);
	I915_WRITE(PP_CYCLE, dev_priv->savePP_CYCLE);
	I915_WRITE(PP_CONTROL, dev_priv->savePP_CONTROL);
	if (dev_priv->savePP_CONTROL & POWER_TARGET_ON)
		intel_lvds_set_power(dev, true);
	else
		intel_lvds_set_power(dev, false);
}

static int intel_lvds_mode_valid(struct drm_output *output,
				 struct drm_display_mode *mode)
{
	struct drm_device *dev = output->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	struct drm_display_mode *fixed_mode = dev_priv->panel_fixed_mode;

	if (fixed_mode)	{
		if (mode->hdisplay > fixed_mode->hdisplay)
			return MODE_PANEL;
		if (mode->vdisplay > fixed_mode->vdisplay)
			return MODE_PANEL;
	}

	if (IS_POULSBO(dev) && sku_bMaxResEnableInt) {
		if (mode->hdisplay > MAX_HDISPLAY)
			return MODE_PANEL;
		if (mode->vdisplay > MAX_VDISPLAY)
			return MODE_PANEL;
	}

	return MODE_OK;
}

static bool intel_lvds_mode_fixup(struct drm_output *output,
				  struct drm_display_mode *mode,
				  struct drm_display_mode *adjusted_mode)
{
	struct drm_device *dev = output->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	struct intel_crtc *intel_crtc = output->crtc->driver_private;
	struct drm_output *tmp_output;

	/* Should never happen!! */
	if (!IS_I965G(dev) && intel_crtc->pipe == 0) {
		DRM_ERROR(KERN_ERR "Can't support LVDS on pipe A\n");
		return false;
	}

	/* Should never happen!! */
	list_for_each_entry(tmp_output, &dev->mode_config.output_list, head) {
		if (tmp_output != output && tmp_output->crtc == output->crtc) {
			DRM_ERROR("Can't enable LVDS and another "
			       "output on the same pipe\n");
			return false;
		}
	}

	/*
	 * If we have timings from the BIOS for the panel, put them in
	 * to the adjusted mode.  The CRTC will be set up for this mode,
	 * with the panel scaling set up to source from the H/VDisplay
	 * of the original mode.
	 */
	if (dev_priv->panel_fixed_mode != NULL) {
		adjusted_mode->hdisplay = dev_priv->panel_fixed_mode->hdisplay;
		adjusted_mode->hsync_start =
			dev_priv->panel_fixed_mode->hsync_start;
		adjusted_mode->hsync_end =
			dev_priv->panel_fixed_mode->hsync_end;
		adjusted_mode->htotal = dev_priv->panel_fixed_mode->htotal;
		adjusted_mode->vdisplay = dev_priv->panel_fixed_mode->vdisplay;
		adjusted_mode->vsync_start =
			dev_priv->panel_fixed_mode->vsync_start;
		adjusted_mode->vsync_end =
			dev_priv->panel_fixed_mode->vsync_end;
		adjusted_mode->vtotal = dev_priv->panel_fixed_mode->vtotal;
		adjusted_mode->clock = dev_priv->panel_fixed_mode->clock;
		drm_mode_set_crtcinfo(adjusted_mode, CRTC_INTERLACE_HALVE_V);
	}

	/*
	 * XXX: It would be nice to support lower refresh rates on the
	 * panels to reduce power consumption, and perhaps match the
	 * user's requested refresh rate.
	 */

	return true;
}

static void intel_lvds_prepare(struct drm_output *output)
{
	struct drm_device *dev = output->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;

	DRM_INFO("intel_lvds_prepare\n");
	dev_priv->saveBLC_PWM_CTL = I915_READ(BLC_PWM_CTL);
	dev_priv->backlight_duty_cycle = (dev_priv->saveBLC_PWM_CTL &
				       BACKLIGHT_DUTY_CYCLE_MASK);

	intel_lvds_set_power(dev, false);
}

static void intel_lvds_commit( struct drm_output *output)
{
	struct drm_device *dev = output->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;

	DRM_INFO("intel_lvds_commit\n");
	if (dev_priv->backlight_duty_cycle == 0)
		//dev_priv->backlight_duty_cycle =
		lvds_backlight =
			intel_lvds_get_max_backlight(dev);

	intel_lvds_set_power(dev, true);
}

static void intel_lvds_mode_set(struct drm_output *output,
				struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode)
{
	struct drm_device *dev = output->dev;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	struct intel_crtc *intel_crtc = output->crtc->driver_private;
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
		if (dev_priv->panel_wants_dither)
			pfit_control |= PANEL_8TO6_DITHER_ENABLE;
	}
	else
		pfit_control |= intel_crtc->pipe << PFIT_PIPE_SHIFT;

	I915_WRITE(PFIT_CONTROL, pfit_control);
}

/**
 * Detect the LVDS connection.
 *
 * This always returns OUTPUT_STATUS_CONNECTED.  This output should only have
 * been set up if the LVDS was actually connected anyway.
 */
static enum drm_output_status intel_lvds_detect(struct drm_output *output)
{
	return output_status_connected;
}

/**
 * Return the list of DDC modes if available.
 */
static int intel_lvds_get_modes(struct drm_output *output)
{
	struct drm_device *dev = output->dev;
	struct intel_output *intel_output = output->driver_private;
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
        struct edid *edid;

	/* Try reading DDC from the adapter */
        edid = (struct edid *)drm_ddc_read(&intel_output->ddc_bus->adapter);

        if (!edid) {
                DRM_INFO("%s: no EDID data from device, reading ACPI _DDC data.\n",
                         output->name);
                edid = kzalloc(sizeof(struct edid), GFP_KERNEL);
                drm_get_acpi_edid(ACPI_EDID_LCD, (char*)edid, 128);
        }

	if (edid)
	        drm_add_edid_modes(output, edid);

	/* Didn't get an EDID */
	if (!output->monitor_info) {
		struct drm_display_info *dspinfo;
		dspinfo = kzalloc(sizeof(*output->monitor_info), GFP_KERNEL);
		if (!dspinfo)
			goto out;

		/* Set wide sync ranges so we get all modes
		 * handed to valid_mode for checking
		 */
		dspinfo->min_vfreq = 0;
		dspinfo->max_vfreq = 200;
		dspinfo->min_hfreq = 0;
		dspinfo->max_hfreq = 200;
		output->monitor_info = dspinfo;
	}

out:
	if (dev_priv->panel_fixed_mode != NULL) {
		struct drm_display_mode *mode =
			drm_mode_duplicate(dev, dev_priv->panel_fixed_mode);
		drm_mode_probed_add(output, mode);
		return 1;
	}

	return 0;
}

/* added by alek du to add /sys/class/backlight interface */
static int update_bl_status(struct backlight_device *bd)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,20)
        int value = bd->props->brightness;
#else
        int value = bd->props.brightness;
#endif
	
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,22)
	struct drm_device *dev = class_get_devdata (&bd->class_dev);
#else
	struct drm_device *dev = bl_get_data(bd);
#endif
	lvds_backlight = value;
	intel_lvds_set_backlight(dev, value);
	/*value = (bd->props.power == FB_BLANK_UNBLANK) ? 1 : 0;
	intel_lvds_set_power(dev,value);*/
	return 0;
}

static int read_brightness(struct backlight_device *bd)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,20)
        return bd->props->brightness;
#else
        return bd->props.brightness;
#endif
}

static struct backlight_device *psbbl_device = NULL;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,20)
static struct backlight_properties psbbl_ops = {
        .get_brightness = read_brightness,
        .update_status = update_bl_status,
        .max_brightness = BRIGHTNESS_MAX_LEVEL, 
};
#else
static struct backlight_ops psbbl_ops = {
        .get_brightness = read_brightness,
        .update_status = update_bl_status,
};
#endif

/**
 * intel_lvds_destroy - unregister and free LVDS structures
 * @output: output to free
 *
 * Unregister the DDC bus for this output then free the driver private
 * structure.
 */
static void intel_lvds_destroy(struct drm_output *output)
{
	struct intel_output *intel_output = output->driver_private;

	if (psbbl_device){
		backlight_device_unregister(psbbl_device);
	}		
	if(dev_OpRegion != NULL)
		iounmap(dev_OpRegion);
	intel_i2c_destroy(intel_output->ddc_bus);
	intel_i2c_destroy(lvds_i2c_bus);
	kfree(output->driver_private);
}

static const struct drm_output_funcs intel_lvds_output_funcs = {
	.dpms = intel_lvds_dpms,
	.save = intel_lvds_save,
	.restore = intel_lvds_restore,
	.mode_valid = intel_lvds_mode_valid,
	.mode_fixup = intel_lvds_mode_fixup,
	.prepare = intel_lvds_prepare,
	.mode_set = intel_lvds_mode_set,
	.commit = intel_lvds_commit,
	.detect = intel_lvds_detect,
	.get_modes = intel_lvds_get_modes,
	.cleanup = intel_lvds_destroy
};

int intel_get_acpi_dod(char *method)
{
	int status;
	int found = 0;
	int i;
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object *dod = NULL;
	union acpi_object *obj;

	status = acpi_evaluate_object(NULL, method, NULL, &buffer);
	if (ACPI_FAILURE(status))
		return -ENODEV;

	dod = buffer.pointer;
	if (!dod || (dod->type != ACPI_TYPE_PACKAGE)) {
		status = -EFAULT;
		goto out;
	}

	DRM_DEBUG("Found %d video heads in _DOD\n", dod->package.count);

	for (i = 0; i < dod->package.count; i++) {
		obj = &dod->package.elements[i];

		if (obj->type != ACPI_TYPE_INTEGER) {
			DRM_DEBUG("Invalid _DOD data\n");
		} else {
			DRM_DEBUG("dod element[%d] = 0x%x\n", i,
				  (int)obj->integer.value);

			/* look for an LVDS type */
			if (obj->integer.value & 0x00000400) 
				found = 1;
		}
	}
      out:
	kfree(buffer.pointer);
	return found;
}
/**
 * intel_lvds_init - setup LVDS outputs on this device
 * @dev: drm device
 *
 * Create the output, register the LVDS DDC bus, and try to figure out what
 * modes we can display on the LVDS panel (if present).
 */
void intel_lvds_init(struct drm_device *dev)
{
	DRM_DRIVER_PRIVATE_T *dev_priv = dev->dev_private;
	struct drm_output *output;
	struct intel_output *intel_output;
	struct drm_display_mode *scan; /* *modes, *bios_mode; */
	struct drm_crtc *crtc;
	u32 lvds;
	int pipe;

	if (!drm_intel_ignore_acpi && !intel_get_acpi_dod(ACPI_DOD))
		return;

	output = drm_output_create(dev, &intel_lvds_output_funcs, "LVDS");
	if (!output)
		return;

	intel_output = kmalloc(sizeof(struct intel_output), GFP_KERNEL);
	if (!intel_output) {
		drm_output_destroy(output);
		return;
	}

	intel_output->type = INTEL_OUTPUT_LVDS;
	output->driver_private = intel_output;
	output->subpixel_order = SubPixelHorizontalRGB;
	output->interlace_allowed = FALSE;
	output->doublescan_allowed = FALSE;

	//initialize the I2C bus and BLC data
	lvds_i2c_bus = intel_i2c_create(dev, GPIOB, "LVDSBLC_B");
	if (!lvds_i2c_bus) {
		dev_printk(KERN_ERR, &dev->pdev->dev, "i2c bus registration "
			   "failed.\n");
		return;
	}
	lvds_i2c_bus->slave_addr = 0x2c;//0x58;
	lvds_backlight = BRIGHTNESS_MAX_LEVEL;
	blc_type = 0;
	blc_pol = 0;

	if (1) { //get the BLC init data from VBT 
		u32 OpRegion_Phys;
		unsigned int OpRegion_Size = 0x100;
		OpRegionPtr OpRegion;
		char *OpRegion_String = "IntelGraphicsMem";

		struct vbt_header *vbt;
		struct bdb_header *bdb;
		int vbt_off, bdb_off, bdb_block_off, block_size;
		int panel_type = -1;
		unsigned char *bios;
		unsigned char *vbt_buf;

		pci_read_config_dword(dev->pdev, 0xFC, &OpRegion_Phys);

		//dev_OpRegion =  phys_to_virt(OpRegion_Phys);
		dev_OpRegion = ioremap(OpRegion_Phys, OpRegion_Size);
		dev_OpRegionSize = OpRegion_Size;

		OpRegion = (OpRegionPtr) dev_OpRegion;

		if (!memcmp(OpRegion->sign, OpRegion_String, 16)) {
			unsigned int OpRegion_NewSize;

			OpRegion_NewSize = OpRegion->size * 1024;

			dev_OpRegionSize = OpRegion_NewSize;
			
			iounmap(dev_OpRegion);
			dev_OpRegion = ioremap(OpRegion_Phys, OpRegion_NewSize);
		} else {
			iounmap(dev_OpRegion);
			dev_OpRegion = NULL;
		}

		if((dev_OpRegion != NULL)&&(dev_OpRegionSize >= OFFSET_OPREGION_VBT)) {
			DRM_INFO("intel_lvds_init: OpRegion has the VBT address\n");
			vbt_buf = dev_OpRegion + OFFSET_OPREGION_VBT;
			vbt = (struct vbt_header *)(dev_OpRegion + OFFSET_OPREGION_VBT);
		} else {		
			DRM_INFO("intel_lvds_init: No OpRegion, use the bios at fixed address 0xc0000\n");
			bios = phys_to_virt(0xC0000);
			if(*((u16 *)bios) != 0xAA55){
				bios = NULL;
				DRM_ERROR("the bios is incorrect\n");
				goto blc_out;		
			}
			vbt_off = bios[0x1a] | (bios[0x1a + 1] << 8);
			DRM_INFO("intel_lvds_init: the vbt off is %x\n", vbt_off);
			vbt_buf = bios + vbt_off;
			vbt = (struct vbt_header *)(bios + vbt_off);
		}

		bdb_off = vbt->bdb_offset;
		bdb = (struct bdb_header *)(vbt_buf + bdb_off);

		DRM_INFO("intel_lvds_init: The bdb->signature is %s, the bdb_off is %d\n",bdb->signature, bdb_off);

		if (memcmp(bdb->signature, "BIOS_DATA_BLOCK ", 16) != 0) {
			DRM_ERROR("the vbt is error\n");
			goto blc_out;
		}

		for (bdb_block_off = bdb->header_size; bdb_block_off < bdb->bdb_size;
				bdb_block_off += block_size) {
			int start = bdb_off + bdb_block_off;
			int id, num_entries;
			struct lvds_bdb_1 *lvds1;
			struct lvds_blc *lvdsblc;
			struct lvds_bdb_blc *bdbblc;

			id = vbt_buf[start];
			block_size = (vbt_buf[start + 1] | (vbt_buf[start + 2] << 8)) + 3;
			switch (id) {
				case 40:
					lvds1 = (struct lvds_bdb_1 *)(vbt_buf+ start);
					panel_type = lvds1->panel_type;
					//if (lvds1->caps & LVDS_CAP_DITHER)
					//	*panelWantsDither = TRUE;
					break;

				case 43:
					bdbblc = (struct lvds_bdb_blc *)(vbt_buf + start);
					num_entries = bdbblc->table_size? (bdbblc->size - \
							sizeof(bdbblc->table_size))/bdbblc->table_size : 0;
					if (num_entries << 16 && bdbblc->table_size == sizeof(struct lvds_blc)) {
						lvdsblc = (struct lvds_blc *)(vbt_buf + start + sizeof(struct lvds_bdb_blc));
						lvdsblc += panel_type;
						blc_type = lvdsblc->type;
						blc_pol = lvdsblc->pol;
						blc_freq = lvdsblc->freq;
						blc_minbrightness = lvdsblc->minbrightness;
						blc_i2caddr = lvdsblc->i2caddr;
						blc_brightnesscmd = lvdsblc->brightnesscmd;
						DRM_INFO("intel_lvds_init: BLC Data in BIOS VBT tables: datasize=%d paneltype=%d \
								type=0x%02x pol=0x%02x freq=0x%04x minlevel=0x%02x    \
								i2caddr=0x%02x cmd=0x%02x \n",
								0,
								panel_type,
								lvdsblc->type,
								lvdsblc->pol,
								lvdsblc->freq,
								lvdsblc->minbrightness,
								lvdsblc->i2caddr,
								lvdsblc->brightnesscmd);
					}
					break;
			}
		}

	}

	if(1){
		//get the Core Clock for calculating MAX PWM value
		//check whether the MaxResEnableInt is 
		struct pci_dev * pci_root = pci_get_bus_and_slot(0, 0);
		u32 clock;
		u32 sku_value = 0;
		unsigned int CoreClocks[] = {
			100,
			133,
			150,
			178,
			200,
			266,
			266,
			266
		};
		if(pci_root)
		{
			pci_write_config_dword(pci_root, 0xD0, 0xD0050300);
			pci_read_config_dword(pci_root, 0xD4, &clock);
			CoreClock = CoreClocks[clock & 0x07];
			DRM_INFO("intel_lvds_init: the CoreClock is %d\n", CoreClock);
			
			pci_write_config_dword(pci_root, 0xD0, PCI_PORT5_REG80_FFUSE);
			pci_read_config_dword(pci_root, 0xD4, &sku_value);
			sku_bMaxResEnableInt = (sku_value & PCI_PORT5_REG80_MAXRES_INT_EN)? true : false;
			DRM_INFO("intel_lvds_init: sku_value is 0x%08x\n", sku_value);
			DRM_INFO("intel_lvds_init: sku_bMaxResEnableInt is %d\n", sku_bMaxResEnableInt);
		}
	}

	if ((blc_type == BLC_I2C_TYPE) || (blc_type == BLC_PWM_TYPE)){	
		/* add /sys/class/backlight interface as standard */
		psbbl_device = backlight_device_register("psblvds", &dev->pdev->dev, dev, &psbbl_ops);
		if (psbbl_device){
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,20)
			down(&psbbl_device->sem);
			psbbl_device->props->max_brightness = BRIGHTNESS_MAX_LEVEL;
			psbbl_device->props->brightness = lvds_backlight;
			psbbl_device->props->power = FB_BLANK_UNBLANK;
			psbbl_device->props->update_status(psbbl_device);
			up(&psbbl_device->sem);
#else
			psbbl_device->props.max_brightness = BRIGHTNESS_MAX_LEVEL;
			psbbl_device->props.brightness = lvds_backlight;
			psbbl_device->props.power = FB_BLANK_UNBLANK;
			backlight_update_status(psbbl_device);
#endif
		}
	}

blc_out:

	/* Set up the DDC bus. */
	intel_output->ddc_bus = intel_i2c_create(dev, GPIOC, "LVDSDDC_C");
	if (!intel_output->ddc_bus) {
		dev_printk(KERN_ERR, &dev->pdev->dev, "DDC bus registration "
			   "failed.\n");
		intel_i2c_destroy(lvds_i2c_bus);
		return;
	}

	/*
	 * Attempt to get the fixed panel mode from DDC.  Assume that the
	 * preferred mode is the right one.
	 */
	intel_lvds_get_modes(output);

	list_for_each_entry(scan, &output->probed_modes, head) {
		if (scan->type & DRM_MODE_TYPE_PREFERRED) {
			dev_priv->panel_fixed_mode = 
				drm_mode_duplicate(dev, scan);
			goto out; /* FIXME: check for quirks */
		}
	}

	/*
	 * If we didn't get EDID, try checking if the panel is already turned
	 * on.  If so, assume that whatever is currently programmed is the
	 * correct mode.
	 */
	lvds = I915_READ(LVDS);
	pipe = (lvds & LVDS_PIPEB_SELECT) ? 1 : 0;
	crtc = intel_get_crtc_from_pipe(dev, pipe);
		
	if (crtc && (lvds & LVDS_PORT_EN)) {
		dev_priv->panel_fixed_mode = intel_crtc_mode_get(dev, crtc);
		if (dev_priv->panel_fixed_mode) {
			dev_priv->panel_fixed_mode->type |=
				DRM_MODE_TYPE_PREFERRED;
			goto out; /* FIXME: check for quirks */
		}
	}

	/* If we still don't have a mode after all that, give up. */
	if (!dev_priv->panel_fixed_mode)
		goto failed;

	/* FIXME: probe the BIOS for modes and check for LVDS quirks */
#if 0
	/* Get the LVDS fixed mode out of the BIOS.  We should support LVDS
	 * with the BIOS being unavailable or broken, but lack the
	 * configuration options for now.
	 */
	bios_mode = intel_bios_get_panel_mode(pScrn);
	if (bios_mode != NULL) {
		if (dev_priv->panel_fixed_mode != NULL) {
			if (dev_priv->debug_modes &&
			    !xf86ModesEqual(dev_priv->panel_fixed_mode,
					    bios_mode))
			{
				xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
					   "BIOS panel mode data doesn't match probed data, "
					   "continuing with probed.\n");
				xf86DrvMsg(pScrn->scrnIndex, X_INFO, "BIOS mode:\n");
				xf86PrintModeline(pScrn->scrnIndex, bios_mode);
				xf86DrvMsg(pScrn->scrnIndex, X_INFO, "probed mode:\n");
				xf86PrintModeline(pScrn->scrnIndex, dev_priv->panel_fixed_mode);
				xfree(bios_mode->name);
				xfree(bios_mode);
			}
		}  else {
			dev_priv->panel_fixed_mode = bios_mode;
		}
	} else {
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			   "Couldn't detect panel mode.  Disabling panel\n");
		goto disable_exit;
	}

	/*
	 * Blacklist machines with BIOSes that list an LVDS panel without
	 * actually having one.
	 */
	if (dev_priv->PciInfo->chipType == PCI_CHIP_I945_GM) {
		/* aopen mini pc */
		if (dev_priv->PciInfo->subsysVendor == 0xa0a0)
			goto disable_exit;

		if ((dev_priv->PciInfo->subsysVendor == 0x8086) &&
		    (dev_priv->PciInfo->subsysCard == 0x7270)) {
			/* It's a Mac Mini or Macbook Pro.
			 *
			 * Apple hardware is out to get us.  The macbook pro
			 * has a real LVDS panel, but the mac mini does not,
			 * and they have the same device IDs.  We'll
			 * distinguish by panel size, on the assumption
			 * that Apple isn't about to make any machines with an
			 * 800x600 display.
			 */

			if (dev_priv->panel_fixed_mode != NULL &&
			    dev_priv->panel_fixed_mode->HDisplay == 800 &&
			    dev_priv->panel_fixed_mode->VDisplay == 600)
			{
				xf86DrvMsg(pScrn->scrnIndex, X_INFO,
					   "Suspected Mac Mini, ignoring the LVDS\n");
				goto disable_exit;
			}
		}
	}

#endif

out:
	return;

failed:
        DRM_DEBUG("No LVDS modes found, disabling.\n");
	drm_output_destroy(output); /* calls intel_lvds_destroy above */
}
