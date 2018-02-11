#include<stdlib.h>
#include<stdio.h>

#include"version.h"

int main(int argc, char const *argv[])
{
	struct version v = {.major = 3,
			    .minor = 5,
			    .flags = 0};

	display_version((&is_unstable_bis), &v);
	printf("\n");

	v.minor++;
	display_version((&is_unstable_bis), &v);
	printf("\n");

	v.major++;
	v.minor = 0;
	display_version((&is_unstable_bis), &v);
	printf("\n");

	v.minor++;
	display_version((&is_unstable_bis), &v);
	printf("\n");

	return 0;
}
