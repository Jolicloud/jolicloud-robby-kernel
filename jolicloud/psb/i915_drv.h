/* i915_drv.h -- Private header for the I915 driver -*- linux-c -*-
 */
/*
 *
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _I915_DRV_H_
#define _I915_DRV_H_

#include "i915_reg.h"

/* General customization:
 */

#define DRIVER_AUTHOR		"Tungsten Graphics, Inc."

#define DRIVER_NAME		"i915"
#define DRIVER_DESC		"Intel Graphics"
#define DRIVER_DATE		"20070209"

#if defined(__linux__)
#define I915_HAVE_FENCE
#define I915_HAVE_BUFFER
#endif

/* Interface history:
 *
 * 1.1: Original.
 * 1.2: Add Power Management
 * 1.3: Add vblank support
 * 1.4: Fix cmdbuffer path, add heap destroy
 * 1.5: Add vblank pipe configuration
 * 1.6: - New ioctl for scheduling buffer swaps on vertical blank
 *      - Support vertical blank on secondary display pipe
 * 1.8: New ioctl for ARB_Occlusion_Query
 * 1.9: Usable page flipping and triple buffering
 * 1.10: Plane/pipe disentangling
 * 1.11: TTM superioctl
 */
#define DRIVER_MAJOR		1
#if defined(I915_HAVE_FENCE) && defined(I915_HAVE_BUFFER)
#define DRIVER_MINOR		11
#else
#define DRIVER_MINOR		6
#endif
#define DRIVER_PATCHLEVEL	0

#define DRM_DRIVER_PRIVATE_T struct drm_i915_private

#ifdef I915_HAVE_BUFFER
#define I915_MAX_VALIDATE_BUFFERS 4096
#endif

struct drm_i915_ring_buffer {
	int tail_mask;
	unsigned long Start;
	unsigned long End;
	unsigned long Size;
	u8 *virtual_start;
	int head;
	int tail;
	int space;
	drm_local_map_t map;
};

struct mem_block {
	struct mem_block *next;
	struct mem_block *prev;
	int start;
	int size;
	struct drm_file *file_priv; /* NULL: free, -1: heap, other: real files */
};

struct drm_i915_vbl_swap {
	struct list_head head;
	drm_drawable_t drw_id;
	unsigned int plane;
	unsigned int sequence;
	int flip;
};

struct drm_i915_private {
	struct drm_buffer_object *ring_buffer;
	drm_local_map_t *sarea;
	drm_local_map_t *mmio_map;

	unsigned long mmiobase;
	unsigned long mmiolen;

	struct drm_i915_sarea *sarea_priv;
	struct drm_i915_ring_buffer ring;

	struct drm_dma_handle *status_page_dmah;
	void *hw_status_page;
	dma_addr_t dma_status_page;
	uint32_t counter;
	unsigned int status_gfx_addr;
	drm_local_map_t hws_map;

	unsigned int cpp;
	int use_mi_batchbuffer_start;

	wait_queue_head_t irq_queue;
	atomic_t irq_received;
	atomic_t irq_emitted;

	int tex_lru_log_granularity;
	int allow_batchbuffer;
	struct mem_block *agp_heap;
	unsigned int sr01, adpa, ppcr, dvob, dvoc, lvds;
	int vblank_pipe;
	DRM_SPINTYPE user_irq_lock;
	int user_irq_refcount;
	int fence_irq_on;
	uint32_t irq_enable_reg;
	int irq_enabled;

#ifdef I915_HAVE_FENCE
	uint32_t flush_sequence;
	uint32_t flush_flags;
	uint32_t flush_pending;
	uint32_t saved_flush_status;
	uint32_t reported_sequence;
	int reported_sequence_valid;
#endif
#ifdef I915_HAVE_BUFFER
	void *agp_iomap;
	unsigned int max_validate_buffers;
	struct mutex cmdbuf_mutex;
#endif

	DRM_SPINTYPE swaps_lock;
	struct drm_i915_vbl_swap vbl_swaps;
	unsigned int swaps_pending;

