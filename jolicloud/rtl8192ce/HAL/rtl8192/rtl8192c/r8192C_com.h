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

#ifndef __REALTEK_HAL8192CCOM_H__
#define __REALTEK_HAL8192CCOM_H__

#include "../rtl_core.h"


typedef struct _TxPowerInfo{
	u8 CCKIndex[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 HT40_1SIndex[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 HT40_2SIndexDiff[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	char HT20IndexDiff[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 OFDMIndexDiff[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 HT40MaxOffset[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 HT20MaxOffset[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 TSSI_A;
	u8 TSSI_B;
}TxPowerInfo, *PTxPowerInfo;

void
ReadTxPowerInfo(
	struct net_device 	*dev,
	u8*				PROMContent,
	bool				AutoLoadFail
	);



#define NORMAL_CHIP  				BIT(4)

#define CHIP_92C_BITMASK  			BIT(0)
#define CHIP_92C_1T2R_MSK			BIT(1)
#define CHIP_92C_1T2R			0x03
#define CHIP_92C					0x01
#define CHIP_88C					0x00

#define IS_NORMAL_CHIP(version)  	((version & NORMAL_CHIP) ? true : false) 
#define IS_92C_SERIAL(version)   		((version & CHIP_92C_BITMASK) ? true : false)
#define IS_92C_1T2R(version)		(((version) & CHIP_92C_BITMASK) && ((version) & CHIP_92C_1T2R_MSK))

typedef enum _VERSION_8192C{
	VERSION_TEST_CHIP_88C = 0x00,
	VERSION_TEST_CHIP_92C = 0x01,
	VERSION_NORMAL_CHIP_88C = 0x10,
	VERSION_NORMAL_CHIP_92C = 0x11,
	VERSION_NORMAL_CHIP_92C_1T2R = 0x13,
}VERSION_8192C,*PVERSION_8192C;

#define CHIP_BONDING_IDENTIFIER(_value)	(((_value)>>22)&0x3)
#define CHIP_BONDING_92C_1T2R	0x1

u8
GetEEPROMSize8192C(struct net_device *dev);


VERSION_8192C
ReadChipVersion(struct net_device *dev);




bool
IsSwChnlInProgress8192C(struct net_device *dev);

u8
GetRFType8192C(struct net_device *dev);

void
SetTxAntenna8192C(struct net_device *dev, u8 SelectedAntenna);




enum ChannelPlan{
	CHPL_FCC	= 0,
	CHPL_IC		= 1,
	CHPL_ETSI	= 2,
	CHPL_SPAIN	= 3,
	CHPL_FRANCE	= 4,
	CHPL_MKK	= 5,
	CHPL_MKK1	= 6,
	CHPL_ISRAEL	= 7,
	CHPL_TELEC	= 8,
	CHPL_GLOBAL	= 9,
	CHPL_WORLD	= 10,
};

void
ReadChannelPlan(
	struct net_device 	*dev,
	u8*				PROMContent,
	bool				AutoLoadFail
	);


void
WKFMCAMAddOneEntry(
	struct net_device 	*dev,
	u8			Index, 
	u16			usConfig
);
void
WKFMCAMDeleteOneEntry(
		struct net_device 	*dev,
		u32 			Index
);
void SetBcnCtrlReg(struct net_device* dev, u8 SetBits, u8 ClearBits);
#endif

