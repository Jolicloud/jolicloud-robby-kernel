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
#include "../rtl_core.h"
#include <linux/types.h>

#ifdef RTL8192CE
bool IsHexDigit(        char chTmp)
{
	if( (chTmp >= '0' && chTmp <= '9') ||
			(chTmp >= 'a' && chTmp <= 'f') ||
			(chTmp >= 'A' && chTmp <= 'F') )
	{
		return true;
	}
	else
	{
		return false;
	}
}

u32 MapCharToHexDigit(char chTmp)
{
	if(chTmp >= '0' && chTmp <= '9')
		return (chTmp - '0');
	else if(chTmp >= 'a' && chTmp <= 'f')
		return (10 + (chTmp - 'a'));
	else if(chTmp >= 'A' && chTmp <= 'F')
		return (10 + (chTmp - 'A'));
	else
		return 0;
}


/*---------------------------Define Local Constant---------------------------*/
#define 	_POWERON_DELAY_
#define 	_PRE_EXECUTE_READ_CMD_

#define		EFUSE_REPEAT_THRESHOLD_			3
#define		EFUSE_ERROE_HANDLE					1


typedef struct _EFUSE_MAP_A{
	u8 offset;		
	u8 word_start;	
	u8 byte_start;	
	u8 byte_cnts;
	
}EFUSE_MAP, *PEFUSE_MAP;

typedef struct PG_PKT_STRUCT_A{
	u8 offset;
	u8 word_en;
	u8 data[8];	
}PGPKT_STRUCT,*PPGPKT_STRUCT;

typedef enum EFUSE_DATA_ITEM{
	EFUSE_CHIP_ID=0,
	EFUSE_LDO_SETTING,
	EFUSE_CLK_SETTING,
	EFUSE_SDIO_SETTING,
	EFUSE_CCCR,
	EFUSE_SDIO_MODE,
	EFUSE_OCR,
	EFUSE_F0CIS,
	EFUSE_F1CIS,
	EFUSE_MAC_ADDR,
	EFUSE_EEPROM_VER,
	EFUSE_CHAN_PLAN,
	EFUSE_TXPW_TAB
}EFUSE_DATA_ITEM;

struct efuse_priv 
{    
	u8		id[2];
	u8		ldo_setting[2];
	u8		clk_setting[2];
	u8		cccr;
	u8		sdio_mode;
	u8		ocr[3];
	u8		cis0[17];
	u8		cis1[48];	
	u8		mac_addr[6];
	u8		eeprom_verno;	
	u8		channel_plan;
	u8		tx_power_b[14];
	u8		tx_power_g[14];	
};

/*---------------------------Define Local Constant---------------------------*/


/*------------------------Define global variable-----------------------------*/
const u8 MAX_PGPKT_SIZE = 9; 
const u8 PGPKT_DATA_SIZE = 8; 
const int EFUSE_MAX_SIZE = 512; 

const u8 EFUSE_OOB_PROTECT_BYTES = 15; 


const EFUSE_MAP RTL8712_SDIO_EFUSE_TABLE[]={
/*ID*/			{0		,0		,0			,2	}, 
/*LDO Setting*/	{0		,1		,0			,2	}, 
/*CLK Setting*/	{0		,2		,0			,2	}, 
/*SDIO Setting*/	{1		,0		,0			,1	}, 
/*CCCR*/		{1		,0		,1			,1	}, 
/*SDIO MODE*/	{1		,1		,0			,1	}, 
/*OCR*/			{1		,1		,1			,3	}, 
/*CCIS*/		{1		,3		,0			,17	}, 
/*F1CIS*/		{3		,3		,1			,48	}, 
/*MAC Addr*/	{10		,0		,0			,6	}, 
/*EEPROM ver*/	{10		,3		,0			,1	}, 
/*Channel plan*/	{10		,3		,1			,1	}, 
/*TxPwIndex */	{11		,0		,0			,28	}  
};

/*------------------------Define global variable-----------------------------*/


/*------------------------Define local variable------------------------------*/

/*------------------------Define local variable------------------------------*/


/*--------------------Define function prototype-----------------------*/
extern	void	
EFUSE_Initialize(struct net_device *dev);
extern	u8	
EFUSE_Read1Byte(	
	struct net_device *dev, 
	u16		Address);
extern	void	
EFUSE_Write1Byte(	
	struct net_device *dev, 
	u16		Address,
	u8		Value);
static	void
ReadEFuseByte(
		struct net_device *dev,
		u16 		_offset, 
		u8 		*pbuf);

static	void
efuse_ShadowRead1Byte(
	struct net_device *dev,
	u16		Offset,
	u8		*Value	);
static	void
efuse_ShadowRead2Byte(
	struct net_device *dev,
	u16		Offset,
	u16		*Value	);
static	void
efuse_ShadowRead4Byte(
	struct net_device *dev,
	u16		Offset,
	u32		*Value	);
static	void
efuse_ShadowWrite1Byte(
	struct net_device *dev,
	u16		Offset,
	u8		Value	);
static	void
efuse_ShadowWrite2Byte(
	struct net_device *dev,
	u16		Offset,
	u16		Value	);
static	void
efuse_ShadowWrite4Byte(
	struct net_device *dev,
	u16		Offset,
	u32		Value	);

static	int
efuse_OneByteRead(
	struct net_device *dev,
	u16		addr,
	u8		*data);
static	int
efuse_OneByteWrite(
	struct net_device *dev,
	u16		addr, 
	u8		data);

static	void 
efuse_ReadAllMap(
	struct net_device *dev,
	u8		*Efuse);

static	int
efuse_PgPacketRead(	
	struct net_device *dev,
	u8			offset,
	u8			*data);
static	int 
efuse_PgPacketWrite(
	struct net_device *dev,
	u8 			offset,
	u8			word_en,
	u8			*data);
static	void
efuse_WordEnableDataRead(	
	u8	word_en,
	u8	*sourdata,
	u8	*targetdata);
static	u8
efuse_WordEnableDataWrite(	
	struct net_device *dev,
	u16		efuse_addr,
	u8		word_en, 
	u8		*data);
static	void
efuse_PowerSwitch(
	struct net_device *dev,
	u8           bWrite,
	u8		PwrState);
static	u16
efuse_GetCurrentSize(
	struct net_device *dev);
static u8
efuse_CalculateWordCnts(
	u8	word_en);

/*--------------------Define function prototype-----------------------*/



/*-----------------------------------------------------------------------------
 * Function:	EFUSE_Initialize
 *
 * Overview:	Copy from WMAC fot EFUSE testing setting init.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 09/23/2008 	MHC		Copy from WMAC.
 *
 *---------------------------------------------------------------------------*/
extern	void	
EFUSE_Initialize(
	struct net_device *dev)
{
	u8	Bytetemp = {0x00};
	u8	temp = {0x00};

	Bytetemp = read_nic_byte(dev, SYS_FUNC_EN+1);
	temp = Bytetemp | 0x20;
	write_nic_byte(dev, SYS_FUNC_EN+1, temp);

	Bytetemp = read_nic_byte(dev, SYS_ISO_CTRL+1);
	temp = Bytetemp & 0xFE;
	write_nic_byte(dev, SYS_ISO_CTRL+1, temp);

	
	Bytetemp = read_nic_byte(dev, EFUSE_TEST+3);
	temp = Bytetemp | 0x80;
	write_nic_byte(dev, EFUSE_TEST+3, temp);

	write_nic_byte(dev, 0x2F8, 0x3);
	
	write_nic_byte(dev, EFUSE_CTRL+3, 0x72);
	
}	/* EFUSE_Initialize */


/*-----------------------------------------------------------------------------
 * Function:	EFUSE_Read1Byte
 *
 * Overview:	Copy from WMAC fot EFUSE read 1 byte.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 09/23/2008 	MHC		Copy from WMAC.
 *
 *---------------------------------------------------------------------------*/
extern	u8	
EFUSE_Read1Byte(	
	struct net_device *dev, 
	u16		Address)
{
	u8	data;
	u8	Bytetemp = {0x00};
	u8	temp = {0x00};
	u32	k=0;

	if (Address < EFUSE_REAL_CONTENT_LEN)	
	{
		temp = Address & 0xFF;	
		write_nic_byte(dev, EFUSE_CTRL+1, temp);	
		Bytetemp = read_nic_byte(dev, EFUSE_CTRL+2);	
		temp = ((Address >> 8) & 0x03) | (Bytetemp & 0xFC);	
		write_nic_byte(dev, EFUSE_CTRL+2, temp);	

		Bytetemp = read_nic_byte(dev, EFUSE_CTRL+3);
		temp = Bytetemp & 0x7F;
		write_nic_byte(dev, EFUSE_CTRL+3, temp);

		Bytetemp = read_nic_byte(dev, EFUSE_CTRL+3);
		while(!(Bytetemp & 0x80))
		{				
			Bytetemp = read_nic_byte(dev, EFUSE_CTRL+3);
			k++;
			if(k==1000)
			{
				k=0;
				break;
			}
		}
		data=read_nic_byte(dev, EFUSE_CTRL);
		return data;
	}
	else
		return 0xFF;
	
}	/* EFUSE_Read1Byte */


