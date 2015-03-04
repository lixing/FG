/*
 * pin-array-test.c 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "FG.h"

#define in0_filename "0.in"
#define in1_filename "1.in"
#define out_filename "combine.out"

#define succeed_or_bail(x) if(!x) { fprintf(stderr, "Aborting\n"); exit(1); }

int main(int argc, char *argv[])
{
    FG_network *nw;
    FG_stage *rs0, *rs1, *cs, *ws;
    FG_pin *pin;

    /* unbuffered stdout makes debugging easier */
    setbuf(stdout, NULL);

    /* load stage definitions */
    fg_init(&argc, &argv);

    /* instantiate network and stages */
    nw = fg_network_create("p0", 4, 256);
    succeed_or_bail(nw);

    rs0 = fg_stage_create(nw, "read-file", "r0");
    succeed_or_bail(rs0);
    fg_stage_set_param(rs0, "filename", in0_filename);

    rs1 = fg_stage_create(nw, "read-file", "r1");
    succeed_or_bail(rs1);
    fg_stage_set_param(rs1, "filename", in1_filename);

    cs = fg_stage_create(nw, "rr-combine", "c");
    succeed_or_bail(cs);

    ws = fg_stage_create(nw, "write-file", "w");
    succeed_or_bail(ws);
    fg_stage_set_param(ws, "filename", out_filename);

    /* connect stages */
    fg_pin_connect(rs0, "data_out", cs, "data_in");
    fg_pin_connect(rs1, "data_out", cs, "data_in");
    fg_pin_connect(cs, "data_out", ws, "data_in");

    pin = fg_stage_pin_get_by_name(rs0, "buf_in");
    fg_pin_set_buffer_count(pin, 2);

    /* run network */
    fg_network_fix(nw);
    fg_network_run(nw);

    /* clean up */
    fg_fini();

    return 0;
}

