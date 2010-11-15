/*
 * Common (OS-independent) definitions for
 * Broadcom 802.11abg Networking Device Driver
 *
 * Copyright (C) 2010, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: wlc_pub.h,v 1.392.2.6 2009/10/30 06:21:23 Exp $
 */

#ifndef _wlc_pub_h_
#define _wlc_pub_h_

#define	MAX_TIMERS	(29 + (2 * WLC_MAXDPT))		

#define	WLC_NUMRATES	16	
#define	MAXMULTILIST	32	
#define	D11_PHY_HDR_LEN	6	

#ifndef LINUX_WLUSER_POSTMOGRIFY_REMOVAL

#define	PHY_TYPE_A	0	
#define	PHY_TYPE_G	2	
#define	PHY_TYPE_N	4	
#define	PHY_TYPE_LP	5	
#define	PHY_TYPE_SSN	6	
#define	PHY_TYPE_LCN	8	

#define WLC_10_MHZ	10	
#define WLC_20_MHZ	20	
#define WLC_40_MHZ	40	

#define CHSPEC_WLC_BW(chanspec)	(CHSPEC_IS40(chanspec) ? WLC_40_MHZ : \
				 CHSPEC_IS20(chanspec) ? WLC_20_MHZ : \
							 WLC_10_MHZ)
#endif 

#define	WLC_RSSI_MINVAL		-200	
#define	WLC_RSSI_NO_SIGNAL	-91	
#define	WLC_RSSI_VERY_LOW	-80	
#define	WLC_RSSI_LOW		-70	
#define	WLC_RSSI_GOOD		-68	
#define	WLC_RSSI_VERY_GOOD	-58	
#define	WLC_RSSI_EXCELLENT	-57	
#define	WLC_RSSI_INVALID	 0	

typedef struct wlc_tunables {
	int ntxd;			
	int nrxd;			
	int rxbufsz;		
	int nrxbufpost;		
	int maxscb;			
	int ampdunummpdu;	
	int maxpktcb;		
	int maxdpt;			
	int maxucodebss;	
	int maxucodebss4;	
	int maxbss;			
	int datahiwat;		
	int ampdudatahiwat;	
	int rxbnd;			
	int txsbnd;			
	int memreserved;	
} wlc_tunables_t;

typedef struct wlc_rateset {
	uint	count;			
	uint8	rates[WLC_NUMRATES];	
	uint8	htphy_membership;	
	uint8	mcs[MCSSET_LEN];	
} wlc_rateset_t;

struct rsn_parms {
	uint8 flags;		
	uint8 multicast;	
	uint8 ucount;		
	uint8 unicast[4];	
	uint8 acount;		
	uint8 auth[4];		
	uint8 PAD[4];		
};

typedef void *wlc_pkt_t;

typedef struct wlc_event {
	wl_event_msg_t event;		
	struct ether_addr *addr;	
	int bsscfgidx;			
	struct wl_if	*wlif;		
	void *data;                     
	struct wlc_event *next;         
} wlc_event_t;

typedef struct wlc_bss_info
{
	struct ether_addr BSSID;	
	uint16		flags;		
	uint8		SSID_len;	
	uint8		SSID[32];	
	int16		RSSI;		
	int16		SNR;		
	uint16		beacon_period;	
	uint16		atim_window;	
	chanspec_t	chanspec;	
	int8		infra;		
	wlc_rateset_t	rateset;	
	uint8		dtim_period;	
	int8		phy_noise;	
	uint16		capability;	
#ifdef WLSCANCACHE
	uint32		timestamp;	
#endif
	struct dot11_bcn_prb *bcn_prb;	
	uint16		bcn_prb_len;	
	uint8		wme_qosinfo;	
	struct rsn_parms wpa;
	struct rsn_parms wpa2;
#ifdef BCMWAPI_WAI
	struct rsn_parms wapi;
#endif 
	uint16		qbss_load_aac;	

	uint8		qbss_load_chan_free;	
	uint8		mcipher;	
	uint8		wpacfg;		
} wlc_bss_info_t;

#ifndef LINUX_WLUSER_POSTMOGRIFY_REMOVAL

struct wlc_if;

#define WLC_ENOIOCTL	1 
#define WLC_EINVAL	2 
#define WLC_ETOOSMALL	3 
#define WLC_ETOOBIG	4 
#define WLC_ERANGE	5 
#define WLC_EDOWN	6 
#define WLC_EUP		7 
#define WLC_ENOMEM	8 
#define WLC_EBUSY	9 

#define IOVF_MFG	(1<<3)  
#define IOVF_WHL	(1<<4)	
#define IOVF_NTRL	(1<<5)	