/*-----------------------------------------------------------------------------
 * Function:	EFUSE_Write1Byte
 *
 * Overview:	Copy from WMAC fot EFUSE write 1 byte.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 09/23/2008 	MHC		Copy from WMAC.
 *
 *---------------------------------------------------------------------------*/
extern	void	
EFUSE_Write1Byte(	
	struct net_device *dev, 
	u16		Address,
	u8		Value)
{
	u8	Bytetemp = {0x00};
	u8	temp = {0x00};
	u32	k=0;

	
	if( Address < EFUSE_REAL_CONTENT_LEN)	
	{
		write_nic_byte(dev, EFUSE_CTRL, Value);

		temp = Address & 0xFF;	
		write_nic_byte(dev, EFUSE_CTRL+1, temp);	
		Bytetemp = read_nic_byte(dev, EFUSE_CTRL+2);	
		
		temp = ((Address >> 8) & 0x03) | (Bytetemp & 0xFC);	
		write_nic_byte(dev, EFUSE_CTRL+2, temp);	

		Bytetemp = read_nic_byte(dev, EFUSE_CTRL+3);
		temp = Bytetemp | 0x80;
		write_nic_byte(dev, EFUSE_CTRL+3, temp);

		Bytetemp = read_nic_byte(dev, EFUSE_CTRL+3);
		while(Bytetemp & 0x80)
		{
			Bytetemp = read_nic_byte(dev, EFUSE_CTRL+3);			
			k++;
			if(k==100)
			{
				k=0;
				break;
			}
		}
	}
	
}	/* EFUSE_Write1Byte */


static	void
ReadEFuseByte(
		struct net_device *dev,
		u16 		_offset, 
		u8 		*pbuf) 
{
	u32  	value32;
	u8 	readbyte;
	u16 	retry;
	

	write_nic_byte(dev, EFUSE_CTRL+1, (_offset & 0xff));  		
	readbyte = read_nic_byte(dev, EFUSE_CTRL+2);
	write_nic_byte(dev, EFUSE_CTRL+2, ((_offset >> 8) & 0x03) | (readbyte & 0xfc));  		

	readbyte = read_nic_byte(dev, EFUSE_CTRL+3);		
	write_nic_byte(dev, EFUSE_CTRL+3, (readbyte & 0x7f));  	
	
	retry = 0;
	value32 = read_nic_dword(dev, EFUSE_CTRL);
	while(!(((value32 >> 24) & 0xff) & 0x80)  && (retry<10000))
	{
		value32 = read_nic_dword(dev, EFUSE_CTRL);
		retry++;
	}

	udelay(50);
	value32 = read_nic_dword(dev, EFUSE_CTRL);
	
	*pbuf = (u8)(value32 & 0xff);
}


void
ReadEFuse(
	struct net_device *dev,
	u16		 _offset,
	u16 		_size_byte,
	u8      	*pbuf
	)
{

	struct r8192_priv *priv = rtllib_priv(dev);
	u8  	efuseTbl[EFUSE_MAP_LEN];
	u8  	rtemp8[1];
	u16 	eFuse_Addr = 0;
	u8  	offset, wren;
	u16  	i, j;
	u16 	eFuseWord[EFUSE_MAX_SECTION][EFUSE_MAX_WORD_UNIT];
	u16	efuse_utilized = 0;
	u16	efuse_usage = 0;
#ifdef RTL8192SE
#ifdef EFUSE_REPG_WORKAROUND 
	bool	efuse_RePGSec1Flag;
	u8	efuse_RePGData[8];
	u16	sec1_addr = 0, sec5_addr = 0;
#endif
#endif

	if((_offset + _size_byte)>EFUSE_MAP_LEN)
	{
		return;
	}

	for (i = 0; i < EFUSE_MAX_SECTION; i++)
		for (j = 0; j < EFUSE_MAX_WORD_UNIT; j++)
			eFuseWord[i][j] = 0xFFFF;


	ReadEFuseByte(dev, eFuse_Addr, rtemp8);	
	if(*rtemp8 != 0xFF)
	{
		efuse_utilized++;
		RTPRINT(FEEPROM, EFUSE_READ_ALL, ("Addr=%d\n", eFuse_Addr));
		eFuse_Addr++;
	}

	while((*rtemp8 != 0xFF) && (eFuse_Addr < EFUSE_REAL_CONTENT_LEN))
	{
		offset = ((*rtemp8 >> 4) & 0x0f);
		
#ifdef RTL8192SE 
#ifdef EFUSE_REPG_WORKAROUND 
		if (IS_HARDWARE_TYPE_8192SE(dev))
		{
			if (offset == 1)
			{
				sec1_addr = eFuse_Addr;	
			}
			else if (offset == 5)
			{
				sec5_addr = eFuse_Addr;
			}
		
			if (sec1_addr > sec5_addr)
				priv->efuse_RePGSec1Flag =efuse_RePGSec1Flag= false;			
			else
				priv->efuse_RePGSec1Flag =efuse_RePGSec1Flag= true;
#ifdef MERGE_TO_DO
			Adapter->HalFunc.SetHalDefVarHandler(dev, HW_DEF_EFUSE_REPG_SECTION1_FLAG, &(efuse_RePGSec1Flag));
		     
#endif
}
#endif
#endif

		if(offset < EFUSE_MAX_SECTION)
		{
			wren = (*rtemp8 & 0x0f);
			RTPRINT(FEEPROM, EFUSE_READ_ALL, ("Offset-%d Worden=%x\n", offset, wren));

			for(i=0; i<EFUSE_MAX_WORD_UNIT; i++)
			{
				if(!(wren & 0x01))
				{
					RTPRINT(FEEPROM, EFUSE_READ_ALL, ("Addr=%d\n", eFuse_Addr));
					ReadEFuseByte(dev, eFuse_Addr, rtemp8);	eFuse_Addr++;
					efuse_utilized++;
					eFuseWord[offset][i] = (*rtemp8 & 0xff);
					

					if(eFuse_Addr >= EFUSE_REAL_CONTENT_LEN) 
						break;

					RTPRINT(FEEPROM, EFUSE_READ_ALL, ("Addr=%d\n", eFuse_Addr));
					ReadEFuseByte(dev, eFuse_Addr, rtemp8);	eFuse_Addr++;
					efuse_utilized++;
					eFuseWord[offset][i] |= (((u16)*rtemp8 << 8) & 0xff00);

					if(eFuse_Addr >= EFUSE_REAL_CONTENT_LEN) 
						break;
				}
				
				wren >>= 1;
				
			}
		}

		RTPRINT(FEEPROM, EFUSE_READ_ALL, ("Addr=%d\n", eFuse_Addr));
		ReadEFuseByte(dev, eFuse_Addr, rtemp8);	
		if(*rtemp8 != 0xFF && (eFuse_Addr < 512))
		{
			efuse_utilized++;
			eFuse_Addr++;
		}
	}

	for(i=0; i<EFUSE_MAX_SECTION; i++)
	{
		for(j=0; j<EFUSE_MAX_WORD_UNIT; j++)
		{
			efuseTbl[(i*8)+(j*2)]=(eFuseWord[i][j] & 0xff);
			efuseTbl[(i*8)+((j*2)+1)]=((eFuseWord[i][j] >> 8) & 0xff);
#ifdef RTL8192SE
#if (defined EFUSE_REPG_WORKAROUND) && (HAL_CODE_BASE==RTL8192_S)
			if ((i == 1) && (efuse_RePGSec1Flag == true))
			{
				priv->efuse_RePGData[j*2]=efuse_RePGData[j*2]=(eFuseWord[i][j] & 0xff);
				priv->efuse_RePGData[(j*2)+1]=efuse_RePGData[(j*2)+1]=((eFuseWord[i][j] >> 8) & 0xff);
			}
#endif
#endif
		}
	}


#ifdef RTL8192SE
#if (defined EFUSE_REPG_WORKAROUND)  && (HAL_CODE_BASE==RTL8192_S)
#ifdef MERGE_TO_DO
	Adapter->HalFunc.SetHalDefVarHandler(dev, HW_DEF_EFUSE_REPG_DATA, (u8*)efuse_RePGData);
#endif
#endif
#endif

	for(i=0; i<_size_byte; i++)
	{		
		pbuf[i] = efuseTbl[_offset+i];
	}

	priv->EfuseUsedBytes = efuse_utilized;
	efuse_usage = (u8)((efuse_utilized*100)/EFUSE_REAL_CONTENT_LEN);
	priv->EfuseUsedPercentage = efuse_usage;	
	priv->rtllib->SetHwRegHandler(dev, HW_VAR_EFUSE_BYTES, (u8*)&efuse_utilized);
	priv->rtllib->SetHwRegHandler(dev, HW_VAR_EFUSE_USAGE, (u8*)&efuse_usage);
}


extern	bool
EFUSE_ShadowUpdateChk(
	struct net_device *dev)
{

