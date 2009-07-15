/*
 * Linux OS Independent Layer
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
 * $Id: linux_osl.c,v 1.127.2.18 2009/02/26 17:07:07 Exp $
 */

#define LINUX_PORT

#include <typedefs.h>
#include <bcmendian.h>
#include <linuxver.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <linux/delay.h>
#include <pcicfg.h>

#define PCI_CFG_RETRY 		10

#define OS_HANDLE_MAGIC		0x1234abcd	
#define BCM_MEM_FILENAME_LEN 	24		

typedef struct bcm_mem_link {
	struct bcm_mem_link *prev;
	struct bcm_mem_link *next;
	uint	size;
	int	line;
	char	file[BCM_MEM_FILENAME_LEN];
} bcm_mem_link_t;

struct osl_info {
	osl_pubinfo_t pub;
	uint magic;
	void *pdev;
	uint malloced;
	uint failed;
	uint bustype;
	bcm_mem_link_t *dbgmem_list;
};

bool g_assert_bypass = FALSE;

static int16 linuxbcmerrormap[] =  \
{	0, 			
	-EINVAL,		
	-EINVAL,		
	-EINVAL,		
	-EINVAL,		
	-EINVAL,		
	-EINVAL,		
	-EINVAL,		
	-EINVAL,		
	-EINVAL,		
	-EINVAL,		
	-EINVAL, 		
	-EINVAL, 		
	-EINVAL, 		
	-E2BIG,			
	-E2BIG,			
	-EBUSY, 		
	-EINVAL, 		
	-EINVAL, 		
	-EINVAL, 		
	-EINVAL, 		
	-EFAULT, 		
	-ENOMEM, 		
	-EOPNOTSUPP,		
	-EMSGSIZE,		
	-EINVAL,		
	-EPERM,			
	-ENOMEM, 		
	-EINVAL, 		
	-ERANGE, 		
	-EINVAL, 		
	-EINVAL, 		
	-EINVAL, 		
	-EINVAL, 		
	-EINVAL,		
	-EIO,			
	-ENODEV,		
	-EINVAL,		
	-EIO,			
	-EIO,			
	-EINVAL			

#if BCME_LAST != -40
#error "You need to add a OS error translation in the linuxbcmerrormap \
	for new error code defined in bcmuitls.h"
#endif 
};

int
osl_error(int bcmerror)
{
	if (bcmerror > 0)
		bcmerror = 0;
	else if (bcmerror < BCME_LAST)
		bcmerror = BCME_ERROR;

	return linuxbcmerrormap[-bcmerror];
}

osl_t *
osl_attach(void *pdev, uint bustype, bool pkttag)
{
	osl_t *osh;

	osh = kmalloc(sizeof(osl_t), GFP_ATOMIC);
	ASSERT(osh);

	bzero(osh, sizeof(osl_t));

	ASSERT(ABS(BCME_LAST) == (ARRAYSIZE(linuxbcmerrormap) - 1));

	osh->magic = OS_HANDLE_MAGIC;
	osh->malloced = 0;
	osh->failed = 0;
	osh->dbgmem_list = NULL;
	osh->pdev = pdev;
	osh->pub.pkttag = pkttag;
	osh->bustype = bustype;

	switch (bustype) {
		case PCI_BUS:
		case SI_BUS:
		case PCMCIA_BUS:
			osh->pub.mmbus = TRUE;
			break;
		case JTAG_BUS:
		case SDIO_BUS:
		case USB_BUS:
		case SPI_BUS:
		case RPC_BUS:
			osh->pub.mmbus = FALSE;
			break;
		default:
			ASSERT(FALSE);
			break;
	}

#ifdef BCMDBG
	if (pkttag) {
		struct sk_buff *skb;
		ASSERT(OSL_PKTTAG_SZ <= sizeof(skb->cb));
	}
#endif
	return osh;
}

void
osl_detach(osl_t *osh)
{
	if (osh == NULL)
		return;

	ASSERT(osh->magic == OS_HANDLE_MAGIC);
	kfree(osh);
}

void * BCMFASTPATH
osl_pktget(osl_t *osh, uint len)
{
	struct sk_buff *skb;

	if ((skb = dev_alloc_skb(len))) {
		skb_put(skb, len);
		skb->priority = 0;

		osh->pub.pktalloced++;
	}

	return ((void*) skb);
}

