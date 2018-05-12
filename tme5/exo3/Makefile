.PHONY: all 
#.SECONDARY:

ifneq ($(KERNELRELEASE),)

obj-m := hanging.o

else
	
  KERNELDIR ?= /lib/modules/4.9.82-pnl/build
  PWD := $(shell pwd)

all :
	$(info obj-m : $(obj-m))
	make -C $(KERNELDIR) M=$(PWD) modules

clean:
	make -C $(KERNELDIR) M=$(PWD) clean

endif