	struct r8192_priv *priv = rtllib_priv(dev);
	u8	SectionIdx, i, Base;
	u16	WordsNeed = 0, HdrNum = 0, TotalBytes = 0, EfuseUsed = 0;
	bool		bWordChanged, bResult = true;

	for (SectionIdx = 0; SectionIdx < 16; SectionIdx++)
	{	
		Base = SectionIdx * 8;
		bWordChanged = false;
		
		for (i = 0; i < 8; i=i+2)
		{
			if((priv->EfuseMap[EFUSE_INIT_MAP][Base+i] != 
				priv->EfuseMap[EFUSE_MODIFY_MAP][Base+i]) ||
				(priv->EfuseMap[EFUSE_INIT_MAP][Base+i+1] != 
				priv->EfuseMap[EFUSE_MODIFY_MAP][Base+i+1]))
			{
				WordsNeed++;
				bWordChanged = true;
			}
		}

		if( bWordChanged==true )
			HdrNum++;
	}
	
	TotalBytes = HdrNum + WordsNeed*2;
	EfuseUsed = priv->EfuseUsedBytes;

	if( (TotalBytes + EfuseUsed) >= (EFUSE_MAX_SIZE-EFUSE_OOB_PROTECT_BYTES))
		bResult = false;
	
	
	return bResult;
}



/*-----------------------------------------------------------------------------
 * Function:	EFUSE_ShadowRead
 *
 * Overview:	Read from efuse init map !!!!!
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/12/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
extern	void
EFUSE_ShadowRead(
	struct net_device *dev,
	u8		Type,
	u16		Offset,
	u32		*Value	)
{
	if (Type == 1)
		efuse_ShadowRead1Byte(dev, Offset, (u8 *)Value);
	else if (Type == 2)
		efuse_ShadowRead2Byte(dev, Offset, (u16 *)Value);
	else if (Type == 4)
		efuse_ShadowRead4Byte(dev, Offset, (u32 *)Value);
	
}	


/*-----------------------------------------------------------------------------
 * Function:	EFUSE_ShadowWrite
 *
 * Overview:	Write efuse modify map for later update operation to use!!!!!
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/12/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
extern	void
EFUSE_ShadowWrite(
	struct net_device *dev,
	u8		Type,
	u16		Offset,
	u32		Value	)
{
	if(IS_HARDWARE_TYPE_8192SE(dev))
	{
	if (Offset >= 0x18 && Offset <= 0x1F)
		return;
	}

	if (Type == 1)
		efuse_ShadowWrite1Byte(dev, Offset, (u8)Value);
	else if (Type == 2)
		efuse_ShadowWrite2Byte(dev, Offset, (u16)Value);
	else if (Type == 4)
		efuse_ShadowWrite4Byte(dev, Offset, (u32)Value);
	
}	


/*-----------------------------------------------------------------------------
 * Function:	EFUSE_ShadowUpdate
 *
 * Overview:	Compare init and modify map to update Efuse!!!!!
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/12/2008 	MHC		Create Version 0.
 * 12/11/2008	MHC		92SE PH workaround to prevent HW autoload fail.
 * 12/30/2008	Roger	Fix the bug that EFUSE will writed out-of boundary.
 * 02/16/2009	Roger	Revise PCIe autoload fail case and compatible with USB interface to 
 *						overcome MP issue.
 *
 *---------------------------------------------------------------------------*/
extern	bool
EFUSE_ShadowUpdate(
	struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u16			i, offset, base = 0;
	u8			word_en = 0x0F;
	u8			first_pg = false; 


	if(!EFUSE_ShadowUpdateChk(dev))
	{
		efuse_ReadAllMap(dev, &priv->EfuseMap[EFUSE_INIT_MAP][0]);
		memcpy((void*)&priv->EfuseMap[EFUSE_MODIFY_MAP][0], 
			(void*)&priv->EfuseMap[EFUSE_INIT_MAP][0], HWSET_MAX_SIZE);

		return false;
	}

	efuse_PowerSwitch(dev, true, true);

	for (offset = 0; offset < 16; offset++)
	{ 

	
		if(IS_HARDWARE_TYPE_8192SE(dev))
		{
			if (offset == 3)
				continue;
		}

		word_en = 0x0F;		
		base = offset * 8;

		for (i = 0; i < 8; i++)
		{
						
			if(IS_HARDWARE_TYPE_8192SE(dev))
			{
				if (offset == 0 && priv->EfuseMap[EFUSE_INIT_MAP][base+i] == 0xFF)				
					first_pg = true;		
			}
			{
			}

			if (first_pg == true && offset == 1 && IS_HARDWARE_TYPE_8192SE(dev))
			{
				continue;
			}

			if (first_pg == true)
			{
			
				word_en &= ~(BIT(i/2));

				priv->EfuseMap[EFUSE_INIT_MAP][base+i] = 
				priv->EfuseMap[EFUSE_MODIFY_MAP][base+i];
			}
			else
			{
			
				if (	priv->EfuseMap[EFUSE_INIT_MAP][base+i] != 
					priv->EfuseMap[EFUSE_MODIFY_MAP][base+i])
				{
					word_en &= ~(BIT(i/2));

					priv->EfuseMap[EFUSE_INIT_MAP][base+i] = 
					priv->EfuseMap[EFUSE_MODIFY_MAP][base+i];
				}
			}
		}
		
		if (word_en != 0x0F)
		{
			u8	tmpdata[8];

			memcpy((void*)tmpdata, 
			(void*)(&priv->EfuseMap[EFUSE_MODIFY_MAP][base]), 8);
			RT_PRINT_DATA(COMP_INIT, DBG_LOUD, ("U-EFUSE\n"), tmpdata, 8);

			if(!efuse_PgPacketWrite(dev,(u8)offset,word_en,tmpdata))
			{
				break;
			}
		}
		
	}

	if (first_pg == true && IS_HARDWARE_TYPE_8192SE(dev))
	{
		u8	tmpdata[8];

		memcpy((void*)tmpdata, 
		(void*)(&priv->EfuseMap[EFUSE_MODIFY_MAP][8]), 8);
		RT_PRINT_DATA(COMP_INIT, DBG_LOUD, ("U-EFUSE(Re-PG Section 1): \n"), tmpdata, 8);
		efuse_PgPacketWrite(dev, 1, 0x0, tmpdata); 
	}
	else
	{
	}

	efuse_PowerSwitch(dev, true, false);
		
	efuse_ReadAllMap(dev, &priv->EfuseMap[EFUSE_INIT_MAP][0]);
	memcpy((void*)&priv->EfuseMap[EFUSE_MODIFY_MAP][0], 
		(void*)&priv->EfuseMap[EFUSE_INIT_MAP][0], HWSET_MAX_SIZE);
		
	return true;
}	


