/* _NVRM_COPYRIGHT_BEGIN_
 *
 * Copyright 1993-2009 by NVIDIA Corporation.  All rights reserved.  All
 * information contained herein is proprietary and confidential to NVIDIA
 * Corporation.  Any use, reproduction, or disclosure without the written
 * permission of NVIDIA Corporation is prohibited.
 *
 * _NVRM_COPYRIGHT_END_
 */

#ifndef _NVACPITYPES_H_
#define _NVACPITYPES_H_

typedef enum _ACPI_DSM_FUNCTION
{
    ACPI_DSM_FUNCTION_NBSI  = 0,
    ACPI_DSM_FUNCTION_NVHG,
    ACPI_DSM_FUNCTION_MXM,
    ACPI_DSM_FUNCTION_SPB,
    ACPI_DSM_FUNCTION_NBCI,
    ACPI_DSM_FUNCTION_NVOP,
    // insert new DSM Functions here
    ACPI_DSM_FUNCTION_COUNT,
    ACPI_DSM_FUNCTION_CURRENT  // pseudo option to select currently available GUID which supports the subfunction.
} ACPI_DSM_FUNCTION;

#endif // _NVACPIFUNCTYPES_H_