	/* LVDS info */
	int backlight_duty_cycle;  /* restore backlight to this value */
	bool panel_wants_dither;
	struct drm_display_mode *panel_fixed_mode;

 	/* Register state */
	u8 saveLBB;
	u32 saveDSPACNTR;
	u32 saveDSPBCNTR;
	u32 savePIPEACONF;
	u32 savePIPEBCONF;
	u32 savePIPEASRC;
	u32 savePIPEBSRC;
	u32 saveFPA0;
	u32 saveFPA1;
	u32 saveDPLL_A;
	u32 saveDPLL_A_MD;
	u32 saveHTOTAL_A;
	u32 saveHBLANK_A;
	u32 saveHSYNC_A;
	u32 saveVTOTAL_A;
	u32 saveVBLANK_A;
	u32 saveVSYNC_A;
	u32 saveBCLRPAT_A;
	u32 saveDSPASTRIDE;
	u32 saveDSPASIZE;
	u32 saveDSPAPOS;
	u32 saveDSPABASE;
	u32 saveDSPASURF;
	u32 saveDSPATILEOFF;
	u32 savePFIT_PGM_RATIOS;
	u32 saveBLC_PWM_CTL;
	u32 saveBLC_PWM_CTL2;
	u32 saveFPB0;
	u32 saveFPB1;
	u32 saveDPLL_B;
	u32 saveDPLL_B_MD;
	u32 saveHTOTAL_B;
	u32 saveHBLANK_B;
	u32 saveHSYNC_B;
	u32 saveVTOTAL_B;
	u32 saveVBLANK_B;
	u32 saveVSYNC_B;
	u32 saveBCLRPAT_B;
	u32 saveDSPBSTRIDE;
	u32 saveDSPBSIZE;
	u32 saveDSPBPOS;
	u32 saveDSPBBASE;
	u32 saveDSPBSURF;
	u32 saveDSPBTILEOFF;
	u32 saveVCLK_DIVISOR_VGA0;
	u32 saveVCLK_DIVISOR_VGA1;
	u32 saveVCLK_POST_DIV;
	u32 saveVGACNTRL;
	u32 saveADPA;
	u32 saveLVDS;
	u32 saveLVDSPP_ON;
	u32 saveLVDSPP_OFF;
	u32 saveDVOA;
	u32 saveDVOB;
	u32 saveDVOC;
	u32 savePP_ON;
	u32 savePP_OFF;
	u32 savePP_CONTROL;
	u32 savePP_CYCLE;
	u32 savePFIT_CONTROL;
	u32 save_palette_a[256];
	u32 save_palette_b[256];
	u32 saveFBC_CFB_BASE;
	u32 saveFBC_LL_BASE;
	u32 saveFBC_CONTROL;
	u32 saveFBC_CONTROL2;
	u32 saveSWF0[16];
	u32 saveSWF1[16];
	u32 saveSWF2[3];
	u8 saveMSR;
	u8 saveSR[8];
	u8 saveGR[24];
	u8 saveAR_INDEX;
	u8 saveAR[20];
	u8 saveDACMASK;
	u8 saveDACDATA[256*3]; /* 256 3-byte colors */
	u8 saveCR[36];
};

enum intel_chip_family {
	CHIP_I8XX = 0x01,
	CHIP_I9XX = 0x02,
	CHIP_I915 = 0x04,
	CHIP_I965 = 0x08,
	CHIP_POULSBO = 0x10,
};

extern struct drm_ioctl_desc i915_ioctls[];
extern int i915_max_ioctl;

				/* i915_dma.c */
extern void i915_kernel_lost_context(struct drm_device * dev);
extern int i915_driver_load(struct drm_device *, unsigned long flags);
extern int i915_driver_unload(struct drm_device *dev);
extern void i915_driver_lastclose(struct drm_device * dev);
extern void i915_driver_preclose(struct drm_device *dev,
				 struct drm_file *file_priv);
extern int i915_driver_device_is_agp(struct drm_device * dev);
extern long i915_compat_ioctl(struct file *filp, unsigned int cmd,
			      unsigned long arg);
