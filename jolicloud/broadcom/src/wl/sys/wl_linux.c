/*
 * Linux-specific portion of
 * Broadcom 802.11abg Networking Device Driver
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
 * $Id: wl_linux.c,v 1.388.2.78.4.2 2009/04/03 02:06:22 Exp $
 */

#define LINUX_PORT

#define __UNDEF_NO_VERSION__

#include <typedefs.h>
#include <linuxver.h>
#include <osl.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
#include <linux/module.h>
#endif

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/ethtool.h>
#include <linux/completion.h>
#include <linux/pci_ids.h>
#define WLC_MAXBSSCFG		1	

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
#include <net/ieee80211.h>
#endif

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/pgtable.h>
#include <asm/uaccess.h>
#include <asm/unaligned.h>

#include <proto/802.1d.h>

#include <epivers.h>
#include <bcmendian.h>
#include <proto/ethernet.h>
#include <bcmutils.h>
#include <pcicfg.h>
#include <wlioctl.h>
#include <wlc_key.h>

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 4, 5)
#error "No support for Kernel Rev <= 2.4.5, As the older kernel revs doesn't support Tasklets"
#endif

typedef void wlc_info_t;
typedef void wlc_hw_info_t;
typedef const struct si_pub	si_t;

#include <wlc_pub.h>
#include <wl_dbg.h>

#ifdef CONFIG_WIRELESS_EXT
#include <wl_iw.h>
struct iw_statistics *wl_get_wireless_stats(struct net_device *dev);
#endif 

#include <wl_export.h>

#include <wl_linux.h>

static void wl_timer(ulong data);
static void _wl_timer(wl_timer_t *t);

static int wl_linux_watchdog(void *ctx);
static
int wl_found = 0;

struct ieee80211_tkip_data {
#define TKIP_KEY_LEN 32
	u8 key[TKIP_KEY_LEN];
	int key_set;

	u32 tx_iv32;
	u16 tx_iv16;
	u16 tx_ttak[5];
	int tx_phase1_done;

	u32 rx_iv32;
	u16 rx_iv16;
	u16 rx_ttak[5];
	int rx_phase1_done;
	u32 rx_iv32_new;
	u16 rx_iv16_new;

	u32 dot11RSNAStatsTKIPReplays;
	u32 dot11RSNAStatsTKIPICVErrors;
	u32 dot11RSNAStatsTKIPLocalMICFailures;

	int key_idx;

	struct crypto_tfm *tfm_arc4;
	struct crypto_tfm *tfm_michael;

	u8 rx_hdr[16], tx_hdr[16];
};

#define	WL_DEV_IF(dev)		((wl_if_t*)(dev)->priv)			
#define	WL_INFO(dev)		((wl_info_t*)(WL_DEV_IF(dev)->wl))	

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
#define	WL_ISR(i, d, p)		wl_isr((i), (d))
#else
#define	WL_ISR(i, d, p)		wl_isr((i), (d), (p))
#endif	

static int wl_open(struct net_device *dev);
static int wl_close(struct net_device *dev);
static int wl_start(struct sk_buff *skb, struct net_device *dev);
static int wl_start_int(wl_if_t *wlif, struct sk_buff *skb);

static struct net_device_stats *wl_get_stats(struct net_device *dev);
static int wl_set_mac_address(struct net_device *dev, void *addr);
static void wl_set_multicast_list(struct net_device *dev);
static void _wl_set_multicast_list(struct net_device *dev);
static int wl_ethtool(wl_info_t *wl, void *uaddr, wl_if_t *wlif);
static void wl_dpc(ulong data);
static void wl_link_up(wl_info_t *wl);
static void wl_link_down(wl_info_t *wl);
static void wl_mic_error(wl_info_t *wl, struct ether_addr *ea, bool group, bool flush_txq);
#if defined(CONFIG_PROC_FS)
static int wl_read_proc(char *buffer, char **start, off_t offset, int length, int *eof, void *data);
#endif 
#if defined(BCMDBG)
static int wl_dump(wl_info_t *wl, struct bcmstrbuf *b);
#endif 
struct wl_if *wl_alloc_if(wl_info_t *wl, int iftype, uint unit, struct wlc_if* wlc_if);
static void wl_free_if(wl_info_t *wl, wl_if_t *wlif);
static void wl_get_driver_info(struct net_device *dev, struct ethtool_drvinfo *info);

MODULE_LICENSE("MIXED/Proprietary");

static struct pci_device_id wl_id_table[] = {
	{ PCI_VENDOR_ID_BROADCOM, 0x4311, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 }, 
	{ PCI_VENDOR_ID_BROADCOM, 0x4312, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 }, 
	{ PCI_VENDOR_ID_BROADCOM, 0x4313, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 }, 
	{ PCI_VENDOR_ID_BROADCOM, 0x4315, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 }, 
	{ PCI_VENDOR_ID_BROADCOM, 0x4328, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 }, 
	{ PCI_VENDOR_ID_BROADCOM, 0x4329, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 }, 
	{ PCI_VENDOR_ID_BROADCOM, 0x432a, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 }, 
	{ PCI_VENDOR_ID_BROADCOM, 0x432b, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 }, 
	{ PCI_VENDOR_ID_BROADCOM, 0x432c, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 }, 
	{ PCI_VENDOR_ID_BROADCOM, 0x432d, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 }, 
	{ PCI_VENDOR_ID_BROADCOM, 0x4353, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 }, 
	{ PCI_VENDOR_ID_BROADCOM, 0x4357, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 }, 
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, wl_id_table);

#ifdef BCMDBG
static int msglevel = 0xdeadbeef;
module_param(msglevel, int, 0);
static int msglevel2 = 0xdeadbeef;
module_param(msglevel2, int, 0);
#endif 

static int oneonly = 0;
module_param(oneonly, int, 0);

static int piomode = 0;
module_param(piomode, int, 0);

static int nompc = 0;
module_param(nompc, int, 0);

static char name[IFNAMSIZ] = "eth%d";
module_param_string(name, name, IFNAMSIZ, 0);

#ifndef	SRCBASE
#define	SRCBASE "."
#endif 

#if WIRELESS_EXT >= 19
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
static struct ethtool_ops wl_ethtool_ops =
#else
static const struct ethtool_ops wl_ethtool_ops =
#endif 
{
	.get_drvinfo = wl_get_driver_info
};
#endif 

static
void wl_if_setup(struct net_device *dev)
{
	dev->open = wl_open;
	dev->stop = wl_close;
	dev->hard_start_xmit = wl_start;
	dev->get_stats = wl_get_stats;
	dev->set_mac_address = wl_set_mac_address;
	dev->set_multicast_list = wl_set_multicast_list;
	dev->do_ioctl = wl_ioctl;
#ifdef CONFIG_WIRELESS_EXT
#if WIRELESS_EXT < 19
	dev->get_wireless_stats = wl_get_wireless_stats;
#endif 
#if WIRELESS_EXT > 12
	dev->wireless_handlers = (struct iw_handler_def *) &wl_iw_handler_def;
#endif 
#if WIRELESS_EXT >= 19
	dev->ethtool_ops = &wl_ethtool_ops;
#endif 

#endif 
}

