#! /bin/bash

# Fixer les variables avec les chemins de vos fichiers
HDA="-hda /home/nicolas/Projects/pnl2018/sopena/pnl-tp.img"
HDB="-hdb /home/nicolas/Projects/pnl2018/sopena/myHome.img"
KERNEL=/home/nicolas/Projects/pnl2018/linux-4.9.82/arch/x86/boot/bzImage

CMDLINE='root=/dev/sda1 rw vga=792 console=ttyS0 kgdboc=ttyS1 init=/bin/bash'

FLAGS="--enable-kvm "

exec qemu-system-x86_64 ${FLAGS} \
     ${HDA} ${HDB} \
     -net user -net nic \
     -serial stdio \
     -boot c -m 2G \
     -kernel "${KERNEL}" -append "${CMDLINE}"

