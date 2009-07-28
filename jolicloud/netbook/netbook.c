/*
 *  netbook.c - Asus netbookPC extras
 *  http://code.google.com/p/netbookpc-linux/
 *
 *  Copyright (C) 2007 Andrew Tipton
 *  Copyright (C) 2008 Jean Fabrice
 *  Copyright (C) 2008 Roberto A. Foglietta
 *  Copyright (C) 2009 Andrew Wyatt
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Ths program is distributed in the hope that it will be useful,
 *  but WITOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTAILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Template Place, Suite 330, Boston, MA  02111-1307 USA
 *  
 *  ---------
 *
 *  WARNING:  This is an extremely *experimental* module!  This code has been
 *  developed through trial-and-error, which means I don't really understand
 *  100% what's going on here...  That means there's a chance that there could
 *  be unintended side-effects which might cause data loss or even physical
 *  damage!
 *
 *  Again, this code comes WITHOUT ANY WARRANTY whatsoever.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>             // For inb() and outb()
#include <linux/i2c.h>
#include <linux/mutex.h>

/* Module info */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Tipton, Jean Fabrice, Roberto A. Foglietta, Andrew Wyatt");
MODULE_DESCRIPTION("Support for netbookPC-specific functionality.");

#define EEE_VERSION "0.5"
#define EEE_NAME "netbook"

MODULE_VERSION(EEE_VERSION);
MODULE_INFO(module_depends, "i2c-i801");

static int writable = 0;
module_param(writable, int, 0644);
MODULE_PARM_DESC(insanelev, "Some functioning parameters" 
	"could be changed but risky");

/* PLL access functions.
 *
 * Note that this isn't really the "proper" way to use the I2C API... :)
 * I2C_SMBUS_BLOCK_MAX is 32, the maximum size of a block read/write.
 */
static bool netbook_pll_init(void);
static bool netbook_pll_read(void);
static bool netbook_pll_write(void);
static void netbook_pll_cleanup(void);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22) 
/* This was removed in Linux 2.6.10 because it had no users, 
 * but has been restored back later in version 2.6.22 
 */
static s32 i2c_smbus_read_block_data(struct i2c_client *client, u8 command, 
								u8 *values) {
	union i2c_smbus_data data;
	if(!client || !client->adapter || !values) {
		printk(KERN_WARNING "%s internal i2c_smbus_read_block_data"
			" received a NULL pointer\n", EEE_NAME);
	}
	if (i2c_smbus_xfer(client->adapter, client->addr, client->flags,
		I2C_SMBUS_READ,command, I2C_SMBUS_BLOCK_DATA, &data)) {
	} else {
		int i;
		for (i = 1; i <= data.block[0]; i++)
			values[i-1] = data.block[i];
		return data.block[0];
	}
	return -1;
}
#endif 

static struct i2c_client netbook_pll_smbus_client = {
	.adapter = NULL,
	.addr = 0x69,
	.flags = 0,
};
static char netbook_pll_data[I2C_SMBUS_BLOCK_MAX];
static int netbook_pll_datalen = 0;

static bool netbook_pll_init(void) {
	/* this module depends on this symbol */
	if(__symbol_get("i2c_get_adapter"))
		netbook_pll_smbus_client.adapter = i2c_get_adapter(0);

	if(!netbook_pll_smbus_client.adapter) {
		printk(KERN_WARNING 
			"%s module requires i2c-i801 module at load time" 
				" if you like to access pll via proc too\n", 
					EEE_NAME);
		return false;
	} 

	/* Fill the netbook_pll_data buffer */
	return netbook_pll_read();
}

static bool reinit_ifneeded(void) {
	if(!netbook_pll_smbus_client.adapter) {
		if(!netbook_pll_init()) {
			return false;	
		} else {
			printk(KERN_INFO 
				"%s module found an i2c_adapter,"
					"pll proc could be read by now\n",
						EEE_NAME);
		}
	} 
	return true;
}