static wl_info_t *
wl_attach(uint16 vendor, uint16 device, ulong regs, uint bustype, void *btparam, uint irq)
{
	struct net_device *dev;
	wl_if_t *wlif;
	wl_info_t *wl;
#if defined(CONFIG_PROC_FS)
	char tmp[128];
#endif
	osl_t *osh;
	int unit;

	unit = wl_found;

	if (oneonly && (unit != 0)) {
		WL_ERROR(("wl%d: wl_attach: oneonly is set, exiting\n", unit));
		return NULL;
	}

	osh = osl_attach(btparam, bustype, TRUE);
	ASSERT(osh);

	if ((wl = (wl_info_t*) MALLOC(osh, sizeof(wl_info_t))) == NULL) {
		WL_ERROR(("wl%d: malloc wl_info_t, out of memory, malloced %d bytes\n", unit,
			MALLOCED(osh)));
		osl_detach(osh);
		return NULL;
	}
	bzero(wl, sizeof(wl_info_t));

	wl->osh = osh;
	atomic_set(&wl->callbacks, 0);

	wlif = wl_alloc_if(wl, WL_IFTYPE_BSS, unit, NULL);
	if (!wlif) {
		WL_ERROR(("wl%d: wl_alloc_if failed\n", unit));
		MFREE(osh, wl, sizeof(wl_info_t));
		osl_detach(osh);
		return NULL;
	}

	dev = wlif->dev;
	wl->dev = dev;
	wl_if_setup(dev);

	dev->base_addr = regs;

	WL_TRACE(("wl%d: Bus: ", unit));
	if (bustype == PCMCIA_BUS) {

		wl->piomode = TRUE;
		WL_TRACE(("PCMCIA\n"));
	} else if (bustype == PCI_BUS) {

		wl->piomode = piomode;
		WL_TRACE(("PCI/%s\n", wl->piomode ? "PIO" : "DMA"));
	}
	else if (bustype == RPC_BUS) {

	} else {
		bustype = PCI_BUS;
		WL_TRACE(("force to PCI\n"));
	}
	wl->bustype = bustype;

	if ((wl->regsva = ioremap_nocache(dev->base_addr, PCI_BAR0_WINSZ)) == NULL) {
		WL_ERROR(("wl%d: ioremap() failed\n", unit));
		goto fail;
	}

	spin_lock_init(&wl->lock);
	spin_lock_init(&wl->isr_lock);

	{
	int err;
	if (!(wl->wlc = wlc_attach((void *) wl, vendor, device, unit, wl->piomode,
		osh, wl->regsva, wl->bustype, btparam, &err))) {
		printf("%s: %s driver failed with code %d\n", dev->name, EPI_VERSION_STR, err);
		goto fail;
	}
	}
	wl->pub = (wlc_pub_t *)wl->wlc;

	if (nompc) {
		if (wlc_iovar_setint(wl->wlc, "mpc", 0)) {
			WL_ERROR(("wl%d: Error setting MPC variable to 0\n", unit));
		}
	}

#if defined(CONFIG_PROC_FS)

	sprintf(tmp, "net/wl%d", wl->pub->unit);
	create_proc_read_entry(tmp, 0, 0, wl_read_proc, (void*)wl);
#endif 

	bcopy(&wl->pub->cur_etheraddr, dev->dev_addr, ETHER_ADDR_LEN);

	tasklet_init(&wl->tasklet, wl_dpc, (ulong)wl);

	{
		if (request_irq(irq, wl_isr, IRQF_SHARED, dev->name, wl)) {
			WL_ERROR(("wl%d: request_irq() failed\n", unit));
			goto fail;
		}
		dev->irq = irq;
	}

	if (wl->bustype == PCI_BUS) {
		struct pci_dev *pci_dev = (struct pci_dev *)btparam;
		if (pci_dev != NULL)
			SET_NETDEV_DEV(dev, &pci_dev->dev);
	}

	if (register_netdev(dev)) {
		WL_ERROR(("wl%d: register_netdev() failed\n", unit));
		goto fail;
	}
	wlif->dev_registed = TRUE;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)

	wl->tkipmodops = ieee80211_get_crypto_ops("TKIP");
	if (wl->tkipmodops == NULL) {
		request_module("ieee80211_crypt_tkip");
		wl->tkipmodops = ieee80211_get_crypto_ops("TKIP");
	}
#endif 
#ifdef CONFIG_WIRELESS_EXT
	wlif->iw.wlinfo = (void *)wl;
#endif

	if (wlc_iovar_setint(wl->wlc, "leddc", 0xa0000)) {
		WL_ERROR(("wl%d: Error setting led duty-cycle\n", unit));
	}
	if (wlc_set(wl->wlc, WLC_SET_PM, PM_FAST)) {
		WL_ERROR(("wl%d: Error setting PM variable to FAST PS\n", unit));
	}

	if (wlc_iovar_setint(wl->wlc, "vlan_mode", OFF)) {
		WL_ERROR(("wl%d: Error setting vlan mode OFF\n", unit));
	}

	if (wlc_set(wl->wlc, WLC_SET_INFRA, 1)) {
		WL_ERROR(("wl%d: Error setting infra_mode to infrastructure\n", unit));
	}

	wlc_module_register(wl->pub, NULL, "linux", wl, NULL, wl_linux_watchdog, NULL);

#ifdef BCMDBG
	wlc_dump_register(wl->pub, "wl", (dump_fn_t)wl_dump, (void *)wl);
#endif

	printf("%s: Broadcom BCM%04x 802.11 Wireless Controller " EPI_VERSION_STR,
		dev->name, device);

#ifdef BCMDBG
	printf(" (Compiled in " SRCBASE " at " __TIME__ " on " __DATE__ ")");
#endif 
	printf("\n");

	wl_found++;
	return wl;

fail:
	wl_free(wl);
	return NULL;
}

#if defined(CONFIG_PROC_FS)
static int
wl_read_proc(char *buffer, char **start, off_t offset, int length, int *eof, void *data)
{
	wl_info_t *wl;
	int len;
	off_t pos;
	off_t begin;

	len = pos = begin = 0;

	wl = (wl_info_t*) data;

	WL_LOCK(wl);

#if defined(BCMDBG)
	wlc_iovar_dump(wl->wlc, "all", strlen("all") + 1, buffer, PAGE_SIZE);
	len = strlen(buffer);
#endif 
	WL_UNLOCK(wl);
	pos = begin + len;

	if (pos < offset) {
		len = 0;
		begin = pos;
	}

	*eof = 1;

	*start = buffer + (offset - begin);
	len -= (offset - begin);

	if (len > length)
		len = length;

	return (len);
}
#endif 

static void __devexit wl_remove(struct pci_dev *pdev);

int __devinit
wl_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int rc;
	wl_info_t *wl;
	uint32 val;

	WL_TRACE(("%s: bus %d slot %d func %d irq %d\n", __FUNCTION__,
	          pdev->bus->number, PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn), pdev->irq));

	if ((pdev->vendor != PCI_VENDOR_ID_BROADCOM) ||
	    (((pdev->device & 0xff00) != 0x4300) &&
	     ((pdev->device & 0xff00) != 0x4700)))
		return (-ENODEV);

	rc = pci_enable_device(pdev);
	if (rc) {
		WL_ERROR(("%s: Cannot enable device %d-%d_%d\n", __FUNCTION__,
		          pdev->bus->number, PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn)));
		return (-ENODEV);
	}
	pci_set_master(pdev);

	pci_read_config_dword(pdev, 0x40, &val);
	if ((val & 0x0000ff00) != 0)
		pci_write_config_dword(pdev, 0x40, val & 0xffff00ff);

	wl = wl_attach(pdev->vendor, pdev->device, pci_resource_start(pdev, 0), PCI_BUS, pdev,
		pdev->irq);
	if (!wl)
		return -ENODEV;

	pci_set_drvdata(pdev, wl);

	return 0;
}

