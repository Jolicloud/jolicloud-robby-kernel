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

#ifndef __INC_HAL8192CPCIDEF_H
#define __INC_HAL8192CPCIDEF_H

#include <linux/types.h>
#include "../../../rtllib/rtllib_endianfree.h"

#define Rtl8192CETestChipImgArrayLength 12264
#define Rtl8192CTestChipFwImgLen	Rtl8192CETestChipImgArrayLength

#define Rtl8192CETestChipRadioA_1TArrayLength 260
#define Rtl8188CTestChipRadioALen	Rtl8192CETestChipRadioA_1TArrayLength

#define HAL_RETRY_LIMIT_INFRA		48	
#define HAL_RETRY_LIMIT_AP_ADHOC	7

/*--------------------------Define MACRO--------------------------------------*/








#define TX_DESC_SIZE										64
#define TX_DESC_AGGR_SUBFRAME_SIZE						32

#define SET_TX_DESC_PKT_SIZE(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc, 0, 16, __Value)
#define SET_TX_DESC_OFFSET(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc, 16, 8, __Value)
#define SET_TX_DESC_BMC(__pTxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pTxDesc, 24, 1, __Value)
#define SET_TX_DESC_HTC(__pTxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pTxDesc, 25, 1, __Value)
#define SET_TX_DESC_LAST_SEG(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc, 26, 1, __Value)
#define SET_TX_DESC_FIRST_SEG(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc, 27, 1, __Value)
#define SET_TX_DESC_LINIP(__pTxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pTxDesc, 28, 1, __Value)
#define SET_TX_DESC_NO_ACM(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc, 29, 1, __Value)
#define SET_TX_DESC_GF(__pTxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pTxDesc, 30, 1, __Value)
#define SET_TX_DESC_OWN(__pTxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pTxDesc, 31, 1, __Value)

#define GET_TX_DESC_PKT_SIZE(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc, 0, 16)
#define GET_TX_DESC_OFFSET(__pTxDesc)					LE_BITS_TO_4BYTE(__pTxDesc, 16, 8)
#define GET_TX_DESC_BMC(__pTxDesc)						LE_BITS_TO_4BYTE(__pTxDesc, 24, 1)
#define GET_TX_DESC_HTC(__pTxDesc)						LE_BITS_TO_4BYTE(__pTxDesc, 25, 1)
#define GET_TX_DESC_LAST_SEG(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc, 26, 1)
#define GET_TX_DESC_FIRST_SEG(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc, 27, 1)
#define GET_TX_DESC_LINIP(__pTxDesc) 						LE_BITS_TO_4BYTE(__pTxDesc, 28, 1)
#define GET_TX_DESC_NO_ACM(__pTxDesc)					LE_BITS_TO_4BYTE(__pTxDesc, 29, 1)
#define GET_TX_DESC_GF(__pTxDesc)						LE_BITS_TO_4BYTE(__pTxDesc, 30, 1)
#define GET_TX_DESC_OWN(__pTxDesc)						LE_BITS_TO_4BYTE(__pTxDesc, 31, 1)

#define SET_TX_DESC_MACID(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+4, 0, 5, __Value)
#define SET_TX_DESC_AGG_BREAK(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+4, 5, 1, __Value)
#define SET_TX_DESC_BK(__pTxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pTxDesc+4, 6, 1, __Value)
#define SET_TX_DESC_RDG_ENABLE(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+4, 7, 1, __Value)
#define SET_TX_DESC_QUEUE_SEL(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+4, 8, 5, __Value)
#define SET_TX_DESC_RDG_NAV_EXT(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+4, 13, 1, __Value)
#define SET_TX_DESC_LSIG_TXOP_EN(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+4, 14, 1, __Value)
#define SET_TX_DESC_PIFS(__pTxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pTxDesc+4, 15, 1, __Value)
#define SET_TX_DESC_RATE_ID(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+4, 16, 4, __Value)
#define SET_TX_DESC_NAV_USE_HDR(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+4, 20, 1, __Value)
#define SET_TX_DESC_EN_DESC_ID(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+4, 21, 1, __Value)
#define SET_TX_DESC_SEC_TYPE(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+4, 22, 2, __Value)
#define SET_TX_DESC_PKT_OFFSET(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+4, 24, 8, __Value)

#define GET_TX_DESC_MACID(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+4, 0, 5)
#define GET_TX_DESC_AGG_ENABLE(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+4, 5, 1)
#define GET_TX_DESC_AGG_BREAK(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+4, 6, 1)
#define GET_TX_DESC_RDG_ENABLE(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+4, 7, 1)
#define GET_TX_DESC_QUEUE_SEL(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+4, 8, 5)
#define GET_TX_DESC_RDG_NAV_EXT(__pTxDesc)		 		LE_BITS_TO_4BYTE(__pTxDesc+4, 13, 1)
#define GET_TX_DESC_LSIG_TXOP_EN(__pTxDesc)		 		LE_BITS_TO_4BYTE(__pTxDesc+4, 14, 1)
#define GET_TX_DESC_PIFS(__pTxDesc) 						LE_BITS_TO_4BYTE(__pTxDesc+4, 15, 1)
#define GET_TX_DESC_RATE_ID(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+4, 16, 4)
#define GET_TX_DESC_NAV_USE_HDR(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+4, 20, 1)
#define GET_TX_DESC_EN_DESC_ID(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+4, 21, 1)
#define GET_TX_DESC_SEC_TYPE(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+4, 22, 2)
#define GET_TX_DESC_PKT_OFFSET(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+4, 24, 8)

#define SET_TX_DESC_RTS_RC(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+8, 0, 6, __Value)
#define SET_TX_DESC_DATA_RC(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+8, 6, 6, __Value)
#define SET_TX_DESC_BAR_RTY_TH(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+8, 14, 2, __Value)
#define SET_TX_DESC_MORE_FRAG(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+8, 17, 1, __Value)
#define SET_TX_DESC_RAW(__pTxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pTxDesc+8, 18, 1, __Value)
#define SET_TX_DESC_CCX(__pTxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pTxDesc+8, 19, 1, __Value)
#define SET_TX_DESC_AMPDU_DENSITY(__pTxDesc, __Value) 	SET_BITS_TO_LE_4BYTE(__pTxDesc+8, 20, 3, __Value)
#define SET_TX_DESC_ANTSEL_A(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+8, 24, 1, __Value)
#define SET_TX_DESC_ANTSEL_B(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+8, 25, 1, __Value)
#define SET_TX_DESC_TX_ANT_CCK(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+8, 26, 2, __Value)
#define SET_TX_DESC_TX_ANTL(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+8, 28, 2, __Value)
#define SET_TX_DESC_TX_ANT_HT(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+8, 30, 2, __Value)