/*-----------------------------------------------------------------------------
 * Function:	EFUSE_ShadowMapUpdate
 *
 * Overview:	Transfer current EFUSE content to shadow init and modify map.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/13/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
extern	void 
EFUSE_ShadowMapUpdate(
	struct net_device *dev)
{	
	struct r8192_priv *priv = rtllib_priv(dev);
		
	if (priv->AutoloadFailFlag == true )
	{
		memset((void*)(&priv->EfuseMap[EFUSE_INIT_MAP][0]), 128, 0xFF);
	}
	else
	{
		efuse_ReadAllMap(dev, &priv->EfuseMap[EFUSE_INIT_MAP][0]);
	}
	
	memcpy((void*)&priv->EfuseMap[EFUSE_MODIFY_MAP][0], 
	(void*)&priv->EfuseMap[EFUSE_INIT_MAP][0], HWSET_MAX_SIZE);
	
}	


/*-----------------------------------------------------------------------------
 * Function:	EFUSE_ForceWriteVendorId
 *
 * Overview:	HW autoload bug workaround method!!!!.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 12/03/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
extern	void 
EFUSE_ForceWriteVendorId(
	struct net_device *dev)
{
	u8	tmpdata[8] = {0xFF, 0xFF, 0xEC, 0x10, 0xFF, 0xFF, 0xFF, 0xFF};

	efuse_PowerSwitch(dev, true, true);
		
	efuse_PgPacketWrite(dev, 1, 0xD, tmpdata);

	efuse_PowerSwitch(dev, true, false);
	
}	


extern	void 
EFUSE_RePgSection(
	struct net_device *dev,
	u8		SectionIdx)
{
#ifdef EFUSE_REPG_WORKAROUND 

	u8		efuse_RePGData[8];
	struct r8192_priv *priv = rtllib_priv(dev);
	
	
	if (priv->efuse_RePGSec1Flag==true)
	{
		efuse_PowerSwitch(dev, true, true);		
		RT_PRINT_DATA(COMP_INIT, DBG_LOUD, ("EFUSE REPG: \n"), efuse_RePGData, 8);
				
		efuse_PgPacketWrite(dev, SectionIdx, 0x0d, efuse_RePGData);	
		efuse_PowerSwitch(dev, true, false);
		priv->efuse_RePGSec1Flag = false; 
	}
#endif	
}	


/*-----------------------------------------------------------------------------
 * Function:	efuse_ShadowRead1Byte
 *			efuse_ShadowRead2Byte
 *			efuse_ShadowRead4Byte
 *
 * Overview:	Read from efuse init map by one/two/four bytes !!!!!
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/12/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
static	void
efuse_ShadowRead1Byte(
	struct net_device *dev,
	u16		Offset,
	u8		*Value	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	*Value = priv->EfuseMap[EFUSE_MODIFY_MAP][Offset];
	
}	

static	void
efuse_ShadowRead2Byte(
	struct net_device *dev,
	u16		Offset,
	u16	*Value	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	*Value = priv->EfuseMap[EFUSE_MODIFY_MAP][Offset];
	*Value |= priv->EfuseMap[EFUSE_MODIFY_MAP][Offset+1]<<8;
	
}	

static	void
efuse_ShadowRead4Byte(
	struct net_device *dev,
	u16		Offset,
	u32	*Value	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	*Value = priv->EfuseMap[EFUSE_MODIFY_MAP][Offset];
	*Value |= priv->EfuseMap[EFUSE_MODIFY_MAP][Offset+1]<<8;
	*Value |= priv->EfuseMap[EFUSE_MODIFY_MAP][Offset+2]<<16;
	*Value |= priv->EfuseMap[EFUSE_MODIFY_MAP][Offset+3]<<24;
	
}	



/*-----------------------------------------------------------------------------
 * Function:	efuse_ShadowWrite1Byte
 *			efuse_ShadowWrite2Byte
 *			efuse_ShadowWrite4Byte
 *
 * Overview:	Write efuse modify map by one/two/four byte.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/12/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
static	void
efuse_ShadowWrite1Byte(
	struct net_device *dev,
	u16		Offset,
	u8		Value	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	priv->EfuseMap[EFUSE_MODIFY_MAP][Offset] = Value;
	
}	

static	void
efuse_ShadowWrite2Byte(
	struct net_device *dev,
	u16		Offset,
	u16		Value	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	priv->EfuseMap[EFUSE_MODIFY_MAP][Offset] = Value&0x00FF;
	priv->EfuseMap[EFUSE_MODIFY_MAP][Offset+1] = Value>>8;
	
}	

static	void
efuse_ShadowWrite4Byte(
	struct net_device *dev,
	u16		Offset,
	u32		Value	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	priv->EfuseMap[EFUSE_MODIFY_MAP][Offset] = (u8)(Value&0x000000FF);
	priv->EfuseMap[EFUSE_MODIFY_MAP][Offset+1] = (u8)((Value>>8)&0x0000FF);
	priv->EfuseMap[EFUSE_MODIFY_MAP][Offset+2] = (u8)((Value>>16)&0x00FF);
	priv->EfuseMap[EFUSE_MODIFY_MAP][Offset+3] = (u8)((Value>>24)&0xFF);
	
}	


/*  11/16/2008 MH Read one byte from real Efuse. */
static	int
efuse_OneByteRead(
	struct net_device *dev, 
	u16		addr,
	u8		*data)
{
	u8 tmpidx = 0;
	int bResult;
	
	write_nic_byte(dev, EFUSE_CTRL+1, (u8)(addr&0xff));		
	write_nic_byte(dev, EFUSE_CTRL+2, ((u8)((addr>>8) &0x03) ) | 
	(read_nic_byte(dev, EFUSE_CTRL+2)&0xFC ));	

#if 1
	write_nic_byte(dev, EFUSE_CTRL+3,  0x72);
#else
	PlatformEFIOWrite4Byte(pAdapter,  REG_EFUSE_CTRL, (PlatformEFIORead4Byte(pAdapter,REG_EFUSE_CTRL)&( ~EF_FLAG)));
#endif

	while(!(0x80 &read_nic_byte(dev, EFUSE_CTRL+3))&&(tmpidx<100))
	{
		tmpidx++;
	}
	if(tmpidx<100)
	{			
		*data=read_nic_byte(dev, EFUSE_CTRL);		
		bResult = _TRUE;
	}
	else
	{
		*data = 0xff;	
		bResult = _FALSE;
	}
	return bResult;
}	
		
/*  11/16/2008 MH Write one byte to reald Efuse. */
static	int
efuse_OneByteWrite(
	struct net_device *dev,  
	u16		addr, 
	u8		data)
{
	u8 tmpidx = 0;
	bool bResult;
	


	write_nic_byte(dev, EFUSE_CTRL+1, (u8)(addr&0xff));		
	write_nic_byte(dev, EFUSE_CTRL+2, 
	(read_nic_byte(dev, EFUSE_CTRL+2)&0xFC ) |(u8)((addr>>8)&0x03) );

	write_nic_byte(dev, EFUSE_CTRL, data);

#if 1
	write_nic_byte(dev, EFUSE_CTRL+3, 0xF2);
#else
	PlatformEFIOWrite4Byte(pAdapter,  REG_EFUSE_CTRL, (PlatformEFIORead4Byte(pAdapter,REG_EFUSE_CTRL)|( EF_FLAG)));
#endif
		
	while((0x80 &  read_nic_byte(dev, EFUSE_CTRL+3)) && (tmpidx<100) ){
		tmpidx++;
	}
	
	if(tmpidx<100)
	{					
		bResult = _TRUE;
	}
	else
	{			
		bResult = _FALSE;
	}		
	
	return bResult;	
}	


/*-----------------------------------------------------------------------------
 * Function:	efuse_ReadAllMap
 *
 * Overview:	Read All Efuse content
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/11/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
static	void 
efuse_ReadAllMap(
	struct net_device *dev, 
	u8		*Efuse)
{	
		
	efuse_PowerSwitch(dev, false, true);

	ReadEFuse(dev, 0, 128, Efuse);

#if 0	
	for(offset = 0;offset<16;offset++)	
	{		
		memset((void*)pg_data, 8, 0xff);
		efuse_PgPacketRead(pAdapter,offset,pg_data);

		memcpy((void*)&Efuse[offset*8], (void*)pg_data, 8);
	}
#endif
	efuse_PowerSwitch(dev, false, false);

}	


/*-----------------------------------------------------------------------------
 * Function:	efuse_WriteAllMap
 *
 * Overview:	Write All Efuse content
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/11/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
#ifdef MERGE_TO_DO
static	void
efuse_WriteAllMap(
	struct net_device *dev,
	u8 			*eeprom,
	int			eeprom_size)
{	
	unsigned char word_en = 0x00;

	unsigned char tmpdata[8];
	unsigned char offset;

	efuse_PowerSwitch(dev, true, true);

	for(offset=0 ; offset< eeprom_size/PGPKT_DATA_SIZE ; offset++)
	{
		if (IS_HARDWARE_TYPE_8192SE(dev))
		{
			if (offset == 3/* || offset > 9*/)	
				continue;
			else
				word_en = 0x00;
		}
		
		
		memcpy((void*)tmpdata, (void*)(eeprom+(offset*PGPKT_DATA_SIZE)), 8);

		RT_PRINT_DATA(COMP_INIT, DBG_LOUD, ("EFUSE\t"), tmpdata, 8);
	
		efuse_PgPacketWrite(dev,offset,word_en,tmpdata);

		
	}

	efuse_PowerSwitch(dev, true, false);
	
}	
#endif

/*-----------------------------------------------------------------------------
 * Function:	efuse_PgPacketRead
 *
 * Overview:	Receive dedicated Efuse are content. For92s, we support 16
 *				area now. It will return 8 bytes content for every area.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/16/2008 	MHC		Reorganize code Arch and assign as local API.
 *
 *---------------------------------------------------------------------------*/
static	int
efuse_PgPacketRead(	struct net_device *dev,
					u8			offset,
					u8			*data)
{	
	u8 ReadState = PG_STATE_HEADER;	
	
	int bContinual = _TRUE;
	int bDataEmpty = _TRUE ;	

	u8 efuse_data,word_cnts=0;
	u16 efuse_addr = 0;
	u8 hoffset=0,hworden=0;	
	u8 tmpidx=0;
	u8 tmpdata[8];
	
	if(data==NULL)	return _FALSE;
	if(offset>15)		return _FALSE;	
	

	memset((void*)data, sizeof(u8)*PGPKT_DATA_SIZE, 0xff);
	memset((void*)tmpdata, sizeof(u8)*PGPKT_DATA_SIZE, 0xff);
	
	
	while(bContinual && (efuse_addr  < EFUSE_MAX_SIZE) )
	{			
		if(ReadState & PG_STATE_HEADER)
		{
			if(efuse_OneByteRead(dev, efuse_addr ,&efuse_data)&&(efuse_data!=0xFF)){				
				hoffset = (efuse_data>>4) & 0x0F;
				hworden =  efuse_data & 0x0F;									
				word_cnts = efuse_CalculateWordCnts(hworden);
				bDataEmpty = _TRUE ;

				if(hoffset==offset){
					for(tmpidx = 0;tmpidx< word_cnts*2 ;tmpidx++){
						if(efuse_OneByteRead(dev, efuse_addr+1+tmpidx ,&efuse_data) ){
							tmpdata[tmpidx] = efuse_data;
							if(efuse_data!=0xff){						
								bDataEmpty = _TRUE;
							}
						}					
					}
					if(bDataEmpty==_TRUE){
						ReadState = PG_STATE_DATA;							
					}else{
						efuse_addr = efuse_addr + (word_cnts*2)+1;
						ReadState = PG_STATE_HEADER; 
					}
				}
				else{
					efuse_addr = efuse_addr + (word_cnts*2)+1;
					ReadState = PG_STATE_HEADER; 
				}
				
			}
			else{
				bContinual = _FALSE ;
			}
		}		
		else if(ReadState & PG_STATE_DATA)
		{
			efuse_WordEnableDataRead(hworden,tmpdata,data);
			efuse_addr = efuse_addr + (word_cnts*2)+1;
			ReadState = PG_STATE_HEADER; 
		}
		
	}			
	
	
	if(	(data[0]==0xff) &&(data[1]==0xff) && (data[2]==0xff)  && (data[3]==0xff) &&
		(data[4]==0xff) &&(data[5]==0xff) && (data[6]==0xff)  && (data[7]==0xff))
		return _FALSE;
	else
		return _TRUE;

}	