static int
wl_suspend(struct pci_dev *pdev, DRV_SUSPEND_STATE_TYPE state)
{
	wl_info_t *wl = (wl_info_t *) pci_get_drvdata(pdev);

	WL_TRACE(("wl: wl_suspend\n"));

	wl = (wl_info_t *) pci_get_drvdata(pdev);
	if (!wl) {
		WL_ERROR(("wl: wl_suspend: pci_get_drvdata failed\n"));
		return -ENODEV;
	}

	WL_LOCK(wl);
	WL_APSTA_UPDN(("wl%d (%s): wl_suspend() -> wl_down()\n", wl->pub->unit, wl->dev->name));
	wl_down(wl);
	wl->pub->hw_up = FALSE;
	WL_UNLOCK(wl);
	PCI_SAVE_STATE(pdev, wl->pci_psstate);
	pci_disable_device(pdev);
	return pci_set_power_state(pdev, PCI_D3hot);
}

static int
wl_resume(struct pci_dev *pdev)
{
	wl_info_t *wl = (wl_info_t *) pci_get_drvdata(pdev);
	int err = 0;
	uint32 val;

	WL_TRACE(("wl: wl_resume\n"));

	if (!wl) {
		WL_ERROR(("wl: wl_resume: pci_get_drvdata failed\n"));
	        return -ENODEV;
	}

	err = pci_set_power_state(pdev, PCI_D0);
	if (err)
		return err;

	PCI_RESTORE_STATE(pdev, wl->pci_psstate);

	err = pci_enable_device(pdev);
	if (err)
		return err;

	pci_set_master(pdev);

	pci_read_config_dword(pdev, 0x40, &val);
	if ((val & 0x0000ff00) != 0)
		pci_write_config_dword(pdev, 0x40, val & 0xffff00ff);

	WL_LOCK(wl);
	WL_APSTA_UPDN(("wl%d: (%s): wl_resume() -> wl_up()\n", wl->pub->unit, wl->dev->name));
	err = wl_up(wl);
	WL_UNLOCK(wl);

	return (err);
}

static void __devexit
wl_remove(struct pci_dev *pdev)
{
	wl_info_t *wl = (wl_info_t *) pci_get_drvdata(pdev);

	if (!wl) {
		WL_ERROR(("wl: wl_remove: pci_get_drvdata failed\n"));
		return;
	}
	if (!wlc_chipmatch(pdev->vendor, pdev->device)) {
		WL_ERROR(("wl: wl_remove: wlc_chipmatch failed\n"));
		return;
	}

	WL_LOCK(wl);
	WL_APSTA_UPDN(("wl%d (%s): wl_remove() -> wl_down()\n", wl->pub->unit, wl->dev->name));
	wl_down(wl);
	WL_UNLOCK(wl);
	wl_free(wl);
	pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);
}

static struct pci_driver wl_pci_driver = {
	name:		"wl",
	probe:		wl_pci_probe,
	suspend:	wl_suspend,
	resume:		wl_resume,
	remove:		__devexit_p(wl_remove),
	id_table:	wl_id_table,
	};

static int __init
wl_module_init(void)
{
	int error = -ENODEV;

#ifdef BCMDBG
	if (msglevel != 0xdeadbeef)
		wl_msg_level = msglevel;
	else {
		char *var = getvar(NULL, "wl_msglevel");
		if (var)
			wl_msg_level = bcm_strtoul(var, NULL, 0);
	}
	printf("%s: msglevel set to 0x%x\n", __FUNCTION__, wl_msg_level);
	if (msglevel2 != 0xdeadbeef)
		wl_msg_level2 = msglevel2;
	else {
		char *var = getvar(NULL, "wl_msglevel2");
		if (var)
			wl_msg_level2 = bcm_strtoul(var, NULL, 0);
	}
	printf("%s: msglevel2 set to 0x%x\n", __FUNCTION__, wl_msg_level2);
#endif 

	if (!(error = pci_module_init(&wl_pci_driver)))
		return (0);

	return (error);
}

static void __exit
wl_module_exit(void)
{

	pci_unregister_driver(&wl_pci_driver);

}

module_init(wl_module_init);
module_exit(wl_module_exit);

void
wl_free(wl_info_t *wl)
{
	wl_timer_t *t, *next;
	osl_t *osh;

	WL_TRACE(("wl: wl_free\n"));
	{
		if (wl->dev && wl->dev->irq)
			free_irq(wl->dev->irq, wl);
	}

	if (wl->dev) {
		wl_free_if(wl, WL_DEV_IF(wl->dev));
		wl->dev = NULL;
	}

	tasklet_kill(&wl->tasklet);

	if (wl->pub) {
		wlc_module_unregister(wl->pub, "linux", wl);
	}

	if (wl->wlc) {
#if defined(CONFIG_PROC_FS)
		char tmp[128];

		sprintf(tmp, "net/wl%d", wl->pub->unit);
		remove_proc_entry(tmp, 0);
#endif 
		wlc_detach(wl->wlc);
		wl->wlc = NULL;
		wl->pub = NULL;
	}

	while (atomic_read(&wl->callbacks) > 0)
		schedule();

	for (t = wl->timers; t; t = next) {
		next = t->next;
#ifdef BCMDBG
		if (t->name)
			MFREE(wl->osh, t->name, strlen(t->name) + 1);
#endif
		MFREE(wl->osh, t, sizeof(wl_timer_t));
	}

	if (wl->monitor) {
		wl_free_if(wl, (wl_if_t *)(wl->monitor->priv));
		wl->monitor = NULL;
	}

	osh = wl->osh;

	if (wl->regsva && BUSTYPE(wl->bustype) != SDIO_BUS &&
	    BUSTYPE(wl->bustype) != JTAG_BUS) {
		iounmap((void*)wl->regsva);
	}
	wl->regsva = NULL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)

	if (wl->tkipmodops != NULL) {
		if (wl->tkip_ucast_data) {
			wl->tkipmodops->deinit(wl->tkip_ucast_data);
			wl->tkip_ucast_data = NULL;
		}
		if (wl->tkip_bcast_data) {
			wl->tkipmodops->deinit(wl->tkip_bcast_data);
			wl->tkip_bcast_data = NULL;
		}
	}
#endif 

	MFREE(osh, wl, sizeof(wl_info_t));

	if (MALLOCED(osh)) {
		printf("Memory leak of bytes %d\n", MALLOCED(osh));
		ASSERT(0);
	}

	osl_detach(osh);
}

static int
wl_open(struct net_device *dev)
{
	wl_info_t *wl;
	int error = 0;

	if (!dev)
		return -ENETDOWN;

	wl = WL_INFO(dev);

	WL_TRACE(("wl%d: wl_open\n", wl->pub->unit));

	WL_LOCK(wl);
	WL_APSTA_UPDN(("wl%d: (%s): wl_open() -> wl_up()\n",
	               wl->pub->unit, wl->dev->name));

	error = wl_up(wl);
	if (!error) {
		error = wlc_set(wl->wlc, WLC_SET_PROMISC, (dev->flags & IFF_PROMISC));
	}
	WL_UNLOCK(wl);

	if (!error)
		OLD_MOD_INC_USE_COUNT;

	return (error? -ENODEV: 0);
}

static int
wl_close(struct net_device *dev)
{
	wl_info_t *wl;

	if (!dev)
		return -ENETDOWN;

	wl = WL_INFO(dev);

	WL_TRACE(("wl%d: wl_close\n", wl->pub->unit));

	WL_LOCK(wl);
	WL_APSTA_UPDN(("wl%d (%s): wl_close() -> wl_down()\n",
		wl->pub->unit, wl->dev->name));
	wl_down(wl);
	WL_UNLOCK(wl);

	OLD_MOD_DEC_USE_COUNT;

	return (0);
}

