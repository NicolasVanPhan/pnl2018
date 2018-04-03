
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
Il y a deux makefile qui interviennent lorsqu'on compile un module kernel
 - Le Makefile spécifique au kernel
 - Le Makefile générique de compilation de kernel, dans les sources du noyau

 1. Le Makefile spécifique appelle le Makefile général pour compiler tous les modules en donnant à ce-dernier le chemin vers nous, le Makefile du module.
 2. Le Makefile général s'exécute, et en compilant les modules, il appelle notre Makefile (car on lui a donné notre chemin via `-M pwd`.
 3. Lorsqu'il se fait appeler, notre Makefile va ajouter notre module à la liste des modules à compiler (via obj-m), puis il se termine.
 4. On retourne au Makefile générique qui effectue la compilation de tous les modules, a fortiori du notre.

Voila !!!

### Question 3

Bah on peut le lire sur la sortie standard deja, puis dans les messages kernel via `dmesg`

### Question 4

c.f. code

### Question 5

Un `modinfo helloWorldParam.ko` affiche les infos sur notre module... 

### Question 6

sysfs est un système de fichiers virtuel servant d'interface avec le noyau.
Autrement dit, c'est un ensemble de fichier contenus dans /sys et via lesquels on peut paramétrer le noyau ou récupérer des informations de celui-ci en écrivant/lisant dans ces fichiers.
Ces fichiers ne sont pas réellement un ensemble de données présents sur le disque mais seulement une interface avec le kernel qui se présente sous la forme d'un ensemble de fichiers (d'où le "virtuel").

Chaque système de fichiers virtuels (procfs, sysfs...) a sa propre arborescence, ses propres fichiers et apprendre ces arborescences et fichiers, c'est apprendre à interagir avec le kernel.

Ici avec sysfs, pour modifier un paramètre d'un module, il faut aller éditer le fichier /sys/modules/mymodule/parameters/myparam qui contient la valeur du paramètre en question. Attention, pour que ce fichier existe et soit modifiable, il faut avoir pris soin de définir ses droits d'accès en troisième argument de module\_param().

Exercice 2 : Mofification d'une variable du noyau à l'aide d'un module 
--------------------------------------------------------------------------------

### Question 1

La variable globale `init_uts_ns` est une structure initialisée dans `/init/version.c` et contenant les champs :
 - `kref` : une structure générique permettant de compter le nombre de références à un objet.
 - `name` : The very name that `uname` should return.

Cette variable est exportée via la macro EXPORT\_SYMBOL\_GPL() donc seuls les modules sous licence GPL peuvent y accéder.

### Question 2

c.f. code

### Question 3

Le nom d'origine doit etre restauré car de manière générale, un module doit restaurer les modification qu'il a apportées au noyau lors de son chargement, cela permet de préserver l'intégrité du noyau a travers les chargements/déchargements de modules.

Exercice 3 : Les limites des modules 
--------------------------------------------------------------------------------

### Question 1

En soi le module en question est faisable, mais le problème est qu'on ne peut pas utiliser la fonction iterate\_supers() car son auteur ne l'a pas exporté (dans linux/fs/super.c, là où la fonction est déclarée, l'auteur n'a pas utilisé EXPORT\_SYMBOL(iterate\_supers).

Ce problème ne peut pas etre résolu par un module car il s'agit ici de modifier le code source d'un module déjà existant (ici ajouter EXPORT\_SYMBOL(iterate\_supers) dans super.c), seul un patch peut résoudre le problème.

### Question 2

Ici, il faut ajouter un champ de type `struct timespec` dans `struct super_block` pour enregistrer les dates des derniers affichages.
Après, il suffit d'itéter avec `iterate_supers_type()` au lieu de `iterate_supers()`, sinon le code reste sensiblement le meme.

