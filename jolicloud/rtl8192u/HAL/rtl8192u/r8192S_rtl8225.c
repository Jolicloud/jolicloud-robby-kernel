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
 * Jerry chuang <wlanfae@realtek.com>
******************************************************************************/
#include "r8192U.h"
#include "r8192S_hw.h"
#include "r8192S_phyreg.h"
#include "r8192S_phy.h"
#include "r8192S_rtl8225.h"

void phy_RF8225_Config_HardCode(struct net_device* dev );
bool phy_RF8225_Config_ParaFile(struct net_device* dev );
void PHY_SetRF8225OfdmTxPower(struct net_device* dev ,u8	powerlevel)
{

}



void PHY_SetRF8225CckTxPower(	struct net_device* dev ,	u8 powerlevel)
{

}


void PHY_SetRF0222DOfdmTxPower(struct net_device* dev ,u8 powerlevel)
{
}



void PHY_SetRF0222DCckTxPower(struct net_device* dev ,u8	powerlevel)
{
}


void PHY_SetRF0222DBandwidth(struct net_device* dev , HT_CHANNEL_WIDTH	 Bandwidth)	
{	
	u8			eRFPath;	
	struct r8192_priv *priv = ieee80211_priv(dev);
	

	if (1)
	{		
#ifndef RTL92SE_FPGA_VERIFY 
		switch(Bandwidth)
		{
			case HT_CHANNEL_WIDTH_20:
#ifdef FIB_MODIFICATION
				write_nic_byte(dev, rFPGA0_AnalogParameter2, 0x58);
#endif
				rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, RF_CHNLBW, BIT10|BIT11, 0x01);
				break;
			case HT_CHANNEL_WIDTH_20_40:
#ifdef FIB_MODIFICATION
				write_nic_byte(dev, rFPGA0_AnalogParameter2, 0x18);
#endif
				rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, RF_CHNLBW, BIT10|BIT11, 0x00);
				break;
			default:
				;
				break;			
		}
#endif	
	}
	else
	{
	for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	{
		switch(Bandwidth)
		{
			case HT_CHANNEL_WIDTH_20:
				break;
			case HT_CHANNEL_WIDTH_20_40:
				break;
			default:
				;
				break;
				
		}
	}
	}

}


void PHY_SetRF8225Bandwidth(struct net_device* dev ,HT_CHANNEL_WIDTH Bandwidth)	
{	
	u8			eRFPath;	
	struct r8192_priv *priv = ieee80211_priv(dev);
	
	for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	{
		switch(Bandwidth)
		{
			case HT_CHANNEL_WIDTH_20:
				break;
			case HT_CHANNEL_WIDTH_20_40:
				RT_TRACE(COMP_DBG, "SetChannelBandwidth8190Pci():8225 does not support 40M mode\n");
				break;
			default:
				RT_TRACE(COMP_DBG, "PHY_SetRF8225Bandwidth(): unknown Bandwidth: %#X\n",Bandwidth );
				break;
				
		}
	}

}

bool PHY_RF8225_Config(struct net_device* dev )
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	bool	rtStatus = true;	
	
	priv->NumTotalRFPath = 2;



			phy_RF8225_Config_HardCode(dev);
			phy_RF8225_Config_ParaFile(dev);

	return rtStatus;
		
}

void phy_RF8225_Config_HardCode(struct net_device* dev)
{
	

	
}

bool phy_RF8225_Config_ParaFile(struct net_device* dev)
{
	u32					u4RegValue = 0;
	u8					eRFPath;
	bool				rtStatus = true;
	struct r8192_priv *priv = ieee80211_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg;	

#if	1
	for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	{

		pPhyReg = &priv->PHYRegDef[eRFPath];
		
		switch(eRFPath)
		{
		case RF90_PATH_A:
		case RF90_PATH_C:
			u4RegValue = rtl8192_QueryBBReg(dev, pPhyReg->rfintfs, bRFSI_RFENV);
			break;
		case RF90_PATH_B :
		case RF90_PATH_D:
			u4RegValue = rtl8192_QueryBBReg(dev, pPhyReg->rfintfs, bRFSI_RFENV<<16);
			break;
		}

		rtl8192_setBBreg(dev, pPhyReg->rfintfe, bRFSI_RFENV<<16, 0x1);
		
		rtl8192_setBBreg(dev, pPhyReg->rfintfo, bRFSI_RFENV, 0x1);

		rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, b3WireAddressLength, 0x0); 	
		rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, b3WireDataLength, 0x0);	


		switch(eRFPath)
		{
		case RF90_PATH_A:
			rtStatus = rtl8192_phy_ConfigRFWithHeaderFile(dev,(RF90_RADIO_PATH_E)eRFPath);
			break;
		case RF90_PATH_B:
			rtStatus = rtl8192_phy_ConfigRFWithHeaderFile(dev,(RF90_RADIO_PATH_E)eRFPath);
			break;
		case RF90_PATH_C:
			break;
		case RF90_PATH_D:
			break;
		}

		switch(eRFPath)
		{
		case RF90_PATH_A:
		case RF90_PATH_C:
			rtl8192_setBBreg(dev, pPhyReg->rfintfs, bRFSI_RFENV, u4RegValue);
			break;
		case RF90_PATH_B :
		case RF90_PATH_D:
			rtl8192_setBBreg(dev, pPhyReg->rfintfs, bRFSI_RFENV<<16, u4RegValue);
			break;
		}

		if(rtStatus == false){
			goto phy_RF8225_Config_ParaFile_Fail;
		}

	}

	return rtStatus;
	
phy_RF8225_Config_ParaFile_Fail:	
#endif
	return rtStatus;
}


