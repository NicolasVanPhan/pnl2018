#! /bin/bash

# Fixer les variables avec les chemins de vos fichiers
HDA="-hda /home/nicolas/Projects/pnl2018/sopena/pnl-tp.img"
HDB="-hdb /home/nicolas/Projects/pnl2018/sopena/myHome.img"
FLAGS="--enable-kvm "

exec qemu-system-x86_64 ${FLAGS} \
     ${HDA} ${HDB} \
     -net user -net nic \
     -boot c -m 2G
