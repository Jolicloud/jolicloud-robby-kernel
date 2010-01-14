NIC_SELECT = RTL8192SU
CC = gcc
KVER  := $(shell uname -r)
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers/net/wireless/$(NIC_SELECT)

ifneq ($(shell uname -r|cut -d. -f1,2), 2.4)
EXTRA_CFLAGS += -I$(TOPDIR)/drivers/net/wireless
EXTRA_CFLAGS += -std=gnu89
EXTRA_CFLAGS += -O2
EXTRA_CFLAGS += -mhard-float -DCONFIG_FORCE_HARD_FLOAT=y

ifeq ($(NIC_SELECT),RTL8192SU)
EXTRA_CFLAGS += -DJACKSON_NEW_RX
EXTRA_CFLAGS += -DTHOMAS_BEACON -DTHOMAS_TURBO
#EXTRA_CFLAGS += -DUSE_ONE_PIPE
EXTRA_CFLAGS += -DENABLE_DOT11D

EXTRA_CFLAGS += -DRTL8192SU
EXTRA_CFLAGS += -DRTL8190_Download_Firmware_From_Header=1
EXTRA_CFLAGS += -DRTL8192S_PREPARE_FOR_NORMAL_RELEASE
EXTRA_CFLAGS += -DRTL8192SU_DISABLE_IQK=1

#EXTRA_CFLAGS += -DEEPROM_OLD_FORMAT_SUPPORT

EXTRA_CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT
#EXTRA_CFLAGS += -DUSB_TX_DRIVER_AGGREGATION_ENABLE=0
#EXTRA_CFLAGS += -DRTL8192SU_DISABLE_CCK_RATE=0
EXTRA_CFLAGS += -DRTL8192S_DISABLE_FW_DM=0 
EXTRA_CFLAGS += -DDISABLE_BB_RF=0 
EXTRA_CFLAGS += -DRTL8192SU_USE_PARAM_TXPWR=0
EXTRA_CFLAGS += -DRTL8192SU_FPGA_UNSPECIFIED_NETWORK=0
#EXTRA_CFLAGS += -DRTL8192SU_FPGA_2MAC_VERIFICATION #=0
EXTRA_CFLAGS += -DRTL8192SU_ASIC_VERIFICATION
EXTRA_CFLAGS += -DRTL8192SU_USB_PHY_TEST=0

#EXTRA_CFLAGS += -DMUTIPLE_BULK_OUT
EXTRA_CFLAGS += -DCONFIG_RTL8192_PM
EXTRA_CFLAGS += -DENABLE_IPS
EXTRA_CFLAGS += -DENABLE_LPS

EXTRA_CFLAGS += -DENABLE_TX_ISR_TASKLET
EXTRA_CFLAGS += -DUSE_FW_SOURCE_HEADER_FILE
EXTRA_CFLAGS += -DTCP_CSUM_OFFLOAD_RX
EXTRA_CFLAGS += -DBUILT_IN_IEEE80211
EXTRA_CFLAGS += -DCONFIG_MP_TOOL
endif

ifeq ($(NIC_SELECT),RTL8192U)
EXTRA_CFLAGS += -DRTL8192U
EXTRA_CFLAGS += -DTHOMAS_TURBO
EXTRA_CFLAGS += -DUSB_TX_DRIVER_AGGREGATION_ENABLE
EXTRA_CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT
EXTRA_CFLAGS += -DUSE_ONE_PIPE
#EXTRA_CFLAGS += -DWIFI_TEST # The USE_ONE_PIPE MUST NOT be defined if WIFI_TEST is defined. 
EXTRA_CFLAGS += -DENABLE_DOT11D
EXTRA_CFLAGS += -DCONFIG_RTL8192_PM
#EXTRA_CFLAGS += -DENABLE_AMSDU

# For RTK DMP platform
#EXTRA_CFLAGS += -DRTK_DMP_PLATFORM
EXTRA_CFLAGS += -DENABLE_TX_ISR_TASKLET
EXTRA_CFLAGS += -DUSE_FW_SOURCE_HEADER_FILE
EXTRA_CFLAGS += -DBUILT_IN_IEEE80211

endif

ifeq ($(NIC_SELECT),RTL8192SU)
r8192s_usb-objs := r8180_93cx6.o r8192U_wx.o r8192S_phy.o r8192S_rtl6052.o r8192S_rtl8225.o r819xU_cmdpkt.o r8192U_dm.o r8192SU_HWImg.o r8192S_firmware.o r8192S_Efuse.o r8192U_core.o r8192U_pm.o r8192SU_led.o \
        ../../ieee80211/ieee80211_rx.o                \
        ../../ieee80211/ieee80211_softmac.o           \
        ../../ieee80211/ieee80211_tx.o                \
        ../../ieee80211/ieee80211_wx.o                \
        ../../ieee80211/ieee80211_module.o            \
        ../../ieee80211/ieee80211_softmac_wx.o        \
        ../../ieee80211/rtl819x_HTProc.o              \
        ../../ieee80211/rtl819x_TSProc.o              \
        ../../ieee80211/rtl819x_BAProc.o              \
        ../../ieee80211/dot11d.o                      \
        ../../ieee80211/ieee80211_crypt.o             \
        ../../ieee80211/ieee80211_crypt_tkip.o        \
        ../../ieee80211/ieee80211_crypt_ccmp.o        \
        ../../ieee80211/ieee80211_crypt_wep.o
