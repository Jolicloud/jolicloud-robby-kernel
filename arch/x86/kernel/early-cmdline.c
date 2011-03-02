/* Various workarounds for chipset bugs.
   This code runs very early and can't use the regular PCI subsystem
   The entries are keyed to PCI bridges which usually identify chipsets
   uniquely.
   This is only for whole classes of chipsets with specific problems which
   need early invasive action (e.g. before the timers are initialized).
   Most PCI device specific workarounds can be done later and should be
   in standard PCI quirks
   Mainboard specific bugs should be handled by DMI entries.
   CPU specific bugs in setup.c */

#include <linux/pci.h>
#include <linux/acpi.h>
#include <linux/pci_ids.h>
#include <asm/pci-direct.h>
#include <asm/dma.h>
#include <asm/io_apic.h>
#include <asm/apic.h>
#include <asm/iommu.h>
#include <asm/gart.h>
#include <asm/setup.h>



#define QFLAG_APPLY_ONCE 	0x1
#define QFLAG_APPLIED		0x2
#define QFLAG_DONE		(QFLAG_APPLY_ONCE|QFLAG_APPLIED)
struct device_cmdline {
	u32 vendor;
	u32 device;
	u32 subvendor;
	u32 subdevice;
	u32 class;
	u32 class_mask;
	u32 flags;
	char* addCmd;
};

static struct device_cmdline early_cmd[] __initdata = {
	{}
};

/**
 * insert_command - insert a device-specific command into the command line
 * @cmdLine: pointer to main command-line
 * @dev: device_cmdline structure
 *
 * Copy the device-specific command-line into a new char array, append a
 * space, then insert it into the beginning of the global command-line char
 * pointer.
 *
 * Print something to dmesg to claim responsibility.
 */
static void __init insert_command(char* cmdLine, struct device_cmdline dev)
{
	char newCmd[COMMAND_LINE_SIZE];

	strlcpy(newCmd, dev.addCmd, COMMAND_LINE_SIZE);
	strlcat(newCmd, " ", COMMAND_LINE_SIZE);
	strlcat(newCmd, cmdLine, COMMAND_LINE_SIZE);
	strlcpy(cmdLine, newCmd, COMMAND_LINE_SIZE);

	printk(KERN_INFO "early-cmdline: Adding '%s' for PCI device "
		"%04x:%04x %04x:%04x\n", dev.addCmd, dev.vendor,
		dev.device, dev.subvendor, dev.subdevice);
}

/**
 * check_dev_quirk - apply early quirks to a given PCI device
 * @bus: bus number
 * @slot: slot number
 * @cmdline: char pointer to the global command-line
 *
 * Check the vendor & device ID against the early quirks table.
 */
static int __init check_dev_cmdline(int bus, int slot, int func, char* cmdline)
{
	u16 class;
	u16 vendor;
	u16 device;
	u16 subvendor;
	u16 subdevice;
	u8 type;
	int i;

	class = read_pci_config_16(bus, slot, func, PCI_CLASS_DEVICE);

	if (class == 0xffff)
		return -1; /* no class, treat as single function */

	vendor = read_pci_config_16(bus, slot, func, PCI_VENDOR_ID);
	device = read_pci_config_16(bus, slot, func, PCI_DEVICE_ID);
	subvendor = read_pci_config_16(bus, slot, func, PCI_SUBSYSTEM_VENDOR_ID);
	subdevice = read_pci_config_16(bus, slot, func, PCI_SUBSYSTEM_ID);

	for (i = 0; early_cmd[i].addCmd != NULL; i++) {
		if (((early_cmd[i].vendor == PCI_ANY_ID) ||
			(early_cmd[i].vendor == vendor)) &&
			((early_cmd[i].device == PCI_ANY_ID) ||
			(early_cmd[i].device == device)) &&
			((early_cmd[i].subvendor == PCI_ANY_ID) ||
			(early_cmd[i].subvendor == subvendor)) &&
			((early_cmd[i].subdevice == PCI_ANY_ID) ||
			(early_cmd[i].subdevice == subdevice)) &&
			(!((early_cmd[i].class ^ class) &
			    early_cmd[i].class_mask))) {
				if ((early_cmd[i].flags &
				     QFLAG_DONE) != QFLAG_DONE)
					insert_command(cmdline,
						early_cmd[i]);
				early_cmd[i].flags |= QFLAG_APPLIED;
			}
	}

	type = read_pci_config_byte(bus, slot, func,
				    PCI_HEADER_TYPE);
	if (!(type & 0x80))
		return -1;

	return 0;
}

void __init early_cmdline(char* cmdline)
{
	int bus, slot, func;

	if (!early_pci_allowed())
		return;

	/* Poor man's PCI discovery */
	/* Only scan the first three busses */
	for (bus = 0; bus < 4; bus++ ) {
	    for (slot = 0; slot < 32; slot++) {
		    for (func = 0; func < 8; func++) {
			    if (check_dev_cmdline(bus, slot, func, cmdline))
				    break;
		    }
	    }
	}
}