void BCMFASTPATH
osl_pktfree(osl_t *osh, void *p, bool send)
{
	struct sk_buff *skb, *nskb;

	skb = (struct sk_buff*) p;

	if (send && osh->pub.tx_fn)
		osh->pub.tx_fn(osh->pub.tx_ctx, p, 0);

	while (skb) {
		nskb = skb->next;
		skb->next = NULL;

		if (skb->destructor) {

			dev_kfree_skb_any(skb);
		} else {

			dev_kfree_skb(skb);
		}

		osh->pub.pktalloced--;

		skb = nskb;
	}
}

uint32
osl_pci_read_config(osl_t *osh, uint offset, uint size)
{
	uint val = 0;
	uint retry = PCI_CFG_RETRY;

	ASSERT((osh && (osh->magic == OS_HANDLE_MAGIC)));

	ASSERT(size == 4);

	do {
		pci_read_config_dword(osh->pdev, offset, &val);
		if (val != 0xffffffff)
			break;
	} while (retry--);

#ifdef BCMDBG
	if (retry < PCI_CFG_RETRY)
		printk("PCI CONFIG READ access to %d required %d retries\n", offset,
		       (PCI_CFG_RETRY - retry));
#endif 

	return (val);
}

void
osl_pci_write_config(osl_t *osh, uint offset, uint size, uint val)
{
	uint retry = PCI_CFG_RETRY;

	ASSERT((osh && (osh->magic == OS_HANDLE_MAGIC)));

	ASSERT(size == 4);

	do {
		pci_write_config_dword(osh->pdev, offset, val);
		if (offset != PCI_BAR0_WIN)
			break;
		if (osl_pci_read_config(osh, offset, size) == val)
			break;
	} while (retry--);

#ifdef BCMDBG
	if (retry < PCI_CFG_RETRY)
		printk("PCI CONFIG WRITE access to %d required %d retries\n", offset,
		       (PCI_CFG_RETRY - retry));
#endif 
}

uint
osl_pci_bus(osl_t *osh)
{
	ASSERT(osh && (osh->magic == OS_HANDLE_MAGIC) && osh->pdev);

	return ((struct pci_dev *)osh->pdev)->bus->number;
}

uint
osl_pci_slot(osl_t *osh)
{
	ASSERT(osh && (osh->magic == OS_HANDLE_MAGIC) && osh->pdev);

	return PCI_SLOT(((struct pci_dev *)osh->pdev)->devfn);
}

static void
osl_pcmcia_attr(osl_t *osh, uint offset, char *buf, int size, bool write)
{
}

void
osl_pcmcia_read_attr(osl_t *osh, uint offset, void *buf, int size)
{
	osl_pcmcia_attr(osh, offset, (char *) buf, size, FALSE);
}

void
osl_pcmcia_write_attr(osl_t *osh, uint offset, void *buf, int size)
{
	osl_pcmcia_attr(osh, offset, (char *) buf, size, TRUE);
}

void * BCMFASTPATH
osl_malloc(osl_t *osh, uint size)
{
	void *addr;

	if (osh)
		ASSERT(osh->magic == OS_HANDLE_MAGIC);

	if ((addr = kmalloc(size, GFP_ATOMIC)) == NULL) {
		if (osh)
			osh->failed++;
		return (NULL);
	}
	if (osh)
		osh->malloced += size;

	return (addr);
}

void BCMFASTPATH
osl_mfree(osl_t *osh, void *addr, uint size)
{
	if (osh) {
		ASSERT(osh->magic == OS_HANDLE_MAGIC);
		osh->malloced -= size;
	}
	kfree(addr);
}

uint
osl_malloced(osl_t *osh)
{
	ASSERT((osh && (osh->magic == OS_HANDLE_MAGIC)));
	return (osh->malloced);
}

uint
osl_malloc_failed(osl_t *osh)
{
	ASSERT((osh && (osh->magic == OS_HANDLE_MAGIC)));
	return (osh->failed);
}

uint
osl_dma_consistent_align(void)
{
	return (PAGE_SIZE);
}

void*
osl_dma_alloc_consistent(osl_t *osh, uint size, ulong *pap)
{
	ASSERT((osh && (osh->magic == OS_HANDLE_MAGIC)));

	return (pci_alloc_consistent(osh->pdev, size, (dma_addr_t*)pap));
}

