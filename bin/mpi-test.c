/*
 * mpi-test.c
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "FG.h"

#define succeed_or_bail(x) if(!x) { fprintf(stderr, "Aborting\n"); exit(1); }

int main(int argc, char *argv[])
{
    FG_network *nw;
    FG_stage *read_stage, *send_stage;
    FG_stage *recv_stage, *write_stage;
    int rc;
    int rank;
    char in_filename[BUFSIZ];
    char out_filename[BUFSIZ];

    /* unbuffered stdout makes debugging multithreaded code easier */
    setbuf(stdout, NULL);

    /* load stage definitions */
    fg_init(&argc, &argv);

    rc = MPI_Init(&argc, &argv);
    if(rc != MPI_SUCCESS) {
        fprintf(stderr, "failed to initialize MPI, error code %d\n", rc);
        exit(1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    sprintf(in_filename, "%d.in", rank);
    sprintf(out_filename, "%d.out", rank);

    printf("\n");

    /* instantiate network and stages */
    nw = fg_network_create("p0", 4, 256);
    succeed_or_bail(nw);

    read_stage = fg_stage_create(nw, "read-file", "read");
    succeed_or_bail(read_stage);
    fg_stage_set_param(read_stage, "filename", in_filename);

    send_stage = fg_stage_create(nw, "mpi-send-next", "send");
    succeed_or_bail(send_stage);

    recv_stage = fg_stage_create(nw, "mpi-recv-prev", "recv");
    succeed_or_bail(recv_stage);

    write_stage = fg_stage_create(nw, "write-file", "write");
    succeed_or_bail(write_stage);
    fg_stage_set_param(write_stage, "filename", out_filename);

    printf("\n");

    /* connect stages */
    fg_pin_connect(read_stage, "data_out", send_stage, "data_in");
    fg_pin_connect(recv_stage, "data_out", write_stage, "data_in");

    /* run network */
    printf("\n");
    fg_network_fix(nw);
    fg_network_run(nw);
    fg_network_destroy(nw);

    /* clean up */
    fg_fini();

    rc = MPI_Finalize();
    if(rc != MPI_SUCCESS) {
        fprintf(stderr, "failed to finalize MPI, error code %d\n", rc);
        exit(1);
    }

    return 0;
}

