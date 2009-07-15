/*
 * wl_linux.c exported functions and definitions
 *
 * Copyright 2008, Broadcom Corporation
 * All Rights Reserved.
 * 
 *  	Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2, available at
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html (the "GPL"), with the
 * following added to such license:
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, regardless of the
 * license terms of these independent modules, and to copy and distribute the
 * resulting executable under terms of your choice, provided that you also meet,
 * for each linked independent module, the terms and conditions of the license
 * of that module. An independent module is a module which is not derived from
 * this software.
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: wl_linux.h,v 1.22.2.3.4.1 2009/06/18 22:40:13 Exp $
 */

#ifndef _wl_linux_h_
#define _wl_linux_h_

typedef struct wl_timer {
	struct timer_list timer;
	struct wl_info *wl;
	void (*fn)(void *);
	void* arg; 
	uint ms;
	bool periodic;
	bool set;
	struct wl_timer *next;
#ifdef BCMDBG
	char* name; 
#endif
} wl_timer_t;

typedef struct wl_task {
	struct work_struct work;
	void *context;
} wl_task_t;

#define WL_IFTYPE_BSS	1 
#define WL_IFTYPE_WDS	2 
#define WL_IFTYPE_MON	3 

typedef struct wl_if {
#ifdef CONFIG_WIRELESS_EXT
	wl_iw_t		iw;		
#endif 
	struct wl_if *next;
	struct wl_info *wl;		
	struct net_device *dev;		
	int type;			
	struct wlc_if *wlcif;		
	struct ether_addr remote;	
	uint subunit;			
	bool dev_registed;		
} wl_if_t;

struct wl_info {
	wlc_pub_t	*pub;		
	void		*wlc;		
	osl_t		*osh;		
	struct net_device *dev;		
	spinlock_t	lock;		
	spinlock_t	isr_lock;	
	uint		bustype;	
	bool		piomode;	
	void *regsva;			
	struct net_device_stats stats;	
	wl_if_t *if_list;		
	struct wl_info *next;		
	atomic_t callbacks;		
	struct wl_timer *timers;	
	struct tasklet_struct tasklet;	
	struct net_device *monitor;	
	bool		resched;	
	uint32		pci_psstate[16];	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
	struct lib80211_crypto_ops *tkipmodops;
#else
	struct ieee80211_crypto_ops *tkipmodops;	
#endif
	struct ieee80211_tkip_data  *tkip_ucast_data;
	struct ieee80211_tkip_data  *tkip_bcast_data;
#endif 

	uint	stats_id;		

	struct net_device_stats stats_watchdog[2];
#ifdef CONFIG_WIRELESS_EXT
	struct iw_statistics wstats_watchdog[2];
	struct iw_statistics wstats;
	int		phy_noise;
#endif 

};

#define WL_LOCK(wl)	spin_lock_bh(&(wl)->lock)
#define WL_UNLOCK(wl)	spin_unlock_bh(&(wl)->lock)

#define WL_ISRLOCK(wl, flags) do {spin_lock(&(wl)->isr_lock); (void)(flags);} while (0)
#define WL_ISRUNLOCK(wl, flags) do {spin_unlock(&(wl)->isr_lock); (void)(flags);} while (0)

#define INT_LOCK(wl, flags)	spin_lock_irqsave(&(wl)->isr_lock, flags)
#define INT_UNLOCK(wl, flags)	spin_unlock_irqrestore(&(wl)->isr_lock, flags)

typedef struct wl_info wl_info_t;

#ifndef PCI_D0
#define PCI_D0		0
#endif

#ifndef PCI_D3hot
#define PCI_D3hot	3
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
extern irqreturn_t wl_isr(int irq, void *dev_id);
#else
extern irqreturn_t wl_isr(int irq, void *dev_id, struct pt_regs *ptregs);
#endif

extern int __devinit wl_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent);
extern void wl_free(wl_info_t *wl);
extern int  wl_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
extern struct net_device * wl_netdev_get(wl_info_t *wl);

#endif 
