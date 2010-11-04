/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
******************************************************************************/
#ifndef __INC_HAL8192CE_FW_IMG_H
#define __INC_HAL8192CE_FW_IMG_H

#include <linux/types.h>

/*Created on  2010/ 4/27, 10:48*/

#define ImgArrayLength 13550
extern u8 Rtl8192CEFwImgArray[ImgArrayLength];
#define MainArrayLength 1
extern u8 Rtl8192CEFwMainArray[MainArrayLength];
#define DataArrayLength 1
extern u8 Rtl8192CEFwDataArray[DataArrayLength];
#define PHY_REG_2TArrayLength 374
extern u32 Rtl8192CEPHY_REG_2TArray[PHY_REG_2TArrayLength];
#define PHY_REG_1TArrayLength 374
extern u32 Rtl8192CEPHY_REG_1TArray[PHY_REG_1TArrayLength];
#define PHY_ChangeTo_1T1RArrayLength 1
extern u32 Rtl8192CEPHY_ChangeTo_1T1RArray[PHY_ChangeTo_1T1RArrayLength];
#define PHY_ChangeTo_1T2RArrayLength 1
extern u32 Rtl8192CEPHY_ChangeTo_1T2RArray[PHY_ChangeTo_1T2RArrayLength];
#define PHY_ChangeTo_2T2RArrayLength 1
extern u32 Rtl8192CEPHY_ChangeTo_2T2RArray[PHY_ChangeTo_2T2RArrayLength];
#define PHY_REG_Array_PGLength 192
extern u32 Rtl8192CEPHY_REG_Array_PG[PHY_REG_Array_PGLength];
#define PHY_REG_Array_MPLength 4
extern u32 Rtl8192CEPHY_REG_Array_MP[PHY_REG_Array_MPLength];
#define RadioA_2TArrayLength 282
extern u32 Rtl8192CERadioA_2TArray[RadioA_2TArrayLength];
#define RadioB_2TArrayLength 78
extern u32 Rtl8192CERadioB_2TArray[RadioB_2TArrayLength];
#define RadioA_1TArrayLength 282
extern u32 Rtl8192CERadioA_1TArray[RadioA_1TArrayLength];
#define RadioB_1TArrayLength 1
extern u32 Rtl8192CERadioB_1TArray[RadioB_1TArrayLength];
#define RadioB_GM_ArrayLength 1
extern u32 Rtl8192CERadioB_GM_Array[RadioB_GM_ArrayLength];
#define MAC_2T_ArrayLength 162
extern u32 Rtl8192CEMAC_2T_Array[MAC_2T_ArrayLength];
#define MACPHY_Array_PGLength 1
extern u32 Rtl8192CEMACPHY_Array_PG[MACPHY_Array_PGLength];
#define AGCTAB_2TArrayLength 320
extern u32 Rtl8192CEAGCTAB_2TArray[AGCTAB_2TArrayLength];
#define AGCTAB_1TArrayLength 320
extern u32 Rtl8192CEAGCTAB_1TArray[AGCTAB_1TArrayLength];

#ifndef __INC_HAL8192CETEST_FW_IMG_H
#define __INC_HAL8192CETEST_FW_IMG_H

/*Created on  2010/ 1/20,  8:37*/
#define Rtl8192CTestFwImgLen 12264
extern u8 Rtl8192CTestFwImg[Rtl8192CTestFwImgLen];
#define Rtl8192CTestPHY_REG_2TLen 478
extern u32 Rtl8192CTestPHY_REG_2T[Rtl8192CTestPHY_REG_2TLen];
#define Rtl8192CTestPHY_REG_1TLen 424
extern u32 Rtl8192CTestPHY_REG_1T[Rtl8192CTestPHY_REG_1TLen];
#define Rtl8192CTestPHY_REG_PGLen 48
extern u32 Rtl8192CTestPHY_REG_PG[Rtl8192CTestPHY_REG_PGLen];
#define Rtl8192CTestRadioA_2TLen 258
extern u32 Rtl8192CTestRadioA_2T[Rtl8192CTestRadioA_2TLen];
#define Rtl8192CTestRadioB_2TLen 48
extern u32 Rtl8192CTestRadioB_2T[Rtl8192CTestRadioB_2TLen];
#define Rtl8192CTestRadioA_1TLen 260
extern u32 Rtl8192CTestRadioA_1T[Rtl8192CTestRadioA_1TLen];
#define Rtl8192CTestRadioB_1TLen 1
extern u32 Rtl8192CTestRadioB_1T[Rtl8192CTestRadioB_1TLen];
#define Rtl8192CTestMAC_2TLen 144
extern u32 Rtl8192CTestMAC_2T[Rtl8192CTestMAC_2TLen];
#define Rtl8192CTestAGCTAB_2TLen 320
extern u32 Rtl8192CTestAGCTAB_2T[Rtl8192CTestAGCTAB_2TLen];
#define Rtl8192CTestAGCTAB_1TLen 320
extern u32 Rtl8192CTestAGCTAB_1T[Rtl8192CTestAGCTAB_1TLen];

#endif 

#endif 

