
#include <stdio.h>
#include <stdlib.h>
#include "version.h"
#include "commit.h"

int main()
{
	struct commit*	c;

	c = malloc(sizeof(struct commit));
	c->id = 42;
	c->version.major = 1;
	c->version.minor = 0;
	c->comment = "Toto";
	c->prev = c;
	c->next = c;
	printf("id : %d\n", c->id);
	printf("version : %ld.%ld\n", c->version.major, c->version.minor);
	printf("comment : %s\n", c->comment);
	printf("prev : %p\n", c->prev);
	printf("next : %p\n", c->next);
}


