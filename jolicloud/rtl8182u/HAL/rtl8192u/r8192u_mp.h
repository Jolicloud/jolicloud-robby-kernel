/* 
	This is part of rtl8180 OpenSource driver - v 0.3
	Copyright (C) Andrea Merello 2004  <andreamrl@tiscali.it> 
	Released under the terms of GPL (General Public Licence)
	
	Parts of this driver are based on the GPL part of the official realtek driver
	Parts of this driver are based on the rtl8180 driver skeleton from Patric Schenke & Andres Salomon
	Parts of this driver are based on the Intel Pro Wireless 2100 GPL driver
	
	We want to tanks the Authors of such projects and the Ndiswrapper project Authors.
*/

/* this file (will) contains wireless extension handlers*/

#ifndef R8192U_MP_H
#define R8192U_MP_H

#ifdef CONFIG_MP_TOOL

#include "r8192U.h"
#include "r8192U_dm.h"

#define GET_ASIFSTIME(x)	((IS_WIRELESS_MODE_A(x)||IS_WIRELESS_MODE_N_24G(x)||IS_WIRELESS_MODE_N_5G(x))? 16 : 10)


/*------------------------------Define structure----------------------------*/ 
/* MP set force data rate base on the definition. */
typedef enum _MPT_RATE_INDEX{
	/* CCK rate. */
	MPT_RATE_1M = 1, 
	MPT_RATE_2M,
	MPT_RATE_55M,
	MPT_RATE_11M,
	
	/* OFDM rate. */
	MPT_RATE_6M = 5,
	MPT_RATE_9M,
	MPT_RATE_12M,
	MPT_RATE_18M,
	MPT_RATE_24M,
	MPT_RATE_36M,
	MPT_RATE_48M,
	MPT_RATE_54M =12,
	
	/* HT rate. */
	MPT_RATE_MCS0 =13,
	MPT_RATE_MCS1,
	MPT_RATE_MCS2,
	MPT_RATE_MCS3,
	MPT_RATE_MCS4,
	MPT_RATE_MCS5,
	MPT_RATE_MCS6,
	MPT_RATE_MCS7,
	MPT_RATE_MCS8,
	MPT_RATE_MCS9,
	MPT_RATE_MCS10,
	MPT_RATE_MCS11,
	MPT_RATE_MCS12,
	MPT_RATE_MCS13,
	MPT_RATE_MCS14,
	MPT_RATE_MCS15 = 28,
	MPT_RATE_LAST
	 
}MPT_RATE_E, *PMPT_RATE_E;


typedef enum _MPT_Bandwidth_Switch_Mode{
	BAND_20MHZ_MODE = 0,
	BAND_40MHZ_DUPLICATE_MODE = 1,
	BAND_40MHZ_LOWER_MODE = 2,
	BAND_40MHZ_UPPER_MODE = 3,
	BAND_40MHZ_DONTCARE_MODE = 4
}MPT_BANDWIDTH_MODE_E, *PMPT_BANDWIDTH_MODE_E;


typedef enum _ANTENNA_PATH{
        ANTENNA_NONE 	= 0x00,
		ANTENNA_D		,
		ANTENNA_C		,
		ANTENNA_CD		,
		ANTENNA_B		,
		ANTENNA_BD		,
		ANTENNA_BC		,
		ANTENNA_BCD		,
		ANTENNA_A		,
		ANTENNA_AD		,
		ANTENNA_AC		,
		ANTENNA_ACD		,
		ANTENNA_AB		,
		ANTENNA_ABD		,
		ANTENNA_ABC		,
		ANTENNA_ABCD	
} ANTENNA_PATH;


