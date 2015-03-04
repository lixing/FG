/*
 * dsort-pass1.c
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "FG.h"

#define succeed_or_bail(x) if(!x) { fprintf(stderr, "Aborting @ %s:%d\n", __FILE__, __LINE__); exit(1); }

int main(int argc, char *argv[])
{
    FG_network *nw;
    FG_stage *read_stage, *sort_stage0, *scatter_stage;
    FG_stage *gather_stage, *sort_stage1, *write_stage;
    int rc;
    int rank;
    char buf[BUFSIZ];
    char splitter_filename[BUFSIZ];

    /* unbuffered stdout makes debugging multithreaded code easier */
    setbuf(stdout, NULL);

    /* load stage definitions */
    fg_init(&argc, &argv);

    rc = MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, NULL);
    if(rc != MPI_SUCCESS) {
        fprintf(stderr, "failed to initialize MPI, error code %d\n", rc);
        exit(1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    printf("\n");

    /* instantiate network and stages */
    nw = fg_network_create("dsort-pass1", 2, 256 * 1024 * 1024);
    succeed_or_bail(nw);

    snprintf(buf, sizeof(buf), "%d.in", rank);
    snprintf(splitter_filename, sizeof(splitter_filename), "splitters-%d",
            rank);
    read_stage = fg_stage_create(nw, "read-file", "read");
    succeed_or_bail(read_stage);
    fg_stage_set_param(read_stage, "filename", buf);

    sort_stage0 = fg_stage_create(nw, "sort", "sort");
    succeed_or_bail(sort_stage0);

    scatter_stage = fg_stage_create(nw, "dsort-scatter", "scatter");
    succeed_or_bail(scatter_stage);
    fg_stage_set_param(scatter_stage, "splitter_filename", splitter_filename);

    gather_stage = fg_stage_create(nw, "dsort-gather", "gather");
    succeed_or_bail(gather_stage);
    sort_stage1 = fg_stage_create(nw, "sort", "sort");
    succeed_or_bail(sort_stage1);

    snprintf(buf, sizeof(buf), "%d-%%d.out", rank);
    write_stage = fg_stage_create(nw, "multiwrite-file", "write");
    succeed_or_bail(write_stage);
    fg_stage_set_param(write_stage, "filename_fmt", buf);

    printf("\n");

    /* connect stages */
    fg_pin_connect(read_stage, "data_out", sort_stage0, "data_in");
    fg_pin_connect(sort_stage0, "data_out", scatter_stage, "data_in");

    fg_pin_connect(gather_stage, "data_out", sort_stage1, "data_in");
    fg_pin_connect(sort_stage1, "data_out", write_stage, "data_in");

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

