LINUX_KSRC_MODULE = /lib/modules/$(shell uname -r)/kernel/drivers/net/wireless/
RTL819x_DIR = $(shell pwd)
KVER  = $(shell uname -r)
KSRC = /lib/modules/$(KVER)/build
RTL819x_FIRM_DIR = $(RTL819x_DIR)/firmware
HAL_SUB_DIR = rtl8192
MODULE_FILE = $(RTL819x_DIR)/rtllib/Module.symvers

export LINUX_KSRC_MODULE_DIR RTL819x_FIRM_DIR

all: 
ifeq ($(shell uname -r|cut -d. -f1,2), 2.4)
#	@make -C $(RTL819x_DIR)/rtllib
	@make -C $(RTL819x_DIR)/HAL/$(HAL_SUB_DIR) 
	@make -C $(RTL819x_DIR)/wapi_supplicant
else	
#	@make -C $(KSRC) SUBDIRS=$(RTL819x_DIR)/rtllib modules 
#	@test -f $(MODULE_FILE) && cp $(MODULE_FILE) $(RTL819x_DIR)/HAL/$(HAL_SUB_DIR) || echo > /dev/null 
	@make -C $(KSRC) SUBDIRS=$(RTL819x_DIR)/HAL/$(HAL_SUB_DIR) modules 
	#@make -C $(RTL819x_DIR)/wapi_supplicant
endif
install: all
	@cp $(RTL819x_DIR)/RadioPower.sh /etc/acpi/events/
	@cp $(RTL819x_DIR)/wireless-rtl-ac-dc-power.sh /etc/acpi/
	@make -C HAL/$(HAL_SUB_DIR)/ install
	#@cp $(RTL819x_DIR)/wapi_supplicant/wapi_supplicant /usr/bin/
uninstall:
	@make -C HAL/$(HAL_SUB_DIR)/ uninstall
	@rm -f /etc/acpi/wireless-rtl-ac-dc-power.sh
	@rm -f /usr/bin/wapi_supplicant
clean:
	@make -C HAL/$(HAL_SUB_DIR)/ clean
	@make -C rtllib/ clean
	@test -r mshclass/ && make -C mshclass/ clean|| echo > /dev/null 
	@test -r wapi_supplicant/ && make -C wapi_supplicant/ clean|| echo > /dev/null 
	@rm -fr Modules.symvers
	@rm -fr Module.symvers
	@rm -fr Module.markers
	@rm -fr modules.order
	@rm -fr *~
