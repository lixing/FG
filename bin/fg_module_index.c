/*
 * fg_module_index.c 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fg_internal.h"

#define succeed_or_bail(x) if(!x) { fprintf(stderr, "Aborting\n"); exit(1); }

int main(int argc, char *argv[])
{
    FG_stage_def **sd;

    /* unbuffered stdout makes debugging easier */
    setbuf(stdout, NULL);

    /* load stage definitions */
    fg_init(&argc, &argv);

    for(sd=fg_get_stage_defs(); *sd; sd++) {
        printf("%s\n", (*sd)->name);
    }

    /* clean up */
    fg_fini();

    return 0;
}

