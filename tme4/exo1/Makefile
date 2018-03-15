.PHONY: all 
#.SECONDARY:

ifneq ($(KERNELRELEASE),)

  obj-m := helloWorld.o

else
	
  KERNELDIR ?= /home/nicolas/Projects/pnl2018/linux-4.9.82/
  PWD := $(shell pwd)

all :
	make -C $(KERNELDIR) M=$(PWD) modules

clean:
	make -C $(KERNELDIR) M=$(PWD) clean

endif