// Takes approx 150ms to execute.
static bool netbook_pll_read(void) {
	if(!reinit_ifneeded()) return false;
	memset(netbook_pll_data, 0, I2C_SMBUS_BLOCK_MAX);
	netbook_pll_datalen = i2c_smbus_read_block_data(&netbook_pll_smbus_client, 
		0, netbook_pll_data);
	return true;
}

// Takes approx 150ms to execute ???
static bool netbook_pll_write(void) {
	if(!reinit_ifneeded()) return false;
	i2c_smbus_write_block_data(&netbook_pll_smbus_client, 
		0, netbook_pll_datalen, netbook_pll_data);
	return true;
}

static void netbook_pll_cleanup(void) {
	if(netbook_pll_smbus_client.adapter)
		i2c_put_adapter(netbook_pll_smbus_client.adapter);
}

/* Embedded controller access functions.
 *
 * The ENE KB3310 embedded controller has a feature known as "Index IO"
 * which allows the entire 64KB address space of the controller to be
 * accessed via a set of ISA I/O ports at 0x380-0x384.  This allows us
 * direct access to all of the controller's ROM, RAM, SFRs, and peripheral
 * registers;  this access bypasses the EC firmware entirely.
 *
 * This is much faster than using ec_transaction(), and it also allows us to
 * do things which are not possible through the EC's official interface.
 *
 * An Indexed IO write to an EC register takes approx. 90us, while an EC
 * transaction takes approx. 2500ms.
 */
#define EC_IDX_ADDRH 0x381
#define EC_IDX_ADDRL 0x382
#define EC_IDX_DATA 0x383
#define HIGH_BYTE(x) ((x & 0xff00) >> 8)
#define LOW_BYTE(x) (x & 0x00ff)
static DEFINE_MUTEX(netbook_ec_mutex);

static unsigned char netbook_ec_read(unsigned short addr) {
    unsigned char data;

    mutex_lock(&netbook_ec_mutex);
    outb(HIGH_BYTE(addr), EC_IDX_ADDRH);
    outb(LOW_BYTE(addr), EC_IDX_ADDRL);
    data = inb(EC_IDX_DATA);
    mutex_unlock(&netbook_ec_mutex);

    return data;
}

static void netbook_ec_write(unsigned short addr, unsigned char data) {
    mutex_lock(&netbook_ec_mutex);
    outb(HIGH_BYTE(addr), EC_IDX_ADDRH);
    outb(LOW_BYTE(addr), EC_IDX_ADDRL);
    outb(data, EC_IDX_DATA);
    mutex_unlock(&netbook_ec_mutex);
}

#define EC_GPIO_PORT 0xFC20
#define EC_GPIO_PIN2PORT(pin) (EC_GPIO_PORT + (((pin) >> 3) & 0x1F))
#define EC_GPIO_PIN2MASK(pin) (1 << ((pin) & 0x07))

static void netbook_ec_gpio_set(int pin, int value) {
    unsigned short port;
    unsigned char mask;

    port = EC_GPIO_PIN2PORT(pin);
    mask = EC_GPIO_PIN2MASK(pin);
    if (value) {
        netbook_ec_write(port, netbook_ec_read(port) | mask);
    } else {
        netbook_ec_write(port, netbook_ec_read(port) & ~mask);
    }
}

static int netbook_ec_gpio_get(int pin) {
    unsigned short port;
    unsigned char mask;
    unsigned char status;

    port = EC_GPIO_PIN2PORT(pin);
    mask = EC_GPIO_PIN2MASK(pin);
    status = netbook_ec_read(port) & mask;

    return (status) ? 1 : 0;
}

/*** Fan and temperature functions ***/
#define EC_ST00 0xF451          // Temperature of CPU (C)
#define EC_SC02 0xF463          // Fan PWM duty cycle (%)
#define EC_SC05 0xF466          // High byte of fan speed (RPM)
#define EC_SC06 0xF467          // Low byte of fan speed (RPM)
#define EC_SFB3 0xF4D3          // Flag byte containing SF25 (FANctrl)
#define MANUAL_FAN_BIT 0x02

