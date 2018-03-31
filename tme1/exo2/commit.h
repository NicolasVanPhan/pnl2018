
#ifndef H_STRUCT_COMMIT
#define H_STRUCT_COMMIT

#include "version.h"

struct commit {
	unsigned long	id;
	struct version	version;
	char		*comment;
	struct commit	*next;
	struct commit	*prev;
};

#endif
