/* $Id: ReadmeReleaseHybridBuild.txt,v 15.4.22.2.6.28 2010/02/06 01:27:51 Exp $ */



Broadcom Linux hybrid wireless driver

DISCLAIMER
----------
This is an Official Release of Broadcom's hybrid Linux driver for use with 
Broadcom based hardware.

WHERE TO GET THE RELEASE
------------------------
http://www.broadcom.com/support/802.11/linux_sta.php

IMPORTANT NOTE AND DISCUSSION OF HYBRID DRIVER
----------------------------------------------
There are separate tarballs for 32 bit and 64 bit x86 CPU architectures.
Make sure you use the appropriate tarball for your machine.

Other than 32 vs 64 bit, the hybrid binary is agnostic to the specific
versions (2.6.X) and distributions (Fedora, Ubuntu, SuSE, etc).  It performs
all interactions with the OS through OS specific files (wl_linux.c, wl_iw.c,
osl_linux.c) that are shipped in source form. You compile this source on
your system and link with a precompiled binary file (wlc_hybrid.o_shipped)
which contains the rest of the driver.

PRECOMPILED DRIVER
-------------------
Some distros (Ubuntu and Fedora at the least) already have a version of
this driver in their repositories precompiled, tested and ready to go.
You just use the package manager to install the proper package.  If
its available for your distro, this is usually an easier solution. See
the end of this document for further discussion.

ABOUT THIS RELEASE
-------------------
This is a rollup release.  It includes and deprecates all previous releases
and patches.  At the time of release there are no existing patches for this
release from Broadcom.

SUPPORTED DEVICES
-----------------
The cards with the following PCI Device IDs are supported with this driver.
Both Broadcom and and Dell product names are described.  Cards not listed here
may also work.

	   BRCM		    PCI		  PCI		  Dell
	  Product Name	  Vendor ID	Device ID	Product ID
          -------------	 ----------	---------   	-----------
          4311 2.4 Ghz	    0x14e4	0x4311  	Dell 1390
          4311 Dualband	    0x14e4	0x4312  	Dell 1490
          4311 5 Ghz	    0x14e4    	0x4313  	
          4312 2.4 Ghz	    0x14e4	0x4315  	Dell 1395
          4313 2.4 Ghz	    0x14e4	0x4727 		Dell 1501
          4321 Dualband	    0x14e4	0x4328  	Dell 1505
          4321 Dualband	    0x14e4	0x4328  	Dell 1500
          4321 2.4 Ghz	    0x14e4	0x4329  	
          4321 5 Ghz        0x14e4	0x432a  	
          4322 	Dualband    0x14e4	0x432b  	Dell 1510
          4322 2.4 Ghz      0x14e4 	0x432c  	
          4322 5 Ghz        0x14e4 	0x432d  	
          43224 Dualband    0x14e4	0x4353  	Dell 1520
          43225 2.4 Ghz     0x14e4	0x4357  	

To find the Device ID's of Broadcom cards on your machines do:
# lspci -n | grep 14e4


REQUIREMENTS
------------
Building this driver requires that your machine have the proper tools,
packages, header files and libraries to build a standard a kernel module.  
This usually is done by installing the kernel developer or kernel source 
package and varies from distro to distro. Consult the documentation for
your specific OS.


If you cannot successfully build a module that comes with your distro's 
kernel developer or kernel source package, you will not be able to build 
this module either.

If you try to build this module but get an error message that looks like 
this:

make: *** /lib/modules/"release"/build: No such file or directory. Stop.

Then you do not have the proper packages installed, since installing the 
proper packages will create /lib/modules/"release"/build on your system.

On Fedora install 'kernel-devel' (Development Package for building kernel
modules to match the kernel) from the Package Manager (System->
Administration-> Add/Remove Software).

On Ubuntu, you will need headers and tools.  Try these commands:
# apt-get install build-essential linux-headers-generic
# apt-get build-dep linux

To check to see if you have this directory do this:

# ls /lib/modules/`uname -r`/build

BUILD INSTRUCTIONS
------------------
1. Setup the directory by untarring the proper tarball:

For 32 bit: 	hybrid-portsrc.tar.gz
For 64 bit: 	hybrid-portsrc-x86_64.tar.gz

# mkdir hybrid_wl
# cd hybrid_wl
# tar xzf <path>/hybrid-portsrc.tar or <path>/hybrid-portsrc-x86_64.tar.gz

2. Build the driver as a Linux loadable kernel module (LKM):

# make clean   (optional)
# make

When the build completes, it will produce a wl.ko file in the top level
directory.

If your driver does not build, check to make sure you have installed the
kernel package described in the requirements above.

INSTALL INSTRUCTIONS
--------------------

Upgrading from a previous version:
---------------------------------

If you were already running a previous version of wl, you'll want to provide
a clean transition from the older driver. (The path to previous driver is
usually /lib/modules/<kernel-version>/kernel/net/wireless)

# rmmod wl 
# mv <path-to-prev-driver>/wl.ko <path-to-prev-driver>/wl.ko.orig
# cp wl.ko <path-to-prev-driver>/wl.ko
# depmod
# modprobe wl

The new wl driver should now be operational and your all done.

Fresh installation:
------------------
1: Remove any other drivers for the Broadcom wireless device.

There are several open source drivers that are used to drive Broadcom 802.11
chips such as b43 and ssb. They will conflict with this driver and need
to be uninstalled before this driver can be installed.  Any previous 
revisions of the wl driver also need to be removed.

Note: On some systems such as Ubuntu 9.10, the ssb module may load during
boot even though it is blacklisted (see note under Common Issues on how to
resolve this). Nevertheless, ssb still must be removed
(by hand or script) before wl is loaded. The wl driver will not function 
properly if ssb the module is loaded.

# lsmod  | grep "b43\|ssb\|wl"

If any of these are installed, remove them:
# rmmod b43
# rmmod ssb
# rmmod wl

To blacklist these drivers and prevent them from loading in the future:
# echo "blacklist ssb" >> /etc/modprobe.d/blacklist.conf
# echo "blacklist b43" >> /etc/modprobe.d/blacklist.conf

2: Insmod the driver.

Otherwise, if you have not previously installed a wl driver, you'll need
to add a security module before using the wl module.  Most newer systems 
use lib80211 while others use ieee80211_crypt_tkip. See which one works for 
your system.

# modprobe lib80211 
  or 
# modprobe ieee80211_crypt_tkip

Then:
# insmod wl.ko

wl.ko is now operational.  It may take several seconds for the Network 
Manager to notice a new network driver has been installed and show the
surrounding wireless networks.

Common issues:
----------------

* After the insmod you may see this message:
  "WARNING: modpost: missing MODULE_LICENSE()"
  It is expected, not harmful and can be ignored.

* You might see this message:
  "insmod: error inserting 'wl.ko': -1 Unknown symbol in module"
  Usually this means that the wlan security module (as mentioned above) is
  not loaded. Try this:
  # modprobe   lib80211 
    or 
  # modprobe ieee80211_crypt_tkip
    and then re-try to insmod the wl driver.
  # insmod wl.ko

* If the wl driver loads but doesn't seem to do anything:
  the ssb module may be the cause.  Sometimes blacklisting ssb may not
  be enough to prevent it from loading and it loads anyway. (This is mostly
  seen on Ubuntu/Debian systems).

  Check to see if ssb, wl or b43 is loaded:
  # lsmod  | grep "b43\|ssb\|wl"

  If any of these are installed, remove them:
  # rmmod b43
  # rmmod ssb
  # rmmod wl

  Back up the current boot ramfs and generate a new one.
  # cp /boot/initrd.img-`uname -r`  somewheresafe
  # update-initramfs -u
  # reboot

3: Setup to always load at boot time.