r8192s_usb-objs	+= r8192u_mp.o
obj-m := r8192s_usb.o
else
r8192_usb-objs := r8192U_core.o r8180_93cx6.o r8192U_wx.o r8190_rtl8256.o r819xU_phy.o r819xU_firmware.o r819xU_cmdpkt.o r8192U_dm.o r819xU_firmware_img.o r8192U_pm.o \
        ../../ieee80211/ieee80211_rx.o                \
        ../../ieee80211/ieee80211_softmac.o           \
        ../../ieee80211/ieee80211_tx.o                \
        ../../ieee80211/ieee80211_wx.o                \
        ../../ieee80211/ieee80211_module.o            \
        ../../ieee80211/ieee80211_softmac_wx.o        \
        ../../ieee80211/rtl819x_HTProc.o              \
        ../../ieee80211/rtl819x_TSProc.o              \
        ../../ieee80211/rtl819x_BAProc.o              \
        ../../ieee80211/dot11d.o                      \
        ../../ieee80211/ieee80211_crypt.o             \
        ../../ieee80211/ieee80211_crypt_tkip.o        \
        ../../ieee80211/ieee80211_crypt_ccmp.o        \
        ../../ieee80211/ieee80211_crypt_wep.o
obj-m := r8192_usb.o
endif

KSRC := /lib/modules/$(KVER)/build
INSTALL_PREFIX :=
IMG_DIR := /lib/firmware/
all: modules

modules:
	$(MAKE) -C $(KSRC) M=$(PWD) CC=$(CC) modules

install: modules
ifeq ($(NIC_SELECT),RTL8192SU)
	install -p -m 644 r8192s_usb.ko  $(MODDESTDIR)
else
	install -p -m 644 r8192_usb.ko  $(MODDESTDIR)
endif
	@depmod -a
	@#copy firmware img to target fold
	$(shell [ -d "$(IMG_DIR)/`uname -r`" ] && cp -fr $(RTL819x_FIRM_DIR)/$(NIC_SELECT) $(IMG_DIR)/`uname -r`/.)
	$(shell [ ! -d "$(IMG_DIR)/`uname -r`" ] && cp -fr $(RTL819x_FIRM_DIR)/$(NIC_SELECT) $(IMG_DIR)/.)
	
uninstall:
	$(shell [ -d $(MODDESTDIR) ] && rm -fr $(MODDESTDIR))
	depmod -a
	#delete the firmware img
	rm -fr /lib/firmware/$(NIC_SELECT)
	rm -fr /lib/firmware/`uname -r`/$(NIC_SELECT)
else
LD := ld
KSRC := /lib/modules/$(KVER)/build
CONFIG_FILE := $(KSRC)/include/linux/autoconf.h
CFLAGS += -DLINUX -D__KERNEL__ -DMODULE -O2 -pipe -Wall
CFLAGS += -I$(KSRC)/include -I.
CFLAGS += -DMODVERSIONS -DEXPORT_SYMTAB -include $(KSRC)/include/linux/modversions.h
#CFLAGS += -D__NO_VERSION__ -DEXPORT_SYMTAB
CFLAGS += -DENABLE_DOT11D
CFLAGS += -DTHOMAS_TURBO

ifeq ($(NIC_SELECT),RTL8192SU)
CFLAGS += -DJACKSON_NEW_RX
CFLAGS += -DTHOMAS_BEACON -DTHOMAS_TURBO
#CFLAGS += -DUSE_ONE_PIPE

CFLAGS += -DRTL8192SU
CFLAGS += -DRTL8190_Download_Firmware_From_Header=1
CFLAGS += -DRTL8192S_PREPARE_FOR_NORMAL_RELEASE
CFLAGS += -DRTL8192SU_DISABLE_IQK=1

#CFLAGS += -DEEPROM_OLD_FORMAT_SUPPORT

#CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT=0
#CFLAGS += -DUSB_TX_DRIVER_AGGREGATION_ENABLE=0
#CFLAGS += -DRTL8192SU_DISABLE_CCK_RATE=0
CFLAGS += -DRTL8192S_DISABLE_FW_DM=0 
CFLAGS += -DDISABLE_BB_RF=0 
CFLAGS += -DRTL8192SU_USE_PARAM_TXPWR=0
CFLAGS += -DRTL8192SU_FPGA_UNSPECIFIED_NETWORK=0
#CFLAGS += -DRTL8192SU_FPGA_2MAC_VERIFICATION #=0
CFLAGS += -DRTL8192SU_ASIC_VERIFICATION
CFLAGS += -DRTL8192SU_USB_PHY_TEST=0

