/*
 * pq_test.c
 */

#include <stdio.h>

#include "pq.h"

int main(int argc, char *argv[])
{
    int i;
    pq *p;
    int *foo;

    p = pq_create(argc - 1);

    for(i=1; i<argc; i++) {
        printf("inserting %d\n", atoi(argv[i]));
        pq_insert(p, atoi(argv[i]), (void *) atoi(argv[i]));
    }

    while(p->occupancy > 0) {
        foo = pq_pop(p, &i);
        printf("popped %d\n", i);
    }
}