typedef enum _TxAGC_Offset{
        TxAGC_Offset_0 	= 0x00,
        TxAGC_Offset_1		,
        TxAGC_Offset_2		,
        TxAGC_Offset_3		,
        TxAGC_Offset_4		,
        TxAGC_Offset_5		,
        TxAGC_Offset_6		,
        TxAGC_Offset_7		,
        TxAGC_Offset_neg8	,
        TxAGC_Offset_neg7	,
        TxAGC_Offset_neg6	,
        TxAGC_Offset_neg5	,
        TxAGC_Offset_neg4	,
        TxAGC_Offset_neg3	,
        TxAGC_Offset_neg2	,
        TxAGC_Offset_neg1
} TxAGC_Offset;

typedef enum _MPT_Band_Mode{
	BAND_24G = 0,
	BAND_5G = 1
}MPT_Band_Mode, *PMPT_Band_Mode;

typedef enum _PREAMBLE {
	Long_Preamble	= 0x01,
	Short_Preamble  	  ,
	Long_GI     		  ,
	Short_GI  		
} PREAMBLE;


typedef enum _TEST_MODE{
    NONE                  =0x0,
    PACKETS_TX                ,
    PACKETS_RX                ,
    CONTINUOUS_TX             ,
	OFDM_Single_Carrier_TX    ,
	OFDM_Single_Tone_TX       ,
	CCK_Carrier_Suppression_TX
}TEST_MODE;
/*------------------------------Define structure----------------------------*/ 


#define MAX_MP_XMITBUF_SZ 256
#define NR_MP_XMITFRAME	8

#ifdef R8192U_MP_C


#define GEN_MP_IOCTL_SUBCODE(code) _MP_IOCTL_ ## code ## _CMD_

enum RTL871X_MP_IOCTL_SUBCODE{
	GEN_MP_IOCTL_SUBCODE(MP_START), 		/*0*/
	GEN_MP_IOCTL_SUBCODE(MP_STOP),			/*1*/
	GEN_MP_IOCTL_SUBCODE(READ_REG),			/*2*/
	GEN_MP_IOCTL_SUBCODE(WRITE_REG),		/*3*/
	GEN_MP_IOCTL_SUBCODE(SET_CHANNEL_MODULATION),	/*4*/
	GEN_MP_IOCTL_SUBCODE(SET_TXPOWER),		/*5*/
	GEN_MP_IOCTL_SUBCODE(SET_DATARATE),		/*6*/	
	GEN_MP_IOCTL_SUBCODE(READ_BB_REG),		/*7*/
	GEN_MP_IOCTL_SUBCODE(WRITE_BB_REG),		/*8*/
	GEN_MP_IOCTL_SUBCODE(READ_RF_REG),		/*9*/
	GEN_MP_IOCTL_SUBCODE(WRITE_RF_REG),		/*10*/
	
	GEN_MP_IOCTL_SUBCODE(READ16_EEPROM),		/*11*/
	GEN_MP_IOCTL_SUBCODE(WRITE16_EEPROM),	/*12*/

	GEN_MP_IOCTL_SUBCODE(SET_BANDWIDTH),	/*13*/
	GEN_MP_IOCTL_SUBCODE(SET_ANTENNA_BB),	/*14*/
	GEN_MP_IOCTL_SUBCODE(SET_TXAGC_OFFSET),


	GEN_MP_IOCTL_SUBCODE(CONTINUOUS_TX),	/*15*/
	GEN_MP_IOCTL_SUBCODE(SINGLE_CARRIER),	/*16*/
	GEN_MP_IOCTL_SUBCODE(SINGLE_TONE),		/*17*/

	GEN_MP_IOCTL_SUBCODE(PACKET_RX),
	GEN_MP_IOCTL_SUBCODE(GET_PACKETS_RX),

	MAX_MP_IOCTL_SUBCODE,
};
#endif

enum MP_MODE{
	
	MP_START_MODE,
		
       MP_STOP_MODE,
       
	MP_ERR_MODE
};
	

struct rwreg_param{
	unsigned int offset;
	unsigned int width;
	unsigned int value;
};

/*
struct bbreg_param{
	unsigned int offset;
	unsigned int phymask;
	unsigned int value;
};
*/

struct bbreg_param{
	unsigned int offset;
	unsigned int value;	
};

