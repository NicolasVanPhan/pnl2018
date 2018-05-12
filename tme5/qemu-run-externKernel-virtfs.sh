#! /bin/bash

# Fixer les variables avec les chemins de vos fichiers
HDA="-hda /home/nicolas/Projects/pnl2018/sopena/pnl-tp-virtfs.img"
HDB="-hdb /home/nicolas/Projects/pnl2018/sopena/myHome.img"
KERNEL=/home/nicolas/Projects/pnl2018/linux-4.9.82/arch/x86/boot/bzImage
KDB=lavie

if [ -n "${KDB}" ]; then
    KGD_WAIT='kgdbwait'
fi

CMDLINE="root=/dev/sda1 rw vga=792 console=ttyS0 kgdboc=ttyS1 ${KGD_WAIT}"

FLAGS="--enable-kvm "
VIRTFS+=" --virtfs local,path=/home/nicolas/Projects/pnl2018/share/,mount_tag=share,security_model=passthrough,id=share "

exec qemu-system-x86_64 ${FLAGS} \
     ${HDA} ${HDB} \
     ${VIRTFS} \
     -net user -net nic \
     -serial stdio -serial tcp::1234,server,nowait \
     -boot c -m 1G \
     -kernel "${KERNEL}" -append "${CMDLINE}"