#define GET_TX_DESC_RTS_RC(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+8, 0, 6)
#define GET_TX_DESC_DATA_RC(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+8, 6, 6)
#define GET_TX_DESC_BAR_RTY_TH(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+8, 14, 2)
#define GET_TX_DESC_MORE_FRAG(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+8, 17, 1) 
#define GET_TX_DESC_RAW(__pTxDesc) 						LE_BITS_TO_4BYTE(__pTxDesc+8, 18, 1) 
#define GET_TX_DESC_CCX(__pTxDesc) 						LE_BITS_TO_4BYTE(__pTxDesc+8, 19, 1)
#define GET_TX_DESC_AMPDU_DENSITY(__pTxDesc) 			LE_BITS_TO_4BYTE(__pTxDesc+8, 20, 3) 
#define GET_TX_DESC_ANTSEL_A(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+8, 24, 1)
#define GET_TX_DESC_ANTSEL_B(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+8, 25, 1)
#define GET_TX_DESC_TX_ANT_CCK(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+8, 26, 2) 
#define GET_TX_DESC_TX_ANTL(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+8, 28, 2)
#define GET_TX_DESC_TX_ANT_HT(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+8, 30, 2) 

#define SET_TX_DESC_NEXT_HEAP_PAGE(__pTxDesc, __Value) 	SET_BITS_TO_LE_4BYTE(__pTxDesc+12, 0, 8, __Value)
#define SET_TX_DESC_TAIL_PAGE(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+12, 8, 8, __Value)
#define SET_TX_DESC_SEQ(__pTxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pTxDesc+12, 16, 12, __Value)
#define SET_TX_DESC_PKT_ID(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+12, 28, 4, __Value)

#define GET_TX_DESC_NEXT_HEAP_PAGE(__pTxDesc) 			LE_BITS_TO_4BYTE(__pTxDesc+12, 0, 8)
#define GET_TX_DESC_TAIL_PAGE(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+12, 8, 8)
#define GET_TX_DESC_SEQ(__pTxDesc) 						LE_BITS_TO_4BYTE(__pTxDesc+12, 16, 12)
#define GET_TX_DESC_PKT_ID(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+12, 28, 4)

#define SET_TX_DESC_RTS_RATE(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 0, 5, __Value)
#define SET_TX_DESC_AP_DCFE(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 5, 1, __Value)
#define SET_TX_DESC_QOS(__pTxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 6, 1, __Value)
#define SET_TX_DESC_HWSEQ_EN(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 7, 1, __Value)
#define SET_TX_DESC_USE_RATE(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 8, 1, __Value)
#define SET_TX_DESC_DISABLE_RTS_FB(__pTxDesc, __Value) 	SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 9, 1, __Value)
#define SET_TX_DESC_DISABLE_FB(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 10, 1, __Value)
#define SET_TX_DESC_CTS2SELF(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 11, 1, __Value)
#define SET_TX_DESC_RTS_ENABLE(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 12, 1, __Value)
#define SET_TX_DESC_HW_RTS_ENABLE(__pTxDesc, __Value) 	SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 13, 1, __Value)
#define SET_TX_DESC_PORT_ID(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 14, 1, __Value)
#define SET_TX_DESC_WAIT_DCTS(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 18, 1, __Value)
#define SET_TX_DESC_CTS2AP_EN(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 19, 1, __Value)
#define SET_TX_DESC_TX_SUB_CARRIER(__pTxDesc, __Value) 	SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 20, 2, __Value)
#define SET_TX_DESC_TX_STBC(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 22, 2, __Value)
#define SET_TX_DESC_DATA_SHORT(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 24, 1, __Value)
#define SET_TX_DESC_DATA_BW(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 25, 1, __Value)
#define SET_TX_DESC_RTS_SHORT(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 26, 1, __Value)
#define SET_TX_DESC_RTS_BW(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 27, 1, __Value)
#define SET_TX_DESC_RTS_SC(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 28, 2, __Value)
#define SET_TX_DESC_RTS_STBC(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+16, 30, 2, __Value)

#define GET_TX_DESC_RTS_RATE(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+16, 0, 5)
#define GET_TX_DESC_AP_DCFE(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+16, 5, 1)
#define GET_TX_DESC_QOS(__pTxDesc) 						LE_BITS_TO_4BYTE(__pTxDesc+16, 6, 1)
#define GET_TX_DESC_HWSEQ_EN(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+16, 7, 1)
#define GET_TX_DESC_USE_RATE(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+16, 8, 1)
#define GET_TX_DESC_DISABLE_RTS_FB(__pTxDesc) 			LE_BITS_TO_4BYTE(__pTxDesc+16, 9, 1)
#define GET_TX_DESC_DISABLE_FB(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+16, 10, 1)
#define GET_TX_DESC_CTS2SELF(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+16, 11, 1)
#define GET_TX_DESC_RTS_ENABLE(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+16, 12, 1)
#define GET_TX_DESC_HW_RTS_ENABLE(__pTxDesc)			LE_BITS_TO_4BYTE(__pTxDesc+16, 13, 1)
#define GET_TX_DESC_PORT_ID(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+16, 14, 1)
#define GET_TX_DESC_WAIT_DCTS(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+16, 18, 1)
#define GET_TX_DESC_CTS2AP_EN(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+16, 19, 1)
#define GET_TX_DESC_TX_SUB_CARRIER(__pTxDesc) 			LE_BITS_TO_4BYTE(__pTxDesc+16, 20, 2)
#define GET_TX_DESC_TX_STBC(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+16, 22, 2) 
#define GET_TX_DESC_DATA_SHORT(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+16, 24, 1)
#define GET_TX_DESC_DATA_BW(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+16, 25, 1)
#define GET_TX_DESC_RTS_SHORT(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+16, 26, 1)
#define GET_TX_DESC_RTS_BW(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+16, 27, 1)
#define GET_TX_DESC_RTS_SC(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+16, 28, 2)
#define GET_TX_DESC_RTS_STBC(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+16, 30, 2)