static unsigned int netbook_get_temperature(void) {
    return netbook_ec_read(EC_ST00);
}

static unsigned int netbook_fan_get_rpm(void) {
    return (netbook_ec_read(EC_SC05) << 8) | netbook_ec_read(EC_SC06);
}

/* 1 if fan is in manual mode, 0 if controlled by the EC */
static int netbook_fan_get_manual(void) {
    return (netbook_ec_read(EC_SFB3) & MANUAL_FAN_BIT) ? 1 : 0;
}

static void netbook_fan_set_manual(int manual) {
    if (manual) {
        /* SF25=1: Prevent the EC from controlling the fan */
        netbook_ec_write(EC_SFB3, netbook_ec_read(EC_SFB3) | MANUAL_FAN_BIT);
    } else {
        /* SF25=0: Allow the EC to control the fan */
        netbook_ec_write(EC_SFB3, netbook_ec_read(EC_SFB3) & ~MANUAL_FAN_BIT);
    }
}

static void netbook_fan_set_speed(unsigned int speed) {
    netbook_ec_write(EC_SC02, (speed > 100) ? 100 : speed);
}

static unsigned int netbook_fan_get_speed(void) {
    return netbook_ec_read(EC_SC02);
}


/*** Voltage functions ***/
#define EC_VOLTAGE_PIN 0x66

enum netbook_voltage { Low=0, High=1 };
static enum netbook_voltage netbook_get_voltage(void) {
    return netbook_ec_gpio_get(EC_VOLTAGE_PIN);
}
static void netbook_set_voltage(enum netbook_voltage voltage) {
    netbook_ec_gpio_set(EC_VOLTAGE_PIN, voltage);
}

/*** FSB functions ***/
static void netbook_get_freq(int *DynA, int *DynB, int *PCI) {
    *DynA = netbook_pll_data[11] & 0xFF;
    *DynB = netbook_pll_data[12] & 0xFF;
    *PCI = netbook_pll_data[15] & 0xFF;
}

static void netbook_set_freq(int DynA, int DynB, int PCI) {
    int current_dyna = 0, current_dynb = 0, current_pci = 0;
    netbook_get_freq(&current_dyna, &current_dynb, &current_pci);
    if (current_dynb != DynA || current_dynb != DynB || current_pci != PCI) {
        netbook_pll_data[11] = DynA;
        netbook_pll_data[12] = DynB;
        netbook_pll_data[15] = PCI;
        netbook_pll_write();
    }
}

/*** /proc file functions ***/

static struct proc_dir_entry *netbook_proc_rootdir;
#define EEE_PROC_READFUNC(NAME) \
    void netbook_proc_readfunc_##NAME (char *buf, int buflen, int *bufpos)
#define EEE_PROC_WRITEFUNC(NAME) \
    void netbook_proc_writefunc_##NAME (const char *buf, int buflen, int *bufpos)
