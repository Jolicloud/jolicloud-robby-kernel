obj-m += vntwusb.o

vntwusb-objs :=	main_usb.o card.o mac.o baseband.o wctl.o 80211mgr.o \
	wcmd.o wmgr.o bssdb.o rxtx.o dpc.o power.o datarate.o  \
	mib.o rc4.o tether.o tcrc.o ioctl.o hostap.o wpa.o key.o tkip.o \
	michael.o rf.o iwctl.o wpactl.o wpa2.o aes_ccmp.o  \
	usbpipe.o channel.o control.o firmware.o int.o

EXTRA_CFLAGS += -DHOSTAP
EXTRA_CFLAGS += -I$(PWD) -I$(PWD)/../include