extern void i915_emit_breadcrumb(struct drm_device *dev);
extern void i915_dispatch_flip(struct drm_device * dev, int pipes, int sync);
extern int i915_emit_mi_flush(struct drm_device *dev, uint32_t flush);
extern int i915_driver_firstopen(struct drm_device *dev);
extern int i915_do_cleanup_pageflip(struct drm_device *dev);
extern int i915_dma_cleanup(struct drm_device *dev);

/* i915_irq.c */
extern int i915_irq_emit(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);
extern int i915_irq_wait(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);

extern void i915_driver_wait_next_vblank(struct drm_device *dev, int pipe);
extern int i915_driver_vblank_wait(struct drm_device *dev, unsigned int *sequence);
extern int i915_driver_vblank_wait2(struct drm_device *dev, unsigned int *sequence);
extern irqreturn_t i915_driver_irq_handler(DRM_IRQ_ARGS);
extern void i915_driver_irq_preinstall(struct drm_device * dev);
extern void i915_driver_irq_postinstall(struct drm_device * dev);
extern void i915_driver_irq_uninstall(struct drm_device * dev);
extern int i915_vblank_pipe_set(struct drm_device *dev, void *data,
				struct drm_file *file_priv);
extern int i915_vblank_pipe_get(struct drm_device *dev, void *data,
				struct drm_file *file_priv);
extern int i915_emit_irq(struct drm_device * dev);
extern void i915_user_irq_on(struct drm_i915_private *dev_priv);
extern void i915_user_irq_off(struct drm_i915_private *dev_priv);
extern void i915_enable_interrupt (struct drm_device *dev);
extern int i915_vblank_swap(struct drm_device *dev, void *data,
			    struct drm_file *file_priv);

/* i915_mem.c */
extern int i915_mem_alloc(struct drm_device *dev, void *data,
			  struct drm_file *file_priv);
extern int i915_mem_free(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);
extern int i915_mem_init_heap(struct drm_device *dev, void *data,
			      struct drm_file *file_priv);
extern int i915_mem_destroy_heap(struct drm_device *dev, void *data,
				 struct drm_file *file_priv);
extern void i915_mem_takedown(struct mem_block **heap);
extern void i915_mem_release(struct drm_device * dev,
			     struct drm_file *file_priv,
			     struct mem_block *heap);
#ifdef I915_HAVE_FENCE
/* i915_fence.c */
extern void i915_fence_handler(struct drm_device *dev);
extern void i915_invalidate_reported_sequence(struct drm_device *dev);

#endif

#ifdef I915_HAVE_BUFFER
/* i915_buffer.c */
extern struct drm_ttm_backend *i915_create_ttm_backend_entry(struct drm_device *dev);
extern int i915_fence_types(struct drm_buffer_object *bo, uint32_t *fclass,
			    uint32_t *type);
extern int i915_invalidate_caches(struct drm_device *dev, uint64_t buffer_flags);
extern int i915_init_mem_type(struct drm_device *dev, uint32_t type,
			       struct drm_mem_type_manager *man);
extern uint32_t i915_evict_mask(struct drm_buffer_object *bo);
extern int i915_move(struct drm_buffer_object *bo, int evict,
		int no_wait, struct drm_bo_mem_reg *new_mem);
void i915_flush_ttm(struct drm_ttm *ttm);
#endif

#ifdef __linux__
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)
extern void intel_init_chipset_flush_compat(struct drm_device *dev);
extern void intel_fini_chipset_flush_compat(struct drm_device *dev);
#endif
#endif


/* modesetting */
extern void intel_modeset_init(struct drm_device *dev);
extern void intel_modeset_cleanup(struct drm_device *dev);


#define I915_READ(reg)          DRM_READ32(dev_priv->mmio_map, (reg))
#define I915_WRITE(reg,val)     DRM_WRITE32(dev_priv->mmio_map, (reg), (val))
#define I915_READ16(reg)	DRM_READ16(dev_priv->mmio_map, (reg))
#define I915_WRITE16(reg,val)	DRM_WRITE16(dev_priv->mmio_map, (reg), (val))

#define I915_VERBOSE 0

