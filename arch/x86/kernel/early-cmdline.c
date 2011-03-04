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
#include <asm/e820.h>
#include <asm/pci-direct.h>
#include <asm/dma.h>
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
	void (*f)(char* newCmd);
};

static void disable_i915_modeset(char* newCmd)
{
	char addCmd[] = "i915.modeset=0";

	strcpy(newCmd, addCmd);
}

static void set_poulsbo_sony_vaiox_memmap(char* newCmd)
{
	char addCmd[] = "memmap=1K#0x7f800000";

	strcpy(newCmd, addCmd);
}

static void set_poulsbo_memory(char* newCmd)
{
	int i;
	char addCmd[] = "mem=1920mb";
	u64 ramSize = 0;

	for (i = 0; i < e820.nr_map; i++) {
		if (e820.map[i].type == E820_RAM) {
			ramSize += e820.map[i].size;
		}
	}
	if ( ramSize < 1200000000LL )
	    strcpy(addCmd, "mem=896mb");

	printk(KERN_INFO "early-cmdline: RAM: %Ld\n", ramSize);

	strcpy(newCmd, addCmd);
}

static struct device_cmdline early_cmd[] __initdata = {
	{ 0x8086, 0x27AE, 0x1025, 0x022F,	// Acer Aspire One D250
	  PCI_CLASS_DISPLAY_VGA, PCI_ANY_ID, QFLAG_APPLY_ONCE,
	  disable_i915_modeset
	},
	{ 0x8086, 0x27AE, 0x1734, 0x115D,	// Fujitsu Amilo Mini Ui 3520
	  PCI_CLASS_DISPLAY_VGA, PCI_ANY_ID, QFLAG_APPLY_ONCE,
	  disable_i915_modeset
	},
	{ 0x8086, 0x27AE, 0x8086, 0x1999,	// ZaReason Terra A20
	  PCI_CLASS_DISPLAY_VGA, PCI_ANY_ID, QFLAG_APPLY_ONCE,
	  disable_i915_modeset
	},
	{ 0x8086, 0x8108, 0x104D, 0x905F,	// Sony Viao X
	  PCI_CLASS_DISPLAY_VGA, PCI_ANY_ID, QFLAG_APPLY_ONCE,
	  set_poulsbo_sony_vaiox_memmap
	},
	{ 0x8086, 0x8108, PCI_ANY_ID, PCI_ANY_ID, // All Poulsbo video cards
	  PCI_CLASS_DISPLAY_VGA, PCI_ANY_ID, QFLAG_APPLY_ONCE,
	  set_poulsbo_memory
	},
	{}
};

/**
 * insert_command - insert a device-specific command into the command line
 * @cmdLine: pointer to main command-line
 * @dev: device_cmdline structure
 *
 * Execute the device-specific function, which generates a new command-line
 * option. That value is appended with a space, then inserted at the
 * beginning of the global command-line char pointer.
 *
 * Print something to dmesg to claim responsibility.
 */
static void __init insert_command(char* cmdLine, struct device_cmdline dev)
{
	char newCmd[COMMAND_LINE_SIZE] = "";

	// Run the function to get the new command
	dev.f(newCmd);

	printk(KERN_INFO "early-cmdline: Adding '%s' for PCI device "
		"%04x:%04x %04x:%04x\n", newCmd, dev.vendor,
		dev.device, dev.subvendor, dev.subdevice);

	// Insert the new command at the start of the existing cmdLine
	strlcat(newCmd, " ", COMMAND_LINE_SIZE);
	strlcat(newCmd, cmdLine, COMMAND_LINE_SIZE);
	strlcpy(cmdLine, newCmd, COMMAND_LINE_SIZE);
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

	for (i = 0; early_cmd[i].f != NULL; i++) {
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
