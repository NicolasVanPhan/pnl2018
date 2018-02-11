
#ifndef COMMIT_H
#define COMMIT_H

#include "version.h"
struct commit {
    unsigned long   id;
    struct version  version;
    char            *comment;
    struct commit   *next;
    struct commit   *prev;
};

void display_commit(struct commit* c);
struct commit *commit_of(struct version *version);
#endif