#define SET_TX_DESC_TX_RATE(__pTxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pTxDesc+20, 0, 6, __Value)
#define SET_TX_DESC_DATA_SHORTGI(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+20, 6, 1, __Value)
#define SET_TX_DESC_CCX_TAG(__pTxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pTxDesc+20, 7, 1, __Value)
#define SET_TX_DESC_DATA_RATE_FB_LIMIT(__pTxDesc, __Value) 	SET_BITS_TO_LE_4BYTE(__pTxDesc+20, 8, 5, __Value)
#define SET_TX_DESC_RTS_RATE_FB_LIMIT(__pTxDesc, __Value) 	SET_BITS_TO_LE_4BYTE(__pTxDesc+20, 13, 4, __Value)
#define SET_TX_DESC_RETRY_LIMIT_ENABLE(__pTxDesc, __Value) 	SET_BITS_TO_LE_4BYTE(__pTxDesc+20, 17, 1, __Value)
#define SET_TX_DESC_DATA_RETRY_LIMIT(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+20, 18, 6, __Value)
#define SET_TX_DESC_USB_TXAGG_NUM(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+20, 24, 8, __Value)

#define GET_TX_DESC_TX_RATE(__pTxDesc) 						LE_BITS_TO_4BYTE(__pTxDesc+20, 0, 6)
#define GET_TX_DESC_DATA_SHORTGI(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+20, 6, 1)
#define GET_TX_DESC_CCX_TAG(__pTxDesc) 						LE_BITS_TO_4BYTE(__pTxDesc+20, 7, 1)
#define GET_TX_DESC_DATA_RATE_FB_LIMIT(__pTxDesc) 			LE_BITS_TO_4BYTE(__pTxDesc+20, 8, 5)
#define GET_TX_DESC_RTS_RATE_FB_LIMIT(__pTxDesc) 			LE_BITS_TO_4BYTE(__pTxDesc+20, 13, 4)
#define GET_TX_DESC_RETRY_LIMIT_ENABLE(__pTxDesc) 			LE_BITS_TO_4BYTE(__pTxDesc+20, 17, 1)
#define GET_TX_DESC_DATA_RETRY_LIMIT(__pTxDesc) 			LE_BITS_TO_4BYTE(__pTxDesc+20, 18, 6)
#define GET_TX_DESC_USB_TXAGG_NUM(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+20, 24, 8)

#define SET_TX_DESC_TXAGC_A(__pTxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pTxDesc+24, 0, 5, __Value)
#define SET_TX_DESC_TXAGC_B(__pTxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pTxDesc+24, 5, 5, __Value)
#define SET_TX_DESC_USE_MAX_LEN(__pTxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pTxDesc+24, 10, 1, __Value)
#define SET_TX_DESC_MAX_AGG_NUM(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+24, 11, 5, __Value)
#define SET_TX_DESC_MCSG1_MAX_LEN(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+24, 16, 4, __Value)
#define SET_TX_DESC_MCSG2_MAX_LEN(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+24, 20, 4, __Value)
#define SET_TX_DESC_MCSG3_MAX_LEN(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+24, 24, 4, __Value)
#define SET_TX_DESC_MCS7_SGI_MAX_LEN(__pTxDesc, __Value) 	SET_BITS_TO_LE_4BYTE(__pTxDesc+24, 28, 4, __Value)

#define GET_TX_DESC_TXAGC_A(__pTxDesc) 						LE_BITS_TO_4BYTE(__pTxDesc+24, 0, 5)
#define GET_TX_DESC_TXAGC_B(__pTxDesc) 						LE_BITS_TO_4BYTE(__pTxDesc+24, 5, 5)
#define GET_TX_DESC_USE_MAX_LEN(__pTxDesc) 					LE_BITS_TO_4BYTE(__pTxDesc+24, 10, 1)
#define GET_TX_DESC_MAX_AGG_NUM(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+24, 11, 5)
#define GET_TX_DESC_MCSG1_MAX_LEN(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+24, 16, 4)
#define GET_TX_DESC_MCSG2_MAX_LEN(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+24, 20, 4)
#define GET_TX_DESC_MCSG3_MAX_LEN(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+24, 24, 4)
#define GET_TX_DESC_MCS7_SGI_MAX_LEN(__pTxDesc) 			LE_BITS_TO_4BYTE(__pTxDesc+24, 28, 4)

#define SET_TX_DESC_TX_BUFFER_SIZE(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+28, 0, 16, __Value)
#define SET_TX_DESC_MCSG4_MAX_LEN(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+28, 16, 4, __Value)
#define SET_TX_DESC_MCSG5_MAX_LEN(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+28, 20, 4, __Value)
#define SET_TX_DESC_MCSG6_MAX_LEN(__pTxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pTxDesc+28, 24, 4, __Value)
#define SET_TX_DESC_MCS15_SGI_MAX_LEN(__pTxDesc, __Value) 	SET_BITS_TO_LE_4BYTE(__pTxDesc+28, 28, 4, __Value)

#define GET_TX_DESC_TX_BUFFER_SIZE(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+28, 0, 16)
#define GET_TX_DESC_MCSG4_MAX_LEN(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+28, 16, 4)
#define GET_TX_DESC_MCSG5_MAX_LEN(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+28, 20, 4)
#define GET_TX_DESC_MCSG6_MAX_LEN(__pTxDesc) 				LE_BITS_TO_4BYTE(__pTxDesc+28, 24, 4)
#define GET_TX_DESC_MCS15_SGI_MAX_LEN(__pTxDesc)			LE_BITS_TO_4BYTE(__pTxDesc+28, 28, 4)

#define SET_TX_DESC_TX_BUFFER_ADDRESS(__pTxDesc, __Value) 	SET_BITS_TO_LE_4BYTE(__pTxDesc+32, 0, 32, __Value)
#define SET_TX_DESC_TX_BUFFER_ADDRESS64(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+36, 0, 32, __Value)

#define GET_TX_DESC_TX_BUFFER_ADDRESS(__pTxDesc) 			LE_BITS_TO_4BYTE(__pTxDesc+32, 0, 32)
#define GET_TX_DESC_TX_BUFFER_ADDRESS64(__pTxDesc) 		LE_BITS_TO_4BYTE(__pTxDesc+36, 0, 32)

#define SET_TX_DESC_NEXT_DESC_ADDRESS(__pTxDesc, __Value) 	SET_BITS_TO_LE_4BYTE(__pTxDesc+40, 0, 32, __Value)
#define SET_TX_DESC_NEXT_DESC_ADDRESS64(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+44, 0, 32, __Value)

#define GET_TX_DESC_NEXT_DESC_ADDRESS(__pTxDesc) 			LE_BITS_TO_4BYTE(__pTxDesc+40, 0, 32)
#define GET_TX_DESC_NEXT_DESC_ADDRESS64(__pTxDesc) 		LE_BITS_TO_4BYTE(__pTxDesc+44, 0, 32)