static int BCMFASTPATH
wl_start(struct sk_buff *skb, struct net_device *dev)
{
	wl_if_t *wlif;

	if (!dev)
		return -ENETDOWN;

	wlif = WL_DEV_IF(dev);

	return wl_start_int(wlif, skb);
}

static int BCMFASTPATH
wl_start_int(wl_if_t *wlif, struct sk_buff *skb)
{
	wl_info_t *wl;
	void *pkt;

	wl = wlif->wl;

	WL_LOCK(wl);

	WL_TRACE(("wl%d: wl_start: len %d summed %d\n", wl->pub->unit, skb->len, skb->ip_summed));

	if ((pkt = PKTFRMNATIVE(wl->osh, skb)) == NULL) {
		WL_ERROR(("wl%d: PKTFRMNATIVE failed!\n", wl->pub->unit));
		WLCNTINCR(wl->pub->_cnt.txnobuf);
		dev_kfree_skb_any(skb);
		WL_UNLOCK(wl);
		return 0;
	}

	if (WME_ENAB(wl->pub) && (PKTPRIO(pkt) == 0))
		pktsetprio(pkt, FALSE);
	wlc_sendpkt(wl->wlc, pkt, wlif->wlcif);
	WL_UNLOCK(wl);
	return (0);
}

void
wl_txflowcontrol(wl_info_t *wl, bool state, int prio)
{
	wl_if_t *wlif;

	ASSERT(prio == ALLPRIO);
	for (wlif = wl->if_list; wlif != NULL; wlif = wlif->next) {
		if (state == ON)
			netif_stop_queue(wlif->dev);
		else
			netif_wake_queue(wlif->dev);
	}
}

struct wl_if *
wl_alloc_if(wl_info_t *wl, int iftype, uint subunit, struct wlc_if* wlcif)
{
	struct net_device *dev;
	wl_if_t *wlif;
	wl_if_t *p;

	if (!(wlif = MALLOC(wl->osh, sizeof(wl_if_t)))) {
		WL_ERROR(("wl%d: wl_alloc_if: out of memory, malloced %d bytes\n",
		          (wl->pub)?wl->pub->unit:subunit, MALLOCED(wl->osh)));
		return NULL;
	}
	bzero(wlif, sizeof(wl_if_t));
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24))
	if (!(dev = MALLOC(wl->osh, sizeof(struct net_device)))) {
		MFREE(wl->osh, wlif, sizeof(wl_if_t));
		WL_ERROR(("wl%d: wl_alloc_if: out of memory, malloced %d bytes\n",
		          (wl->pub)?wl->pub->unit:subunit, MALLOCED(wl->osh)));
		return NULL;
	}
	bzero(dev, sizeof(struct net_device));
	ether_setup(dev);
	strncpy(dev->name, name, IFNAMSIZ);
#else

	dev = alloc_netdev(0, name, ether_setup);
	if (!dev) {
		MFREE(wl->osh, wlif, sizeof(wl_if_t));
		WL_ERROR(("wl%d: wl_alloc_if: out of memory, alloc_netdev\n",
			(wl->pub)?wl->pub->unit:subunit));
		return NULL;
	}
#endif 

	wlif->type = iftype;
	wlif->dev = dev;
	wlif->wl = wl;
	wlif->wlcif = wlcif;
	wlif->subunit = subunit;
	dev->priv = wlif;

	if (iftype != WL_IFTYPE_MON && wl->dev && netif_queue_stopped(wl->dev))
		netif_stop_queue(dev);

	if (wl->if_list == NULL)
		wl->if_list = wlif;
	else {
		p = wl->if_list;
		while (p->next != NULL)
			p = p->next;
		p->next = wlif;
	}
	return wlif;
}

static void
wl_free_if(wl_info_t *wl, wl_if_t *wlif)
{
	wl_if_t *p;

	if (wlif->dev_registed)
		unregister_netdev(wlif->dev);

	p = wl->if_list;
	if (p == wlif)
		wl->if_list = p->next;
	else {
		while (p != NULL && p->next != wlif)
			p = p->next;
		if (p != NULL)
			p->next = p->next->next;
	}

	if (wlif->dev) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24))
		MFREE(wl->osh, wlif->dev, sizeof(struct net_device));
#else
		free_netdev(wlif->dev);
#endif
	}
	MFREE(wl->osh, wlif, sizeof(wl_if_t));
}

char *
wl_ifname(wl_info_t *wl, wl_if_t *wlif)
{
	if (wlif)
		return wlif->dev->name;
	else
		return wl->dev->name;
}

void
wl_init(wl_info_t *wl)
{
	WL_TRACE(("wl%d: wl_init\n", wl->pub->unit));

	wl_reset(wl);

	wlc_init(wl->wlc);
}

uint
wl_reset(wl_info_t *wl)
{
	WL_TRACE(("wl%d: wl_reset\n", wl->pub->unit));

	wlc_reset(wl->wlc);

	wl->resched = 0;

	return (0);
}

void BCMFASTPATH
wl_intrson(wl_info_t *wl)
{
	unsigned long flags;

	INT_LOCK(wl, flags);
	wlc_intrson(wl->wlc);
	INT_UNLOCK(wl, flags);
}

bool
wl_alloc_dma_resources(wl_info_t *wl, uint addrwidth)
{
	return TRUE;
}

uint32 BCMFASTPATH
wl_intrsoff(wl_info_t *wl)
{
	unsigned long flags;
	uint32 status;

	INT_LOCK(wl, flags);
	status = wlc_intrsoff(wl->wlc);
	INT_UNLOCK(wl, flags);
	return status;
}

void
wl_intrsrestore(wl_info_t *wl, uint32 macintmask)
{
	unsigned long flags;

	INT_LOCK(wl, flags);
	wlc_intrsrestore(wl->wlc, macintmask);
	INT_UNLOCK(wl, flags);
}

int
wl_up(wl_info_t *wl)
{
	int error = 0;

	WL_TRACE(("wl%d: wl_up\n", wl->pub->unit));

	if (wl->pub->up)
		return (0);

	error = wlc_up(wl->wlc);

	if (!error)
		wl_txflowcontrol(wl, OFF, ALLPRIO);

	return (error);
}

void
wl_down(wl_info_t *wl)
{
	wl_if_t *wlif;
	uint callbacks, ret_val = 0;

	WL_TRACE(("wl%d: wl_down\n", wl->pub->unit));

	for (wlif = wl->if_list; wlif != NULL; wlif = wlif->next) {
		WL_INFORM(("%s: stop interfaces\n", __FUNCTION__));
		netif_down(wlif->dev);
		netif_stop_queue(wlif->dev);
	}

	ret_val = wlc_down(wl->wlc);
	callbacks = atomic_read(&wl->callbacks) - ret_val;

	WL_UNLOCK(wl);

	SPINWAIT((atomic_read(&wl->callbacks) > callbacks), 100 * 1000);
	ASSERT(atomic_read(&wl->callbacks) == callbacks);

	WL_LOCK(wl);
}

static int
wl_toe_get(wl_info_t *wl, uint32 *toe_ol)
{
	if (wlc_iovar_getint(wl->wlc, "toe_ol", toe_ol) != 0)
		return -EOPNOTSUPP;

	return 0;
}

