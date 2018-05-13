
Exercice 3 : Manipulation des processus dans le noyau Linux
--------------------------------------------------------------------------------

### Question 1

La `struct pid` est un moyen de garder une référence vers un processus en
évitant les conflits (garder seulement un PID pose des conflits si un
processus meurt et un autre prend son PID) et tout en restant économe
en mémoire (une deuxième solution consisterait à garder une `struct task_struct`
mais cela prend beaucoup de place en mémoire).

### Question 2

Les champs `utime` et `stime` contiennent des durées en cputime, une unité
interne au système, tout comme le tick.
Le nombre de cputime dans une seconde est défini par la macro `CPUTIME_PER_SEC`.
