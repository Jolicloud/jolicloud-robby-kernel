/**
 * \file rename_drm-psb_symbols.h
 * Rename pouslbo-specific DRM symbols to not conflict with standard DRM
 *
 * \author Adam McDaniel <adam@jolicloud.org>
 */

/*
 * Copyright 2009 Jolicloud SAS, Paris, France.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _RENAME_PSB_DRM_SYMBOLS_H_
#define _RENAME_PSB_DRM_SYMBOLS_H_

#define drm_ati_pcigart_cleanup drmpsb_ati_pcigart_cleanup
#define drm_ati_pcigart_init drmpsb_ati_pcigart_init
#define ati_pcigart_init_ttm psb_ati_pcigart_init_ttm
#define drm_agp_info drmpsb_agp_info
#define drm_agp_acquire drmpsb_agp_acquire
#define drm_agp_release drmpsb_agp_release
#define drm_agp_enable drmpsb_agp_enable
#define drm_agp_alloc drmpsb_agp_alloc
#define drm_agp_unbind drmpsb_agp_unbind
#define drm_agp_bind drmpsb_agp_bind
#define drm_agp_free drmpsb_agp_free
#define drm_agp_bind_memory drmpsb_agp_bind_memory
#define drm_agp_init_ttm drmpsb_agp_init_ttm
#define drm_agp_flush_chipset drmpsb_agp_flush_chipset
#define drm_bo_wait drmpsb_bo_wait
#define drm_bo_usage_deref_locked drmpsb_bo_usage_deref_locked
#define drm_bo_usage_deref_unlocked drmpsb_bo_usage_deref_unlocked
#define drm_putback_buffer_objects drmpsb_putback_buffer_objects
#define drm_fence_buffer_objects drmpsb_fence_buffer_objects
#define drm_bo_mem_space drmpsb_bo_mem_space
#define drm_lookup_buffer_object drmpsb_lookup_buffer_object
#define drm_bo_fill_rep_arg drmpsb_bo_fill_rep_arg
#define drm_bo_do_validate drmpsb_bo_do_validate
#define drm_bo_handle_validate drmpsb_bo_handle_validate
#define drm_buffer_object_create drmpsb_buffer_object_create
#define drm_bo_clean_mm drmpsb_bo_clean_mm
#define drm_bo_init_mm drmpsb_bo_init_mm
#define drm_bo_driver_finish drmpsb_bo_driver_finish
#define drm_bo_driver_init drmpsb_bo_driver_init
#define drm_mem_reg_is_pci drmpsb_mem_reg_is_pci
#define drm_bo_read_unlock drmpsb_bo_read_unlock
#define drm_bo_read_lock drmpsb_bo_read_lock
#define drm_bo_move_ttm drmpsb_bo_move_ttm
#define drm_mem_reg_ioremap drmpsb_mem_reg_ioremap
#define drm_mem_reg_iounmap drmpsb_mem_reg_iounmap
#define drm_bo_move_memcpy drmpsb_bo_move_memcpy
#define drm_bo_move_accel_cleanup drmpsb_bo_move_accel_cleanup
#define drm_bo_same_page drmpsb_bo_same_page
#define drm_bo_offset_end drmpsb_bo_offset_end
#define drm_bo_kmap drmpsb_bo_kmap
#define drm_bo_kunmap drmpsb_bo_kunmap
#define drm_get_resource_start drmpsb_get_resource_start
#define drm_get_resource_len drmpsb_get_resource_len
#define drm_find_matching_map drmpsb_find_matching_map
#define drm_addmap drmpsb_addmap
#define drm_rmmap_locked drmpsb_rmmap_locked
#define drm_rmmap drmpsb_rmmap
#define drm_addbufs_agp drmpsb_addbufs_agp
#define drm_addbufs_pci drmpsb_addbufs_pci
#define drm_addbufs_fb drmpsb_addbufs_fb
#define drm_order drmpsb_order
#define drm_bo_vm_nopfn drmpsb_bo_vm_nopfn
#define drm_init_pat drmpsb_init_pat
#define drm_framebuffer_create drmpsb_framebuffer_create
#define drm_framebuffer_destroy drmpsb_framebuffer_destroy
#define drm_crtc_create drmpsb_crtc_create
#define drm_crtc_destroy drmpsb_crtc_destroy
#define drm_crtc_in_use drmpsb_crtc_in_use
#define drm_crtc_probe_output_modes drmpsb_crtc_probe_output_modes
#define drm_crtc_set_mode drmpsb_crtc_set_mode
#define drm_disable_unused_functions drmpsb_disable_unused_functions
#define drm_mode_probed_add drmpsb_mode_probed_add
#define drm_mode_remove drmpsb_mode_remove
#define drm_output_create drmpsb_output_create
#define drm_output_destroy drmpsb_output_destroy
#define drm_output_rename drmpsb_output_rename
#define drm_mode_create drmpsb_mode_create
#define drm_mode_destroy drmpsb_mode_destroy
#define drm_mode_config_init drmpsb_mode_config_init
#define drm_init_mode drmpsb_init_mode
#define drm_init_xres drmpsb_init_xres
#define drm_init_yres drmpsb_init_yres
#define drm_pick_crtcs drmpsb_pick_crtcs
#define drm_initial_config drmpsb_initial_config
#define drm_mode_config_cleanup drmpsb_mode_config_cleanup
#define drm_mode_addmode drmpsb_mode_addmode
#define drm_mode_rmmode drmpsb_mode_rmmode
#define drm_mode_attachmode_crtc drmpsb_mode_attachmode_crtc
#define drm_mode_detachmode_crtc drmpsb_mode_detachmode_crtc
#define drm_property_create drmpsb_property_create
#define drm_property_add_enum drmpsb_property_add_enum
#define drm_property_destroy drmpsb_property_destroy
#define drm_output_attach_property drmpsb_output_attach_property
#define drm_core_reclaim_buffers drmpsb_core_reclaim_buffers
#define drm_get_drawable_info drmpsb_get_drawable_info
#define drm_cleanup_pci drmpsb_cleanup_pci
#define drm_init drmpsb_init
#define drm_exit drmpsb_exit
#define drm_ioctl drmpsb_ioctl
#define drm_unlocked_ioctl drmpsb_unlocked_ioctl
#define drm_getsarea drmpsb_getsarea
#define drm_get_acpi_edid drmpsb_get_acpi_edid
#define drm_ddc_read drmpsb_ddc_read
#define drm_get_edid drmpsb_get_edid
#define drm_add_edid_modes drmpsb_add_edid_modes
#define drmfb_probe psb_drmfb_probe
#define drmfb_remove psb_drmfb_remove
#define drm_fence_wait_polling drmpsb_fence_wait_polling
#define drm_fence_handler drmpsb_fence_handler
#define drm_fence_usage_deref_locked drmpsb_fence_usage_deref_locked
#define drm_fence_usage_deref_unlocked drmpsb_fence_usage_deref_unlocked
#define drm_fence_reference_unlocked drmpsb_fence_reference_unlocked
#define drm_fence_object_signaled drmpsb_fence_object_signaled
#define drm_fence_object_flush drmpsb_fence_object_flush
#define drm_fence_flush_old drmpsb_fence_flush_old
#define drm_fence_object_wait drmpsb_fence_object_wait
#define drm_fence_object_emit drmpsb_fence_object_emit
#define drm_fence_add_user_object drmpsb_fence_add_user_object
#define drm_fence_object_create drmpsb_fence_object_create
#define drm_fence_fill_arg drmpsb_fence_fill_arg
#define drm_open drmpsb_open
#define drm_fasync drmpsb_fasync
#define drm_release drmpsb_release
#define drm_poll drmpsb_poll
#define drm_compat_ioctl drmpsb_compat_ioctl
#define drm_irq_install drmpsb_irq_install
#define drm_irq_uninstall drmpsb_irq_uninstall
#define drm_vbl_send_signals drmpsb_vbl_send_signals
#define drm_locked_tasklet drmpsb_locked_tasklet
#define drm_idlelock_take drmpsb_idlelock_take
#define drm_idlelock_release drmpsb_idlelock_release
#define drm_i_have_hw_lock drmpsb_i_have_hw_lock
#define drm_free_memctl drmpsb_free_memctl
#define drm_query_memctl drmpsb_query_memctl
#define drm_calloc drmpsb_calloc
#define drm_core_ioremap drmpsb_core_ioremap
#define drm_core_ioremapfree drmpsb_core_ioremapfree
#define drm_alloc drmpsb_alloc
#define drm_calloc drmpsb_calloc
#define drm_realloc drmpsb_realloc
#define drm_free drmpsb_free
#define drm_mm_put_block drmpsb_mm_put_block
#define drm_mm_init drmpsb_mm_init
#define drm_mm_takedown drmpsb_mm_takedown
#define drm_mode_debug_printmodeline drmpsb_mode_debug_printmodeline
#define drm_mode_set_name drmpsb_mode_set_name
#define drm_mode_width drmpsb_mode_width
#define drm_mode_height drmpsb_mode_height
#define drm_mode_vrefresh drmpsb_mode_vrefresh
#define drm_mode_set_crtcinfo drmpsb_mode_set_crtcinfo
#define drm_mode_duplicate drmpsb_mode_duplicate
#define drm_mode_equal drmpsb_mode_equal
#define drm_mode_validate_size drmpsb_mode_validate_size
#define drm_mode_validate_clocks drmpsb_mode_validate_clocks
#define drm_add_user_object drmpsb_add_user_object
#define drm_lookup_user_object drmpsb_lookup_user_object
#define drm_lookup_ref_object drmpsb_lookup_ref_object
#define drm_remove_ref_object drmpsb_remove_ref_object
#define drm_pci_alloc drmpsb_pci_alloc
#define drm_pci_free drmpsb_pci_free
#define drm_regs_alloc drmpsb_regs_alloc
#define drm_regs_fence drmpsb_regs_fence
#define drm_regs_free drmpsb_regs_free
#define drm_regs_add drmpsb_regs_add
#define drm_regs_init drmpsb_regs_init
#define drm_sg_cleanup drmpsb_sg_cleanup
#define drm_sg_alloc drmpsb_sg_alloc
#define drm_sman_takedown drmpsb_sman_takedown
#define drm_sman_init drmpsb_sman_init
#define drm_sman_set_range drmpsb_sman_set_range
#define drm_sman_set_manager drmpsb_sman_set_manager
#define drm_sman_alloc drmpsb_sman_alloc
#define drm_sman_free_key drmpsb_sman_free_key
#define drm_sman_owner_clean drmpsb_sman_owner_clean
#define drm_sman_owner_cleanup drmpsb_sman_owner_cleanup
#define drm_sman_cleanup drmpsb_sman_cleanup
#define drm_debug drmpsb_debug
#define drm_get_dev drmpsb_get_dev
#define drm_ttm_cache_flush drmpsb_ttm_cache_flush
#define drm_ttm_get_page drmpsb_ttm_get_page
#define drm_bind_ttm drmpsb_bind_ttm
#define drm_core_get_map_ofs drmpsb_core_get_map_ofs
#define drm_core_get_reg_ofs drmpsb_core_get_reg_ofs
#define drm_mmap drmpsb_mmap
#define drm_bo_vm_nopfn drmpsb_bo_vm_nopfn

#endif