The procedure to make a module load at boot time varies from distro to
distro.  Consult the docs for your specific distro to see how.  The 
following seems to work for my setup on Fedora and Ubuntu.  Check your 
docs to see the procedure for your distro.

Follow these steps to have the driver load as part of the boot process:

# load driver as described above
# cp wl.ko /lib/modules/`uname -r`/kernel/drivers/net/wireless 
# depmod -a

# echo modeprobe wl >> /etc/rc.local  (Fedora/SUSE)

Ubuntu ships a version of wl.ko, so those need to be disabled.  On my 
system the were several versions, so I searched and renamed the .ko's
like this:

# sh: for i in `find /lib /var -name wl\.ko`; do mv $i ${i}.orig; done


TX POWER EXPLAINED
------------------
'iwconfig eth1 txpower' & 'iwlist eth1 txpower' set and get the drivers 
user-requested transmit power level. This can go up to 32 dbm and allows
the user to lower the tx power to levels below the regulatory limit.
Internally, the actual tx power is always kept within regulatory limits
no matter what the user request is set to.

WHAT'S NEW IN THIS RELEASE
----------------------------
+ Supports up to linux kernel 2.6.31. 2.6.32 support is there also but 
not tested (although reports from users suggests it works fine.)
+ Supports hidden networks
+ Supports rfkill in kernels <  2.6.31
+ Setting power level via 'iwconfig eth1 txpower X' now operational
+ Support for 4313
+ Additional channels in both 2.4 and 5 Ghz bands.
+ Fixed issue with tkip group keys that caused this message to repeat often:
	TKIP: RX tkey->key_idx=2 frame keyidx=1 priv=ffff8800cf80e840
+ Following fixes
    Issue #72216 - Ubuntu 8.04: standby/resume with WPA2 and wpa_supplicant causes
                               a continuous assoc/disassoc loop (issue in 2.6.24 kernel)
    Issue #72324 - Ubuntu 8.04: cannot ping when Linux STA is IBSS creator with WEP
    Issue #76739 - Ubuntu 9.04: unable to connect to hidden network after stdby/resume
    Issue #80392 - S4 resume hang with SuSE SLED 11 and 43225
    Issue #80792 - LSTA is not able to associate to AP with transition from AES to TKIP encryption

KNOWN ISSUES AND LIMITATIONS
----------------------------
#72238 - 20% lower throughput on channels 149, 153, 157, and 161
#76743 - Ubuntu9.04: Network manager displays n/w's with radio disabled
#76793 - Ubuntu9.04: STA fails to create IBSS network in 5 Ghz band
#81392 - Unable to transfer data over ad-hoc network created by NetworkManager (iwconfig OK)
#81452 - STA unable to associate to AP when PEAPv1-MSCHAPv2 authentication is used

HOW TO INSTALL A PRE-COMPILED DRIVER
-----------------------------------
Some of the major linux distros already supply a version of this driver, so
you don't have to compile your own.  Most of the distros keep this driver
along with other proprietary or non-GPL drivers in a separate repository.

For further information see the documentation for your specific distro.

Fedora:
------
su -c 'rpm -Uvh
http://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-stable.noarch.rpm
http://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-stable.noarch.rpm'

su -
yum update
yum install kmod-wl

Ubuntu:
------
Go to System->Administration->Hardware Drivers
Choose the Broadcom STA wireless driver
Activate

Sometimes the driver does not show up in the Hardware Drivers choices.  In
this case, try reintalling the driver from the GUI or shell like this:

From the GUI:
Package Manager (System>Administration>Synaptic Package Manager). Click the 
Reload button in the upper left corner of Synaptic to refresh your index then 
search for and reinstall the package named bcmwl-kernel-source.

From the shell:
sudo apt-get update
sudo apt-get --reinstall install bcmwl-kernel-source

In either GUI or text case, after reinstalling, reboot your machine.

Now go back to System->Administration->Hardware Drivers
and you should see the driver enabled and working.
