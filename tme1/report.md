
Phan
Nicolas
PNL 2018

Rapport de TME1
================================================================================

Exercice 1
--------------------------------------------------------------------------------

#### Question 1

Toutes les versions sont stables !!! Normalement seules les versions paires devraient l'etre, ce n'est pas correct.

#### Question 2

 - On a v, une variable qui contient un nombre a priori quelconque.
 - Lorsqu'on cast v en (char\*), on indique au compilateur que v est un pointeur vers la premiere case d'un tableau de char, cela aura comme influence que si on incremente v ou si on utilise la notation v[42], alors le compilateur ajoutera 1 * (sizeof(char)) ou 42 * sizeof(char) a v.
 - Ensuite, apres le cast, on accede a v[sizeof(unsigned short)] = v[1] = v + 1 \* sizeof(char) = v + 1. On fait ca parce que dans la memoire, on a la structure suivante :

 | adresse	| variable	| commentaire		  |
 |--------------|---------------|-------------------------|
 | n		| major		| debut structure version | 
 | n + 1	| minor		| v[1] 			  |
 | n + 2	| minor		| v[2] 			  |
 | n + 3	| minor		| 			  |
 | n + 4	| minor		| 			  |
 | n + 5	| minor		| 			  |
 | n + 6	| minor		| 			  |
 | n + 7	| minor		| 			  |
 | n + 8	| minor		| 			  |
 | n + 9	| flags		|			  |

 Sauf que ce tableau est errone, car en memoire les informations sont organises en blocs (ici blocs de 8 octets, 8 adresses) et par souci pratique, le compilateur fait en sorte que les variables ne soient pas à cheval entre deux blocs donc la vraie organisation est la suivante :

 | adresse	| variable	| commentaire		  |
 |--------------|---------------|-------------------------|
 | n		| major		| debut structure version | 
 | n + 1	| nothing	| v[1] /!\		  |
 | n + 2	| nothing	| v[2]			  | 
 | n + 3	| nothing	| v[3]			  | 
 | n + 4	| nothing	| v[4]			  | 
 | n + 5	| nothing	| v[5]			  | 
 | n + 6	| nothing	| v[6]			  | 
 | n + 7	| nothing	| v[7]			  | 
 |--------------|---------------|-------------------------|
 | n + 8	| minor 	| v[8] 			  | 
 | n + 9	| minor		| ... 			  | 
 | n + 10	| minor		| 			  | 
 | n + 11	| minor		| 			  | 
 | n + 12	| minor		| 			  | 
 | n + 13	| minor		| 			  | 
 | n + 14	| minor		| 			  | 
 | n + 15	| minor		| 			  | 
 |--------------|---------------|-------------------------|
 | n + 16	| flags		| 			  | 
 | n + 17	| nothing	| 			  | 
 | n + 18	| nothing	| 			  | 
 | n + 19	| nothing	| 			  | 
 | n + 20	| nothing	| 			  | 

 Ainsi en réalité, v[1] pointe sur une case mémoire contenant une valeur non assignée, autrement dit n'importe quoi.

#### Question 5

L'empreinte mémoire de struct version est de 24 octets, soit 3 blocs mémoire.
Cela rejoint la représentation du tableau ci-dessus.