#define IOVF_SET_UP	(1<<6)	
#define IOVF_SET_DOWN	(1<<7)	
#define IOVF_SET_CLK	(1<<8)	
#define IOVF_SET_BAND	(1<<9)	

#define IOVF_GET_UP	(1<<10)	
#define IOVF_GET_DOWN	(1<<11)	
#define IOVF_GET_CLK	(1<<12)	
#define IOVF_GET_BAND	(1<<13)	
#define IOVF_OPEN_ALLOW	(1<<14)	

#define IOVF_BMAC_IOVAR	(1<<15) 

typedef int (*watchdog_fn_t)(void *handle);
typedef int (*down_fn_t)(void *handle);
typedef int (*dump_fn_t)(void *handle, struct bcmstrbuf *b);

typedef int (*iovar_fn_t)(void *handle, const bcm_iovar_t *vi, uint32 actionid,
	const char *name, void *params, uint plen, void *arg, int alen,
	int vsize, struct wlc_if *wlcif);
#endif 

typedef struct wlc_pub {
	void		*wlc;
	uint		unit;			
	uint		corerev;		
	osl_t		*osh;			
	si_t		*sih;			
	char		*vars;			
	bool		up;			
	bool		hw_off;			
	wlc_tunables_t *tunables;	
	bool		hw_up;			
	bool		_piomode;		 
	uint		_nbands;		
	uint		now;			

	bool		promisc;		
	bool		delayed_down;		
	bool		_ap;			
	bool		_apsta;			
	bool		_assoc_recreate;	
	int		_wme;			
	uint8		_mbss;			
#ifdef WLBDD
	bool		_bdd;			
#endif
#ifdef WLP2P
	bool		_p2p;			
#endif
	bool		allmulti;		
	bool		associated;		

	bool            phytest_on;             
	bool		bf_preempt_4306;	
	uint		txqstopped;		

	bool		_ampdu;			
	bool		_amsdu_tx;		
	bool		_cac;			
	int		_n_enab;		
	int		_n_reqd;		
	int8		_coex;			

	bool		_priofc;	

	struct ether_addr	cur_etheraddr;	

	struct ether_addr	*multicast; 
	uint		nmulticast;		

	uint32		wlfeatureflag;		
	int		psq_pkts_total;		

	uint		_activity;		

	uint16		txmaxpkts;		

	uint32		swdecrypt;		

	int 		bcmerror;		

	mbool		radio_disabled;		
	uint16		roam_time_thresh;	
	bool		align_wd_tbtt;		

	uint16		boardrev;		
	uint8		sromrev;		
	uint32		boardflags;		
	uint32		boardflags2;		

	wl_cnt_t	*_cnt;			
	wl_wme_cnt_t	*_wme_cnt;		

	bool		_win7;			
	bool		_extsta;		
	bool		_pkt_filter;		

	bool		_lmac;			

	uint16		bt_period; 		
	bool		phy_11ncapable;		
} wlc_pub_t;

typedef struct	wl_rxsts {
	uint	pkterror;		
	uint	phytype;		
	uint	channel;		
	uint	datarate;		
	uint	antenna;		
	uint	pktlength;		
	uint32	mactime;		
	uint	sq;			
	int32	signal;			
	int32	noise;			
	uint	preamble;		
	uint	encoding;		
	uint	nfrmtype;		
	struct wl_if *wlif;	
} wl_rxsts_t;

struct wlc_info;
struct wlc_hw_info;
struct wlc_bsscfg;
struct wlc_if;

#define	AP_ENAB(pub)	(0)

#define APSTA_ENAB(pub)	(0)

#define STA_ONLY(pub)	(!AP_ENAB(pub))
#define AP_ONLY(pub)	(AP_ENAB(pub) && !APSTA_ENAB(pub))

#ifndef LINUX_WLUSER_POSTMOGRIFY_REMOVAL

#define WLC_PREC_BMP_ALL		MAXBITVAL(WLC_PREC_COUNT)

#define WLC_PREC_BMP_AC_BE	(NBITVAL(WLC_PRIO_TO_PREC(PRIO_8021D_BE)) |	\
				NBITVAL(WLC_PRIO_TO_HI_PREC(PRIO_8021D_BE)) |	\
				NBITVAL(WLC_PRIO_TO_PREC(PRIO_8021D_EE)) |	\
				NBITVAL(WLC_PRIO_TO_HI_PREC(PRIO_8021D_EE)))