#define RING_LOCALS	unsigned int outring, ringmask, outcount; \
			volatile char *virt;

#define BEGIN_LP_RING(n) do {				\
	if (I915_VERBOSE)				\
		DRM_DEBUG("BEGIN_LP_RING(%d) in %s\n",	\
	                         (n), __FUNCTION__);           \
	if (dev_priv->ring.space < (n)*4)                      \
		i915_wait_ring(dev, (n)*4, __FUNCTION__);      \
	outcount = 0;					\
	outring = dev_priv->ring.tail;			\
	ringmask = dev_priv->ring.tail_mask;		\
	virt = dev_priv->ring.virtual_start;		\
} while (0)

#define OUT_RING(n) do {					\
	if (I915_VERBOSE) DRM_DEBUG("   OUT_RING %x\n", (int)(n));	\
	*(volatile unsigned int *)(virt + outring) = (n);		\
	outcount++;						\
	outring += 4;						\
	outring &= ringmask;					\
} while (0)

#define ADVANCE_LP_RING() do {						\
	if (I915_VERBOSE) DRM_DEBUG("ADVANCE_LP_RING %x\n", outring);	\
	dev_priv->ring.tail = outring;					\
	dev_priv->ring.space -= outcount * 4;				\
	I915_WRITE(LP_RING + RING_TAIL, outring);			\
} while(0)

#define MI_NOOP	(0x00 << 23)

extern int i915_wait_ring(struct drm_device * dev, int n, const char *caller);

/*
 * The Bridge device's PCI config space has information about the
 * fb aperture size and the amount of pre-reserved memory.
 */
#define INTEL_GMCH_CTRL		0x52
#define INTEL_GMCH_ENABLED	0x4
#define INTEL_GMCH_MEM_MASK	0x1
#define INTEL_GMCH_MEM_64M	0x1
#define INTEL_GMCH_MEM_128M	0

#define INTEL_855_GMCH_GMS_MASK		(0x7 << 4)
#define INTEL_855_GMCH_GMS_DISABLED	(0x0 << 4)
#define INTEL_855_GMCH_GMS_STOLEN_1M	(0x1 << 4)
#define INTEL_855_GMCH_GMS_STOLEN_4M	(0x2 << 4)
#define INTEL_855_GMCH_GMS_STOLEN_8M	(0x3 << 4)
#define INTEL_855_GMCH_GMS_STOLEN_16M	(0x4 << 4)
#define INTEL_855_GMCH_GMS_STOLEN_32M	(0x5 << 4)

#define INTEL_915G_GMCH_GMS_STOLEN_48M	(0x6 << 4)
#define INTEL_915G_GMCH_GMS_STOLEN_64M	(0x7 << 4)

/* Extended config space */
#define LBB 0xf4

/* VGA stuff */

#define VGA_ST01_MDA 0x3ba
#define VGA_ST01_CGA 0x3da

#define VGA_MSR_WRITE 0x3c2
#define VGA_MSR_READ 0x3cc
#define   VGA_MSR_MEM_EN (1<<1)
#define   VGA_MSR_CGA_MODE (1<<0)

#define VGA_SR_INDEX 0x3c4
#define VGA_SR_DATA 0x3c5

#define VGA_AR_INDEX 0x3c0
#define   VGA_AR_VID_EN (1<<5)
#define VGA_AR_DATA_WRITE 0x3c0
#define VGA_AR_DATA_READ 0x3c1

#define VGA_GR_INDEX 0x3ce
#define VGA_GR_DATA 0x3cf
/* GR05 */
#define   VGA_GR_MEM_READ_MODE_SHIFT 3
#define     VGA_GR_MEM_READ_MODE_PLANE 1
/* GR06 */
#define   VGA_GR_MEM_MODE_MASK 0xc
#define   VGA_GR_MEM_MODE_SHIFT 2
#define   VGA_GR_MEM_A0000_AFFFF 0
#define   VGA_GR_MEM_A0000_BFFFF 1
#define   VGA_GR_MEM_B0000_B7FFF 2
#define   VGA_GR_MEM_B0000_BFFFF 3

