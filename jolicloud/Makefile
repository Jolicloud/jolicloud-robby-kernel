#
# Makefile for the Linux kernel Jolicloud supplied third-party device drivers.
#

obj-$(CONFIG_ASUS_EEE)		+= asus_eee/
obj-$(CONFIG_BROADCOM_4312)	+= broadcom/
obj-$(CONFIG_DRM_PSB)		+= psb/
obj-$(CONFIG_DRM_VIA_CHROME9)	+= via_chrome9/
obj-$(CONFIG_RTL8192SE)		+= rtl8192se/

# This is a stupid trick to get kbuild to create jolicloud/built-in.o
obj- += foo.o
