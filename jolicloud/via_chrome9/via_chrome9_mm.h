/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice
 * (including the next paragraph) shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL VIA, S3 GRAPHICS, AND/OR
 * ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef _VIA_CHROME9_MM_H_
#define _VIA_CHROME9_MM_H_

#define BRANCH_BUFFER_SIZE      0x100000  /* branch buf size is 1024 K bytes */
#define BRANCH_BUFFER_NUMBER	0x10       /* branch buf num is 16 */
#define BRANCH_BUF_ARRAY_NUMBER 3
#define PREPARED_BUF_ARRAY      0
#define ALLOCATED_BUF_ARRAY     1
#define FLUSHED_BUF_ARRAY       2

/* special macro for fence id write back system memory */
/* The fence address of 353 is 0x10 align. 336/364 also use it now */
#define FENCE_SIZE    0x10
#define FENCE_NUM    BRANCH_BUFFER_NUMBER
#define align_to_power2_bytes(x, align)    (((unsigned long)(x) + \
			(align - 1)) & ~(align - 1))
#define align_to_16_byte(x)    align_to_power2_bytes(x, 16)


struct drm_via_chrome9_pciemem_ctrl {
	enum {
		pciemem_copy_from_user = 0,
		pciemem_copy_to_user,
		pciemem_memset,
		} ctrl_type;
	unsigned int pcieoffset;
	unsigned int size;/*in Byte*/
	unsigned char memsetdata;/*for memset*/
	void *usermode_data;/*user mode data pointer*/
};

extern int via_chrome9_map_init(struct drm_device *dev,
	struct drm_via_chrome9_init *init);
extern int via_chrome9_heap_management_init(struct drm_device
	*dev, struct drm_via_chrome9_init *init);
extern void via_chrome9_memory_destroy_heap(struct drm_device
	*dev, struct drm_via_chrome9_private *dev_priv);
extern int via_chrome9_ioctl_check_vidmem_size(struct drm_device
	*dev, void *data, struct drm_file *file_priv);
extern int via_chrome9_ioctl_pciemem_ctrl(struct drm_device *dev,
	void *data, struct drm_file *file_priv);
extern int via_chrome9_ioctl_allocate_aperture(struct drm_device
	*dev, void *data, struct drm_file *file_priv);
extern int via_chrome9_ioctl_free_aperture(struct drm_device *dev,
	void *data, struct drm_file *file_priv);
extern int via_chrome9_ioctl_allocate_mem_base(struct drm_device
	*dev, void *data, struct drm_file *file_priv);
extern int via_chrome9_ioctl_allocate_mem_wrapper(
	struct drm_device *dev,	void *data, struct drm_file *file_priv);
extern int via_chrome9_ioctl_freemem_base(struct drm_device
	*dev, void *data, struct drm_file *file_priv);
extern int via_chrome9_ioctl_free_mem_wrapper(struct drm_device
	*dev, void *data, struct drm_file *file_priv);
extern void via_chrome9_reclaim_buffers_locked(struct drm_device
	*dev, struct drm_file *file_priv);
extern int via_chrome9_branch_buf_request(struct drm_device *dev,
void *data, struct drm_file *file_priv);
extern int via_chrome9_branch_buf_init(struct drm_device *dev,
	struct drm_via_chrome9_init *init, struct drm_file *file_priv);
extern int via_chrome9_branch_buf_cleanup(struct drm_device *dev,
	struct drm_via_chrome9_init *init, struct drm_file *file_priv);
extern struct kmd_branch_buf_rec *
get_from_branch_buf_array(struct drm_via_chrome9_private *dev_priv,
	u32 condition, u32 array_index);
extern void
add_into_branch_buf_array(struct drm_via_chrome9_private *dev_priv,
	struct kmd_branch_buf_rec *p, u32 array_index);

#endif

