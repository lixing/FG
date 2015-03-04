/*
 * param-rename-test.c 
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
    FG_network *nw;
    FG_stage *rs, *ss, *ws;

    /* unbuffered stdout makes debugging easier */
    setbuf(stdout, NULL);

    /* load stage definitions */
    fg_init(&argc, &argv);

    /* instantiate network and stages */
    nw = fg_network_create("p0", 4, 256);
    succeed_or_bail(nw);
    rs = fg_stage_create(nw, "read-file", "r");
    succeed_or_bail(rs);
    ss = fg_stage_create(nw, "sort", "s");
    succeed_or_bail(ss);
    ws = fg_stage_create(nw, "write-file", "w");
    succeed_or_bail(ws);

    fg_network_rename_param(nw, "r", "filename", "input-filename");
    fg_network_rename_param(nw, "w", "filename", "output-filename");

    fg_network_set_param(nw, "input-filename", in_filename);
    fg_network_set_param(nw, "output-filename", out_filename);

    /* connect stages */
    fg_pin_connect(rs, "data_out", ss, "data_in");
    fg_pin_connect(ss, "data_out", ws, "data_in");

    /* run network */
    fg_network_fix(nw);
    fg_network_run(nw);

    /* clean up */
    fg_fini();

    return 0;
}