#if (EARLYMODE_ENABLE_FOR_92D== 1)
#define SET_EARLYMODE_PKTNUM(__pAddr, __Value) 			SET_BITS_TO_LE_4BYTE(__pAddr, 0, 3, __Value)
#define SET_EARLYMODE_LEN0(__pAddr, __Value) 				SET_BITS_TO_LE_4BYTE(__pAddr, 4, 12, __Value)
#define SET_EARLYMODE_LEN1(__pAddr, __Value) 				SET_BITS_TO_LE_4BYTE(__pAddr, 16, 12, __Value)
#define SET_EARLYMODE_LEN2_1(__pAddr, __Value) 			SET_BITS_TO_LE_4BYTE(__pAddr, 28, 4, __Value)
#define SET_EARLYMODE_LEN2_2(__pAddr, __Value) 			SET_BITS_TO_LE_4BYTE(__pAddr+4, 0, 8, __Value)
#define SET_EARLYMODE_LEN3(__pAddr, __Value) 				SET_BITS_TO_LE_4BYTE(__pAddr+4, 8, 12, __Value)
#define SET_EARLYMODE_LEN4(__pAddr, __Value) 				SET_BITS_TO_LE_4BYTE(__pAddr+4, 20, 12, __Value)

#endif

#define RX_DESC_SIZE				32
#define RX_DRV_INFO_SIZE_UNIT	8

#define GET_RX_DESC_PKT_LEN(__pRxDesc) 					LE_BITS_TO_4BYTE(__pRxDesc, 0, 14)
#define GET_RX_DESC_CRC32(__pRxDesc) 					LE_BITS_TO_4BYTE(__pRxDesc, 14, 1)
#define GET_RX_DESC_ICV(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc, 15, 1)
#define GET_RX_DESC_DRV_INFO_SIZE(__pRxDesc) 			LE_BITS_TO_4BYTE(__pRxDesc, 16, 4)
#define GET_RX_DESC_SECURITY(__pRxDesc) 					LE_BITS_TO_4BYTE(__pRxDesc, 20, 3)
#define GET_RX_DESC_QOS(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc, 23, 1)
#define GET_RX_DESC_SHIFT(__pRxDesc) 					LE_BITS_TO_4BYTE(__pRxDesc, 24, 2)
#define GET_RX_DESC_PHYST(__pRxDesc) 					LE_BITS_TO_4BYTE(__pRxDesc, 26, 1)
#define GET_RX_DESC_SWDEC(__pRxDesc) 					LE_BITS_TO_4BYTE(__pRxDesc, 27, 1)
#define GET_RX_DESC_LS(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc, 28, 1)
#define GET_RX_DESC_FS(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc, 29, 1)
#define GET_RX_DESC_EOR(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc, 30, 1)
#define GET_RX_DESC_OWN(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc, 31, 1)

#define SET_RX_DESC_PKT_LEN(__pRxDesc, __Value) 			SET_BITS_TO_LE_4BYTE(__pRxDesc, 0, 14, __Value)
#define SET_RX_DESC_EOR(__pRxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pRxDesc, 30, 1, __Value)
#define SET_RX_DESC_OWN(__pRxDesc, __Value) 				SET_BITS_TO_LE_4BYTE(__pRxDesc, 31, 1, __Value)

#define GET_RX_DESC_MACID(__pRxDesc) 					LE_BITS_TO_4BYTE(__pRxDesc+4, 0, 5)
#define GET_RX_DESC_TID(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+4, 5, 4)
#define GET_RX_DESC_HWRSVD(__pRxDesc) 					LE_BITS_TO_4BYTE(__pRxDesc+4, 9, 5)
#define GET_RX_DESC_PAGGR(__pRxDesc) 					LE_BITS_TO_4BYTE(__pRxDesc+4, 14, 1)
#define GET_RX_DESC_FAGGR(__pRxDesc) 					LE_BITS_TO_4BYTE(__pRxDesc+4, 15, 1)
#define GET_RX_DESC_A1_FIT(__pRxDesc) 					LE_BITS_TO_4BYTE(__pRxDesc+4, 16, 4)
#define GET_RX_DESC_A2_FIT(__pRxDesc) 					LE_BITS_TO_4BYTE(__pRxDesc+4, 20, 4)
#define GET_RX_DESC_PAM(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+4, 24, 1)
#define GET_RX_DESC_PWR(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+4, 25, 1)
#define GET_RX_DESC_MD(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+4, 26, 1)
#define GET_RX_DESC_MF(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+4, 27, 1)
#define GET_RX_DESC_TYPE(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+4, 28, 2)
#define GET_RX_DESC_MC(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+4, 30, 1)
#define GET_RX_DESC_BC(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+4, 31, 1)

#define GET_RX_DESC_SEQ(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+8, 0, 12)
#define GET_RX_DESC_FRAG(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+8, 12, 4)
#define GET_RX_DESC_NEXT_PKT_LEN(__pRxDesc) 			LE_BITS_TO_4BYTE(__pRxDesc+8, 16, 14)
#define GET_RX_DESC_NEXT_IND(__pRxDesc) 					LE_BITS_TO_4BYTE(__pRxDesc+8, 30, 1)
#define GET_RX_DESC_RSVD(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+8, 31, 1)

#define GET_RX_DESC_RXMCS(__pRxDesc) 					LE_BITS_TO_4BYTE(__pRxDesc+12, 0, 6)
#define GET_RX_DESC_RXHT(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+12, 6, 1)
#define GET_RX_DESC_SPLCP(__pRxDesc) 					LE_BITS_TO_4BYTE(__pRxDesc+12, 8, 1)
#define GET_RX_DESC_BW(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+12, 9, 1)
#define GET_RX_DESC_HTC(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+12, 10, 1)
#define GET_RX_DESC_HWPC_ERR(__pRxDesc) 				LE_BITS_TO_4BYTE(__pRxDesc+12, 14, 1)
#define GET_RX_DESC_HWPC_IND(__pRxDesc) 				LE_BITS_TO_4BYTE(__pRxDesc+12, 15, 1)
#define GET_RX_DESC_IV0(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+12, 16, 16)

#define GET_RX_DESC_IV1(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+16, 0, 32)

#define GET_RX_DESC_TSFL(__pRxDesc) 						LE_BITS_TO_4BYTE(__pRxDesc+20, 0, 32)