#define WLC_PREC_BMP_AC_BK	(NBITVAL(WLC_PRIO_TO_PREC(PRIO_8021D_BK)) |	\
				NBITVAL(WLC_PRIO_TO_HI_PREC(PRIO_8021D_BK)) |	\
				NBITVAL(WLC_PRIO_TO_PREC(PRIO_8021D_NONE)) |	\
				NBITVAL(WLC_PRIO_TO_HI_PREC(PRIO_8021D_NONE)))
#define WLC_PREC_BMP_AC_VI	(NBITVAL(WLC_PRIO_TO_PREC(PRIO_8021D_CL)) |	\
				NBITVAL(WLC_PRIO_TO_HI_PREC(PRIO_8021D_CL)) |	\
				NBITVAL(WLC_PRIO_TO_PREC(PRIO_8021D_VI)) |	\
				NBITVAL(WLC_PRIO_TO_HI_PREC(PRIO_8021D_VI)))
#define WLC_PREC_BMP_AC_VO	(NBITVAL(WLC_PRIO_TO_PREC(PRIO_8021D_VO)) |	\
				NBITVAL(WLC_PRIO_TO_HI_PREC(PRIO_8021D_VO)) |	\
				NBITVAL(WLC_PRIO_TO_PREC(PRIO_8021D_NC)) |	\
				NBITVAL(WLC_PRIO_TO_HI_PREC(PRIO_8021D_NC)))

#endif 

#define WME_ENAB(pub) ((pub)->_wme != OFF)
#define WME_AUTO(wlc) ((wlc)->pub->_wme == AUTO)

#define WLC_USE_COREFLAGS	0xffffffff	

#define WLC_UPDATE_STATS(wlc)	1	
#define WLCNTINCR(a)		((a)++)	
#define WLCNTDECR(a)		((a)--)	
#define WLCNTADD(a,delta)	((a) += (delta)) 
#define WLCNTSET(a,value)	((a) = (value)) 
#define WLCNTVAL(a)		(a)	

#ifndef LINUX_WLUSER_POSTMOGRIFY_REMOVAL

extern void * wlc_attach(void *wl, uint16 vendor, uint16 device, uint unit, bool piomode,
	osl_t *osh, void *regsva, uint bustype, void *btparam, uint *perr);
extern uint wlc_detach(struct wlc_info *wlc);
extern int  wlc_up(struct wlc_info *wlc);
extern uint wlc_down(struct wlc_info *wlc);

extern int wlc_set(struct wlc_info *wlc, int cmd, int arg);
extern int wlc_get(struct wlc_info *wlc, int cmd, int *arg);
extern int wlc_iovar_getint(struct wlc_info *wlc, const char *name, int *arg);
extern int wlc_iovar_setint(struct wlc_info *wlc, const char *name, int arg);
extern bool wlc_chipmatch(uint16 vendor, uint16 device);
extern void wlc_init(struct wlc_info *wlc);
extern void wlc_reset(struct wlc_info *wlc);

extern void wlc_intrson(struct wlc_info *wlc);
extern uint32 wlc_intrsoff(struct wlc_info *wlc);
extern void wlc_intrsrestore(struct wlc_info *wlc, uint32 macintmask);
extern bool wlc_intrsupd(struct wlc_info *wlc);
extern bool wlc_isr(struct wlc_info *wlc, bool *wantdpc);
extern bool wlc_dpc(struct wlc_info *wlc, bool bounded);
extern bool wlc_sendpkt(struct wlc_info *wlc, void *sdu, struct wlc_if *wlcif);
extern bool wlc_send80211_raw(struct wlc_info *wlc, void *p, uint ac);
extern int wlc_iovar_op(struct wlc_info *wlc, const char *name, void *params, int p_len, void *arg,
	int len, bool set, struct wlc_if *wlcif);
extern int wlc_ioctl(struct wlc_info *wlc, int cmd, void *arg, int len, struct wlc_if *wlcif);

extern void wlc_statsupd(struct wlc_info *wlc);
#endif 

extern wlc_pub_t *wlc_pub(void *wlc);

#ifndef LINUX_WLUSER_POSTMOGRIFY_REMOVAL
extern int wlc_module_register(wlc_pub_t *pub, const bcm_iovar_t *iovars,
                               const char *name, void *hdl, iovar_fn_t iovar_fn,
                               watchdog_fn_t watchdog_fn, down_fn_t down_fn);
extern int wlc_module_unregister(wlc_pub_t *pub, const char *name, void *hdl);
#endif 

#ifndef LINUX_WLUSER_POSTMOGRIFY_REMOVAL

#define WLC_RPCTX_PARAMS        32

#endif 
#endif 
