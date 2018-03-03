#! /bin/bash

# Fixer les variables avec les chemins de vos fichiers
HDA="-hda pnl-tp.img"
HDB="-hdb myHome.img"
FLAGS="--enable-kvm "

exec qemu-system-x86_64 ${FLAGS} \
     ${HDA} ${HDB} \
     -net user -net nic \
     -boot c -m 2G