/*-----------------------------------------------------------------------------
 * Function:	efuse_PgPacketWrite
 *
 * Overview:	Send A G package for different section in real efuse area.
 *				For 92S, One PG package contain 8 bytes content and 4 word
 *				unit. PG header = 0x[bit7-4=offset][bit3-0word enable]
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/16/2008 	MHC		Reorganize code Arch and assign as local API.
 *
 *---------------------------------------------------------------------------*/
static	int 
efuse_PgPacketWrite(struct net_device *dev, 
					u8 			offset,
					u8			word_en,
					u8			*data)
{
	u8 WriteState = PG_STATE_HEADER;		

	int bContinual = _TRUE,bDataEmpty=_TRUE, bResult = _TRUE;
	u16 efuse_addr = 0;
	u8 efuse_data;

	u8 pg_header = 0;

	u8 tmp_word_cnts=0,target_word_cnts=0;
	u8 tmp_header,match_word_en,tmp_word_en;

	PGPKT_STRUCT target_pkt;	
	PGPKT_STRUCT tmp_pkt;
	
	u8 originaldata[sizeof(u8)*8];
	u8 tmpindex = 0,badworden = 0x0F;

	static int repeat_times = 0;

	if( efuse_GetCurrentSize(dev) >= (EFUSE_MAX_SIZE-EFUSE_OOB_PROTECT_BYTES))
	{
		RTPRINT(FEEPROM, EFUSE_PG, ("efuse_PgPacketWrite error \n"));
		return _FALSE;
	}

	target_pkt.offset = offset;
	target_pkt.word_en= word_en;

	memset((void*)target_pkt.data, sizeof(u8)*8, 0xFF);
	
	efuse_WordEnableDataRead(word_en,data,target_pkt.data);
	target_word_cnts = efuse_CalculateWordCnts(target_pkt.word_en);

	RTPRINT(FEEPROM, EFUSE_PG, ("EFUSE Power ON\n"));

	while( bContinual && (efuse_addr  < (EFUSE_MAX_SIZE-EFUSE_OOB_PROTECT_BYTES)) )
	{
		
		if(WriteState==PG_STATE_HEADER)
		{	
			bDataEmpty=_TRUE;
			badworden = 0x0F;		
			RTPRINT(FEEPROM, EFUSE_PG, ("EFUSE PG_STATE_HEADER\n"));
			if (	efuse_OneByteRead(dev, efuse_addr ,&efuse_data) &&
				(efuse_data!=0xFF))
			{ 	
				tmp_header  =  efuse_data;
				
				tmp_pkt.offset 	= (tmp_header>>4) & 0x0F;
				tmp_pkt.word_en 	= tmp_header & 0x0F;					
				tmp_word_cnts =  efuse_CalculateWordCnts(tmp_pkt.word_en);

				if(tmp_pkt.offset  != target_pkt.offset)
				{
					efuse_addr = efuse_addr + (tmp_word_cnts*2) +1; 
					#if (EFUSE_ERROE_HANDLE == 1)
					WriteState = PG_STATE_HEADER;
					#endif
				}
				else
				{	
					for(tmpindex=0 ; tmpindex<(tmp_word_cnts*2) ; tmpindex++)
					{
						if(efuse_OneByteRead(dev, (efuse_addr+1+tmpindex) ,&efuse_data)&&(efuse_data != 0xFF)){
							bDataEmpty = _FALSE;	
						}
					}	
					if(bDataEmpty == _FALSE)
					{						
						efuse_addr = efuse_addr + (tmp_word_cnts*2) +1; 
						#if (EFUSE_ERROE_HANDLE == 1)
						WriteState=PG_STATE_HEADER;
						#endif
					}
					else
					{
						match_word_en = 0x0F;
						if(   !( (target_pkt.word_en&BIT0)|(tmp_pkt.word_en&BIT0)  ))
						{
							 match_word_en &= (~BIT0);
						}	
						if(   !( (target_pkt.word_en&BIT1)|(tmp_pkt.word_en&BIT1)  ))
						{
							 match_word_en &= (~BIT1);
						}
						if(   !( (target_pkt.word_en&BIT2)|(tmp_pkt.word_en&BIT2)  ))
						{
							 match_word_en &= (~BIT2);
						}
						if(   !( (target_pkt.word_en&BIT3)|(tmp_pkt.word_en&BIT3)  ))
						{
							 match_word_en &= (~BIT3);
						}					
												
						if((match_word_en&0x0F)!=0x0F)
						{							
							badworden = efuse_WordEnableDataWrite(dev,efuse_addr+1, tmp_pkt.word_en ,target_pkt.data);
							
							if(0x0F != (badworden&0x0F))
							{														
								u8 reorg_offset = offset;
								u8 reorg_worden=badworden;								
								efuse_PgPacketWrite(dev,reorg_offset,reorg_worden,originaldata);	
							}	

							tmp_word_en = 0x0F;	
							if(  (target_pkt.word_en&BIT0)^(match_word_en&BIT0)  )
							{
								tmp_word_en &= (~BIT0);
							}
							if(   (target_pkt.word_en&BIT1)^(match_word_en&BIT1) )
							{
								tmp_word_en &=  (~BIT1);
							}
							if(   (target_pkt.word_en&BIT2)^(match_word_en&BIT2) )
							{
								tmp_word_en &= (~BIT2);
							}						
							if(   (target_pkt.word_en&BIT3)^(match_word_en&BIT3) )
							{
								tmp_word_en &=(~BIT3);
							}							
						
							if((tmp_word_en&0x0F)!=0x0F){
								efuse_addr = efuse_GetCurrentSize(dev);
								target_pkt.offset = offset;
								target_pkt.word_en= tmp_word_en;					
							}else{								
								bContinual = _FALSE;
							}
							#if (EFUSE_ERROE_HANDLE == 1)
							WriteState=PG_STATE_HEADER;
							repeat_times++;
							if(repeat_times>EFUSE_REPEAT_THRESHOLD_){
								bContinual = _FALSE;
								bResult = _FALSE;
							}
							#endif
						}
						else{
							efuse_addr = efuse_addr + (2*tmp_word_cnts) +1;
							target_pkt.offset = offset;
							target_pkt.word_en= target_pkt.word_en;					
							#if (EFUSE_ERROE_HANDLE == 1)
							WriteState=PG_STATE_HEADER;
							#endif
						}		
					}				
				}				
				RTPRINT(FEEPROM, EFUSE_PG, ("EFUSE PG_STATE_HEADER-1\n"));
			}
			else		
			{
				pg_header = ((target_pkt.offset << 4)&0xf0) |target_pkt.word_en;

				efuse_OneByteWrite(dev,efuse_addr, pg_header);
				efuse_OneByteRead(dev,efuse_addr, &tmp_header);		
		
				if(tmp_header == pg_header)
				{ 
					WriteState = PG_STATE_DATA;						
				}				
				#if (EFUSE_ERROE_HANDLE == 1)
				else if(tmp_header == 0xFF){
					WriteState = PG_STATE_HEADER;					
					repeat_times++;
					if(repeat_times>EFUSE_REPEAT_THRESHOLD_){
						bContinual = _FALSE;
						bResult = _FALSE;
					}
				}
				#endif
				else
				{
					tmp_pkt.offset = (tmp_header>>4) & 0x0F;
					tmp_pkt.word_en=  tmp_header & 0x0F;					
					tmp_word_cnts =  efuse_CalculateWordCnts(tmp_pkt.word_en);
																											
					memset((void*)originaldata, sizeof(u8)*8, 0xff);
					
					if(efuse_PgPacketRead( dev, tmp_pkt.offset,originaldata))
					{	
						badworden = efuse_WordEnableDataWrite(dev,efuse_addr+1,tmp_pkt.word_en,originaldata);	
						if(0x0F != (badworden&0x0F))
						{														
							u8 reorg_offset = tmp_pkt.offset;
							u8 reorg_worden=badworden;								
							efuse_PgPacketWrite(dev,reorg_offset,reorg_worden,originaldata);	
							efuse_addr = efuse_GetCurrentSize(dev);	 
						}
						else{
							efuse_addr = efuse_addr + (tmp_word_cnts*2) +1; 
						}
					}
					else
					{
						efuse_addr = efuse_addr + (tmp_word_cnts*2) +1; 
					}

					#if (EFUSE_ERROE_HANDLE == 1)
					WriteState=PG_STATE_HEADER;	
					repeat_times++;
					if(repeat_times>EFUSE_REPEAT_THRESHOLD_){
						bContinual = _FALSE;
						bResult = _FALSE;
					}
					#endif
					
					RTPRINT(FEEPROM, EFUSE_PG, ("EFUSE PG_STATE_HEADER-2\n"));
				}

			}

		}
		else if(WriteState==PG_STATE_DATA) 
		{	
			RTPRINT(FEEPROM, EFUSE_PG, ("EFUSE PG_STATE_DATA\n"));
			badworden = 0x0f;
			badworden = efuse_WordEnableDataWrite(dev,efuse_addr+1,target_pkt.word_en,target_pkt.data);	
			if((badworden&0x0F)==0x0F)
			{ 
				bContinual = _FALSE;
			}
			else
			{
				efuse_addr = efuse_addr + (2*target_word_cnts) +1;
										
				target_pkt.offset = offset;
				target_pkt.word_en= badworden;		
				target_word_cnts =  efuse_CalculateWordCnts(target_pkt.word_en); 
				#if (EFUSE_ERROE_HANDLE == 1)
				WriteState=PG_STATE_HEADER;	
				repeat_times++;
				if(repeat_times>EFUSE_REPEAT_THRESHOLD_){
					bContinual = _FALSE;
					bResult = _FALSE;
				}
				#endif
				RTPRINT(FEEPROM, EFUSE_PG, ("EFUSE PG_STATE_HEADER-3\n"));
			}
		}
	}

	if(efuse_addr  >= (EFUSE_MAX_SIZE-EFUSE_OOB_PROTECT_BYTES))
	{
	}
	
	return _TRUE;
}	


