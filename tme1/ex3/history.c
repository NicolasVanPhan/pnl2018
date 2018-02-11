#include<stdlib.h>
#include<stdio.h>

#include"history.h"

/**
  * new_history - alloue, initialise et retourne un historique.
  *
  * @name: nom de l'historique
  */
struct history *new_history(char *name)
{
	/* TODO : Exercice 3 - Question 2 */
    struct history* history = (struct history*)malloc(sizeof(struct history));
    history->name = name;
    history->commit_count = 0;
    history->commit_list = (struct commit*)malloc(sizeof(struct commit));
    history->commit_list->prev = history->commit_list;
    history->commit_list->next = history->commit_list;
	return history;
}

/**
  * last_commit - retourne l'adresse du dernier commit de l'historique.
  *
  * @h: pointeur vers l'historique
  */
struct commit *last_commit(struct history *h)
{
	/* TODO : Exercice 3 - Question 2 */
    return h->commit_list->prev;
}

/**
  * display_history - affiche tout l'historique, i.e. l'ensemble des commits de
  *                   la liste
  *
  * @h: pointeur vers l'historique a afficher
  */
void display_history(struct history *h)
{
	/* TODO : Exercice 3 - Question 2 */
    struct commit* p;
    for (p = h->commit_list->next;
            p != h->commit_list; p = p->next)
    {
        printf("id : %ld\n\n", p->id);
        printf("%s\n\n\n", p->comment); 
    }
}

/**
  * infos - affiche le commit qui a pour numero de version <major>-<minor> ou
  *         'Not here !!!' s'il n'y a pas de commit correspondant.
  *
  * @major: major du commit affiche
  * @minor: minor du commit affiche
  */
void infos(struct history *h, int major, unsigned long minor)
{
	/* TODO : Exercice 3 - Question 2 */
	return NULL;
}
