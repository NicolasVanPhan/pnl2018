#include<stdlib.h>
#include<stdio.h>

#include"commit.h"

static int nextId;

/**
  * new_commit - alloue et initialise une structure commit correspondant aux
  *              parametres
  *
  * @major: numero de version majeure
  * @minor: numero de version mineure
  * @comment: pointeur vers une chaine de caracteres contenant un commentaire
  *
  * @return: retourne un pointeur vers la structure allouee et initialisee
  */
struct commit *new_commit(unsigned short major, unsigned long minor,
			  char *comment)
{
    struct commit* commit = (struct commit*)malloc(sizeof(struct commit));
    commit->id = nextId++;
    commit->version.major = major;
    commit->version.minor = minor;
    commit->comment = comment;
    commit->next = NULL;
    commit->prev = NULL;
    return commit;
}

/**
  * insert_commit - insere sans le modifier un commit dans la liste doublement
  *                 chainee
  *
  * @from: commit qui deviendra le predecesseur du commit insere
  * @new: commit a inserer - seuls ses champs next et prev seront modifies
  *
  * @return: retourne un pointeur vers la structure inseree
  */
static struct commit *insert_commit(struct commit *from, struct commit *new)
{
    from->next->prev = new;
    new->next = from->next;
    from->next = new;
    new->prev = from;
    return new;
}

/**
  * add_minor_commit - genere et insere un commit correspondant a une version
  *                    mineure
  *
  * @from: commit qui deviendra le predecesseur du commit insere
  * @comment: commentaire du commit
  *
  * @return: retourne un pointeur vers la structure inseree
  */
struct commit *add_minor_commit(struct commit *from, char *comment)
{
    struct commit* commit;

    commit = new_commit(from->version.major, from->version.minor + 1, comment);
    insert_commit(from, commit);
    return commit;
}

/**
  * add_major_commit - genere et insere un commit correspondant a une version
  *                    majeure
  *
  * @from: commit qui deviendra le predecesseur du commit insere
  * @comment: commentaire du commit
  *
  * @return: retourne un pointeur vers la structure inseree
  */
struct commit *add_major_commit(struct commit *from, char *comment)
{
    struct commit* commit;

    commit = new_commit(from->version.major + 1, 0, comment);
    insert_commit(from, commit);
    return commit;
}

/**
  * del_commit - extrait le commit de l'historique
  *
  * @victim: commit qui sera sorti de la liste doublement chainee
  *
  * @return: retourne un pointeur vers la structure extraite
  */
struct commit *del_commit(struct commit *victim)
{
    victim->prev->next = victim->next;
    victim->next->prev = victim->prev;
    victim->prev = NULL;
    victim->next = NULL;
    return victim;
}

/**
  * display_commit - affiche un commit : "2:  0-2 (stable) 'Work 2'"
  *
  * @c: commit qui sera affiche
  */
void display_commit(struct commit *c)
{
	/* TODO : Exercice 3 - Question 4 */
    printf("%ld:\t", c->id);
    display_version(is_unstable_bis, &(c->version));
    if (c->comment != NULL)
        printf("\t'%s'\n", c->comment);
    else
        printf("\tno comment\n");
}

/**
  * commitOf - retourne le commit qui contient la version passee en parametre
  *
  * @version: pointeur vers la structure version dont on recherche le commit
  *
  * @return: un pointeur vers la structure commit qui contient 'version'
  *
  * Note:      cette fonction continue de fonctionner meme si l'on modifie
  *            l'ordre et le nombre des champs de la structure commit.
  */
struct commit *commitOf(struct version *version)
{
	/* TODO : Exercice 2 - Question 2 */
	int offset = (int)(((char*)(&(((struct commit*)0x0)->version))) - (char*)&version);
    return (struct commit*)((char*)version - offset);
}
