/*
 *  asus_eee.c - Asus eeePC extras
 *  http://code.google.com/p/eeepc-linux/
 *
 *  Copyright (C) 2007 Andrew Tipton
 *  Copyright (C) 2008 Jean Fabrice
 *  Copyright (C) 2008 Roberto A. Foglietta
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
#include <linux/pci.h>

struct pci_device_id eeepc_pci_tbl[] = {
	{
		.vendor			= PCI_ANY_ID,
		.device			= PCI_ANY_ID,
		.subvendor		= PCI_VENDOR_ID_ASUSTEK,
		.subdevice		= 0x82d9, // EeePC 700, 900
	},{
		.vendor			= PCI_ANY_ID,
		.device			= PCI_ANY_ID,
		.subvendor		= PCI_VENDOR_ID_ASUSTEK,
		.subdevice		= 0x8340, // EeePC 900A,
	},{
		.vendor			= PCI_ANY_ID,
		.device			= PCI_ANY_ID,
		.subvendor		= PCI_VENDOR_ID_ASUSTEK,
		.subdevice		= 0x830f, // EeePC 901, 1000
	},{
	}
};

/* Module info */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Tipton, Jean Fabrice, Roberto A. Foglietta");
MODULE_DESCRIPTION("Support for eeePC-specific functionality.");

#define EEE_VERSION "0.3"
#define EEE_NAME "asus_eee"

MODULE_VERSION(EEE_VERSION);
MODULE_INFO(module_depends, "i2c-i801");
MODULE_DEVICE_TABLE(pci, eeepc_pci_tbl);

static int writable = 0;
module_param(writable, int, 0644);
MODULE_PARM_DESC(insanelev, "Some functioning parameters" 
	"could be changed but risky");

/* PLL access functions.
 *
 * Note that this isn't really the "proper" way to use the I2C API... :)
 * I2C_SMBUS_BLOCK_MAX is 32, the maximum size of a block read/write.
 */
static bool eee_pll_init(void);
static bool eee_pll_read(void);
static bool eee_pll_write(void);
static void eee_pll_cleanup(void);

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

static struct i2c_client eee_pll_smbus_client = {
	.adapter = NULL,
	.addr = 0x69,
	.flags = 0,
};
static char eee_pll_data[I2C_SMBUS_BLOCK_MAX];
static int eee_pll_datalen = 0;

static bool eee_pll_init(void) {
	/* this module depends on this symbol */
	if(__symbol_get("i2c_get_adapter"))
		eee_pll_smbus_client.adapter = i2c_get_adapter(0);

	if(!eee_pll_smbus_client.adapter) {
		printk(KERN_WARNING 
			"%s module requires i2c-i801 module at load time" 
				" if you like to access pll via proc too\n", 
					EEE_NAME);
		return false;
	} 

	/* Fill the eee_pll_data buffer */
	return eee_pll_read();
}