void
osl_dma_free_consistent(osl_t *osh, void *va, uint size, ulong pa)
{
	ASSERT((osh && (osh->magic == OS_HANDLE_MAGIC)));

	pci_free_consistent(osh->pdev, size, va, (dma_addr_t)pa);
}

uint BCMFASTPATH
osl_dma_map(osl_t *osh, void *va, uint size, int direction)
{
	ASSERT((osh && (osh->magic == OS_HANDLE_MAGIC)));

	if (direction == DMA_TX)
		return (pci_map_single(osh->pdev, va, size, PCI_DMA_TODEVICE));
	else {
		return (pci_map_single(osh->pdev, va, size, PCI_DMA_FROMDEVICE));
	}
}

void BCMFASTPATH
osl_dma_unmap(osl_t *osh, uint pa, uint size, int direction)
{
	int dir;

	ASSERT((osh && (osh->magic == OS_HANDLE_MAGIC)));
	dir = (direction == DMA_TX)? PCI_DMA_TODEVICE: PCI_DMA_FROMDEVICE;
	pci_unmap_single(osh->pdev, (uint32)pa, size, dir);
}

#if defined(BCMDBG_ASSERT)
void
osl_assert(char *exp, char *file, int line)
{
	char tempbuf[256];
#ifdef BCMDBG_ASSERT
	snprintf(tempbuf, 256, "assertion \"%s\" failed: file \"%s\", line %d\n", exp, file, line);

	if (!in_interrupt()) {
		const int delay = 3;
		printk("%s", tempbuf);
		printk("panic in %d seconds\n", delay);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(delay * HZ);
	}
	if (!g_assert_bypass)
		panic("%s", tempbuf);
#endif

}
#endif 

void
osl_delay(uint usec)
{
	uint d;

	while (usec > 0) {
		d = MIN(usec, 1000);
		udelay(d);
		usec -= d;
	}
}

void *
osl_pktdup(osl_t *osh, void *skb)
{
	void * p;

	if ((p = skb_clone((struct sk_buff*)skb, GFP_ATOMIC)) == NULL)
		return NULL;

	if (osh->pub.pkttag)
		bzero((void*)((struct sk_buff *)p)->cb, OSL_PKTTAG_SZ);

	osh->pub.pktalloced++;
	return (p);
}

uint
osl_pktalloced(osl_t *osh)
{
	return (osh->pub.pktalloced);
}

int
osl_printf(const char *format, ...)
{
	va_list args;
	char buf[1024];
	int len;

	va_start(args, format);
	len = vsnprintf(buf, 1024, format, args);
	va_end(args);

	if (len > sizeof(buf)) {
		printk("osl_printf: buffer overrun\n");
		return (0);
	}

	return (printk("%s", buf));
}

int
osl_sprintf(char *buf, const char *format, ...)
{
	va_list args;
	int rc;

	va_start(args, format);
	rc = vsprintf(buf, format, args);
	va_end(args);
	return (rc);
}

int
osl_snprintf(char *buf, size_t n, const char *format, ...)
{
	va_list args;
	int rc;

	va_start(args, format);
	rc = vsnprintf(buf, n, format, args);
	va_end(args);
	return (rc);
}

int
osl_vsprintf(char *buf, const char *format, va_list ap)
{
	return (vsprintf(buf, format, ap));
}

int
osl_vsnprintf(char *buf, size_t n, const char *format, va_list ap)
{
	return (vsnprintf(buf, n, format, ap));
}

int
osl_strcmp(const char *s1, const char *s2)
{
	return (strcmp(s1, s2));
}

int
osl_strncmp(const char *s1, const char *s2, uint n)
{
	return (strncmp(s1, s2, n));
}

int
osl_strlen(const char *s)
{
	return (strlen(s));
}

char*
osl_strcpy(char *d, const char *s)
{
	return (strcpy(d, s));
}

char*
osl_strncpy(char *d, const char *s, uint n)
{
	return (strncpy(d, s, n));
}

char*
osl_strchr(const char *s, int c)
{
	return (strchr(s, c));
}

char*
osl_strrchr(const char *s, int c)
{
	return (strrchr(s, c));
}

void*
osl_memset(void *d, int c, size_t n)
{
	return memset(d, c, n);
}