#define GET_RX_DESC_BUFF_ADDR(__pRxDesc) 				LE_BITS_TO_4BYTE(__pRxDesc+24, 0, 32)
#define GET_RX_DESC_BUFF_ADDR64(__pRxDesc) 				LE_BITS_TO_4BYTE(__pRxDesc+28, 0, 32)

#define SET_RX_DESC_BUFF_ADDR(__pRxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pRxDesc+24, 0, 32, __Value)
#define SET_RX_DESC_BUFF_ADDR64(__pRxDesc, __Value) 		SET_BITS_TO_LE_4BYTE(__pRxDesc+28, 0, 32, __Value)



#define	TX_DESC_NEXT_DESC_OFFSET	40

#define	CLEAR_PCI_TX_DESC_CONTENT(__pTxDesc, _size)				\
{																\
		if(_size > TX_DESC_NEXT_DESC_OFFSET)						\
			memset((void*)__pTxDesc, 0, TX_DESC_NEXT_DESC_OFFSET);	\
		else														\
			memset((void*)__pTxDesc, 0, _size);						\
}

#define	C2H_RX_CMD_HDR_LEN		8 
#define	GET_C2H_CMD_CMD_LEN(__pRxHeader)	LE_BITS_TO_4BYTE((__pRxHeader), 0, 16)
#define	GET_C2H_CMD_ELEMENT_ID(__pRxHeader)	LE_BITS_TO_4BYTE((__pRxHeader), 16, 8)
#define	GET_C2H_CMD_CMD_SEQ(__pRxHeader)	LE_BITS_TO_4BYTE((__pRxHeader), 24, 7)
#define	GET_C2H_CMD_CONTINUE(__pRxHeader)	LE_BITS_TO_4BYTE((__pRxHeader), 31, 1)
#define	GET_C2H_CMD_CONTENT(__pRxHeader)	((u8*)(__pRxHeader) + C2H_RX_CMD_HDR_LEN)

#define	GET_C2H_CMD_FEEDBACK_ELEMENT_ID(__pCmdFBHeader)	LE_BITS_TO_4BYTE((__pCmdFBHeader), 0, 8)
#define	GET_C2H_CMD_FEEDBACK_CCX_LEN(__pCmdFBHeader)	LE_BITS_TO_4BYTE((__pCmdFBHeader), 8, 8)
#define	GET_C2H_CMD_FEEDBACK_CCX_CMD_CNT(__pCmdFBHeader)	LE_BITS_TO_4BYTE((__pCmdFBHeader), 16, 16)
#define	GET_C2H_CMD_FEEDBACK_CCX_MAC_ID(__pCmdFBHeader)	LE_BITS_TO_4BYTE(((__pCmdFBHeader) + 4), 0, 5)
#define	GET_C2H_CMD_FEEDBACK_CCX_VALID(__pCmdFBHeader)	LE_BITS_TO_4BYTE(((__pCmdFBHeader) + 4), 7, 1)
#define	GET_C2H_CMD_FEEDBACK_CCX_RETRY_CNT(__pCmdFBHeader)	LE_BITS_TO_4BYTE(((__pCmdFBHeader) + 4), 8, 5)
#define	GET_C2H_CMD_FEEDBACK_CCX_TOK(__pCmdFBHeader)	LE_BITS_TO_4BYTE(((__pCmdFBHeader) + 4), 15, 1)
#define	GET_C2H_CMD_FEEDBACK_CCX_QSEL(__pCmdFBHeader)	LE_BITS_TO_4BYTE(((__pCmdFBHeader) + 4), 16, 4)
#define	GET_C2H_CMD_FEEDBACK_CCX_SEQ(__pCmdFBHeader)	LE_BITS_TO_4BYTE(((__pCmdFBHeader) + 4), 20, 12)

/*--------------------------Forward declaration--------------------------------------*/
typedef	struct _ADAPTER	ADAPTER, *PADAPTER;

typedef enum _RTL819X_LOOPBACK{
	RTL819X_NO_LOOPBACK = 0,
	RTL819X_MAC_LOOPBACK = 1,
	RTL819X_DMA_LOOPBACK = 2,
	RTL819X_CCK_LOOPBACK = 3,
}rtl819x_loopback_e, RTL819X_LOOPBACK_E;


#define RESET_DELAY_8185				20

#define RT_IBSS_INT_MASKS				(IMR_BcnInt | IMR_TBDOK | IMR_TBDER)
#define RT_AC_INT_MASKS				(IMR_VIDOK | IMR_VODOK | IMR_BEDOK|IMR_BKDOK)


#define 	MAX_SILENT_RESET_RX_SLOT_NUM	10

#define NUM_OF_FIRMWARE_QUEUE				10
#define NUM_OF_PAGES_IN_FW					0x100
#define NUM_OF_PAGE_IN_FW_QUEUE_BK			0x07
#define NUM_OF_PAGE_IN_FW_QUEUE_BE			0x07
#define NUM_OF_PAGE_IN_FW_QUEUE_VI			0x07
#define NUM_OF_PAGE_IN_FW_QUEUE_VO			0x07
#define NUM_OF_PAGE_IN_FW_QUEUE_HCCA		0x0
#define NUM_OF_PAGE_IN_FW_QUEUE_CMD		0x0
#define NUM_OF_PAGE_IN_FW_QUEUE_MGNT		0x02
#define NUM_OF_PAGE_IN_FW_QUEUE_HIGH		0x02
#define NUM_OF_PAGE_IN_FW_QUEUE_BCN			0x2
#define NUM_OF_PAGE_IN_FW_QUEUE_PUB			0xA1

#define NUM_OF_PAGE_IN_FW_QUEUE_BK_DTM		0x026
#define NUM_OF_PAGE_IN_FW_QUEUE_BE_DTM		0x048
#define NUM_OF_PAGE_IN_FW_QUEUE_VI_DTM		0x048
#define NUM_OF_PAGE_IN_FW_QUEUE_VO_DTM		0x026
#define NUM_OF_PAGE_IN_FW_QUEUE_PUB_DTM		0x00
                                        	

extern u8	DescString8192SE[];


extern u8	DescString8192CE[];
/*typedef enum _VERSION_8190{
	VERSION_8190_BD=0x3,
	VERSION_8190_BE
}VERSION_8190,*PVERSION_8190;*/


/* 2007/11/02 MH Define RF mode temporarily for test. */
typedef enum tag_Rf_OpType
{    
    RF_OP_By_SW_3wire = 0,
    RF_OP_By_FW,   
    RF_OP_MAX
}RF_OpType_E;

/* Power save related */
typedef enum _RF_POWER_STATE{
	RF_ON,
	RF_OFF,
	RF_SLEEP,
	RF_SHUT_DOWN,		
}RF_POWER_STATE;

