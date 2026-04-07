ifeq (,$(KDIR))
	KDIR := /lib/modules/$(shell uname -r)/build
endif
PWD := $(shell pwd)

obj-m += clk/
obj-m += gpio/
obj-m += hw_random/
obj-m += irqchip/
obj-m += pinctrl/
obj-m += rtc/
obj-m += serial/
obj-m += usb/
obj-m += watchdog/

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean

