
Phan
Nicolas
TP4

Exercice 1 : Mes premiers modules
--------------------------------------------------------------------------------

### Question 1

C'est le fichier objet helloworld.ko qui sera compilé et chargé dans le noyau.
Les autres ne sont que des fichiers temporaires intermédiaires.


Using the qemu-external-virtfs.sh with the pnl-tp-virtfs.img works!
All others combinaisons don't.
When I want to compile the module on the VM, there's a problem,
there is no /lib/modules/linux-4.9.81 !!!
I thought, well there is /lib/modules/linux-4.13, the version included in the pnl-tp.img, but there is no entry in /lib/modules for our external linux kernel (aka 4.9.81), I thought that the presence of such directory depends on the compiled linux image itself, maybe there's an option in make nconfig that says "hey, keep the sources in the image so that one can build new modules"
That's what I'll look for...

Well there are no options for that...
I tried to install the linux-headers-4.9.82 on the host to have /lib/modules/linux-4.9.82/everything and compile kernel modules on the host but there are no such packages on the debian repos.
I tried to install external linux-headers-4.9.82.deb packages from the Internet but apt can't install them since they refer to dependencies that are no in Universe...
I tried to build linux-headers from the sources I downloaded from www.kernel.org by using make kernel\_headers/ but it does not create /lib/modules/linux-4.9.82
Tried the same using make deb-pkg, and I indeed obtain a linux-headers-4.9.82.deb, and when I install it on the host I indeed get my /lib/modules/4.9.82-pnl/build/
YEEEEES ! I can compile kernel modules for 4.9.82 !!! (on the host, at least)

### Question 2

### Question 3

Bah on peut le lire sur la sortie standard deja, puis dans les messages kernel via `dmesg`