void*
osl_memcpy(void *d, const void *s, size_t n)
{
	return memcpy(d, s, n);
}

void*
osl_memmove(void *d, const void *s, size_t n)
{
	return memmove(d, s, n);
}

int
osl_memcmp(const void *s1, const void *s2, size_t n)
{
	return memcmp(s1, s2, n);
}

uint32
osl_readl(volatile uint32 *r)
{
	return (readl(r));
}

uint16
osl_readw(volatile uint16 *r)
{
	return (readw(r));
}

uint8
osl_readb(volatile uint8 *r)
{
	return (readb(r));
}

void
osl_writel(uint32 v, volatile uint32 *r)
{
	writel(v, r);
}

void
osl_writew(uint16 v, volatile uint16 *r)
{
	writew(v, r);
}

void
osl_writeb(uint8 v, volatile uint8 *r)
{
	writeb(v, r);
}

void *
osl_uncached(void *va)
{
	return ((void*)va);
}

uint
osl_getcycles(void)
{
	uint cycles;

#if defined(__i386__)
	rdtscl(cycles);
#else
	cycles = 0;
#endif 
	return cycles;
}

void *
osl_reg_map(uint32 pa, uint size)
{
	return (ioremap_nocache((unsigned long)pa, (unsigned long)size));
}

void
osl_reg_unmap(void *va)
{
	iounmap(va);
}

int
osl_busprobe(uint32 *val, uint32 addr)
{
	*val = readl((uint32 *)(uintptr)addr);
	return 0;
}

bool
osl_pktshared(void *skb)
{
	return (((struct sk_buff*)skb)->cloned);
}

uchar*
osl_pktdata(osl_t *osh, void *skb)
{
	return (((struct sk_buff*)skb)->data);
}

uint
osl_pktlen(osl_t *osh, void *skb)
{
	return (((struct sk_buff*)skb)->len);
}

uint
osl_pktheadroom(osl_t *osh, void *skb)
{
	return (uint) skb_headroom((struct sk_buff *) skb);
}

uint
osl_pkttailroom(osl_t *osh, void *skb)
{
	return (uint) skb_tailroom((struct sk_buff *) skb);
}

void*
osl_pktnext(osl_t *osh, void *skb)
{
	return (((struct sk_buff*)skb)->next);
}

void
osl_pktsetnext(void *skb, void *x)
{
	((struct sk_buff*)skb)->next = (struct sk_buff*)x;
}

void
osl_pktsetlen(osl_t *osh, void *skb, uint len)
{
	__skb_trim((struct sk_buff*)skb, len);
}

uchar*
osl_pktpush(osl_t *osh, void *skb, int bytes)
{
	return (skb_push((struct sk_buff*)skb, bytes));
}

uchar*
osl_pktpull(osl_t *osh, void *skb, int bytes)
{
	return (skb_pull((struct sk_buff*)skb, bytes));
}

void*
osl_pkttag(void *skb)
{
	return ((void*)(((struct sk_buff*)skb)->cb));
}

void*
osl_pktlink(void *skb)
{
	return (((struct sk_buff*)skb)->prev);
}

void
osl_pktsetlink(void *skb, void *x)
{
	((struct sk_buff*)skb)->prev = (struct sk_buff*)x;
}

uint
osl_pktprio(void *skb)
{
	return (((struct sk_buff*)skb)->priority);
}

void
osl_pktsetprio(void *skb, uint x)
{
	((struct sk_buff*)skb)->priority = x;
}

struct sk_buff *
osl_pkt_tonative(osl_t *osh, void *pkt)
{
	struct sk_buff *nskb;

	if (osh->pub.pkttag)
		bzero((void*)((struct sk_buff *)pkt)->cb, OSL_PKTTAG_SZ);

	for (nskb = (struct sk_buff *)pkt; nskb; nskb = nskb->next) {
		osh->pub.pktalloced--;
	}
	return (struct sk_buff *)pkt;
}

void *
osl_pkt_frmnative(osl_t *osh, void *pkt)
{
	struct sk_buff *nskb;

	if (osh->pub.pkttag)
		bzero((void*)((struct sk_buff *)pkt)->cb, OSL_PKTTAG_SZ);

	for (nskb = (struct sk_buff *)pkt; nskb; nskb = nskb->next) {
		osh->pub.pktalloced++;
	}
	return (void *)pkt;
}
