#include <stdio.h>
#include <stdlib.h>
#include "version.h"
#include "commit.h"

void display_commit(struct commit* c)
{
    printf("id : %p\n", &(c->id));
    printf("version : %p\n", &(c->version));
    printf("comment : %p\n", &(c->comment));
    printf("prev: %p\n", c->prev);
    printf("next : %p\n", c->next);
}

struct commit *commit_of(struct version *version)
{
    char *offset = (char*)&(((struct commit*)0x0)->version);
    return (struct commit*)((char*)version - offset);
}
