/*
 * dsort-pass2.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "FG.h"

#define succeed_or_bail(x) if(!x) { fprintf(stderr, "Aborting @ %s:%d\n", __FILE__, __LINE__); exit(1); }

int main(int argc, char *argv[])
{
    FG_network *nw;
    int rank;

    if(argc < 2) {
        printf("Usage: %s /path/to/dsort-pass2.cfg\n", argv[0]);
        exit(1);
    }

    rank = atoi(argv[1]);

    printf("dsort pass2, node %d\n", rank);

    /* unbuffered stdout makes debugging multithreaded code easier */
    setbuf(stdout, NULL);

    fg_init(&argc, &argv);

    /* instantiate network and stages */
    nw = fg_network_from_config("dsort pass2", argv[1]);
    succeed_or_bail(nw);

    /* run network */
    printf("\n");
    fg_network_fix(nw);
    fg_network_run(nw);
    fg_network_destroy(nw);

    /* clean up */
    fg_fini();

    return 0;
}

