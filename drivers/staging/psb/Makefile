obj-$(CONFIG_DRM_PSB)		+= psb.o

ccflags-y := -Idrivers/gpu/drm -Idrivers/staging/psb/ttm -I$(PWD)

psb-objs :=	\
	psb_drv.o	\
	psb_mmu.o	\
	psb_sgx.o	\
	psb_irq.o	\
	psb_fence.o	\
	psb_buffer.o	\
	psb_gtt.o	\
	psb_schedule.o	\
	psb_scene.o	\
	psb_reset.o	\
	psb_xhw.o	\
	psb_msvdx.o	\
	lnc_topaz.o	\
	lnc_topazinit.o	\
	psb_msvdxinit.o	\
	psb_ttm_glue.o	\
	psb_fb.o	\
	psb_setup.o	\
	ttm/ttm_object.o		\
	ttm/ttm_lock.o			\
	ttm/ttm_fence_user.o		\
	ttm/ttm_fence.o			\
	ttm/ttm_tt.o			\
	ttm/ttm_execbuf_util.o		\
	ttm/ttm_bo.o			\
	ttm/ttm_bo_util.o		\
	ttm/ttm_placement_user.o	\
	ttm/ttm_bo_vm.o			\
	ttm/ttm_pat_compat.o		\
	ttm/ttm_agp_backend.o		\
	ttm/ttm_memory.o
