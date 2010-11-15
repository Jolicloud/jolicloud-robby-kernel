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
#ifndef __INC_FIRMWARE92C_H__
#define __INC_FIRMWARE92C_H__

#define FW_CMD_IO_QUERY(priv)	(u16)(priv->FwCmdIOMap)
#define FW_CMD_IO_PARA_QUERY(priv)	((u32)(priv->FwCmdIOParam))

#define PageNum_128(_Len)		(u32)(((_Len)>>7) + ((_Len)&0x7F ? 1:0))

#define IS_FW_HEADER_EXIST(_pFwHdr)	((_pFwHdr->Signature&0xFFF0) == 0x92C0 ||\
									(_pFwHdr->Signature&0xFFF0) == 0x88C0)
									
typedef enum _DESC_PACKET_TYPE{
	DESC_PACKET_TYPE_INIT = 0,
	DESC_PACKET_TYPE_NORMAL = 1,	
}DESC_PACKET_TYPE;



#define FW_8192C_SIZE					0x4000
#define FW_8192C_START_ADDRESS			0x1000
#define FW_8192C_END_ADDRESS			0x3FFF

#define MAX_PAGE_SIZE					4096	

typedef enum _FIRMWARE_SOURCE{
	FW_SOURCE_IMG_FILE = 0,
	FW_SOURCE_HEADER_FILE = 1,		
}firmware_source_e, *pfirmware_source_e, FIRMWARE_SOURCE, *PFIRMWARE_SOURCE;

typedef struct _RT_FIRMWARE{	
	FIRMWARE_SOURCE	eFWSource;	
	u8			szFwBuffer[FW_8192C_SIZE];
	u32			ulFwLength;
}rt_firmware, *prt_firmware, RT_FIRMWARE, *PRT_FIRMWARE, RT_FIRMWARE_92C, *PRT_FIRMWARE_92C;

typedef struct _H2C_JOINBSSRPT_PARM{	
	u8	OpMode;	
	u8	Ps_Qos_Info;
	u8	Bssid[6];	
	u16	BcnItv;	
	u16	Aid;
}H2C_JOINBSSRPT_PARM, *PH2C_JOINBSSRPT_PARM;


typedef struct _C2H_EVT_HDR{
	u8	CmdHeader; 
	u8	CmdSeq;
}C2H_EVT_HDR, *PC2H_EVT_HDR;

typedef struct _SETPWRMODE_PARM{
	u8 	Mode;
	u8 	SmartPS;
	u8	BcnPassTime;	
}SETPWRMODE_PARM, *PSETPWRMODE_PARM;

typedef struct JOINBSSRPT_PARM{
	u8	OpMode;	
}JOINBSSRPT_PARM, *PJOINBSSRPT_PARM;


typedef struct _RSVDPAGE_LOC{
	u8 	LocProbeRsp;
	u8 	LocPsPoll;
	u8	LocNullData;
}RSVDPAGE_LOC, *PRSVDPAGE_LOC;

typedef struct _RT_8192C_FIRMWARE_HDR {

	u16		Signature;	
	u8		Category;	
	u8		Function; 	
	u16		Version;		
	u8		Subversion;	
	u8		Rsvd1;		


	u8		Month;	
	u8		Date;	
	u8		Hour;	
	u8		Minute;	
	u16		RamCodeSize;	
	u16		Rsvd2;

	u32		SvnIdx;	
	u32		Rsvd3;

	u32		Rsvd4;
	u32		Rsvd5;

}RT_8192C_FIRMWARE_HDR, *PRT_8192C_FIRMWARE_HDR;

typedef enum _RTL8192C_H2C_CMD 
{
	H2C_AP_OFFLOAD = 0,		/*0*/
	H2C_SETPWRMODE = 1,		/*1*/
	H2C_JOINBSSRPT = 2,		/*2*/
	H2C_RSVDPAGE = 3,
	H2C_RSSI_REPORT = 5,
	H2C_RA_MASK = 6,
	MAX_H2CCMD
}RTL8192C_H2C_CMD;


typedef enum _RTL8192C_C2H_EVT
{
	C2H_DBG,
	C2H_TSF,
	MAX_C2HEVENT
}RTL8192C_C2H_EVT;




bool
FirmwareDownload92C(struct net_device *dev);


u8
FirmwareHeaderMapRfType(struct net_device *dev);

void
FillH2CCmd92C(struct net_device *dev,u8 	ElementID,u32 CmdLen, u8*	pCmdBuffer);
void FirmwareSelfReset(struct net_device *dev);
#endif