#define VGA_DACMASK 0x3c6
#define VGA_DACRX 0x3c7
#define VGA_DACWX 0x3c8
#define VGA_DACDATA 0x3c9

#define VGA_CR_INDEX_MDA 0x3b4
#define VGA_CR_DATA_MDA 0x3b5
#define VGA_CR_INDEX_CGA 0x3d4
#define VGA_CR_DATA_CGA 0x3d5

#define GFX_OP_USER_INTERRUPT		((0<<29)|(2<<23))
#define GFX_OP_BREAKPOINT_INTERRUPT	((0<<29)|(1<<23))
#define CMD_REPORT_HEAD			(7<<23)
#define CMD_STORE_DWORD_IDX		((0x21<<23) | 0x1)
#define CMD_OP_BATCH_BUFFER  ((0x0<<29)|(0x30<<23)|0x1)

#define CMD_MI_FLUSH         (0x04 << 23)
#define MI_NO_WRITE_FLUSH    (1 << 2)
#define MI_READ_FLUSH        (1 << 0)
#define MI_EXE_FLUSH         (1 << 1)
#define MI_END_SCENE         (1 << 4) /* flush binner and incr scene count */
#define MI_SCENE_COUNT       (1 << 3) /* just increment scene count */

/* Packet to load a register value from the ring/batch command stream:
 */
#define CMD_MI_LOAD_REGISTER_IMM	((0x22 << 23)|0x1)

#define BB1_START_ADDR_MASK   (~0x7)
#define BB1_PROTECTED         (1<<0)
#define BB1_UNPROTECTED       (0<<0)
#define BB2_END_ADDR_MASK     (~0x7)

#define I915REG_HWS_PGA		0x02080

/* Framebuffer compression */
#define FBC_CFB_BASE		0x03200 /* 4k page aligned */
#define FBC_LL_BASE		0x03204 /* 4k page aligned */
#define FBC_CONTROL		0x03208
#define   FBC_CTL_EN		(1<<31)
#define   FBC_CTL_PERIODIC	(1<<30)
#define   FBC_CTL_INTERVAL_SHIFT (16)
#define   FBC_CTL_UNCOMPRESSIBLE (1<<14)
#define   FBC_CTL_STRIDE_SHIFT	(5)
#define   FBC_CTL_FENCENO	(1<<0)
#define FBC_COMMAND		0x0320c
#define   FBC_CMD_COMPRESS	(1<<0)
#define FBC_STATUS		0x03210
#define   FBC_STAT_COMPRESSING	(1<<31)
#define   FBC_STAT_COMPRESSED	(1<<30)
#define   FBC_STAT_MODIFIED	(1<<29)
#define   FBC_STAT_CURRENT_LINE	(1<<0)
#define FBC_CONTROL2		0x03214
#define   FBC_CTL_FENCE_DBL	(0<<4)
#define   FBC_CTL_IDLE_IMM	(0<<2)
#define   FBC_CTL_IDLE_FULL	(1<<2)
#define   FBC_CTL_IDLE_LINE	(2<<2)
#define   FBC_CTL_IDLE_DEBUG	(3<<2)
#define   FBC_CTL_CPU_FENCE	(1<<1)
#define   FBC_CTL_PLANEA	(0<<0)
#define   FBC_CTL_PLANEB	(1<<0)
#define FBC_FENCE_OFF		0x0321b

#define FBC_LL_SIZE		(1536)
#define FBC_LL_PAD		(32)

/* Interrupt bits:
 */
#define USER_INT_FLAG    (1<<1)
#define VSYNC_PIPEB_FLAG (1<<5)
#define VSYNC_PIPEA_FLAG (1<<7)
#define HWB_OOM_FLAG     (1<<13) /* binner out of memory */

#define I915REG_HWSTAM		0x02098
#define I915REG_INT_IDENTITY_R	0x020a4
#define I915REG_INT_MASK_R	0x020a8
#define I915REG_INT_ENABLE_R	0x020a0
#define I915REG_INSTPM	        0x020c0

#define I915REG_PIPEASTAT	0x70024
#define I915REG_PIPEBSTAT	0x71024

