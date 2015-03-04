/*
 * merge-test.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "FG.h"

#define in_filename_pat "%d.in"
#define out_filename "merge.out"

#define succeed_or_bail(x) if(!x) { fprintf(stderr, "Aborting\n"); exit(1); }

int main(int argc, char *argv[])
{
    FG_network *nw;
    FG_stage *ms, *rs, *ss, *ws;
    int merge_width;
    int i;
    char filename[BUFSIZ];

    /* command-line args */
    if(argc < 2) {
        fprintf(stderr, "usage: %s [merge-width]\n", argv[0]);
        exit(1);
    }

    merge_width = atoi(argv[1]);

    /* unbuffered stdout makes debugging easier */
    setbuf(stdout, NULL);

    /* load stage definitions */
    fg_init(&argc, &argv);

    /* instantiate network and stages */
    nw = fg_network_create("p0", 4, 256);
    succeed_or_bail(nw);

    ms = fg_stage_create(nw, "merge", "m");
    succeed_or_bail(ms);

    ws = fg_stage_create(nw, "write-file", "w");
    succeed_or_bail(ws);
    fg_stage_set_param(ws, "filename", out_filename);

    for(i=0; i<merge_width; i++) {
        snprintf(filename, BUFSIZ, in_filename_pat, i);

        rs = fg_stage_create(nw, "read-file", "r0");
        succeed_or_bail(rs);
        fg_stage_set_param(rs, "filename", filename);

        ss = fg_stage_create(nw, "sort", "s0");
        succeed_or_bail(ss);

        fg_pin_connect(rs, "data_out", ss, "data_in");
        fg_pin_connect(ss, "data_out", ms, "data_in");
    }

    /* connect stages */
    fg_pin_connect(ms, "data_out", ws, "data_in");

    /* run network */
    fg_network_fix(nw);
    fg_network_run(nw);

    /* clean up */
    fg_fini();

    return 0;
}

