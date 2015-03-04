/*
 * network-copy-test.c 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "FG.h"

#define in_filename "sort.in"
#define out_filename "sort.out"

#define succeed_or_bail(x) if(!x) { fprintf(stderr, "Aborting\n"); exit(1); }

int main(int argc, char *argv[])
{
    FG_network *nw1, *nw2;
    FG_stage *rs, *ss, *ws;

    /* unbuffered stdout makes debugging easier */
    setbuf(stdout, NULL);

    /* load stage definitions */
    fg_init(&argc, &argv);

    /* instantiate network and stages */
    nw1 = fg_network_create("p0", 4, 256);
    succeed_or_bail(nw1);
    rs = fg_stage_create(nw1, "read-file", "r");
    succeed_or_bail(rs);
    ss = fg_stage_create(nw1, "sort", "s");
    succeed_or_bail(ss);
    ws = fg_stage_create(nw1, "write-file", "w");
    succeed_or_bail(ws);

    fg_network_rename_param(nw1, "r", "filename", "input-filename");
    fg_network_rename_param(nw1, "w", "filename", "output-filename");

    fg_network_set_param(nw1, "input-filename", in_filename);
    fg_network_set_param(nw1, "output-filename", out_filename);

    /* connect stages */
    fg_pin_connect(rs, "data_out", ss, "data_in");
    fg_pin_connect(ss, "data_out", ws, "data_in");

    /* make a copy and merge them together into one network */
    nw2 = fg_network_copy(nw1, "p1");

    /* nuke the first one, and see that the second one works */
    fg_network_destroy(nw1);
    fg_network_fix(nw2);
    fg_network_run(nw2);

    /* clean up */
    fg_fini();

    return 0;
}