#define I915_VBLANK_INTERRUPT_ENABLE	(1UL<<17)
#define I915_VBLANK_CLEAR		(1UL<<1)

#define GPIOA			0x5010
#define GPIOB			0x5014
#define GPIOC			0x5018
#define GPIOD			0x501c
#define GPIOE			0x5020
#define GPIOF			0x5024
#define GPIOG			0x5028
#define GPIOH			0x502c
# define GPIO_CLOCK_DIR_MASK		(1 << 0)
# define GPIO_CLOCK_DIR_IN		(0 << 1)
# define GPIO_CLOCK_DIR_OUT		(1 << 1)
# define GPIO_CLOCK_VAL_MASK		(1 << 2)
# define GPIO_CLOCK_VAL_OUT		(1 << 3)
# define GPIO_CLOCK_VAL_IN		(1 << 4)
# define GPIO_CLOCK_PULLUP_DISABLE	(1 << 5)
# define GPIO_DATA_DIR_MASK		(1 << 8)
# define GPIO_DATA_DIR_IN		(0 << 9)
# define GPIO_DATA_DIR_OUT		(1 << 9)
# define GPIO_DATA_VAL_MASK		(1 << 10)
# define GPIO_DATA_VAL_OUT		(1 << 11)
# define GPIO_DATA_VAL_IN		(1 << 12)
# define GPIO_DATA_PULLUP_DISABLE	(1 << 13)

/* p317, 319
 */
#define VCLK2_VCO_M        0x6008 /* treat as 16 bit? (includes msbs) */
#define VCLK2_VCO_N        0x600a
#define VCLK2_VCO_DIV_SEL  0x6012

#define VCLK_DIVISOR_VGA0   0x6000
#define VCLK_DIVISOR_VGA1   0x6004
#define VCLK_POST_DIV	    0x6010
/** Selects a post divisor of 4 instead of 2. */
# define VGA1_PD_P2_DIV_4	(1 << 15)
/** Overrides the p2 post divisor field */
# define VGA1_PD_P1_DIV_2	(1 << 13)
# define VGA1_PD_P1_SHIFT	8
/** P1 value is 2 greater than this field */
# define VGA1_PD_P1_MASK	(0x1f << 8)
/** Selects a post divisor of 4 instead of 2. */
# define VGA0_PD_P2_DIV_4	(1 << 7)
/** Overrides the p2 post divisor field */
# define VGA0_PD_P1_DIV_2	(1 << 5)
# define VGA0_PD_P1_SHIFT	0
/** P1 value is 2 greater than this field */
# define VGA0_PD_P1_MASK	(0x1f << 0)

#define POST_DIV_SELECT        0x70
#define POST_DIV_1             0x00
#define POST_DIV_2             0x10
#define POST_DIV_4             0x20
#define POST_DIV_8             0x30
#define POST_DIV_16            0x40
#define POST_DIV_32            0x50
#define VCO_LOOP_DIV_BY_4M     0x00
#define VCO_LOOP_DIV_BY_16M    0x04

#define SRX_INDEX		0x3c4
#define SRX_DATA		0x3c5
#define SR01			1
#define SR01_SCREEN_OFF		(1<<5)

#define PPCR			0x61204
#define PPCR_ON			(1<<0)

#define DVOA			0x61120
#define DVOA_ON			(1<<31)
#define DVOB			0x61140
#define DVOB_ON			(1<<31)
#define DVOC			0x61160
#define DVOC_ON			(1<<31)
#define LVDS			0x61180
#define LVDS_ON			(1<<31)

#define ADPA			0x61100
#define ADPA_DPMS_MASK		(~(3<<10))
#define ADPA_DPMS_ON		(0<<10)
#define ADPA_DPMS_SUSPEND	(1<<10)
#define ADPA_DPMS_STANDBY	(2<<10)
#define ADPA_DPMS_OFF		(3<<10)

#define NOPID                   0x2094
#define LP_RING			0x2030
#define HP_RING			0x2040
/* The binner has its own ring buffer:
 */
