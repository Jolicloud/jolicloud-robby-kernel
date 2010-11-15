#include <drm/drmP.h>
#include <drm/drm.h>
#include <drm/drm_crtc.h>
#include <drm/drm_edid.h>
#include "intel_drv.h"
#include "psb_drv.h"
#include "intel_reg.h"

/* Fixed name */
#define ACPI_EDID_LCD	"\\_SB_.PCI0.GFX0.DD04._DDC"
#define ACPI_DOD	"\\_SB_.PCI0.GFX0._DOD"

#include "intel_i2c.c"
#include "intel_sdvo.c"
#include "intel_modes.c"
#include "intel_lvds.c"
#include "intel_dsi.c"
#include "intel_display.c"
