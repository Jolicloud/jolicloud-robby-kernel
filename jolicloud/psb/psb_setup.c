#include "drmP.h"
#include "drm.h"
#include "drm_crtc.h"
#include "drm_edid.h"
#include "intel_drv.h"
#include "psb_drv.h"
#include "i915_reg.h"
#include "intel_crt.c"

/* Fixed name */
#define ACPI_EDID_LCD	"\\_SB_.PCI0.GFX0.DD04._DDC"
#define ACPI_DOD	"\\_SB_.PCI0.GFX0._DOD"

#include "intel_lvds.c"
#include "intel_sdvo.c"
#include "intel_display.c"
#include "intel_modes.c"
