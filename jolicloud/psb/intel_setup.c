#include "drmP.h"
#include "drm.h"
#include "drm_crtc.h"
#include "drm_edid.h"
#include "intel_drv.h"
#include "i915_drm.h"
#include "i915_drv.h"
#include "i915_reg.h"
#include "intel_crt.c"

/* don't define */
#define ACPI_EDID_LCD NULL
#define ACPI_DOD NULL

#include "intel_lvds.c"
#include "intel_sdvo.c"
#include "intel_display.c"
#include "intel_modes.c"
