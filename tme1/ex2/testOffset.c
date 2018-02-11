#include <stdlib.h>
#include <stdio.h>

#include "version.h"
#include "commit.h"

int main(void)
{
	struct version v = {.major = 3,
	        		    .minor = 5,
		        	    .flags = 0};

    char comm[5] = "toto";
    struct commit c = { .id = 42,
                        .version = v,
                        .comment = comm,
                        .next = NULL,
                        .prev = NULL};
    display_commit(&c);
    printf("commitOf : %p", commit_of(&(c.version)));
	return 0;
}