#CFLAGS += -DMUTIPLE_BULK_OUT
CFLAGS += -DBUILT_IN_IEEE80211
endif

ifeq ($(NIC_SELECT),RTL8192U)
CFLAGS += -DRTL8192U
CFLAGS += -DTHOMAS_TURBO
CFLAGS += -DUSB_TX_DRIVER_AGGREGATION_ENABLE
CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT
CFLAGS += -DUSE_ONE_PIPE
#CFLAGS += -DCONFIG_RTL8192_PM
#CFLAGS += -DENABLE_AMSDU
CFLAGS += -DBUILT_IN_IEEE80211
endif

SMP := $(shell $(CC) $(CFLAGS) -E -dM $(CONFIG_FILE) | \
   grep CONFIG_SMP | awk '{print $$3}')
ifneq ($(SMP),1)
   SMP := 0
endif
ifeq ($(SMP),1)
   CFLAGS    += -D__SMP__
endif

OBJS := ${patsubst %.c, %.o, ${wildcard *.c}}
                                                                                
ifeq ($(NIC_SELECT),RTL8192SU)
all:r8192s_usb.o
r8192s_usb.o: \
	r8192U_core.o \
	r8180_93cx6.o \
	r8192U_wx.o \
	r8192S_phy.o \
	r8192S_rtl6052.o \
	r8192S_rtl8225.o \
	r819xU_cmdpkt.o \
	r8192SU_HWImg.o \
	r8192S_firmware.o \
	r8192S_Efuse.o \
	r8192SU_led.o \
        ../../ieee80211/ieee80211_rx.o                \
        ../../ieee80211/ieee80211_softmac.o           \
        ../../ieee80211/ieee80211_tx.o                \
        ../../ieee80211/ieee80211_wx.o                \
        ../../ieee80211/ieee80211_module.o            \
        ../../ieee80211/ieee80211_softmac_wx.o        \
        ../../ieee80211/rtl819x_HTProc.o              \
        ../../ieee80211/rtl819x_TSProc.o              \
        ../../ieee80211/rtl819x_BAProc.o              \
        ../../ieee80211/dot11d.o                      \
        ../../ieee80211/ieee80211_crypt.o             \
        ../../ieee80211/ieee80211_crypt_tkip.o        \
        ../../ieee80211/ieee80211_crypt_ccmp.o        \
        ../../ieee80211/ieee80211_crypt_wep.o	
	$(LD) -r $^ -o $@
install: r8192s_usb.o
	install -p -m 644 r8192_usb.o  $(MODDESTDIR)
	@depmod -a
endif
                                                                                
ifeq ($(NIC_SELECT),RTL8192U)
all:r8192_usb.o
r8192_usb.o: \
	r8192U_core.o \
	r8180_93cx6.o \
	r8192U_wx.o \
	r8190_rtl8256.o \
	r819xU_phy.o \
	r819xU_firmware.o \
	r819xU_cmdpkt.o \
	r8192U_dm.o \
	r819xU_firmware_img.o \
	r8192U_pm.o \
        ../../ieee80211/ieee80211_rx.o                \
        ../../ieee80211/ieee80211_softmac.o           \
        ../../ieee80211/ieee80211_tx.o                \
        ../../ieee80211/ieee80211_wx.o                \
        ../../ieee80211/ieee80211_module.o            \
        ../../ieee80211/ieee80211_softmac_wx.o        \
        ../../ieee80211/rtl819x_HTProc.o              \
        ../../ieee80211/rtl819x_TSProc.o              \
        ../../ieee80211/rtl819x_BAProc.o              \
        ../../ieee80211/dot11d.o                      \
        ../../ieee80211/ieee80211_crypt.o             \
        ../../ieee80211/ieee80211_crypt_tkip.o        \
        ../../ieee80211/ieee80211_crypt_ccmp.o        \
        ../../ieee80211/ieee80211_crypt_wep.o	
	$(LD) -r $^ -o $@
install: r8192_usb.o
	install -p -m 644 r8192_usb.o  $(MODDESTDIR)
	@depmod -a
endif

uninstall:
	$(shell [ -d $(MODDESTDIR) ] && rm -fr $(MODDESTDIR))
	@depmod -a
endif

.PHONY:clean
clean:
	rm -fr *.mod.c *.mod *.o .*.cmd *.ko *~
	rm -rf .tmp_versions
	rm -fr Modules.symvers
	rm -fr Module.symvers
	rm -fr Module.markers
	rm -fr modules.order
	rm -fr tags