/*-----------------------------------------------------------------------------
 * Function:	efuse_WordEnableDataRead
 *
 * Overview:	Read allowed word in current efuse section data.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/16/2008 	MHC		Create Version 0.
 * 11/21/2008 	MHC		Fix Write bug when we only enable late word.
 *
 *---------------------------------------------------------------------------*/
static	void
efuse_WordEnableDataRead(	u8	word_en,
							u8	*sourdata,
							u8	*targetdata)
{	



	if (!(word_en&BIT0))
	{
		targetdata[0] = sourdata[0];
		targetdata[1] = sourdata[1];
	}
	if (!(word_en&BIT1))
	{
		targetdata[2] = sourdata[2];
		targetdata[3] = sourdata[3];
	}
	if (!(word_en&BIT2))
	{
		targetdata[4] = sourdata[4];
		targetdata[5] = sourdata[5];
	}
	if (!(word_en&BIT3))
	{
		targetdata[6] = sourdata[6];
		targetdata[7] = sourdata[7];
	}
}	


/*-----------------------------------------------------------------------------
 * Function:	efuse_WordEnableDataWrite
 *
 * Overview:	Write necessary word unit into current efuse section!
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/16/2008 	MHC		Reorganize Efuse operate flow!!.
 *
 *---------------------------------------------------------------------------*/
static	u8
efuse_WordEnableDataWrite(	struct net_device *dev,
							u16		efuse_addr,
							u8		word_en, 
							u8		*data)
{		
	u16 tmpaddr = 0;
	u16 start_addr = efuse_addr;
	u8 badworden = 0x0F;
	u8 tmpdata[8]; 
	
	memset((void*)tmpdata, PGPKT_DATA_SIZE, 0xff);


	if(!(word_en&BIT0))
	{
		tmpaddr = start_addr;
		efuse_OneByteWrite(dev,start_addr++, data[0]);
		efuse_OneByteWrite(dev,start_addr++, data[1]);

		efuse_OneByteRead(dev,tmpaddr, &tmpdata[0]);
		efuse_OneByteRead(dev,tmpaddr+1, &tmpdata[1]);
		if((data[0]!=tmpdata[0])||(data[1]!=tmpdata[1])){
			badworden &= (~BIT0);
		}
	}
	if(!(word_en&BIT1))
	{
		tmpaddr = start_addr;
		efuse_OneByteWrite(dev,start_addr++, data[2]);
		efuse_OneByteWrite(dev,start_addr++, data[3]);
				
		efuse_OneByteRead(dev,tmpaddr    , &tmpdata[2]);
		efuse_OneByteRead(dev,tmpaddr+1, &tmpdata[3]);
		if((data[2]!=tmpdata[2])||(data[3]!=tmpdata[3])){
			badworden &=( ~BIT1);
		}
	}
	if(!(word_en&BIT2))
	{
		tmpaddr = start_addr;
		efuse_OneByteWrite(dev,start_addr++, data[4]);
		efuse_OneByteWrite(dev,start_addr++, data[5]);

		efuse_OneByteRead(dev,tmpaddr, &tmpdata[4]);							
		efuse_OneByteRead(dev,tmpaddr+1, &tmpdata[5]);
		if((data[4]!=tmpdata[4])||(data[5]!=tmpdata[5])){
			badworden &=( ~BIT2);
		}
	}
	if(!(word_en&BIT3))
	{
		tmpaddr = start_addr;
		efuse_OneByteWrite(dev,start_addr++, data[6]);
		efuse_OneByteWrite(dev,start_addr++, data[7]);

		efuse_OneByteRead(dev,tmpaddr, &tmpdata[6]);
		efuse_OneByteRead(dev,tmpaddr+1, &tmpdata[7]);
		if((data[6]!=tmpdata[6])||(data[7]!=tmpdata[7])){
			badworden &=( ~BIT3);
		}
	}
	return badworden;
}	


/*-----------------------------------------------------------------------------
 * Function:	efuse_PowerSwitch
 *
 * Overview:	When we want to enable write operation, we should change to 
 *				pwr on state. When we stop write, we should switch to 500k mode
 *				and disable LDO 2.5V.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/17/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
 enum{
		VOLTAGE_V25						= 0x03,
		LDOE25_SHIFT						= 28 ,
	};
static	void
efuse_PowerSwitch(
	struct net_device *dev,
	u8           bWrite,
	u8		PwrState)
{
	u8	tempval;
	u16	tmpV16;
	
#ifdef RTL8192CE
	if(PwrState == true){
		tmpV16 = read_nic_word(dev,REG_SYS_ISO_CTRL);
		if( ! (tmpV16 & PWC_EV12V ) ){
			tmpV16 |= PWC_EV12V ;
			 write_nic_word(dev,REG_SYS_ISO_CTRL,tmpV16);
		}
		tmpV16 =  read_nic_word(dev,REG_SYS_FUNC_EN);
		if( !(tmpV16 & FEN_ELDR) ){
			tmpV16 |= FEN_ELDR ;
			write_nic_word(dev,REG_SYS_FUNC_EN,tmpV16);
		}
		
		tmpV16 = read_nic_word(dev,REG_SYS_CLKR);
		if( (!(tmpV16 & LOADER_CLK_EN) )  ||(!(tmpV16 & ANA8M) ) ){
			tmpV16 |= (LOADER_CLK_EN |ANA8M ) ;
			write_nic_word(dev,REG_SYS_CLKR,tmpV16);
		}		
	} 	
#endif	

	if (PwrState == true)
	{
		if(bWrite == true){
		tempval = read_nic_byte(dev, EFUSE_TEST+3);
			tempval &= 0x0F;
			tempval |= (VOLTAGE_V25 << 4);
		write_nic_byte(dev, EFUSE_TEST+3, (tempval | 0x80));
		}

#ifdef RTL8192SE
		write_nic_byte(dev, EFUSE_CLK, 0x03);
#endif
	}
	else
	{
		if(bWrite == true){
		tempval = read_nic_byte(dev, EFUSE_TEST+3);
		write_nic_byte(dev, EFUSE_TEST+3, (tempval & 0x7F));
		}


#ifdef RTL8192SE
		write_nic_byte(dev, EFUSE_CLK, 0x02);
#endif
	}	
	
}	/* efuse_PowerSwitch */


