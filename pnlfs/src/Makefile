#
# Makefile for the pnlfs filesystem
#

.PHONY: all 
#.SECONDARY:

ifneq ($(KERNELRELEASE),)

obj-m += pnlfs.o

pnlfs-objs := super.o structs.o iops.o fops.o inode.o file.o dir.o misc.o
ccflags-y := -DDEBUG -O0

else
	
  KERNELDIR ?= /lib/modules/4.9.82-pnl/build
  PWD := $(shell pwd)

all :
	$(info obj-m : $(obj-m))
	make -C $(KERNELDIR) M=$(PWD) modules

clean:
	make -C $(KERNELDIR) M=$(PWD) clean

endif