static int
wl_toe_set(wl_info_t *wl, uint32 toe_ol)
{
	if (wlc_iovar_setint(wl->wlc, "toe_ol", toe_ol) != 0)
		return -EOPNOTSUPP;

	if (wlc_iovar_setint(wl->wlc, "toe", (toe_ol != 0)) != 0)
		return -EOPNOTSUPP;

	return 0;
}

static void
wl_get_driver_info(struct net_device *dev, struct ethtool_drvinfo *info)
{
	wl_info_t *wl = WL_INFO(dev);
	bzero(info, sizeof(struct ethtool_drvinfo));
	sprintf(info->driver, "wl%d", wl->pub->unit);
	strcpy(info->version, EPI_VERSION_STR);
}

static int
wl_ethtool(wl_info_t *wl, void *uaddr, wl_if_t *wlif)
{
	struct ethtool_drvinfo info;
	struct ethtool_value edata;
	uint32 cmd;
	uint32 toe_cmpnt = 0, csum_dir;
	int ret;

	WL_TRACE(("wl%d: %s\n", wl->pub->unit, __FUNCTION__));
	if (copy_from_user(&cmd, uaddr, sizeof(uint32)))
		return (-EFAULT);

	switch (cmd) {
	case ETHTOOL_GDRVINFO:
		wl_get_driver_info(wl->dev, &info);
		info.cmd = cmd;
		if (copy_to_user(uaddr, &info, sizeof(info)))
			return (-EFAULT);
		break;

	case ETHTOOL_GRXCSUM:
	case ETHTOOL_GTXCSUM:
		if ((ret = wl_toe_get(wl, &toe_cmpnt)) < 0)
			return ret;

		csum_dir = (cmd == ETHTOOL_GTXCSUM) ? TOE_TX_CSUM_OL : TOE_RX_CSUM_OL;

		edata.cmd = cmd;
		edata.data = (toe_cmpnt & csum_dir) ? 1 : 0;

		if (copy_to_user(uaddr, &edata, sizeof(edata)))
			return (-EFAULT);
		break;

	case ETHTOOL_SRXCSUM:
	case ETHTOOL_STXCSUM:
		if (copy_from_user(&edata, uaddr, sizeof(edata)))
			return (-EFAULT);

		if ((ret = wl_toe_get(wl, &toe_cmpnt)) < 0)
			return ret;

		csum_dir = (cmd == ETHTOOL_STXCSUM) ? TOE_TX_CSUM_OL : TOE_RX_CSUM_OL;

		if (edata.data != 0)
			toe_cmpnt |= csum_dir;
		else
			toe_cmpnt &= ~csum_dir;

		if ((ret = wl_toe_set(wl, toe_cmpnt)) < 0)
			return ret;

		if (cmd == ETHTOOL_STXCSUM) {
			if (edata.data)
				wl->dev->features |= NETIF_F_IP_CSUM;
			else
				wl->dev->features &= ~NETIF_F_IP_CSUM;
		}

		break;

	default:
		return (-EOPNOTSUPP);

	}

	return (0);
}

int
wl_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	wl_info_t *wl;
	wl_if_t *wlif;
	void *buf = NULL;
	wl_ioctl_t ioc;
	int bcmerror;

	if (!dev)
		return -ENETDOWN;

	wl = WL_INFO(dev);
	wlif = WL_DEV_IF(dev);
	if (wlif == NULL || wl == NULL)
		return -ENETDOWN;

	bcmerror = 0;

	WL_TRACE(("wl%d: wl_ioctl: cmd 0x%x\n", wl->pub->unit, cmd));

#ifdef CONFIG_PREEMPT
	if (preempt_count())
		WL_ERROR(("wl%d: wl_ioctl: cmd = 0x%x, preempt_count=%d\n",
			wl->pub->unit, cmd, preempt_count()));
#endif

#ifdef CONFIG_WIRELESS_EXT

	if ((cmd >= SIOCIWFIRST) && (cmd <= SIOCIWLAST)) {

		return wl_iw_ioctl(dev, ifr, cmd);
	}
#endif 

	if (cmd == SIOCETHTOOL)
		return (wl_ethtool(wl, (void*)ifr->ifr_data, wlif));

	switch (cmd) {
		case SIOCDEVPRIVATE :
			break;
		default:
			bcmerror = BCME_UNSUPPORTED;
			goto done2;
	}

	if (copy_from_user(&ioc, ifr->ifr_data, sizeof(wl_ioctl_t))) {
		bcmerror = BCME_BADADDR;
		goto done2;
	}

	if (segment_eq(get_fs(), KERNEL_DS))
		buf = ioc.buf;

	else if (ioc.buf) {
		if (!(buf = (void *) MALLOC(wl->osh, MAX(ioc.len, WLC_IOCTL_MAXLEN)))) {
			bcmerror = BCME_NORESOURCE;
			goto done2;
		}

		if (copy_from_user(buf, ioc.buf, ioc.len)) {
			bcmerror = BCME_BADADDR;
			goto done1;
		}
	}

	WL_LOCK(wl);
	if (!capable(CAP_NET_ADMIN)) {
		bcmerror = BCME_EPERM;
	} else {
		bcmerror = wlc_ioctl(wl->wlc, ioc.cmd, buf, ioc.len, wlif->wlcif);
	}
	WL_UNLOCK(wl);

done1:
	if (ioc.buf && (ioc.buf != buf)) {
		if (copy_to_user(ioc.buf, buf, ioc.len))
			bcmerror = BCME_BADADDR;
		MFREE(wl->osh, buf, MAX(ioc.len, WLC_IOCTL_MAXLEN));
	}

done2:
	ASSERT(VALID_BCMERROR(bcmerror));
	if (bcmerror != 0)
		wl->pub->bcmerror = bcmerror;
	return (OSL_ERROR(bcmerror));
}

static struct net_device_stats*
wl_get_stats(struct net_device *dev)
{
	struct net_device_stats *stats_watchdog = NULL;
	struct net_device_stats *stats = NULL;
	wl_info_t *wl;

	if (!dev)
		return NULL;

	if ((wl = WL_INFO(dev)) == NULL)
		return NULL;

	if (!(wl->pub))
		return NULL;

	WL_TRACE(("wl%d: wl_get_stats\n", wl->pub->unit));

	if ((stats = &wl->stats) == NULL)
		return NULL;

	ASSERT(wl->stats_id < 2);
	stats_watchdog = &wl->stats_watchdog[wl->stats_id];

	memcpy(stats, stats_watchdog, sizeof(struct net_device_stats));
	return (stats);
}

