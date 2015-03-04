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
    FG_stage *ms, *rs, *ws, *ss;
    FG_pin *pin;
    int rank;
    int i;
    char buf[BUFSIZ];
    char in_filename[BUFSIZ];
    struct stat sbuf;

    rank = atoi(argv[1]);

    printf("dsort pass2, node %d\n", rank);

    /* unbuffered stdout makes debugging multithreaded code easier */
    setbuf(stdout, NULL);

    fg_init(&argc, &argv);

    /* instantiate network and stages */
    nw = fg_network_create("nw", 3, 8 * 1024 * 1024);
    succeed_or_bail(nw);

    ms = fg_stage_create(nw, "merge", "merge");
    succeed_or_bail(ms);

    snprintf(buf, sizeof(buf), "%d.out", rank);
    ws = fg_stage_create(nw, "write-file", "write");
    succeed_or_bail(ws);
    fg_stage_set_param(ws, "filename", buf);

    i = 0;
    snprintf(in_filename, sizeof(in_filename), "%d-%d.out", rank, i);
    while(stat(in_filename, &sbuf) == 0) {
        snprintf(buf, sizeof(buf), "read-part%d", i);
        rs = fg_stage_create(nw, "read-file", buf);
        succeed_or_bail(rs);
        fg_stage_set_param(rs, "filename", in_filename);

        snprintf(buf, sizeof(buf), "sort-part%d", i);
        ss = fg_stage_create(nw, "sort", buf);
        succeed_or_bail(ss);

        fg_pin_connect(rs, "data_out", ss, "data_in");
        fg_pin_connect(ss, "data_out", ms, "data_in");

        i++;
        snprintf(in_filename, sizeof(in_filename), "%d-%d.out", rank, i);
    }

    /* connect stages */
    fg_pin_connect(ms, "data_out", ws, "data_in");

    /* set buffer counts and sizes */
    pin = fg_stage_pin_get_by_name(ms, "buf_in");
    fg_pin_set_buffer_size(pin, 256 * 1024 * 1024);
    fg_pin_set_buffer_count(pin, 2);

    /* run network */
    fg_network_fix(nw);
    fg_network_run(nw);
    fg_network_destroy(nw);

    /* clean up */
    fg_fini();

    return 0;
}