typedef	enum _POWER_SAVE_MODE	
{
	POWER_SAVE_MODE_ACTIVE,
	POWER_SAVE_MODE_SAVE,
}POWER_SAVE_MODE;

typedef enum _POWER_POLICY_CONFIG
{
	POWERCFG_MAX_POWER_SAVINGS,		
	POWERCFG_GLOBAL_POWER_SAVINGS,		
	POWERCFG_LOCAL_POWER_SAVINGS,		
	POWERCFG_LENOVO,						
}POWER_POLICY_CONFIG;

typedef	enum _INTERFACE_SELECT_8190PCI{
	INTF_SEL1_MINICARD	= 0,		
	INTF_SEL0_PCIE			= 1,		
	INTF_SEL2_RSV			= 2,		
	INTF_SEL3_RSV			= 3,		
} INTERFACE_SELECT_8190PCI, *PINTERFACE_SELECT_8190PCI;





typedef struct _RX_DRIVER_INFO_8192S{
	
	/*u32			gain_0:7;
	u32			trsw_0:1;
	u32			gain_1:7;
	u32			trsw_1:1;
	u32			gain_2:7;
	u32			trsw_2:1;
	u32			gain_3:7;
	u32			trsw_3:1;	*/
	u8			gain_trsw[4];

	/*u32			pwdb_all:8;
	u32			cfosho_0:8;
	u32			cfosho_1:8;
	u32			cfosho_2:8;*/
	u8			pwdb_all;
	u8			cfosho[4];

	/*u32			cfosho_3:8;
	u32			cfotail_0:8;
	u32			cfotail_1:8;
	u32			cfotail_2:8;*/
	u8			cfotail[4];

	/*u32			cfotail_3:8;
	u32			rxevm_0:8;
	u32			rxevm_1:8;
	u32			rxsnr_0:8;*/
	char			rxevm[2];
	char			rxsnr[4];

	/*u32			rxsnr_1:8;
	u32			rxsnr_2:8;
	u32			rxsnr_3:8;
	u32			pdsnr_0:8;*/
	u8			pdsnr[2];

	/*u32			pdsnr_1:8;
	u32			csi_current_0:8;
	u32			csi_current_1:8;
	u32			csi_target_0:8;*/
	u8			csi_current[2];
	u8			csi_target[2];

	/*u32			csi_target_1:8;
	u32			sigevm:8;
	u32			max_ex_pwr:8;
	u32			ex_intf_flag:1;
	u32			sgi_en:1;
	u32			rxsc:2;
	u32			reserve:4;*/
	u8			sigevm;
	u8			max_ex_pwr;
	u8			ex_intf_flag:1;
	u8			sgi_en:1;
	u8			rxsc:2;
	u8			reserve:4;
	
}rx_fwinfo, *prx_fwinfo;


typedef struct _LOG_INTERRUPT_8190
{
	u32	nIMR_COMDOK;
	u32	nIMR_MGNTDOK;
	u32	nIMR_HIGH;
	u32	nIMR_VODOK;
	u32	nIMR_VIDOK;
	u32	nIMR_BEDOK;
	u32	nIMR_BKDOK;
	u32	nIMR_ROK;
	u32	nIMR_RCOK;
	u32	nIMR_TBDOK;
	u32	nIMR_BDOK;
	u32	nIMR_RXFOVW;
	u32	nIMR_RDU;
	u32	nIMR_C2HCMD;
} LOG_INTERRUPT, *PLOG_INTERRUPT, LOG_INTERRUPT_8190_T, *PLOG_INTERRUPT_8190_T;

typedef struct _TX_DESC_CMD_8192SE{  
	u32		PktSize:16;
	u32		Offset:8;	
	u32		Rsvd0:2;
	u32		FirstSeg:1;
	u32		LastSeg:1;
	u32		LINIP:1;
	u32		Rsvd1:2;
	u32		OWN:1;	
	
	u32		Rsvd2;
	u32		Rsvd3;
	u32		Rsvd4;
	u32		Rsvd5;
	u32		Rsvd6;
	u32		Rsvd7;	

	u32		TxBufferSize:16;
	u32		Rsvd8:16;

	u32		TxBufferAddr;

	u32		NextTxDescAddress;

	u32		Reserve_Pass_92S_PCIE_MM_Limit[6];
	
}tx_desc_cmd, *ptx_desc_cmd;

typedef struct _TX_DESC_8192CE{


	u32		PktSize:16;
	u32		Offset:8;
	u32		BMC:1;
	u32		HTC:1;
	u32		LastSeg:1;
	u32		FirstSeg:1;
	u32		LINIP:1;
	u32		NOACM:1;
	u32		GF:1;
	u32		OWN:1;	
	
	u32		MacID:5;			
	u32		AggEn:1;
	u32		Bk:1;
	u32		RdgEn:1;
	u32		QueueSel:5;
	u32		RdNavExt:1;
	u32		LSigTxopEn:1;
	u32		PIFS:1;
	u32		RateId:4;
	u32		NavUseHdr:1;
	u32		EnDescID:1;
	u32		SecType:2;	
	u32		PktOffset:8;		
	
	u32		RTSRC:6;	
	u32		DATARC:6;	
	u32		Rsvd0:2;	
	u32		BarRetryHT:2;
	u32		Rsvd1:1;
	u32		MoreFrag:1;
	u32		Raw:1;
	u32		Ccx:1;
	u32		AmpduDensity:3;
	u32		Rsvd2:1;
	u32		AntSelA:1;
	u32		AntSelB:1;
	u32		TxAntCck:2;
	u32		TxAntl:2;
	u32		TxAntHt:2;
	
	u32		NextHeadPage:8;
	u32		TailPage:8;
	u32		Seq:12;
	u32		PktID:4;	

	u32		RTSRate:5;
	u32		APDcfe:1;
	u32		QoS:1;
	u32		HWSeqEnable:1;
	u32		UserRate:1;	
	u32		DisRTSFB:1;
	u32		DisDataFB:1;
	u32		CTS2Self:1;
	u32		RTSEn:1;
	u32		HwRTSEn:1;
	u32		PortId:1;
	u32		Rsvd3:3;
	u32		WaitDcts:1;
	u32		CTS2APEn:1;
	u32		TxSc:2;
	u32		STBC:2;
	u32		TxShort:1;
	u32		TxBW:1;
	u32		RTSShort:1;
	u32		RTSBW:1;
	u32		RTSSC:2;
	u32		RTSSTBC:2;
	
	u32		TxRate:6;
	u32		ShortGI:1;
	u32		CCXT:1;
	u32		TxRateFBLmt:5;
	u32		RTSRateFBLmt:4;
	u32		RetryLmtEn:1;
	u32		TxRetryLmt:6;
	u32		USBTxAGGNum:8;

	u32		TXAGCA:5;
	u32		TXAGCB:5;
	u32		UseMaxLen:1;
	u32		MaxAGGNum:5;
	u32		MCSG1MaxLen:4;
	u32		MCSG2MaxLen:4;
	u32		MCSG3MaxLen:4;
	u32		MCS7SGIMaxLen:4;	

	u32		TxBufferSize:16;
	u32		MCSG4MaxLen:4;
	u32		MCSG5MaxLen:4;
	u32		MCSG6MaxLen:4;
	u32		MCSG15SGIMaxLen:4;

	u32		TxBuffAddr;
	u32		TxBufferAddr64;
	u32		NextDescAddress;
	u32		NextDescAddress64;

	u32		Reserve_Pass_PCIE_MM_Limit[4];
	
} tx_desc, *ptx_desc;

