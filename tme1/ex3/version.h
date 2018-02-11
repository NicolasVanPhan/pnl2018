#ifndef VERSION_H
#define VERSION_H

struct version {
	unsigned long minor;
	unsigned short major;
	char flags;
};

int is_unstable(struct version *v);

int is_unstable_bis(struct version *v);

void display_version(int(*f)(struct version*),struct version *v);

int cmp_version(struct version *v, unsigned short major, unsigned long minor);

#endif