#define EEE_PROC_PRINTF(FMT, ARGS...) \
    *bufpos += snprintf(buf + *bufpos, buflen - *bufpos, FMT, ##ARGS)
#define EEE_PROC_SCANF(COUNT, FMT, ARGS...) \
    do { \
        int len = 0; \
        int cnt = sscanf(buf + *bufpos, FMT "%n", ##ARGS, &len); \
        if (cnt < COUNT) { \
            printk(KERN_DEBUG "%s:  scanf(\"%s\") wanted %d args, but got %d.\n", EEE_NAME, FMT, COUNT, cnt); \
            return; \
        } \
        *bufpos += len; \
    } while (0)
#define EEE_PROC_MEMCPY(SRC, SRCLEN) \
    do { \
        int len = SRCLEN; \
        if (len > (buflen - *bufpos)) \
            len = buflen - *bufpos; \
        memcpy(buf + *bufpos, SRC, (SRCLEN > (buflen - *bufpos)) ? (buflen - *bufpos) : SRCLEN); \
        *bufpos += len; \
    } while (0)
#define EEE_PROC_FILES_BEGIN \
    static struct netbook_proc_file netbook_proc_files[] = {
#define EEE_PROC_RW(NAME, MODE) \
    { #NAME, MODE, &netbook_proc_readfunc_##NAME, &netbook_proc_writefunc_##NAME }
#define EEE_PROC_RO(NAME, MODE) \
    { #NAME, MODE, &netbook_proc_readfunc_##NAME, NULL }
#define EEE_PROC_FILES_END \
    { NULL, 0, NULL, NULL } };

struct netbook_proc_file {
    char *name;
    int mode;
    void (*readfunc)(char *buf, int buflen, int *bufpos);
    void (*writefunc)(const char *buf, int buflen, int *bufpos);
};


EEE_PROC_READFUNC(bus_control) {
    int DynA = 0;
    int DynB = 0;
    int PCI = 0;
    int voltage = 0;
    netbook_get_freq(&DynA, &DynB, &PCI);
    voltage = (int)netbook_get_voltage();
    EEE_PROC_PRINTF("%d %d %d %d\n", DynA, DynB, PCI, voltage);
}

EEE_PROC_WRITEFUNC(bus_control) {
    int DynA = 0;     // sensible defaults
    int DynB = 0;
    int PCI = 0;
    int voltage = 0;
    EEE_PROC_SCANF(4, "%i %i %i %i", &DynA, &DynB, &PCI, &voltage);
    netbook_set_freq(DynA, DynB, PCI);
    netbook_set_voltage(voltage);
}

EEE_PROC_READFUNC(pll) {
    netbook_pll_read();
    EEE_PROC_MEMCPY(netbook_pll_data, netbook_pll_datalen);
}

EEE_PROC_READFUNC(fan_speed) {
    int speed = netbook_fan_get_speed();
    EEE_PROC_PRINTF("%d\n", speed);
}

EEE_PROC_WRITEFUNC(fan_speed) {
    unsigned int speed = 0;
    EEE_PROC_SCANF(1, "%u", &speed);
    netbook_fan_set_speed(speed);
}

EEE_PROC_READFUNC(fan_rpm) {
    int rpm = netbook_fan_get_rpm();
    EEE_PROC_PRINTF("%d\n", rpm);
}

EEE_PROC_READFUNC(fan_control) {
    EEE_PROC_PRINTF("%d\n", netbook_fan_get_manual());
}

EEE_PROC_WRITEFUNC(fan_control) {
    int manual = 0;
    EEE_PROC_SCANF(1, "%i", &manual);
    netbook_fan_set_manual(manual);
}

#if 0
EEE_PROC_READFUNC(fan_mode) {
    enum netbook_fan_mode mode = netbook_fan_get_mode();
    switch (mode) {
        case Manual:    EEE_PROC_PRINTF("manual\n");
                        break;
        case Automatic: EEE_PROC_PRINTF("auto\n");
                        break;
        case Embedded:  EEE_PROC_PRINTF("embedded\n");
                        break;
    }
}

EEE_PROC_WRITEFUNC(fan_mode) {
    enum netbook_fan_mode mode = Automatic;
    char inputstr[16];
    EEE_PROC_SCANF(1, "%15s", inputstr);
    if (strcmp(inputstr, "manual") == 0) {
        mode = Manual;
    } else if (strcmp(inputstr, "auto") == 0) {
        mode = Automatic;
    } else if (strcmp(inputstr, "embedded") == 0) {
        mode = Embedded;
    }
    netbook_fan_set_mode(mode);
}
#endif

EEE_PROC_READFUNC(temperature) {
    unsigned int t = netbook_get_temperature();
    EEE_PROC_PRINTF("%d\n", t);
}

EEE_PROC_FILES_BEGIN
    EEE_PROC_RO(pll,            0400),	/* this has to be the first */
    EEE_PROC_RW(bus_control,            0644),
    EEE_PROC_RW(fan_speed,      0644),
    EEE_PROC_RO(fan_rpm,        0444),
    EEE_PROC_RW(fan_control,     0644),
    EEE_PROC_RO(temperature,    0444),
EEE_PROC_FILES_END

int netbook_proc_readfunc(char *buffer, char **buffer_location, off_t offset,
                      int buffer_length, int *eof, void *data)
{
    struct netbook_proc_file *procfile = (struct netbook_proc_file *)data;
    int bufpos = 0;

    if (!procfile || !procfile->readfunc) {
        return -EIO;
    }

    *eof = 1;
    if (offset > 0) {
        return 0;
    }

    (*procfile->readfunc)(buffer, buffer_length, &bufpos);
    return bufpos;
}

int netbook_proc_writefunc(struct file *file, const char *buffer,
                       unsigned long count, void *data)
{
    char userdata[129];
    int bufpos = 0;
    struct netbook_proc_file *procfile = (struct netbook_proc_file *)data;

    if (!procfile || !procfile->writefunc) {
        return -EIO;
    }

    if (copy_from_user(userdata, buffer, (count > 128) ? 128 : count)) {
        printk(KERN_DEBUG "%s: copy_from_user() failed\n", EEE_NAME);
        return -EIO;
    }
    userdata[128] = 0;      // So that sscanf() doesn't overflow...

    (*procfile->writefunc)(userdata, count, &bufpos);
    return count;
}

static void netbook_proc_cleanup(void) {
    int i;
    for (i = 0; netbook_proc_files[i].name; i++) {
        remove_proc_entry(netbook_proc_files[i].name, netbook_proc_rootdir);
    }
    remove_proc_entry("netbook", NULL);
}

#define WRITABLE(x) do { if(!writable) (x) &= 0x0577; } while(0)

static int __init netbook_proc_init(void) {
    int i;

    /* Create the /proc/netbook directory. */
    netbook_proc_rootdir = proc_mkdir("netbook", NULL);
    if (!netbook_proc_rootdir) {
        printk(KERN_ERR "%s: Unable to create /proc/netbook\n", EEE_NAME);
        return false;
    }
   /* netbook_proc_rootdir->owner = THIS_MODULE;*/

    /* Create the individual proc files but avoid
     * to create pll if i2c_adapter was not found
     */
    i = (netbook_pll_smbus_client.adapter)?0:1;
    for (; netbook_proc_files[i].name; i++) {
        struct proc_dir_entry *proc_file;
        struct netbook_proc_file *f = &netbook_proc_files[i];

	/* Protect dummy user to destroy they Eee PC for error */
	WRITABLE(f->mode);

        proc_file = create_proc_entry(f->name, f->mode, netbook_proc_rootdir);
        if (!proc_file) {
            printk(KERN_ERR "%s: Unable to create /proc/netbook/%s", EEE_NAME, f->name);
            goto proc_init_fail;
        }
        proc_file->read_proc = &netbook_proc_readfunc;
        if (f->writefunc) {
            proc_file->write_proc = &netbook_proc_writefunc;
        }
        proc_file->data = f;
      /*  proc_file->owner = THIS_MODULE;*/
        proc_file->mode = S_IFREG | f->mode;
        proc_file->uid = 0;
        proc_file->gid = 0;
    }
    return true;

    /* We had an error, so cleanup all of the proc files... */
proc_init_fail:
#if 0 /* using netbook_proc_cleanup seems to me better than duplicating code */
    for (; i >= 0; i--) {
        remove_proc_entry(netbook_proc_files[i].name, netbook_proc_rootdir);
    }
    remove_proc_entry("netbook", NULL);
#endif
    netbook_proc_cleanup();
    return false;
}


/*** Module initialization ***/

int __init init_module(void) {
	netbook_pll_init();
	if(netbook_proc_init()) {
		printk(KERN_NOTICE "%s version %s init sucessfully\n", 
			EEE_NAME, EEE_VERSION);
		if(writable)
			printk(KERN_NOTICE "%s informs writable=1 is dangerous"
				": think before writing, do not let your CPU burns out!\n", 
					EEE_NAME);
		return 0;
	}
	return -1;
}

void __exit cleanup_module(void) {
	netbook_pll_cleanup();
	netbook_proc_cleanup();
}