/*-----------------------------------------------------------------------------
 * Function:	efuse_GetCurrentSize
 *
 * Overview:	Get current efuse size!!!
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/16/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
static	u16
efuse_GetCurrentSize(struct net_device *dev)
{
	int bContinual = _TRUE;

	u16 efuse_addr = 0;
	u8 hoffset=0,hworden=0;	
	u8 efuse_data,word_cnts=0;
		
	while (	bContinual && 
			efuse_OneByteRead(dev, efuse_addr ,&efuse_data) && 
			(efuse_addr  < EFUSE_MAX_SIZE) )
	{			
		if(efuse_data!=0xFF)
		{					
			hoffset = (efuse_data>>4) & 0x0F;
			hworden =  efuse_data & 0x0F;									
			word_cnts = efuse_CalculateWordCnts(hworden);
			efuse_addr = efuse_addr + (word_cnts*2)+1;						
		}
		else
		{
			bContinual = _FALSE ;			
		}
	}

	return efuse_addr;
	
}	


/*  11/16/2008 MH Add description. Get current efuse area enabled word!!. */
static u8
efuse_CalculateWordCnts(u8	word_en)
{
	u8 word_cnts = 0;
	if(!(word_en & BIT0))	word_cnts++; 
	if(!(word_en & BIT1))	word_cnts++;
	if(!(word_en & BIT2))	word_cnts++;
	if(!(word_en & BIT3))	word_cnts++;
	return word_cnts;
}	


/*-----------------------------------------------------------------------------
 * Function:	efuse_ResetLoader
 *
 * Overview:	When read Efuse Fail we must reset loader!!!!
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/22/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
extern	void
EFUSE_ResetLoader(
	struct net_device *dev)
{
	u16	tmpU2b;

	tmpU2b = read_nic_word(dev, SYS_FUNC_EN);	
	write_nic_word(dev, SYS_FUNC_EN, (tmpU2b&~(BIT12)));
	udelay(10000);	
	write_nic_word(dev, SYS_FUNC_EN, (tmpU2b|BIT12));
	udelay(10000);	
	
}	


/*-----------------------------------------------------------------------------
 * Function:	EFUSE_ProgramMap
 *
 * Overview:	Read EFUSE map file and execute PG.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/10/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/

extern	bool	
EFUSE_ProgramMap(
	struct net_device *dev,
	char* 		pFileName,
	u8		TableType)		
{
	return true;
}	/* EFUSE_ProgramMap */


/*-----------------------------------------------------------------------------
 * Function:	efuse_ParsingMap
 *
 * Overview:	
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/08/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
#ifdef MERGE_TO_DO
static	bool	
efuse_ParsingMap(
	char*			szStr,
	u32*			pu4bVal,
	u32*			pu4bMove)
{
	char*		szScan = szStr;

	if(szStr == NULL || pu4bVal == NULL || pu4bMove == NULL)
	{
		return false;
	}

	*pu4bMove = 0;
	*pu4bVal = 0;

	while(	*szScan != '\0' && 
			(*szScan == ' ' || *szScan == '\t') )
	{
		szScan++;
		(*pu4bMove)++;
	}

	if(!IsHexDigit(*szScan))
	{
		return false;
	}

	do
	{
		(*pu4bVal) <<= 4;
		*pu4bVal += MapCharToHexDigit(*szScan);

		szScan++;
		(*pu4bMove)++;
	} while(IsHexDigit(*szScan));

	return true;
	
}	/* efuse_ParsingMap */
#endif

int efuse_one_byte_rw(struct net_device *dev, u8 bRead, u16 addr, u8 *data)
{
	unsigned char tmpidx = 0;
	int bResult;
	u8 tmpv8=0;
	

	write_nic_byte(dev, EFUSE_CTRL+1, (u8)(addr&0xff));		
	tmpv8 = ((u8)((addr>>8) &0x03) ) | (read_nic_byte(dev, EFUSE_CTRL+2)&0xFC );	
	write_nic_byte(dev, EFUSE_CTRL+2, tmpv8);

	if(true==bRead){
		
		write_nic_byte(dev, EFUSE_CTRL+3,  0x72);

		while(!(0x80 & read_nic_byte(dev, EFUSE_CTRL+3)) && (tmpidx<100) ){
			tmpidx++;
		}
		if(tmpidx<100){			
			*data=read_nic_byte(dev, EFUSE_CTRL);
			bResult = _TRUE;
		}
		else
		{
			*data = 0;
			bResult = _FALSE;
		}
		
	}
	else{
		write_nic_byte(dev, EFUSE_CTRL, *data);
		
		write_nic_byte(dev, EFUSE_CTRL+3, 0xF2);
		
		while((0x80 & read_nic_byte(dev, EFUSE_CTRL+3)) && (tmpidx<100) ){
			tmpidx++;
		}
		if(tmpidx<100)
		{			
			*data=read_nic_byte(dev, EFUSE_CTRL);
			bResult = _TRUE;
		}
		else
		{
			*data = 0;
			bResult = _FALSE;
		}
		
	}
	return bResult;	
}
void efuse_access(struct net_device *dev, u8 bRead,u16 start_addr, u8 cnts, u8 *data)
{
	u8	efuse_clk_ori,efuse_clk_new;
	int 	i = 0;

	if(start_addr>0x200) return;
	efuse_clk_ori = read_nic_byte(dev,SYS_FUNC_EN+1);
	efuse_clk_new = efuse_clk_ori|0x20;
	
	if(efuse_clk_new!= efuse_clk_ori){
		write_nic_byte(dev, SYS_FUNC_EN+1, efuse_clk_new);
	}
#ifdef _POWERON_DELAY_
	mdelay(10);
#endif	
	write_nic_byte(dev, EFUSE_TEST+3, (read_nic_byte(dev, EFUSE_TEST+3)|0x80));
	write_nic_byte(dev, EFUSE_CLK_CTRL, (read_nic_byte(dev, EFUSE_CLK_CTRL)|0x03));
	
#ifdef _PRE_EXECUTE_READ_CMD_
	{
		unsigned char tmpdata;
		efuse_OneByteRead(dev, 0,&tmpdata);
	}		
#endif	
	
	for(i=0;i<cnts;i++){		
		efuse_one_byte_rw(dev,bRead, start_addr+i , data+i);		
	}
	write_nic_byte(dev, EFUSE_TEST+3, read_nic_byte(dev, EFUSE_TEST+3)&0x7f);
	write_nic_byte(dev, EFUSE_CLK_CTRL, read_nic_byte(dev, EFUSE_CLK_CTRL)&0xfd);
			
	if(efuse_clk_new != efuse_clk_ori)	write_nic_byte(dev, 0x10250003, efuse_clk_ori);
	
}
#ifdef MERGE_TO_DO
static	void efuse_reg_ctrl(struct net_device *dev,u8 bPowerOn)
{		
	if(_TRUE==bPowerOn){
		write_nic_byte(dev, SYS_FUNC_EN+1,  read_nic_byte(dev,SYS_FUNC_EN+1)|0x20);
#ifdef _POWERON_DELAY_	
		mdelay(10);	
#endif
		write_nic_byte(dev, EFUSE_TEST+4, (read_nic_byte(dev, EFUSE_TEST+4)|0x80));
		write_nic_byte(dev, EFUSE_CLK_CTRL, (read_nic_byte(dev, EFUSE_CLK_CTRL)|0x03));
#ifdef _PRE_EXECUTE_READ_CMD_
		{
			unsigned char tmpdata;
			efuse_OneByteRead(dev, 0,&tmpdata);
		}
		
#endif
	}
	else{
		write_nic_byte(dev, EFUSE_TEST+4, read_nic_byte(dev, EFUSE_TEST+4)&0x7f);
		write_nic_byte(dev, EFUSE_CLK_CTRL, read_nic_byte(dev, EFUSE_CLK_CTRL)&0xfd);

	}

	
}
#endif


