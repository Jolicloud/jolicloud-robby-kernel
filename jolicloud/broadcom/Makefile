
#
# Makefile fragment for Linux 2.6
# Broadcom 802.11abg Networking Device Driver
#
# Copyright 2008, Broadcom Corporation
# All Rights Reserved.
# 
#      Unless you and Broadcom execute a separate written software license
# agreement governing use of this software, this software is licensed to you
# under the terms of the GNU General Public License version 2, available at
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html (the "GPL"), with the
# following added to such license:
#      As a special exception, the copyright holders of this software give you
# permission to link this software with independent modules, regardless of the
# license terms of these independent modules, and to copy and distribute the
# resulting executable under terms of your choice, provided that you also meet,
# for each linked independent module, the terms and conditions of the license
# of that module. An independent module is a module which is not derived from
# this software.
# 
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
# $Id: Makefile_kbuild_portsrc,v 1.1.2.4 2008/05/02 18:33:39 Exp $

ifeq ($(shell dpkg-architecture -qDEB_HOST_ARCH),amd64)
BINARCH = x86_64
else
BINARCH = i386
endif

obj-m              += wl.o

wl-objs            := 
wl-objs            += src/wl/sys/wl_linux.o
wl-objs            += src/wl/sys/wl_iw.o
wl-objs            += src/shared/linux_osl.o

EXTRA_CFLAGS       :=
EXTRA_CFLAGS       += -I$(src)/src/include
EXTRA_CFLAGS       += -I$(src)/src/wl/sys

HYBRID_MODULE      := lib/wlc_hybrid_$(BINARCH).o
HYBRID_MODULE_UU   := $(KBUILD_SRC)/$(obj)/$(HYBRID_MODULE).uu
HYBRID_MODULE_OBJ  := $(O)/$(obj)/$(HYBRID_MODULE)

EXTRA_LDFLAGS      := $(HYBRID_MODULE_OBJ)

$(HYBRID_MODULE_OBJ): $(HYBRID_MODULE_UU)
	mkdir -p `dirname $@`
	uudecode -o $@ $^

$(src)/wl.o: $(HYBRID_MODULE_OBJ)