#ifdef CONFIG_WIRELESS_EXT
struct iw_statistics *
wl_get_wireless_stats(struct net_device *dev)
{
	int res = 0;
	wl_info_t *wl;
	wl_if_t *wlif;
	struct iw_statistics *wstats = NULL;
	struct iw_statistics *wstats_watchdog = NULL;
	int phy_noise, rssi;

	if (!dev)
		return NULL;

	wl = WL_INFO(dev);
	wlif = WL_DEV_IF(dev);

	WL_TRACE(("wl%d: wl_get_wireless_stats\n", wl->pub->unit));

	wstats = &wl->wstats;
	ASSERT(wl->stats_id < 2);
	wstats_watchdog = &wl->wstats_watchdog[wl->stats_id];

	phy_noise = wl->phy_noise;
#if WIRELESS_EXT > 11
	wstats->discard.nwid = 0;
	wstats->discard.code = wstats_watchdog->discard.code;
	wstats->discard.fragment = wstats_watchdog->discard.fragment;
	wstats->discard.retries = wstats_watchdog->discard.retries;
	wstats->discard.misc = wstats_watchdog->discard.misc;

	wstats->miss.beacon = 0;
#endif 

	if (AP_ENAB(wl->pub))
		rssi = 0;
	else {
		scb_val_t scb;
		res = wlc_ioctl(wl->wlc, WLC_GET_RSSI, &scb, sizeof(scb), wlif->wlcif);
		if (res) {
			WL_ERROR(("wl%d: %s: WLC_GET_RSSI failed (%d)\n",
				wl->pub->unit, __FUNCTION__, res));
			return NULL;
		}
		rssi = scb.val;
	}

	if (rssi <= WLC_RSSI_NO_SIGNAL)
		wstats->qual.qual = 0;
	else if (rssi <= WLC_RSSI_VERY_LOW)
		wstats->qual.qual = 1;
	else if (rssi <= WLC_RSSI_LOW)
		wstats->qual.qual = 2;
	else if (rssi <= WLC_RSSI_GOOD)
		wstats->qual.qual = 3;
	else if (rssi <= WLC_RSSI_VERY_GOOD)
		wstats->qual.qual = 4;
	else
		wstats->qual.qual = 5;

	wstats->qual.level = 0x100 + rssi;
	wstats->qual.noise = 0x100 + phy_noise;
#if WIRELESS_EXT > 18
	wstats->qual.updated |= (IW_QUAL_ALL_UPDATED | IW_QUAL_DBM);
#else
	wstats->qual.updated |= 7;
#endif 

	return wstats;
}
#endif 

static int
wl_set_mac_address(struct net_device *dev, void *addr)
{
	int err = 0;
	wl_info_t *wl;
	struct sockaddr *sa = (struct sockaddr *) addr;

	if (!dev)
		return -ENETDOWN;

	wl = WL_INFO(dev);

	WL_TRACE(("wl%d: wl_set_mac_address\n", wl->pub->unit));

	WL_LOCK(wl);

	bcopy(sa->sa_data, dev->dev_addr, ETHER_ADDR_LEN);
	err = wlc_iovar_op(wl->wlc, "cur_etheraddr", NULL, 0, sa->sa_data, ETHER_ADDR_LEN,
		IOV_SET, (WL_DEV_IF(dev))->wlcif);
	WL_UNLOCK(wl);
	if (err)
		WL_ERROR(("wl%d: wl_set_mac_address: error setting MAC addr override\n",
			wl->pub->unit));
	return err;
}

static void
wl_set_multicast_list(struct net_device *dev)
{
	_wl_set_multicast_list(dev);
}