static bool reinit_ifneeded(void) {
	if(!eee_pll_smbus_client.adapter) {
		if(!eee_pll_init()) {
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
static bool eee_pll_read(void) {
	if(!reinit_ifneeded()) return false;
	memset(eee_pll_data, 0, I2C_SMBUS_BLOCK_MAX);
	eee_pll_datalen = i2c_smbus_read_block_data(&eee_pll_smbus_client, 
		0, eee_pll_data);
	return true;
}

// Takes approx 150ms to execute ???
static bool eee_pll_write(void) {
	if(!reinit_ifneeded()) return false;
	i2c_smbus_write_block_data(&eee_pll_smbus_client, 
		0, eee_pll_datalen, eee_pll_data);
	return true;
}

static void eee_pll_cleanup(void) {
	if(eee_pll_smbus_client.adapter)
		i2c_put_adapter(eee_pll_smbus_client.adapter);
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
static DEFINE_MUTEX(eee_ec_mutex);

static unsigned char eee_ec_read(unsigned short addr) {
    unsigned char data;

    mutex_lock(&eee_ec_mutex);
    outb(HIGH_BYTE(addr), EC_IDX_ADDRH);
    outb(LOW_BYTE(addr), EC_IDX_ADDRL);
    data = inb(EC_IDX_DATA);
    mutex_unlock(&eee_ec_mutex);

    return data;
}

static void eee_ec_write(unsigned short addr, unsigned char data) {
    mutex_lock(&eee_ec_mutex);
    outb(HIGH_BYTE(addr), EC_IDX_ADDRH);
    outb(LOW_BYTE(addr), EC_IDX_ADDRL);
    outb(data, EC_IDX_DATA);
    mutex_unlock(&eee_ec_mutex);
}

#define EC_GPIO_PORT 0xFC20
#define EC_GPIO_PIN2PORT(pin) (EC_GPIO_PORT + (((pin) >> 3) & 0x1F))
#define EC_GPIO_PIN2MASK(pin) (1 << ((pin) & 0x07))

static void eee_ec_gpio_set(int pin, int value) {
    unsigned short port;
    unsigned char mask;

    port = EC_GPIO_PIN2PORT(pin);
    mask = EC_GPIO_PIN2MASK(pin);
    if (value) {
        eee_ec_write(port, eee_ec_read(port) | mask);
    } else {
        eee_ec_write(port, eee_ec_read(port) & ~mask);
    }
}

static int eee_ec_gpio_get(int pin) {
    unsigned short port;
    unsigned char mask;
    unsigned char status;

    port = EC_GPIO_PIN2PORT(pin);
    mask = EC_GPIO_PIN2MASK(pin);
    status = eee_ec_read(port) & mask;

    return (status) ? 1 : 0;
}

/*** Fan and temperature functions ***/
#define EC_ST00 0xF451          // Temperature of CPU (C)
#define EC_SC02 0xF463          // Fan PWM duty cycle (%)
#define EC_SC05 0xF466          // High byte of fan speed (RPM)
#define EC_SC06 0xF467          // Low byte of fan speed (RPM)
#define EC_SFB3 0xF4D3          // Flag byte containing SF25 (FANctrl)
#define MANUAL_FAN_BIT 0x02

static unsigned int eee_get_temperature(void) {
    return eee_ec_read(EC_ST00);
}

static unsigned int eee_fan_get_rpm(void) {
    return (eee_ec_read(EC_SC05) << 8) | eee_ec_read(EC_SC06);
}

/* 1 if fan is in manual mode, 0 if controlled by the EC */
static int eee_fan_get_manual(void) {
    return (eee_ec_read(EC_SFB3) & MANUAL_FAN_BIT) ? 1 : 0;
}

static void eee_fan_set_manual(int manual) {
    if (manual) {
        /* SF25=1: Prevent the EC from controlling the fan */
        eee_ec_write(EC_SFB3, eee_ec_read(EC_SFB3) | MANUAL_FAN_BIT);
    } else {
        /* SF25=0: Allow the EC to control the fan */
        eee_ec_write(EC_SFB3, eee_ec_read(EC_SFB3) & ~MANUAL_FAN_BIT);
    }
}

static void eee_fan_set_speed(unsigned int speed) {
    eee_ec_write(EC_SC02, (speed > 100) ? 100 : speed);
}

static unsigned int eee_fan_get_speed(void) {
    return eee_ec_read(EC_SC02);
}


/*** Voltage functions ***/
#define EC_VOLTAGE_PIN 0x66

enum eee_voltage { Low=0, High=1 };
static enum eee_voltage eee_get_voltage(void) {
    return eee_ec_gpio_get(EC_VOLTAGE_PIN);
}
static void eee_set_voltage(enum eee_voltage voltage) {
    eee_ec_gpio_set(EC_VOLTAGE_PIN, voltage);
}

/*** FSB functions ***/
#define FSB_MSB_INDEX 11
#define FSB_LSB_INDEX 12
#define FSB_MSB_MASK 0x3F
#define FSB_LSB_MASK 0xFF

static void eee_get_freq(int *n, int *m) {
    *m = eee_pll_data[FSB_MSB_INDEX] & FSB_MSB_MASK;
    *n = eee_pll_data[FSB_LSB_INDEX];
}

static void eee_set_freq(int n, int m) {
    int current_n = 0, current_m = 0;
    eee_get_freq(&current_n, &current_m);
    if (current_n != n || current_m != m) {
        eee_pll_data[FSB_MSB_INDEX] = m & FSB_MSB_MASK;
        eee_pll_data[FSB_LSB_INDEX] = n & FSB_LSB_MASK;
        eee_pll_write();
    }
}

/*** /proc file functions ***/

static struct proc_dir_entry *eee_proc_rootdir;
#define EEE_PROC_READFUNC(NAME) \
    void eee_proc_readfunc_##NAME (char *buf, int buflen, int *bufpos)
#define EEE_PROC_WRITEFUNC(NAME) \
    void eee_proc_writefunc_##NAME (const char *buf, int buflen, int *bufpos)
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
    static struct eee_proc_file eee_proc_files[] = {
#define EEE_PROC_RW(NAME, MODE) \
    { #NAME, MODE, &eee_proc_readfunc_##NAME, &eee_proc_writefunc_##NAME }
#define EEE_PROC_RO(NAME, MODE) \
    { #NAME, MODE, &eee_proc_readfunc_##NAME, NULL }
#define EEE_PROC_FILES_END \
    { NULL, 0, NULL, NULL } };

struct eee_proc_file {
    char *name;
    int mode;
    void (*readfunc)(char *buf, int buflen, int *bufpos);
    void (*writefunc)(const char *buf, int buflen, int *bufpos);
};


EEE_PROC_READFUNC(fsb) {
    int n = 0;
    int m = 0;
    int voltage = 0;
    eee_get_freq(&n, &m);
    voltage = (int)eee_get_voltage();
    EEE_PROC_PRINTF("%d %d %d\n", n, m, voltage);
}

EEE_PROC_WRITEFUNC(fsb) {
    int n = 70;     // sensible defaults
    int m = 24;
    int voltage = 0;
    EEE_PROC_SCANF(3, "%i %i %i", &n, &m, &voltage);
    eee_set_freq(n, m);
    eee_set_voltage(voltage);
}

EEE_PROC_READFUNC(pll) {
    eee_pll_read();
    EEE_PROC_MEMCPY(eee_pll_data, eee_pll_datalen);
}

EEE_PROC_READFUNC(fan_speed) {
    int speed = eee_fan_get_speed();
    EEE_PROC_PRINTF("%d\n", speed);
}

EEE_PROC_WRITEFUNC(fan_speed) {
    unsigned int speed = 0;
    EEE_PROC_SCANF(1, "%u", &speed);
    eee_fan_set_speed(speed);
}

EEE_PROC_READFUNC(fan_rpm) {
    int rpm = eee_fan_get_rpm();
    EEE_PROC_PRINTF("%d\n", rpm);
}

EEE_PROC_READFUNC(fan_manual) {
    EEE_PROC_PRINTF("%d\n", eee_fan_get_manual());
}

EEE_PROC_WRITEFUNC(fan_manual) {
    int manual = 0;
    EEE_PROC_SCANF(1, "%i", &manual);
    eee_fan_set_manual(manual);
}

#if 0
EEE_PROC_READFUNC(fan_mode) {
    enum eee_fan_mode mode = eee_fan_get_mode();
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
    enum eee_fan_mode mode = Automatic;
    char inputstr[16];
    EEE_PROC_SCANF(1, "%15s", inputstr);
    if (strcmp(inputstr, "manual") == 0) {
        mode = Manual;
    } else if (strcmp(inputstr, "auto") == 0) {
        mode = Automatic;
    } else if (strcmp(inputstr, "embedded") == 0) {
        mode = Embedded;
    }
    eee_fan_set_mode(mode);
}
#endif

EEE_PROC_READFUNC(temperature) {
    unsigned int t = eee_get_temperature();
    EEE_PROC_PRINTF("%d\n", t);
}

EEE_PROC_FILES_BEGIN
    EEE_PROC_RO(pll,            0400),	/* this has to be the first */
    EEE_PROC_RW(fsb,            0644),
    EEE_PROC_RW(fan_speed,      0644),
    EEE_PROC_RO(fan_rpm,        0444),
    EEE_PROC_RW(fan_manual,     0644),
    EEE_PROC_RO(temperature,    0444),
EEE_PROC_FILES_END

int eee_proc_readfunc(char *buffer, char **buffer_location, off_t offset,
                      int buffer_length, int *eof, void *data)
{
    struct eee_proc_file *procfile = (struct eee_proc_file *)data;
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

int eee_proc_writefunc(struct file *file, const char *buffer,
                       unsigned long count, void *data)
{
    char userdata[129];
    int bufpos = 0;
    struct eee_proc_file *procfile = (struct eee_proc_file *)data;

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

static void eee_proc_cleanup(void) {
    int i;
    for (i = 0; eee_proc_files[i].name; i++) {
        remove_proc_entry(eee_proc_files[i].name, eee_proc_rootdir);
    }
    remove_proc_entry("eee", NULL);
}

#define WRITABLE(x) do { if(!writable) (x) &= 0x0577; } while(0)

static int __init eee_proc_init(void) {
    int i;

    /* Create the /proc/eee directory. */
    eee_proc_rootdir = proc_mkdir("eee", NULL);
    if (!eee_proc_rootdir) {
        printk(KERN_ERR "%s: Unable to create /proc/eee\n", EEE_NAME);
        return false;
    }
    eee_proc_rootdir->owner = THIS_MODULE;

    /* Create the individual proc files but avoid
     * to create pll if i2c_adapter was not found
     */
    i = (eee_pll_smbus_client.adapter)?0:1;
    for (; eee_proc_files[i].name; i++) {
        struct proc_dir_entry *proc_file;
        struct eee_proc_file *f = &eee_proc_files[i];

	/* Protect dummy user to destroy they Eee PC for error */
	WRITABLE(f->mode);

        proc_file = create_proc_entry(f->name, f->mode, eee_proc_rootdir);
        if (!proc_file) {
            printk(KERN_ERR "%s: Unable to create /proc/eee/%s", EEE_NAME, f->name);
            goto proc_init_fail;
        }
        proc_file->read_proc = &eee_proc_readfunc;
        if (f->writefunc) {
            proc_file->write_proc = &eee_proc_writefunc;
        }
        proc_file->data = f;
        proc_file->owner = THIS_MODULE;
        proc_file->mode = S_IFREG | f->mode;
        proc_file->uid = 0;
        proc_file->gid = 0;
    }
    return true;

    /* We had an error, so cleanup all of the proc files... */
proc_init_fail:
#if 0 /* using eee_proc_cleanup seems to me better than duplicating code */
    for (; i >= 0; i--) {
        remove_proc_entry(eee_proc_files[i].name, eee_proc_rootdir);
    }
    remove_proc_entry("eee", NULL);
#endif
    eee_proc_cleanup();
    return false;
}


/*** Module initialization ***/

int __init init_module(void) {
	eee_pll_init();
	if(eee_proc_init()) {
		printk(KERN_NOTICE "%s version %s init sucessfully\n", 
			EEE_NAME, EEE_VERSION);
		if(writable)
			printk(KERN_NOTICE "%s informs writable=1 is dangerous"
				": think before writing, do not let your CPU burns out!\n", 
					EEE_NAME);
		return true;
	}
	return false;
}

void __exit cleanup_module(void) {
	eee_pll_cleanup();
	eee_proc_cleanup();
}