typedef struct _RX_DESC_8192CE{
	u32		Length:14;
	u32		CRC32:1;
	u32		ICVError:1;
	u32		DrvInfoSize:4;
	u32		Security:3;
	u32		Qos:1;
	u32		Shift:2;
	u32		PHYStatus:1;
	u32		SWDec:1;
	u32		LastSeg:1;
	u32		FirstSeg:1;
	u32		EOR:1;
	u32		OWN:1;	

	u32		MACID:5;
	u32		TID:4;
	u32		HwRsvd:5;
	u32		PAGGR:1;
	u32		FAGGR:1;
	u32		A1_FIT:4;
	u32		A2_FIT:4;
	u32		PAM:1;
	u32		PWR:1;
	u32		MoreData:1;
	u32		MoreFrag:1;
	u32		Type:2;
	u32		MC:1;
	u32		BC:1;

	u32		Seq:12;
	u32		Frag:4;
	u32		NextPktLen:14;
	u32		NextIND:1;
	u32		Rsvd:1;

	u32		RxMCS:6;
	u32		RxHT:1;
	u32		AMSDU:1;
	u32		SPLCP:1;
	u32		BandWidth:1;
	u32		HTC:1;
	u32		TCPChkRpt:1;
	u32		IPChkRpt:1;
	u32		TCPChkValID:1;
	u32		HwPCErr:1;
	u32		HwPCInd:1;
	u32		IV0:16;

	u32		IV1;

	u32		TSFL;

	u32		BufferAddress;
	u32		BufferAddress64;


#if 0		
	u32		BA_SSN:12;
	u32		BA_VLD:1;
	u32		RSVD:19;
#endif

} rx_desc, *prx_desc;


#if 0
void  rtl8192ce_BT_VAR_INIT(struct net_device* dev);
void  rtl8192ce_BT_HW_INIT(struct net_device* dev);
u8  rtl8192ce_BT_FW_PRIVE_NOTIFY(struct net_device* dev);
void  rtl8192ce_BT_SWITCH(struct net_device* dev);
void  rtl8192ce_BT_ServiceChange(struct net_device* dev);
#endif

typedef	enum _BT_Ant_NUM{
	Ant_x2	= 0,		
	Ant_x1	= 1
} BT_Ant_NUM, *PBT_Ant_NUM;

typedef	enum _BT_CoType{
	BT_2Wire		= 0,		
	BT_ISSC_3Wire	= 1,
	BT_Accel		= 2,
	BT_CSR			= 3,
	BT_CSR_ENHAN	= 4,
	BT_RTL8756		= 5,
} BT_CoType, *PBT_CoType;

typedef	enum _BT_CurState{
	BT_OFF		= 0,	
	BT_ON		= 1,
} BT_CurState, *PBT_CurState;

typedef	enum _BT_ServiceType{
	BT_SCO		= 0,	
	BT_A2DP	= 1,
	BT_HID		= 2,
	BT_HID_Idle	= 3,
	BT_Scan		= 4,
	BT_Idle		= 5,
	BT_OtherAction	= 6,
	BT_Busy		= 7,
	BT_OtherBusy	= 8,
} BT_ServiceType, *PBT_ServiceType;

typedef	enum _BT_RadioShared{
	BT_Radio_Shared 	= 0,	
	BT_Radio_Individual	= 1,
} BT_RadioShared, *PBT_RadioShared;

typedef struct _BT_COEXIST_STR{
	u8					BluetoothCoexist;
	u8					BT_Ant_Num;
	u8					BT_CoexistType;
	u8					BT_State;
	u8					BT_CUR_State;		
	u8					BT_Ant_isolation;	
	u8					BT_PapeCtrl;		
	u8					BT_Service;			
	u8					BT_RadioSharedType;
	u32	Ratio_Tx;
	u32	Ratio_PRI;
	u8					BtRfRegOrigin1E;
	u8					BtRfRegOrigin1F;
	u8					BtRssiState;
	u32					BtEdcaUL;
	u32					BtEdcaDL;
}BT_COEXIST_STR, *PBT_COEXIST_STR;


typedef enum _HAL_FW_C2H_CMD_ID
{
	HAL_FW_C2H_CMD_Read_MACREG = 0,
	HAL_FW_C2H_CMD_Read_BBREG = 1,
	HAL_FW_C2H_CMD_Read_RFREG = 2,
	HAL_FW_C2H_CMD_Read_EEPROM = 3,
	HAL_FW_C2H_CMD_Read_EFUSE = 4,
	HAL_FW_C2H_CMD_Read_CAM = 5,
	HAL_FW_C2H_CMD_Get_BasicRate = 6,			
	HAL_FW_C2H_CMD_Get_DataRate = 7,				
	HAL_FW_C2H_CMD_Survey = 8 ,
	HAL_FW_C2H_CMD_SurveyDone = 9,
	HAL_FW_C2H_CMD_JoinBss = 10,
	HAL_FW_C2H_CMD_AddSTA = 11,
	HAL_FW_C2H_CMD_DelSTA = 12,
	HAL_FW_C2H_CMD_AtimDone = 13,
	HAL_FW_C2H_CMD_TX_Report = 14,
	HAL_FW_C2H_CMD_CCX_Report = 15,
	HAL_FW_C2H_CMD_DTM_Report = 16,
	HAL_FW_C2H_CMD_TX_Rate_Statistics = 17,
	HAL_FW_C2H_CMD_C2HLBK = 18,
	HAL_FW_C2H_CMD_C2HDBG = 19,
	HAL_FW_C2H_CMD_C2HFEEDBACK = 20,
	HAL_FW_C2H_CMD_MAX
}HAL_FW_C2H_CMD_ID;

