.PHONY: all 
#.SECONDARY:

ifneq ($(KERNELRELEASE),)

obj-m := helloioctl.o

else
	
  KERNELDIR ?= /lib/modules/4.9.82-pnl/build
  PWD := $(shell pwd)

all :
	$(info obj-m : $(obj-m))
	make -C $(KERNELDIR) M=$(PWD) modules
	gcc user.c -o user

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	rm user

endif
