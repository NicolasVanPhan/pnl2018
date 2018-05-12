
Exercice 1 : Connection du débogueur KGDB
--------------------------------------------------------------------------------

### Question 2

On connecte KGDB de la VM à GDB la machine hote via les sockets tcp,
ici la connexion à kgdb se fait à l'adresse localhost sur le port 1234.

### Question 4

Lorsqu'on invoque `info threads`, c'est gdb lui-même qui affiche la liste des threads qu'il connait
à ce jour, tandis qu'avec `monitor ps`, gdb envoie la commande `ps` à kgdb de la machine virtuelle
et en récupère le résultat.

### Question 5

La commande continue permet de reprendre l'exécution de la VM.

### Bilan : Procédure de mist en place d'une session de debug noyau :

Lorsqu'on exécute le script qemu-run-external-virtfs.sh, la VM démarre puis KGDB se lance et se fige, bloquant la VM. À partir de ce moment là, KGDB attend qu'on s'y connecte depuis la machine hote (c'est dû à l'option kgdbwait).

Dans un premier terminal :
```bash
./qemu-virtfs-external.sh
```

Puis, sur la machine hote, on lance gdb, et on se connecte à kgdb de la VM grace à la commande `target remote localhost:1234` (on a choisi le port 1234 par convention).

Ouvrir un deuxième terminal et y lancer gdb :
```bash
gdb linux-4.9.82/vmlinux
```
Dans gdb, se connecter à kgdb :
```bash
target remote localhost:1234
```
Une fois connectés, nous allons souvent switcher entre le mode où le kernel sur la VM tourne et GDB est figé, et le mode inverse où la VM est figée et GDB s'exécute. Pour reprendre l'exécution du kernel sur la VM, il faut utiliser `continue` depuis GDB, et pour repasser la main à GDB, il faut utiliser `echo g > /proc/sysrq-trigger`.

Dans gdb, résumer l'exécution du noyau :
```bash
continue
```

Exercice 2 : Prise en main de KGDB
--------------------------------------------------------------------------------

### Question 1

La variable globale `init_uts_ns` est une instance de la structure `uts_namespace` déclarée et définie dans `/init/version.c`, elle contient l'ensemble des informations sur le noyau affichées par la commande `uname` (c'est-à-dire le nom du noyau, sa version, le nom de la machine...)

### Question 2

La commande gdb `print init_uts_ns.name.sysname` permet d'afficher le nom du noyau,
et `set init_uts_ns.name.sysname = "Toto"` permet de renommer le noyau en "Toto".


Exercice 3 : Mon premier bug
--------------------------------------------------------------------------------

### Question 1

Le module hanging lance à son chargement un nouveau thread noyau appelé `my_hanging_fn` qui exécute
la fonction éponyme, et à son déchargement, elle stoppe le thread s'il est toujours existant.

La fonction `my_hanging_fn()`, exécutée par le thread crée, se contente de s'endormir pendant 60 secondes,
en ignorant les interruptions, avant de se terminer en renvoyant 0.

### Question 2

Le noyau ne crash pas mais affiche un message d'information indiquant que le thread lancé est resté inactif pendant 30 secondes.

### Question 3

L'option a cocher est Kernel Hacking -> Debug Lockups and Hangs -> Panic (Reboot) on Hung Tasks

### Question 4

L'ensemble des fonctions présentes dans la pile affichée par `backtrace` ne correspond pas au code de notre module car la commande `backtrace` affiche la pile du thread `khungtaskd`, un tache d'arrière-plan s'occupant des taches endormies, au lieu de la pile du thread lancé (my\_hanging\_fn).
Ceci est du au fait que lorsque la kernel panic a eue lieu, le thread en cours d'exécution était le daemon `khungtaskd` et `backtrace` affiche par défaut la pile du thread en cours.

### Question 5

La commande `monitor ps` nous permet d'obtenir le résultat de la commande `ps` à travers gdb et ainsi connaitre le processus en cours mais auss le PID du processus my\_hanging\_fn.
Puis `monitor btp PID` permet d'afficher la pile du processus d'id PID.

On y retrouve bien la fonction my\_hanging\_fn() appelant schedule\_timeout() appelant schedule()...

### Question 6

La seconde adresse est l'adresse de base du segment mémoire contenant le code du module + les variables globales.

Cette adresse se retrouve à partir de la `struct module` via `mystructmodule.core_layout.base`

### Question 7

Une solution ne modifiant que le code du module est d'attendre la minute par pas de 20 secondes < 30 secondes, autrement dit exécuter trois `schedule_timeout(20*HZ)`.
Une solution ne modifiant que la configuration du noyau est de décocher l'option Kernel Hacking -> Debug Lockups and Hangs -> Detect Hung Tasks.

Exercice 4 : Affichages dynamiques
--------------------------------------------------------------------------------

### Question 1

Aucun message n'apparait dans les logs du noyau parce que d'après la documentation (dynamic-debug-howto.txt),
par défaut, les messages émis par `pr_debug()` ne sont pas affichés.
Pour activer l'affichage des `pr_debug()` d'un fichier toto.txt, il faut le signaler à l'OS
via l'interface debugfs en écrivant "toto.c +p" dans `my_debugfs_mountpoint/dynamic_debug/control`.

### Question 2

Cette commande écrit "module prdebug +p" dans le fichier de control de l'affichage debug dynamique.
`+p` avec p comme print, cela active l'affichage des message print avec `pr_debug()` pour le module
prdebug.

### Question 3

Pour activer l'affichage du nom du module, du nom des fonctions et des numéros de ligne, il faut
ajouter les flags `m`, `f` et `l` respectivement. Ainsi la commande devient :

```bash
echo -n "module prdebug +pmfl" > path_to_debugfs/dynamic_debug/control
```

### Question 4

Pour afficher uniquement le message indiquant le nombre d'interruptions (c'est-à-dire le message
de la ligne 13 du code du module), il faut préciser ce numéro de ligne dans la cible des flags :
```bash
echo -n "module prdebug line:13 +pmfl" > path_to_debugfs/dynamic_debug/control
```

Et si le print était déja actif pour tout le fichier, il faut le désactiver avant :
```bash
echo -n "module prdebug -p" > path_to_debugfs/dynamic_debug/control
```

Exercice 5 : Débogage d'un module 
--------------------------------------------------------------------------------

### Question 2

Au bout de quelques secondes, le thread exécutant `my_kcpustat_fn()` provoque une
erreur de segmentation.

### Question 3

La commande gdb `monitor lsmod` renvoie :
```
Module                  Size  modstruct     Used by
kcpustat                2342  0xffffffffa00004c0    0  (Live) 0xffffffffa0000000 [ ]
```
Cela nous indique notamment l'adresse où charger les symboles de notre module
(ici 0xffffffffa0000000).
Il ne reste plus qu'à les charger avec : `add-symbol-file path/to/my_module.ko 0xffffffffa0000000`.

### Question 4

L'erreur se situe à la ligne 75, la ligne contenant l'addition permettant de sommer les dernières stats enregistrées. 
