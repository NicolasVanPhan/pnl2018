#! /bin/bash

# Fixer les variables avec les chemins de vos fichiers
HDA=/home/nicolas/Projects/pnl2018/pnl-tp.img
HDB=/home/nicolas/Projects/pnl2018/myHome.img

exec qemu-system-x86_64 -hda "${HDA}" -hdb "${HDB}" -m 2G -net nic -net user -boot c