#define HWB_RING		0x2400

#define RING_TAIL		0x00
#define TAIL_ADDR		0x001FFFF8
#define RING_HEAD		0x04
#define HEAD_WRAP_COUNT		0xFFE00000
#define HEAD_WRAP_ONE		0x00200000
#define HEAD_ADDR		0x001FFFFC
#define RING_START		0x08
#define START_ADDR		0x0xFFFFF000
#define RING_LEN		0x0C
#define RING_NR_PAGES		0x001FF000
#define RING_REPORT_MASK	0x00000006
#define RING_REPORT_64K		0x00000002
#define RING_REPORT_128K	0x00000004
#define RING_NO_REPORT		0x00000000
#define RING_VALID_MASK		0x00000001
#define RING_VALID		0x00000001
#define RING_INVALID		0x00000000

/* Instruction parser error reg:
 */
#define IPEIR			0x2088

/* Scratch pad debug 0 reg:
 */
#define SCPD0			0x209c

/* Error status reg:
 */
#define ESR			0x20b8

/* Secondary DMA fetch address debug reg:
 */
#define DMA_FADD_S		0x20d4

/* Cache mode 0 reg.
 *  - Manipulating render cache behaviour is central
 *    to the concept of zone rendering, tuning this reg can help avoid
 *    unnecessary render cache reads and even writes (for z/stencil)
 *    at beginning and end of scene.
 *
 * - To change a bit, write to this reg with a mask bit set and the
 * bit of interest either set or cleared.  EG: (BIT<<16) | BIT to set.
 */
#define Cache_Mode_0		0x2120
#define CM0_MASK_SHIFT          16
#define CM0_IZ_OPT_DISABLE      (1<<6)
#define CM0_ZR_OPT_DISABLE      (1<<5)
#define CM0_DEPTH_EVICT_DISABLE (1<<4)
#define CM0_COLOR_EVICT_DISABLE (1<<3)
#define CM0_DEPTH_WRITE_DISABLE (1<<1)
#define CM0_RC_OP_FLUSH_DISABLE (1<<0)


/* Graphics flush control.  A CPU write flushes the GWB of all writes.
 * The data is discarded.
 */
#define GFX_FLSH_CNTL		0x2170

/* Binner control.  Defines the location of the bin pointer list:
 */
#define BINCTL			0x2420
#define BC_MASK			(1 << 9)

/* Binned scene info.
 */
#define BINSCENE		0x2428
#define BS_OP_LOAD		(1 << 8)
#define BS_MASK			(1 << 22)

/* Bin command parser debug reg:
 */
#define BCPD			0x2480

/* Bin memory control debug reg:
 */
#define BMCD			0x2484

/* Bin data cache debug reg:
 */
#define BDCD			0x2488

/* Binner pointer cache debug reg:
 */
#define BPCD			0x248c

/* Binner scratch pad debug reg:
 */
#define BINSKPD			0x24f0

/* HWB scratch pad debug reg:
 */
#define HWBSKPD			0x24f4

/* Binner memory pool reg:
 */
#define BMP_BUFFER		0x2430
#define BMP_PAGE_SIZE_4K	(0 << 10)
#define BMP_BUFFER_SIZE_SHIFT	1
#define BMP_ENABLE		(1 << 0)

/* Get/put memory from the binner memory pool:
 */
#define BMP_GET			0x2438
#define BMP_PUT			0x2440
#define BMP_OFFSET_SHIFT	5

/* 3D state packets:
 */
#define GFX_OP_RASTER_RULES    ((0x3<<29)|(0x7<<24))

#define GFX_OP_SCISSOR         ((0x3<<29)|(0x1c<<24)|(0x10<<19))
#define SC_UPDATE_SCISSOR       (0x1<<1)
#define SC_ENABLE_MASK          (0x1<<0)
#define SC_ENABLE               (0x1<<0)

#define GFX_OP_LOAD_INDIRECT   ((0x3<<29)|(0x1d<<24)|(0x7<<16))