struct rfreg_param{
	unsigned int path;
	unsigned int offset;
	unsigned int value;
};


struct packets_rx_param{
	unsigned int rxok;
	unsigned int rxerr;
};


struct rfchannel_param{
	unsigned int	ch;
	unsigned int	modem;
};

struct bandwidth_param{
	unsigned int	bandwidth;
};

struct txagc_offset_param{
	unsigned int txagc_offset;
};


struct txpower_param{
	unsigned int pwr_index;
};


struct datarate_param{
	unsigned int rate_index;
};

struct preamble_param{
	unsigned int preamble;
};

struct antenna_param{
	unsigned int antenna_path;
};

struct rfintfs_parm {
	unsigned int rfintfs;
};

struct mp_xmit_packet{	
	unsigned int len;
	unsigned int mem[(MAX_MP_XMITBUF_SZ >> 2)];	
};

struct psmode_param{
	unsigned int ps_mode;
	unsigned int smart_ps;
};

struct eeprom_rw_param{
	unsigned int offset;
	unsigned short value;
};


struct mp_ioctl_handler{

	unsigned int paramsize;

	int (*handler)(struct net_device *dev, unsigned char  set, void *param);	

};

extern int mp_ioctl_start_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_stop_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_set_ch_modulation_hdl(struct net_device *dev, unsigned char set, void *param);
extern int mp_ioctl_set_txpower_hdl(struct net_device *dev, unsigned char set, void *param);
extern int mp_ioctl_read_reg_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_write_reg_hdl(struct net_device *dev, unsigned char set, void *param);
extern int mp_ioctl_set_datarate_hdl(struct net_device *dev, unsigned char set, void *param);
extern int mp_ioctl_read_bbreg_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_write_bbreg_hdl(struct net_device *dev, unsigned char set, void *param);
extern int mp_ioctl_read_rfreg_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_write_rfreg_hdl(struct net_device *dev, unsigned char set, void *param);
extern int mp_ioctl_read16_eeprom_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_write16_eeprom_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_set_bandwidth_hdl(struct net_device *dev, unsigned char set, void*param);

extern int mp_ioctl_set_antenna_bb_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_continuous_tx_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_single_carrier_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_single_tone_hdl(struct net_device *dev, unsigned char set, void*param);

extern int mp_ioctl_packet_rx_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_get_packets_rx_hdl(struct net_device *dev, unsigned char set, void*param);
extern int mp_ioctl_set_txagc_offset_hdl(struct net_device *dev, unsigned char set, void*param);


#ifdef R8192U_MP_C


#define GEN_MP_IOCTL_HANDLER(sz, subcode) {sz, &mp_ioctl_ ## subcode ## _hdl},

struct mp_ioctl_handler mp_ioctl_hdl[] = {

	GEN_MP_IOCTL_HANDLER(sizeof(unsigned int), start)	/*0*/
	GEN_MP_IOCTL_HANDLER(sizeof(unsigned int), stop)	/*1*/
	GEN_MP_IOCTL_HANDLER(sizeof(struct rwreg_param), read_reg)		/*2*/
	GEN_MP_IOCTL_HANDLER(sizeof(struct rwreg_param), write_reg)		/*3*/
	GEN_MP_IOCTL_HANDLER(sizeof(struct rfchannel_param), set_ch_modulation)	/*4*/
	GEN_MP_IOCTL_HANDLER(sizeof(struct txpower_param), set_txpower)			/*5*/
	GEN_MP_IOCTL_HANDLER(sizeof(struct datarate_param), set_datarate)		/*6*/	
	GEN_MP_IOCTL_HANDLER(sizeof(struct bbreg_param), read_bbreg)	/*7*/
	GEN_MP_IOCTL_HANDLER(sizeof(struct bbreg_param), write_bbreg)	/*8*/	
	GEN_MP_IOCTL_HANDLER(sizeof(struct rwreg_param), read_rfreg)	/*9*/
	GEN_MP_IOCTL_HANDLER(sizeof(struct rwreg_param), write_rfreg)	/*10*/
	

