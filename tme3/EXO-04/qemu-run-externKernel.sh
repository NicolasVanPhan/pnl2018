#! /bin/bash

# Fixer les variables avec les chemins de vos fichiers
HDA=/home/nicolas/Projects/pnl2018/pnl-tp.img
HDB=/home/nicolas/Projects/pnl2018/myHome.img
KERNEL=/home/nicolas/Projects/pnl2018/linux-4.9.82/arch/x86/boot/bzImage

# Si besoin ajouter une option pour le kernel
CMDLINE='root=/dev/sda1 rw vga=792 console=ttyS0 kgdboc=ttyS1 init=bash'

exec qemu-system-x86_64 -hda "${HDA}" -hdb "${HDB}" -chardev stdio,signal=off,id=serial0 -serial chardev:serial0 -serial tcp::1234,server,nowait -net nic -net user -boot c -m 2G -kernel "${KERNEL}" -append "${CMDLINE}"