#define GFX_OP_SCISSOR_INFO    ((0x3<<29)|(0x1d<<24)|(0x81<<16)|(0x1))
#define SCI_YMIN_MASK      (0xffff<<16)
#define SCI_XMIN_MASK      (0xffff<<0)
#define SCI_YMAX_MASK      (0xffff<<16)
#define SCI_XMAX_MASK      (0xffff<<0)

#define GFX_OP_SCISSOR_ENABLE	 ((0x3<<29)|(0x1c<<24)|(0x10<<19))
#define GFX_OP_SCISSOR_RECT	 ((0x3<<29)|(0x1d<<24)|(0x81<<16)|1)
#define GFX_OP_COLOR_FACTOR      ((0x3<<29)|(0x1d<<24)|(0x1<<16)|0x0)
#define GFX_OP_STIPPLE           ((0x3<<29)|(0x1d<<24)|(0x83<<16))
#define GFX_OP_MAP_INFO          ((0x3<<29)|(0x1d<<24)|0x4)
#define GFX_OP_DESTBUFFER_VARS   ((0x3<<29)|(0x1d<<24)|(0x85<<16)|0x0)
#define GFX_OP_DRAWRECT_INFO     ((0x3<<29)|(0x1d<<24)|(0x80<<16)|(0x3))

#define GFX_OP_DRAWRECT_INFO_I965  ((0x7900<<16)|0x2)

#define SRC_COPY_BLT_CMD                ((2<<29)|(0x43<<22)|4)
#define XY_SRC_COPY_BLT_CMD		((2<<29)|(0x53<<22)|6)
#define XY_MONO_SRC_COPY_IMM_BLT	((2<<29)|(0x71<<22)|5)
#define XY_SRC_COPY_BLT_WRITE_ALPHA	(1<<21)
#define XY_SRC_COPY_BLT_WRITE_RGB	(1<<20)
#define   BLT_DEPTH_8			(0<<24)
#define   BLT_DEPTH_16_565		(1<<24)
#define   BLT_DEPTH_16_1555		(2<<24)
#define   BLT_DEPTH_32			(3<<24)
#define   BLT_ROP_GXCOPY		(0xcc<<16)

#define MI_BATCH_BUFFER		((0x30<<23)|1)
#define MI_BATCH_BUFFER_START	(0x31<<23)
#define MI_BATCH_BUFFER_END	(0xA<<23)
#define MI_BATCH_NON_SECURE	(1)

#define MI_BATCH_NON_SECURE_I965 (1<<8)

#define MI_WAIT_FOR_EVENT       ((0x3<<23))
#define MI_WAIT_FOR_PLANE_B_FLIP      (1<<6)
#define MI_WAIT_FOR_PLANE_A_FLIP      (1<<2)
#define MI_WAIT_FOR_PLANE_A_SCANLINES (1<<1)

#define MI_LOAD_SCAN_LINES_INCL  ((0x12<<23))

#define CMD_OP_DISPLAYBUFFER_INFO ((0x0<<29)|(0x14<<23)|2)
#define ASYNC_FLIP                (1<<22)
#define DISPLAY_PLANE_A           (0<<20)
#define DISPLAY_PLANE_B           (1<<20)

/* Display regs */
#define DSPACNTR                0x70180
#define DSPBCNTR                0x71180
#define DISPPLANE_SEL_PIPE_MASK                 (1<<24)

/* Define the region of interest for the binner:
 */
#define CMD_OP_BIN_CONTROL	 ((0x3<<29)|(0x1d<<24)|(0x84<<16)|4)

#define CMD_OP_DESTBUFFER_INFO	 ((0x3<<29)|(0x1d<<24)|(0x8e<<16)|1)

#define BREADCRUMB_BITS 31
#define BREADCRUMB_MASK ((1U << BREADCRUMB_BITS) - 1)

#define READ_BREADCRUMB(dev_priv)  (((volatile u32*)(dev_priv->hw_status_page))[5])
#define READ_HWSP(dev_priv, reg)  (((volatile u32*)(dev_priv->hw_status_page))[reg])

#define PRIMARY_RINGBUFFER_SIZE		(128*1024)

#define BLC_PWM_CTL2		0x61250

#endif
