/*
 * config-test.c
 */

#include <stdio.h>

#include "FG.h"

int main(int argc, char *argv[])
{
    FG_network *nw;

    /* unbuffered stdout makes debugging easier */
    setbuf(stdout, NULL);

    fg_init(&argc, &argv);

    nw = fg_network_from_config("test network", argv[1]);
    fg_network_fix(nw);
    fg_network_run(nw);
    fg_network_destroy(nw);

    fg_fini();

    return 0;
}

