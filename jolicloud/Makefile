#
# Makefile for the Linux kernel Jolicloud supplied third-party device drivers.
#

obj-$(CONFIG_ASUS_EEE)		+= asus_eee/
obj-$(CONFIG_BROADCOM_4312)	+= broadcom/
obj-$(CONFIG_NETBOOK)		+= netbook/
obj-$(CONFIG_TLSUP)			+= tlsup/

# This is a stupid trick to get kbuild to create jolicloud/built-in.o
obj- += foo.o