void efuse_read_data(struct net_device *dev,u8 efuse_read_item,u8 *data,int data_size)
{
	u8 offset, word_start,byte_start,byte_cnts;
	u8	efusedata[EFUSE_REAL_CONTENT_LEN];
	u8 *tmpdata = NULL;

	u8 pg_pkt_cnts ;
	
	u8 tmpidx;
	u8 pg_data[8];

	if(efuse_read_item>  (sizeof(RTL8712_SDIO_EFUSE_TABLE)/sizeof(EFUSE_MAP))){
		return ;
	}
		
	offset		= RTL8712_SDIO_EFUSE_TABLE[efuse_read_item].offset ;
	word_start	= RTL8712_SDIO_EFUSE_TABLE[efuse_read_item].word_start;
	byte_start 	= RTL8712_SDIO_EFUSE_TABLE[efuse_read_item].byte_start;
	byte_cnts   	= RTL8712_SDIO_EFUSE_TABLE[efuse_read_item].byte_cnts;

	if(data_size!=byte_cnts){
		return;		
	}
	
	pg_pkt_cnts = (byte_cnts /PGPKT_DATA_SIZE) +1;		
	
	if(pg_pkt_cnts > 1){
		tmpdata = efusedata;

		if(tmpdata!=NULL)
		{
			memset((void*)pg_data, pg_pkt_cnts*PGPKT_DATA_SIZE, 0xff);
						
			for(tmpidx=0;tmpidx<pg_pkt_cnts;tmpidx++)
			{				
				memset((void*)pg_data, PGPKT_DATA_SIZE, 0xff);
				if(_TRUE== efuse_PgPacketRead(dev,offset+tmpidx,pg_data))
				{	
					memcpy((void*)(tmpdata+(PGPKT_DATA_SIZE*tmpidx)), (void*)pg_data, PGPKT_DATA_SIZE);
				}
			}
			memcpy((void*)data, (void*)(tmpdata+ (2*word_start)+byte_start ), data_size);
		}		
	}
	else
	{
		memset((void*)pg_data, PGPKT_DATA_SIZE, 0xff);
		if(_TRUE==efuse_PgPacketRead(dev,offset,pg_data)){	
			memcpy((void*)data, (void*)(pg_data+ (2*word_start)+byte_start), data_size);
		}
	}	
	
}
void efuse_write_data(struct net_device *dev,u8 efuse_write_item,u8 *data,int data_size,int bWordUnit)
{
	u8 offset, word_start,byte_start,byte_cnts;	
	u8 word_en = 0x0f,word_cnts;
	u8 pg_pkt_cnts ;
	
	u8 tmpidx,tmpbitmask;
	u8 pg_data[8],tmpbytes=0;

	if(efuse_write_item>  (sizeof(RTL8712_SDIO_EFUSE_TABLE)/sizeof(EFUSE_MAP))){
		return ;
	}
	
	offset		= RTL8712_SDIO_EFUSE_TABLE[efuse_write_item].offset ;
	word_start	= RTL8712_SDIO_EFUSE_TABLE[efuse_write_item].word_start;
	byte_start 	= RTL8712_SDIO_EFUSE_TABLE[efuse_write_item].byte_start;
	byte_cnts   	= RTL8712_SDIO_EFUSE_TABLE[efuse_write_item].byte_cnts;
	
	if(data_size >  byte_cnts){
		return;		
	}
	pg_pkt_cnts = (byte_cnts /PGPKT_DATA_SIZE) +1;	
	word_cnts = byte_cnts /2 ;

	if(byte_cnts %2){
		word_cnts+=1;
	}		
	if((byte_start==1)||((byte_cnts%2)==1)){

		if((efuse_write_item==EFUSE_F0CIS)||(efuse_write_item==EFUSE_F1CIS)){
			memset((void*)pg_data, PGPKT_DATA_SIZE, 0xff);
			efuse_PgPacketRead(dev,offset,pg_data);		
			
			if(efuse_write_item==EFUSE_F0CIS){
				word_en = 0x07;
				memcpy((void*)(pg_data+word_start*2+byte_start), (void*)data, sizeof(u8)*2);
				efuse_PgPacketWrite(dev,offset,word_en,pg_data+(word_start*2));	

				word_en = 0x00;
				efuse_PgPacketWrite(dev,(offset+1),word_en,data+2);

				word_en = 0x00;
				efuse_PgPacketRead(dev,offset+2,pg_data);
				memcpy((void*)(pg_data), (void*)(data+2+8), sizeof(u8)*7);
				
				efuse_PgPacketWrite(dev,(offset+2),word_en,pg_data);
			}
			else if(efuse_write_item==EFUSE_F1CIS){
				word_en = 0x07;
				efuse_PgPacketRead(dev,offset,pg_data);
				pg_data[7] = data[0];
				efuse_PgPacketWrite(dev,offset,word_en,pg_data+(word_start*2));	
				
				word_en = 0x00;
				for(tmpidx = 0 ;tmpidx<(word_cnts/4);tmpidx++){
					efuse_PgPacketWrite(dev,(offset+1+tmpidx),word_en,data+1+(tmpidx*PGPKT_DATA_SIZE));
				}				
			}
			
		}
		else{			
			memset((void*)pg_data, PGPKT_DATA_SIZE, 0xff);
			if((efuse_write_item==EFUSE_SDIO_SETTING)||(efuse_write_item==EFUSE_CCCR)){
				word_en = 0x0e ;
				tmpbytes = 2;
			}
			else if(efuse_write_item == EFUSE_SDIO_MODE){
				word_en = 0x0d ;
				tmpbytes = 2;
			}
			else if(efuse_write_item == EFUSE_OCR){
				word_en = 0x09 ;
				tmpbytes = 4;
			}
			else if((efuse_write_item == EFUSE_EEPROM_VER)||(efuse_write_item==EFUSE_CHAN_PLAN)){
				word_en = 0x07 ;
				tmpbytes = 2;
			}
			if(bWordUnit==_TRUE){
				memcpy((void*)(pg_data+word_start*2), (void*)(data), sizeof(u8)*tmpbytes);
			}
			else{																
				efuse_PgPacketRead(dev,offset,pg_data);
				memcpy((void*)(pg_data+(2*word_start)+byte_start), (void*)(data), sizeof(u8)*byte_cnts);
			}
					
			efuse_PgPacketWrite(dev,offset,word_en,pg_data+(word_start*2));	
			
		}		
		
	}
	else if(pg_pkt_cnts>1){
		if(word_start==0){
			word_en = 0x00;
			for(tmpidx = 0 ;tmpidx<(word_cnts/4);tmpidx++)
			{
				efuse_PgPacketWrite(dev,(offset+tmpidx),word_en,data+(tmpidx*PGPKT_DATA_SIZE));
			}
			word_en = 0x0f;
			for(tmpidx= 0; tmpidx<(word_cnts%4) ; tmpidx++)
			{
				tmpbitmask =tmpidx;
				word_en &= (~BIT(tmpbitmask));
			}		
			efuse_PgPacketWrite(dev,offset+(word_cnts/4),word_en,data+((word_cnts/4)*PGPKT_DATA_SIZE));
		}else
		{

		}
	}
	else{
		word_en = 0x0f;
		for(tmpidx= 0; tmpidx<word_cnts ; tmpidx++)
		{			
			tmpbitmask = word_start + tmpidx ;
			word_en &= ~BIT(tmpbitmask);
		}
		efuse_PgPacketWrite(dev,offset,word_en,data);
	}
		
}

void efuset_test_func_read(struct net_device *dev)
{
	u8 chipid[2];
	u8 ocr[3];
	u8 macaddr[6];	
	u8 txpowertable[28];
	
	memset(chipid,0,sizeof(u8)*2);	
	efuse_read_data(dev,EFUSE_CHIP_ID,chipid,sizeof(chipid));
	
	memset(ocr,0,sizeof(u8)*3);	
	efuse_read_data(dev,EFUSE_CCCR,ocr,sizeof(ocr));

	memset(macaddr,0,sizeof(u8)*6);
	efuse_read_data(dev,EFUSE_MAC_ADDR,macaddr,sizeof(macaddr));
	
	memset(txpowertable,0,sizeof(u8)*28);
	efuse_read_data(dev,EFUSE_TXPW_TAB,txpowertable,sizeof(txpowertable));
}

void efuset_test_func_write(struct net_device *dev)
{
	int bWordUnit = _TRUE;	
	u8 CCCR=0x02,SDIO_SETTING = 0xFF;
	u8 tmpdata[2];
	
	u8 macaddr[6] = {0x00,0xe0,0x4c,0x87,0x12,0x66};	
	efuse_write_data(dev,EFUSE_MAC_ADDR,macaddr,sizeof(macaddr),bWordUnit);

	bWordUnit = _FALSE;
	efuse_write_data(dev,EFUSE_CCCR,&CCCR,sizeof(u8),bWordUnit);
	
	bWordUnit = _FALSE;
	efuse_write_data(dev,EFUSE_SDIO_SETTING,&SDIO_SETTING,sizeof(u8),bWordUnit);

	bWordUnit = _TRUE;	
	tmpdata[0] =SDIO_SETTING ;
	tmpdata[1] =CCCR ;
	efuse_write_data(dev,EFUSE_SDIO_SETTING,tmpdata,sizeof(tmpdata),bWordUnit);
	
}


#ifdef MERGE_TO_DO
bool
efuse_ChkSectionEmpty(
	struct net_device *dev, 
	u8	MapIdx,
	u8	SectionIdx)
{

	struct r8192_priv *priv = rtllib_priv(dev);
	bool		bIsEmpty = false;
	u8	tmpdata[EFUSE_MAX_WORD_UNIT*2]; 
	u8	ByteIdx = 0;

	RT_ASSERT(SectionIdx<EFUSE_MAX_SECTION, ("efuse_ChkSectionEmpty(): Invalid SectionIdx(%#x)!!\n", SectionIdx));
	RT_ASSERT(MapIdx<2, ("efuse_ChkSectionEmpty(): Invalid MapIdx(%#x)!!\n", MapIdx));
	
	memset((void*)tmpdata, EFUSE_MAX_WORD_UNIT*2, 0xff); 
	ByteIdx = SectionIdx<<3;

	RT_ASSERT(ByteIdx<HWSET_MAX_SIZE, ("efuse_ChkSectionEmpty(): Invalid ByteIdx(%#x)!!\n", ByteIdx));

	if(!memcmp( tmpdata, (void*)(&priv->EfuseMap[MapIdx][ByteIdx]), EFUSE_MAX_WORD_UNIT*2))
		bIsEmpty = true;

	return bIsEmpty;

}
#endif
#endif	
