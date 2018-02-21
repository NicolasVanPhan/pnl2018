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
	struct commit	*phantom_menace;
	struct history	*history;

	/* Initialization of the phantom commit */
	phantom_menace = new_commit(0, 0, NULL);

	/* Initialization of the new history */
	history = (struct history*)malloc(sizeof(struct history));
	history->name = name;
	history->commit_count = 0;
	history->commit_list = phantom_menace;
	return history;
}

/**
  * last_commit - retourne l'adresse du dernier commit de l'historique.
  *
  * @h: pointeur vers l'historique
  */
struct commit *last_commit(struct history *h)
{
	return list_last_entry(&(h->commit_list->hook), struct commit, hook);
}

/**
  * display_history - affiche tout l'historique, i.e. l'ensemble des commits de
  *                   la liste
  *
  * @h: pointeur vers l'historique a afficher
  */
void display_history(struct history *h)
{
	struct commit	*c;

	printf("History of '%s'\n", h->name);
	list_for_each_entry(c, &(h->commit_list->hook), hook) {
    		display_commit(c);
	}
	printf("\n");
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
	struct commit   *c;

	list_for_each_entry(c, &(h->commit_list->hook), hook) {
		if (c->version.major == major && c->version.minor == minor)
	    		break;
    	}
    	if (c != h->commit_list) {
        	printf("%ld:\t", c->id);
        	display_version(is_unstable_bis, &(c->version));
        	printf("\t%s\n", c->comment);
    	}
    	else {
        	printf("Not here !!!\n");
    	}
}
