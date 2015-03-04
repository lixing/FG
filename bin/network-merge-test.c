/*
 * network-merge-test.c
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "FG.h"

#define succeed_or_bail(x) if(!x) { fprintf(stderr, "Aborting\n"); exit(1); }

int main(int argc, char *argv[])
{
    FG_network *nw1, *nw2, *nw3;
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

    /* create sending network */
    nw1 = fg_network_create("sender", 4, 256);
    succeed_or_bail(nw1);

    read_stage = fg_stage_create(nw1, "read-file", "read");
    succeed_or_bail(read_stage);
    fg_network_rename_param(nw1, "read", "filename", "input_file");

    send_stage = fg_stage_create(nw1, "mpi-send-next", "send");
    succeed_or_bail(send_stage);

    fg_pin_connect(read_stage, "data_out", send_stage, "data_in");


    /* create receiving network */
    nw2 = fg_network_create("receiver", 4, 256);
    succeed_or_bail(nw2);

    recv_stage = fg_stage_create(nw2, "mpi-recv-prev", "recv");
    succeed_or_bail(recv_stage);

    write_stage = fg_stage_create(nw2, "write-file", "write");
    succeed_or_bail(write_stage);
    fg_network_rename_param(nw2, "write", "filename", "output_file");

    fg_pin_connect(recv_stage, "data_out", write_stage, "data_in");

    /* merge nw1 and nw2 into (empty) nw3 */
    /* note that nw1 and nw2 are implicitly destroyed by fg_network_merge() */
    fg_network_print(nw1);
    fg_network_print(nw2);

    nw3 = fg_network_create("combined", 4, 256);
    fg_network_merge(nw3, nw1);
    fg_network_merge(nw3, nw2);

    fg_network_print(nw3);

    fg_network_set_param(nw3, "sender.input_file", in_filename);
    fg_network_set_param(nw3, "receiver.output_file", out_filename);

    /* run network */
    fg_network_fix(nw3);
    fg_network_run(nw3);
    fg_network_destroy(nw3);

    /* clean up */
    fg_fini();

    rc = MPI_Finalize();
    if(rc != MPI_SUCCESS) {
        fprintf(stderr, "failed to finalize MPI, error code %d\n", rc);
        exit(1);
    }

    return 0;
}