#define	HAL_FW_C2H_CMD_C2HFEEDBACK_CCX_PER_PKT_RPT			0x04
#define	HAL_FW_C2H_CMD_C2HFEEDBACK_DTM_TX_STATISTICS_RPT	0x05


#define USB_HWDESC_HEADER_LEN	32

#endif 

#ifndef __REALTEK_HAL8192CDEFCOM_H__
#define __REALTEK_HAL8192CDEFCOM_H__

#define	HAL_DM_DIG_DISABLE				BIT0	
#define	HAL_DM_HIPWR_DISABLE				BIT1	

#define	PHY_RSSI_SLID_WIN_MAX				100
#define	PHY_LINKQUALITY_SLID_WIN_MAX		20
#define	PHY_Beacon_RSSI_SLID_WIN_MAX		10

#define CrcLength						4


#if 0
#define BK_QUEUE							0		
#define BE_QUEUE							1		
#define VI_QUEUE							2		
#define VO_QUEUE						3		
#define HCCA_QUEUE						4		
#define TXCMD_QUEUE						5		
#define MGNT_QUEUE						6	
#define HIGH_QUEUE						7	
#define BEACON_QUEUE					8	

#define LOW_QUEUE						BE_QUEUE
#define NORMAL_QUEUE					VO_QUEUE
#endif

#define QSLT_BK							0x2
#define QSLT_BE							0x0
#define QSLT_VI							0x5
#define QSLT_VO							0x7
#define QSLT_BEACON						0x10
#define QSLT_HIGH						0x11
#define QSLT_MGNT						0x12
#define QSLT_CMD						0x13

#define RX_MPDU_QUEUE					0
#define RX_CMD_QUEUE					1
#define RX_MAX_QUEUE					2


#define AC2QUEUEID(_AC)					(_AC)




#define DESC92C_RATE1M					0x00
#define DESC92C_RATE2M					0x01
#define DESC92C_RATE5_5M				0x02
#define DESC92C_RATE11M					0x03

#define DESC92C_RATE6M					0x04
#define DESC92C_RATE9M					0x05
#define DESC92C_RATE12M					0x06
#define DESC92C_RATE18M					0x07
#define DESC92C_RATE24M					0x08
#define DESC92C_RATE36M					0x09
#define DESC92C_RATE48M					0x0a
#define DESC92C_RATE54M					0x0b

#define DESC92C_RATEMCS0				0x0c
#define DESC92C_RATEMCS1				0x0d
#define DESC92C_RATEMCS2				0x0e
#define DESC92C_RATEMCS3				0x0f
#define DESC92C_RATEMCS4				0x10
#define DESC92C_RATEMCS5				0x11
#define DESC92C_RATEMCS6				0x12
#define DESC92C_RATEMCS7				0x13
#define DESC92C_RATEMCS8				0x14
#define DESC92C_RATEMCS9				0x15
#define DESC92C_RATEMCS10				0x16
#define DESC92C_RATEMCS11				0x17
#define DESC92C_RATEMCS12				0x18
#define DESC92C_RATEMCS13				0x19
#define DESC92C_RATEMCS14				0x1a
#define DESC92C_RATEMCS15				0x1b
#define DESC92C_RATEMCS15_SG			0x1c
#define DESC92C_RATEMCS32				0x20


#define DESC92S_RATE1M					0x00
#define DESC92S_RATE2M					0x01
#define DESC92S_RATE5_5M				0x02
#define DESC92S_RATE11M					0x03
#define DESC92S_RATE6M					0x04
#define DESC92S_RATE9M					0x05
#define DESC92S_RATE12M					0x06
#define DESC92S_RATE18M					0x07
#define DESC92S_RATE24M					0x08
#define DESC92S_RATE36M					0x09
#define DESC92S_RATE48M					0x0a
#define DESC92S_RATE54M					0x0b
#define DESC92S_RATEMCS0				0x0c
#define DESC92S_RATEMCS1				0x0d
#define DESC92S_RATEMCS2				0x0e
#define DESC92S_RATEMCS3				0x0f
#define DESC92S_RATEMCS4				0x10
#define DESC92S_RATEMCS5				0x11
#define DESC92S_RATEMCS6				0x12
#define DESC92S_RATEMCS7				0x13
#define DESC92S_RATEMCS8				0x14
#define DESC92S_RATEMCS9				0x15
#define DESC92S_RATEMCS10				0x16
#define DESC92S_RATEMCS11				0x17
#define DESC92S_RATEMCS12				0x18
#define DESC92S_RATEMCS13				0x19
#define DESC92S_RATEMCS14				0x1a
#define DESC92S_RATEMCS15				0x1b
#define DESC92S_RATEMCS15_SG			0x1c
#define DESC92S_RATEMCS32				0x20



#define IS_HT_RATE(rate)				(((rate) & 0x80) ? TRUE : FALSE)



#define SHORT_SLOT_TIME					9
#define NON_SHORT_SLOT_TIME			20

#define MAX_LINES_HWCONFIG_TXT			1000
#define MAX_BYTES_LINE_HWCONFIG_TXT	256

#define SW_THREE_WIRE					0
#define HW_THREE_WIRE					2

#define BT_DEMO_BOARD					0 
#define BT_QA_BOARD						1 
#define BT_FPGA							2 

#define	Rx_Smooth_Factor					20

#define HAL_PRIME_CHNL_OFFSET_DONT_CARE	0
#define HAL_PRIME_CHNL_OFFSET_LOWER		1
#define HAL_PRIME_CHNL_OFFSET_UPPER		2

#define MAX_H2C_QUEUE_NUM					10

typedef struct _Phy_CCK_Rx_Status_Report_8192S
{
	/* For CCK rate descriptor. This is a unsigned 8:1 variable. LSB bit presend
	   0.5. And MSB 7 bts presend a signed value. Range from -64~+63.5. */
	u8	adc_pwdb_X[4];
	u8	sq_rpt;	
	u8	cck_agc_rpt;
}phy_sts_cck_819xpci_t, phy_sts_cck_8192s_t, PHY_STS_CCK_8192S_T;

typedef struct _H2C_CMD_8192C
{
	u8 	ElementID;
	u32 	CmdLen;
	u8*	pCmdBuffer;
}H2C_CMD_8192C,*PH2C_CMD_8192C;


#endif


