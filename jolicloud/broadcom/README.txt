
DISCLAIMER
----------

This is an OFFICAL-RELEASE of Broadcom's hybrid Linux driver for use with Broadcom
BCM4312 based hardware (device ID 4315).

IMPORTANT NOTE AND DISCUSSION OF HYBRID DRIVER
----------------------------------------------

There are different tar's for 32 bit and 64 bit x86 CPU architectures.  Make sure you use the
appropriate tar, as the hybrid binary must be of the appropriate architectural type.

Otherwise the hybrid binary is agnostic to the specific version of Linux kernel
because it is designed to perform all interactions with the OS through OS specific
files (wl_linux.c, wl_iw.c) and an OS abstraction layer file (osl_linux.c). 
All of these interactions are done through functions which make the hybrid binary
OS version independent.  All Linux OS specific code is provided in source form
allowing re-targeting to different kernel versions and fixing OS related issues.

BUILD AND INSTALLATION INSTRUCTIONS
-----------------------------------

hybrid-portsrc.tar.gz
hybrid-portsrc-x86_64.tar.gz

On the target machine, setup the source/hybrid/build directory

1.  Create a new directory:                 mkdir hybrid_wl
2.  Go to that directory:                   cd hybrid_wl
3.  Untar the appropriate 32/64 bit file
      to that directory
        32 bit:                             tar -xzf <path>/hybrid-portsrc.tar.gz
        64 bit:                             tar -xzf <path>/hybrid-portsrc-x86_64.tar.gz

After untar'ing you should have a src and lib sub directory plus a Linux
2.6 "kbuild" external makefile (Makefile).   The lib sub directory has the pre-built
binary, wlc_hybrid.o_shipped.  

You use the standard Linux 2.6 kernel build system as follows to make a Linux loadable
kernel module (LKM):

On the target machine, and cd'ed to the directory that contains the Makefile (fragment)

4.  Cleanup (optional):                  make -C /lib/modules/<2.6.xx.xx>/build M=`pwd` clean
5.  Build the LKM, i.e. wl.ko:           make -C /lib/modules/<2.6.xx.xx>/build M=`pwd`

You should now have a LKM, wl.ko inside this directory.

On this or a machine with the same kernel version, install the driver.

1.  Validate you don't have loaded (or built into the kernel) the Linux community provided
      driver for Broadcom hardware.  This exists in two forms: either "bcm43xx" or a split form
      of "b43" plus "b43legacy".  If these modules were loaded you would either
        a) rmmod bcm43xx or
        b) rmmod b43; rmmod b43legacy
2.  Make available 802.11 TKIP crypto module:      modprobe ieee80211_crypt_tkip
3.  Insert the Broadcom wl module:                 insmod <path>/wl.ko
