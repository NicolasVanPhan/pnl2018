
Exercice 2 : Récupération automatique de la mémoire, le *shrinker*
--------------------------------------------------------------------------------

### Question 1

L'ajout d'un shrinker nécessite l'implémentation de deux fonctions `count()`
et `scan()`, qui permettent respectivement de compter le nombre d'éléments
de la liste en question et de libérer le maximum (jusqu´à `nr_to_scan`)
d'éléments possibles de la liste.

Exercice 3 : Gestion efficace de la mémoire avec les slabs 
--------------------------------------------------------------------------------

### Question 1

Alors que `sizeof()` donne la taille que prend théoriquement un objet en
mémoire, `ksize()` donne la taille réservée par `kmalloc()` pour une
instance de l'objet.
La taille (en octets) renvoyée par `ksize()` peut être plus grande que celle
renvoyée par `sizeof()` à cause du comportement de `kmalloc()`, en effet cette
fonction alloue parfois plus d'espace que ce que l'utilisateur a demandé 
en argument.

Ceci est du au fait que la taille (en octets) d'une structure peut être
n'importe quel entier tandis que la taille de l'espace alloué par `kmalloc()`
est dans un ensemble bien défini : {8, 16, 32, 64, 96, 128, 192, 256, 512,
1024, 2048, 4096, 8192}, on peut voir cet ensemble via `cat /proc/slabinfo`.
Ainsi, `kmalloc(128)` alloue 128 octets mais `kmalloc(129)` en allour 192,
plus que nécessaire.

### Question 2

Le slab est une technique de gestion de la mémoire consistant à pré-allouer
de grandes plages mémoires (aka les slabs) au démarrage de la machine
puis en donner des tranches à la demande, à certaines taches lorsqu'elles ont
besoin de mémoire
Ainsi, lorsqu'une tache veut de la place en mémoire pour un nouvel objet,
elle effectue une requête vers le gestionnaire de slabs qui lui fournira
un zone d'un slab au lieu d'allouer elle-même via `kmalloc()`.
Cela permet non seulement d'êviter la fragmentation de la mémoire due
à l'aspect aléatoire de l'adresse retournée par `kmalloc()` mais surtout
de supprimer les espaces vides résultants de la "sur-allocation"
de `kmalloc()` vue à la Question 1.