static void
_wl_set_multicast_list(struct net_device *dev)
{
	wl_info_t *wl;
	struct dev_mc_list *mclist;
	int i;

	if (!dev)
		return;
	wl = WL_INFO(dev);

	WL_TRACE(("wl%d: wl_set_multicast_list\n", wl->pub->unit));

	WL_LOCK(wl);

	if (wl->pub->up) {
		wl->pub->allmulti = (dev->flags & IFF_ALLMULTI)? TRUE: FALSE;

		for (i = 0, mclist = dev->mc_list; mclist && (i < dev->mc_count);
			i++, mclist = mclist->next) {
			if (i >= MAXMULTILIST) {
				wl->pub->allmulti = TRUE;
				i = 0;
				break;
			}
			wl->pub->multicast[i] = *((struct ether_addr*) mclist->dmi_addr);
		}
		wl->pub->nmulticast = i;
		wlc_set(wl->wlc, WLC_SET_PROMISC, (dev->flags & IFF_PROMISC));
	}

	WL_UNLOCK(wl);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
irqreturn_t BCMFASTPATH
wl_isr(int irq, void *dev_id)
#else
irqreturn_t BCMFASTPATH
wl_isr(int irq, void *dev_id, struct pt_regs *ptregs)
#endif 
{
	wl_info_t *wl;
	bool ours, wantdpc;
	unsigned long flags;

	wl = (wl_info_t*) dev_id;

	WL_ISRLOCK(wl, flags);

	if ((ours = wlc_isr(wl->wlc, &wantdpc))) {

		if (wantdpc) {

			ASSERT(wl->resched == FALSE);
			tasklet_schedule(&wl->tasklet);
		}
	}

	WL_ISRUNLOCK(wl, flags);

	return IRQ_RETVAL(ours);
}

static void BCMFASTPATH
wl_dpc(ulong data)
{
	wl_info_t *wl;

	wl = (wl_info_t*) data;

	WL_LOCK(wl);

	if (wl->pub->up) {
		if (wl->resched) {
			unsigned long flags;

			INT_LOCK(wl, flags);
			wlc_intrsupd(wl->wlc);
			INT_UNLOCK(wl, flags);
		}

		wl->resched = wlc_dpc(wl->wlc, TRUE);
	}

	if (!wl->pub->up)
		goto done;

	if (wl->resched)
		tasklet_schedule(&wl->tasklet);
	else {

		wl_intrson(wl);
	}

done:
	WL_UNLOCK(wl);
}

void BCMFASTPATH
wl_sendup(wl_info_t *wl, wl_if_t *wlif, void *p, int numpkt)
{
	struct sk_buff *skb;

	WL_TRACE(("wl%d: wl_sendup: %d bytes\n", wl->pub->unit, PKTLEN(wl->osh, p)));

	if (wlif) {

		if (!netif_device_present(wlif->dev)) {
			WL_ERROR(("wl%d: wl_sendup: interface not ready\n", wl->pub->unit));
			PKTFREE(wl->osh, p, FALSE);
			return;
		}

		skb = PKTTONATIVE(wl->osh, p);
		skb->dev = wlif->dev;
	} else {

		skb = PKTTONATIVE(wl->osh, p);
		skb->dev = wl->dev;
	}

	skb->protocol = eth_type_trans(skb, skb->dev);
	if (!ISALIGNED((uintptr)skb->data, 4)) {
		WL_ERROR(("Unaligned assert. skb %p. skb->data %p.\n", skb, skb->data));
		if (wlif) {
			WL_ERROR(("wl_sendup: dev name is %s (wlif) \n", wlif->dev->name));
			WL_ERROR(("wl_sendup: hard header len  %d (wlif) \n",
				wlif->dev->hard_header_len));
		}
		WL_ERROR(("wl_sendup: dev name is %s (wl) \n", wl->dev->name));
		WL_ERROR(("wl_sendup: hard header len %d (wl) \n", wl->dev->hard_header_len));
		ASSERT(ISALIGNED((uintptr)skb->data, 4));
	}

	WL_APSTA_RX(("wl%d: wl_sendup(): pkt %p summed %d on interface %p (%s)\n",
		wl->pub->unit, p, skb->ip_summed, wlif, skb->dev->name));

	netif_rx(skb);
}

void
wl_dump_ver(wl_info_t *wl, struct bcmstrbuf *b)
{
	bcm_bprintf(b, "wl%d: %s %s version %s\n", wl->pub->unit,
		__DATE__, __TIME__, EPI_VERSION_STR);
}

#if defined(BCMDBG)
static int
wl_dump(wl_info_t *wl, struct bcmstrbuf *b)
{
	wl_if_t *p;
	int i;

	wl_dump_ver(wl, b);

	bcm_bprintf(b, "name %s dev %p tbusy %d callbacks %d malloced %d\n",
	       wl->dev->name, wl->dev, (uint)netif_queue_stopped(wl->dev),
	       atomic_read(&wl->callbacks), MALLOCED(wl->osh));

	p = wl->if_list;
	if (p)
		p = p->next;
	for (i = 0; p != NULL; p = p->next, i++) {
		if ((i % 4) == 0) {
			if (i != 0)
				bcm_bprintf(b, "\n");
			bcm_bprintf(b, "Interfaces:");
		}
		bcm_bprintf(b, " name %s dev %p", p->dev->name, p->dev);
	}
	if (i)
		bcm_bprintf(b, "\n");

	return 0;
}
#endif	

static void
wl_link_up(wl_info_t *wl)
{
	WL_ERROR(("wl%d: %s: link up\n", wl->pub->unit, __FUNCTION__));
}

static void
wl_link_down(wl_info_t *wl)
{
	WL_ERROR(("wl%d: %s: link down\n", wl->pub->unit, __FUNCTION__));
}

void
wl_event(wl_info_t *wl, char *ifname, wlc_event_t *e)
{
#ifdef CONFIG_WIRELESS_EXT
	wl_iw_event(wl->dev, &(e->event), e->data);
#endif 

	switch (e->event.event_type) {
	case WLC_E_LINK:
	case WLC_E_NDIS_LINK:
		if (e->event.flags&WLC_EVENT_MSG_LINK)
			wl_link_up(wl);
		else
			wl_link_down(wl);
		break;
	case WLC_E_MIC_ERROR:
		wl_mic_error(wl, e->addr,
			e->event.flags&WLC_EVENT_MSG_GROUP,
			e->event.flags&WLC_EVENT_MSG_FLUSHTXQ);
		break;
	}
}

static void
wl_timer(ulong data)
{
	_wl_timer((wl_timer_t*)data);
}

static void
_wl_timer(wl_timer_t *t)
{
	WL_LOCK(t->wl);

	if (t->set) {
		if (t->periodic) {
			t->timer.expires = jiffies + t->ms*HZ/1000;
			atomic_inc(&t->wl->callbacks);
			add_timer(&t->timer);
			t->set = TRUE;
		} else
			t->set = FALSE;

		t->fn(t->arg);
	}

	atomic_dec(&t->wl->callbacks);

	WL_UNLOCK(t->wl);
}

wl_timer_t *
wl_init_timer(wl_info_t *wl, void (*fn)(void *arg), void *arg, const char *name)
{
	wl_timer_t *t;

	if (!(t = MALLOC(wl->osh, sizeof(wl_timer_t)))) {
		WL_ERROR(("wl%d: wl_init_timer: out of memory, malloced %d bytes\n", wl->pub->unit,
			MALLOCED(wl->osh)));
		return 0;
	}

	bzero(t, sizeof(wl_timer_t));

	init_timer(&t->timer);
	t->timer.data = (ulong) t;
	t->timer.function = wl_timer;
	t->wl = wl;
	t->fn = fn;
	t->arg = arg;
	t->next = wl->timers;
	wl->timers = t;

#ifdef BCMDBG
	if ((t->name = MALLOC(wl->osh, strlen(name) + 1)))
		strcpy(t->name, name);
#endif

	return t;
}

void
wl_add_timer(wl_info_t *wl, wl_timer_t *t, uint ms, int periodic)
{
	ASSERT(!t->set);

	t->ms = ms;
	t->periodic = (bool) periodic;
	t->set = TRUE;
	t->timer.expires = jiffies + ms*HZ/1000;

	atomic_inc(&wl->callbacks);
	add_timer(&t->timer);
}

bool
wl_del_timer(wl_info_t *wl, wl_timer_t *t)
{
	if (t->set) {
		t->set = FALSE;
		if (!del_timer(&t->timer)) {
#ifdef BCMDBG
			WL_INFORM(("wl%d: Failed to delete timer %s\n", wl->pub->unit, t->name));
#endif
			return FALSE;
		}
		atomic_dec(&wl->callbacks);
	}

	return TRUE;
}

void
wl_free_timer(wl_info_t *wl, wl_timer_t *t)
{
	wl_timer_t *tmp;

	wl_del_timer(wl, t);

	if (wl->timers == t) {
		wl->timers = wl->timers->next;
#ifdef BCMDBG
		if (t->name)
			MFREE(wl->osh, t->name, strlen(t->name) + 1);
#endif
		MFREE(wl->osh, t, sizeof(wl_timer_t));
		return;

	}

	tmp = wl->timers;
	while (tmp) {
		if (tmp->next == t) {
			tmp->next = t->next;
#ifdef BCMDBG
			if (t->name)
				MFREE(wl->osh, t->name, strlen(t->name) + 1);
#endif
			MFREE(wl->osh, t, sizeof(wl_timer_t));
			return;
		}
		tmp = tmp->next;
	}

}

static void
wl_mic_error(wl_info_t *wl, struct ether_addr *ea, bool group, bool flush_txq)
{
	WL_WSEC(("wl%d: mic error using %s key\n", wl->pub->unit,
		(group) ? "group" : "pairwise"));

}

void
wl_monitor(wl_info_t *wl, wl_rxsts_t *rxsts, void *p)
{
}

void
wl_set_monitor(wl_info_t *wl, int val)
{
}

#if LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 15)
const char *
print_tainted()
{
	return "";
}
#endif	

struct net_device *
wl_netdev_get(wl_info_t *wl)
{
	return wl->dev;
}

int
wl_set_pktlen(osl_t *osh, void *p, int len)
{
	PKTSETLEN(osh, p, len);
	return len;
}

void *
wl_get_pktbuffer(osl_t *osh, int len)
{
	return (PKTGET(osh, len, FALSE));
}

uint
wl_buf_to_pktcopy(osl_t *osh, void *p, uchar *buf, int len, uint offset)
{
	if (PKTLEN(osh, p) < len + offset)
		return 0;
	bcopy(buf, (char *)PKTDATA(osh, p) + offset, len);
	return len;
}

int
wl_tkip_miccheck(wl_info_t *wl, void *p, int hdr_len, bool group_key, int key_index)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
	struct sk_buff *skb = (struct sk_buff *)p;
	skb->dev = wl->dev;

	if (wl->tkipmodops) {
		if (group_key && wl->tkip_bcast_data)
			return (wl->tkipmodops->decrypt_msdu(skb, key_index, hdr_len,
				wl->tkip_bcast_data));
		else if (!group_key && wl->tkip_ucast_data)
			return (wl->tkipmodops->decrypt_msdu(skb, key_index, hdr_len,
				wl->tkip_ucast_data));
	}
#endif 
	WL_ERROR(("%s: No tkip mod ops\n", __FUNCTION__));
	return -1;

}

int
wl_tkip_micadd(wl_info_t *wl, void *p, int hdr_len)
{
	int error = -1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
	struct sk_buff *skb = (struct sk_buff *)p;
	skb->dev = wl->dev;

	if (wl->tkipmodops) {
		if (wl->tkip_ucast_data)
			error = wl->tkipmodops->encrypt_msdu(skb, hdr_len, wl->tkip_ucast_data);
		if (error)
			WL_ERROR(("Error encrypting MSDU %d\n", error));
	}
	else
#endif 
		WL_ERROR(("%s: No tkip mod ops\n", __FUNCTION__));
	return error;
}

int
wl_tkip_encrypt(wl_info_t *wl, void *p, int hdr_len)
{
	int error = -1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
	struct sk_buff *skb = (struct sk_buff *)p;
	skb->dev = wl->dev;

	if (wl->tkipmodops) {
		if (wl->tkip_ucast_data)
			error = wl->tkipmodops->encrypt_mpdu(skb, hdr_len, wl->tkip_ucast_data);
		if (error) {
			WL_ERROR(("Error encrypting MPDU %d\n", error));
		}
	}
	else
#endif 
		WL_ERROR(("%s: No tkip mod ops\n", __FUNCTION__));
	return error;

}

