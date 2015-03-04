/*
 * dsort-pass1.c
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "FG.h"

#define succeed_or_bail(x) if(!x) { fprintf(stderr, "Aborting @ %s:%d\n", __FILE__, __LINE__); exit(1); }

int main(int argc, char *argv[])
{
    FG_network *nw;
    FG_stage *s;
    int rc;
    int rank;
    char buf[BUFSIZ];
    char splitter_filename[BUFSIZ];
    char cfg_filename[BUFSIZ];
    char filename[BUFSIZ];
    FILE *f;
    int n;

    /* unbuffered stdout makes debugging multithreaded code easier */
    setbuf(stdout, NULL);

    if(argc < 2) {
        printf("Usage: %s /path/to/dsort-pass1.cfg\n", argv[0]);
        exit(1);
    }

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
    nw = fg_network_from_config("dsort pass1", argv[1]);
    succeed_or_bail(nw);

    snprintf(buf, sizeof(buf), "%d.in", rank);
    snprintf(splitter_filename, sizeof(splitter_filename), "splitters-%d",
            rank);
    s = fg_network_get_stage_by_name(nw, "r");
    fg_stage_set_param(s, "filename", buf);

    s = fg_network_get_stage_by_name(nw, "scatter");
    fg_stage_set_param(s, "splitter_filename", splitter_filename);

    snprintf(buf, sizeof(buf), "%d-%%d.out", rank);
    s = fg_network_get_stage_by_name(nw, "w");
    fg_stage_set_param(s, "filename_fmt", buf);

    printf("\n");

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

    /* count number of files (sorted runs) produced on this node */
    n = 0;
    do {
        sprintf(filename, "%d-%d.out", rank, n);
        n++;
    } while(access(filename, F_OK) == 0);
    n--;

    /* generate config file for pass 2 on this node */
    snprintf(cfg_filename, sizeof(cfg_filename), "pass2-%d.fcg", rank);
    f = fopen(cfg_filename, "w");
    fprintf(f, "stage merge m\n"
               "stage write-file w\n"
               "set w.filename %d.out\n"
               "loop %d stage read-file r$\n"
               "loop %d set r$.filename %d-$.out\n"
               "loop %d stage sort s$\n"
               "loop %d connect r$.data_out s$.data_in\n"
               "loop %d connect s$.data_out m.data_in\n"
               "connect m.data_out w.data_in\n",
               rank, n, n, rank, n, n, n);
    fclose(f);

    return 0;
}