	GEN_MP_IOCTL_HANDLER(sizeof(struct eeprom_rw_param), read16_eeprom)		/*11*/
	GEN_MP_IOCTL_HANDLER(sizeof(struct eeprom_rw_param), write16_eeprom)	/*12*/

	GEN_MP_IOCTL_HANDLER(sizeof(struct bandwidth_param), set_bandwidth)	/*13*/
	GEN_MP_IOCTL_HANDLER(sizeof(struct antenna_param), set_antenna_bb)	/*14*/
	GEN_MP_IOCTL_HANDLER(sizeof(struct txagc_offset_param), set_txagc_offset) /*15*/

	GEN_MP_IOCTL_HANDLER(0, continuous_tx)	/*16*/
	GEN_MP_IOCTL_HANDLER(0, single_carrier)	/*17*/
	GEN_MP_IOCTL_HANDLER(0, single_tone)	/*18*/

	GEN_MP_IOCTL_HANDLER(0, packet_rx)
	GEN_MP_IOCTL_HANDLER(sizeof(struct packets_rx_param), get_packets_rx)


};

#endif



struct mp_ioctl_param{

	unsigned int subcode;
	unsigned int len;
	unsigned char data[0];

};


int r8192u_mp_ioctl_handle(struct net_device *dev, 
		struct iw_request_info *info, union iwreq_data *wrqu, char *extra);


struct mp_priv {

	struct r8192_priv *priv;

	u32	test_start;


	u32	bandwidth;			

	u32	rate_index;			

	u8	current_channel;
	u8	switch_to_channel;	

	u32 current_modulation;

	u8	in_progress;

	u8 bCckContTx;
	u8 bOfdmContTx;

	u8 bStartContTx;
	u8 bSingleCarrier;
	u8 bSingleTone;

	u8 bPacketRx;
	u8 bmp_mode;
	u32 rcr;

	u32 antenna_tx_path;
	u32 antenna_rx_path;
	u32 current_preamble;

	
#if 0
	struct mp_wiparam workparam;
	u8 act_in_progress;	
	

	struct wlan_network	mp_network;	 
	sint prev_fw_state;

	uint mode;

	u8 TID;
	u32 tx_pktcount;
  
	u32 rx_pktcount;
	u32 rx_crcerrpktcount;
	u32 rx_pktloss;
#endif

#if 0    
	struct recv_stat rxstat;

  	u8 *pallocated_mp_xmitframe_buf;
	u8 *pmp_xmtframe_buf;
	_queue	free_mp_xmitqueue;
	uint free_mp_xmitframe_cnt;
#endif 

};

typedef struct _R_ANTENNA_SELECT_OFDM{	
	u32	r_tx_antenna:4;	
	u32 r_ant_l:4;
	u32	r_ant_non_ht:4;	
	u32	r_ant_ht1:4;
	u32	r_ant_ht2:4;
	u32	r_ant_ht_s1:4;
	u32	r_ant_non_ht_s1:4;
	u32	OFDM_TXSC:2;
	u32	Reserved:2;
}R_ANTENNA_SELECT_OFDM;

typedef struct _R_ANTENNA_SELECT_CCK{
	u8			r_cckrx_enable_2:2;	
	u8			r_cckrx_enable:2;
	u8			r_ccktx_enable:4;
}R_ANTENNA_SELECT_CCK;


#define IS_WIRELESS_MODE_A(x)		(x == WIRELESS_MODE_A)
#define IS_WIRELESS_MODE_B(x)		(x == WIRELESS_MODE_B)
#define IS_WIRELESS_MODE_G(x)		(x == WIRELESS_MODE_G)
#define IS_WIRELESS_MODE_N_24G(x)	(x == WIRELESS_MODE_N_24G)
#define IS_WIRELESS_MODE_N_5G(x)	(x == WIRELESS_MODE_N_5G)


#define MP_DEFAULT_TX_ESSID "rtl8192u_mp_test"


#endif 

#endif 