int
wl_tkip_decrypt(wl_info_t *wl, void *p, int hdr_len, bool group_key)
{
	int err = -1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
	struct sk_buff *skb = (struct sk_buff *)p;
	skb->dev = wl->dev;

	if (wl->tkipmodops) {
		if (group_key && wl->tkip_bcast_data)
			err = wl->tkipmodops->decrypt_mpdu(skb, hdr_len, wl->tkip_bcast_data);
		else if (!group_key && wl->tkip_ucast_data)
			err = wl->tkipmodops->decrypt_mpdu(skb, hdr_len, wl->tkip_ucast_data);
	}
	else
		WL_ERROR(("%s: No tkip mod ops\n", __FUNCTION__));

#endif 

	return err;
}

int
wl_tkip_keyset(wl_info_t *wl, wsec_key_t *key)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
	bool group_key = FALSE;
	uchar	rxseq[IW_ENCODE_SEQ_MAX_SIZE];

	if (key->len != 0) {
		WL_WSEC(("%s: Key Length is Not zero\n", __FUNCTION__));
		if (key->algo != CRYPTO_ALGO_TKIP) {
			WL_WSEC(("%s: Algo is Not TKIP %d\n", __FUNCTION__, key->algo));
			return 0;
		}
		WL_WSEC(("%s: Trying to set a key in TKIP Mod\n", __FUNCTION__));
	}
	else
		WL_WSEC(("%s: Trying to Remove a Key from TKIP Mod\n", __FUNCTION__));

	if (ETHER_ISNULLADDR(&key->ea) || ETHER_ISBCAST(&key->ea)) {
		group_key = TRUE;
		WL_WSEC(("Group Key index %d\n", key->id));
	}
	else
		WL_WSEC(("Unicast Key index %d\n", key->id));

	if (wl->tkipmodops) {
		uint8 keybuf[8];
		if (group_key) {
			if (key->len) {
				if (!wl->tkip_bcast_data) {
					WL_WSEC(("Init TKIP Bcast Module\n"));
					WL_UNLOCK(wl);
					wl->tkip_bcast_data = wl->tkipmodops->init(key->id);
					WL_LOCK(wl);
				}
				if (wl->tkip_bcast_data) {
					WL_WSEC(("TKIP SET BROADCAST KEY******************\n"));
					bzero(rxseq, IW_ENCODE_SEQ_MAX_SIZE);
					bcopy(&key->rxiv, rxseq, 6);
					bcopy(&key->data[24], keybuf, sizeof(keybuf));
					bcopy(&key->data[16], &key->data[24], sizeof(keybuf));
					bcopy(keybuf, &key->data[16], sizeof(keybuf));
					wl->tkipmodops->set_key(&key->data, key->len,
						(uint8 *)&key->rxiv, wl->tkip_bcast_data);
				}
			}
			else {
				if (wl->tkip_bcast_data) {
					WL_WSEC(("Deinit TKIP Bcast Module\n"));
					wl->tkipmodops->deinit(wl->tkip_bcast_data);
					wl->tkip_bcast_data = NULL;
				}
			}
		}
		else {
			if (key->len) {
				if (!wl->tkip_ucast_data) {
					WL_WSEC(("Init TKIP Ucast Module\n"));
					WL_UNLOCK(wl);
					wl->tkip_ucast_data = wl->tkipmodops->init(key->id);
					WL_LOCK(wl);
				}
				if (wl->tkip_ucast_data) {
					WL_WSEC(("TKIP SET UNICAST KEY******************\n"));
					bzero(rxseq, IW_ENCODE_SEQ_MAX_SIZE);
					bcopy(&key->rxiv, rxseq, 6);
					bcopy(&key->data[24], keybuf, sizeof(keybuf));
					bcopy(&key->data[16], &key->data[24], sizeof(keybuf));
					bcopy(keybuf, &key->data[16], sizeof(keybuf));
					wl->tkipmodops->set_key(&key->data, key->len,
						(uint8 *)&key->rxiv, wl->tkip_ucast_data);
				}
			}
			else {
				if (wl->tkip_ucast_data) {
					WL_WSEC(("Deinit TKIP Ucast Module\n"));
					wl->tkipmodops->deinit(wl->tkip_ucast_data);
					wl->tkip_ucast_data = NULL;
				}
			}
		}
	}
	else
#endif 
		WL_WSEC(("%s: No tkip mod ops\n", __FUNCTION__));
	return 0;
}

void
wl_tkip_printstats(wl_info_t *wl, bool group_key)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
	char debug_buf[512];
	if (wl->tkipmodops) {
		if (group_key && wl->tkip_bcast_data)
			wl->tkipmodops->print_stats(debug_buf, wl->tkip_bcast_data);
		else if (!group_key && wl->tkip_ucast_data)
			wl->tkipmodops->print_stats(debug_buf, wl->tkip_ucast_data);
		else
			return;
		printk("%s: TKIP stats from module: %s\n", debug_buf, group_key?"Bcast":"Ucast");
	}
#endif 
}

static int
wl_linux_watchdog(void *ctx)
{
	wl_info_t *wl = (wl_info_t *) ctx;
	struct net_device_stats *stats = NULL;
	uint id;
#ifdef CONFIG_WIRELESS_EXT
	struct iw_statistics *wstats = NULL;
	int phy_noise;
#endif

	if (wl->pub->up) {
		ASSERT(wl->stats_id < 2);

		id = 1 - wl->stats_id;

		stats = &wl->stats_watchdog[id];
#ifdef CONFIG_WIRELESS_EXT
		wstats = &wl->wstats_watchdog[id];
#endif
		stats->rx_packets = WLCNTVAL(wl->pub->_cnt.rxframe);
		stats->tx_packets = WLCNTVAL(wl->pub->_cnt.txframe);
		stats->rx_bytes = WLCNTVAL(wl->pub->_cnt.rxbyte);
		stats->tx_bytes = WLCNTVAL(wl->pub->_cnt.txbyte);
		stats->rx_errors = WLCNTVAL(wl->pub->_cnt.rxerror);
		stats->tx_errors = WLCNTVAL(wl->pub->_cnt.txerror);
		stats->collisions = 0;

		stats->rx_length_errors = 0;
		stats->rx_over_errors = WLCNTVAL(wl->pub->_cnt.rxoflo);
		stats->rx_crc_errors = WLCNTVAL(wl->pub->_cnt.rxcrc);
		stats->rx_frame_errors = 0;
		stats->rx_fifo_errors = WLCNTVAL(wl->pub->_cnt.rxoflo);
		stats->rx_missed_errors = 0;

		stats->tx_fifo_errors = WLCNTVAL(wl->pub->_cnt.txuflo);
#ifdef CONFIG_WIRELESS_EXT
#if WIRELESS_EXT > 11
		wstats->discard.nwid = 0;
		wstats->discard.code = WLCNTVAL(wl->pub->_cnt.rxundec);
		wstats->discard.fragment = WLCNTVAL(wl->pub->_cnt.rxfragerr);
		wstats->discard.retries = WLCNTVAL(wl->pub->_cnt.txfail);
		wstats->discard.misc = WLCNTVAL(wl->pub->_cnt.rxrunt) +
			WLCNTVAL(wl->pub->_cnt.rxgiant);

		wstats->miss.beacon = 0;
#endif 
#endif 

		wl->stats_id = id;

#ifdef CONFIG_WIRELESS_EXT
		if (!wlc_get(wl->wlc, WLC_GET_PHY_NOISE, &phy_noise))
			wl->phy_noise = phy_noise;
#endif 
	}

	return 0;
}
