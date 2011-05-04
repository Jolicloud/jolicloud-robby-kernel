/******************************************************************************
 *
 * Copyright(c) 2007 - 2010 Realtek Corporation. All rights reserved.
 *                                        
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
#ifndef _RTL8192C_XMIT_H_
#define _RTL8192C_XMIT_H_

#define HWXMIT_ENTRY	4

#define VO_QUEUE_INX	0
#define VI_QUEUE_INX	1
#define BE_QUEUE_INX	2
#define BK_QUEUE_INX	3
#define TS_QUEUE_INX	4
#define MGT_QUEUE_INX	5
#define BMC_QUEUE_INX	6
#define BCN_QUEUE_INX	7

#define HW_QUEUE_ENTRY	8

//
// Queue Select Value in TxDesc
//
#define QSLT_BK							0x2//0x01
#define QSLT_BE							0x0
#define QSLT_VI							0x5//0x4
#define QSLT_VO							0x7//0x6
#define QSLT_BEACON						0x10
#define QSLT_HIGH						0x11
#define QSLT_MGNT						0x12
#define QSLT_CMD						0x13

#define TXDESC_SIZE 32
#define PACKET_OFFSET_SZ (8)
#define TXDESC_OFFSET (TXDESC_SIZE + PACKET_OFFSET_SZ)

#if USB_TX_AGGREGATION_92C
#define MAX_TX_AGG_PACKET_NUMBER 0xFF
#endif

#define tx_cmd tx_desc

//
//defined for TX DESC Operation
//

#define MAX_TID (15)

//OFFSET 0
#define OFFSET_SZ (0)
#define OFFSET_SHT (16)
#define OWN 	BIT(31)
#define FSG	BIT(27)
#define LSG	BIT(26)

//OFFSET 4
#define PKT_OFFSET_SZ (0)
#define QSEL_SHT (8)
#define NAVUSEHDR BIT(20)
#define HWPC BIT(31)

//OFFSET 8
#define BMC BIT(7)
#define BK BIT(30)
#define AGG_EN BIT(29)

//OFFSET 12
#define SEQ_SHT (16)

//OFFSET 16
#define TXBW BIT(18)

//OFFSET 20
#define DISFB BIT(15)

struct tx_desc{

	//DWORD 0
	u32 txdw0;

	u32 txdw1;

	u32 txdw2;

	u32 txdw3;

	u32 txdw4;

	u32 txdw5;

	u32 txdw6;

	u32 txdw7;	

};


union txdesc {
	struct tx_desc txdesc;
	u32 value[TXDESC_SIZE>>2];	
};

void cal_txdesc_chksum(struct tx_desc	*ptxdesc);
s32 rtw_update_txdesc(struct xmit_frame *pxmitframe, u32 *ptxdesc, s32 sz);
void rtw_dump_xframe(_adapter *padapter, struct xmit_frame *pxmitframe);

s32 rtw_xmitframe_complete(_adapter *padapter, struct xmit_priv *pxmitpriv, struct xmit_buf *pxmitbuf);

void rtw_do_queue_select(_adapter *padapter, struct pkt_attrib *pattrib);
u32 rtw_get_ff_hwaddr(struct xmit_frame	*pxmitframe);

s32 pre_xmitframe(_adapter *padapter, struct xmit_frame *pxmitframe);
s32 xmitframe_direct(_adapter *padapter, struct xmit_frame *pxmitframe);


#endif

