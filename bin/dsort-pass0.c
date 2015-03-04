/*
 * dsort-pass0.c
 *
 * Calculates and distributes splitters.
 *
 * Usage: dsort-pass0 oversampling-ratio filename
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

enum {
    keylen = 8,
    reclen = 64
};

typedef struct {
    int64_t key;
    int64_t proc;
    int64_t index;
} splitter;

#define MPI_TAG_SPLITTERS 1
#define MPI_TAG_SPLITTER_DIST 2

void calc_splitters(splitter *splitters, const char *filename, int rank,
        int os_ratio);
void scatter_splitters(splitter *splitters, int os_ratio);
void gather_splitters(splitter *splitters, int nprocs, int os_ratio);
void distribute_splitters(splitter *splitters, int nprocs);
void accept_splitters(splitter *splitters, int nprocs);
void write_splitters(splitter *splitters, int nprocs, const char *filename);
int rec_cmp(const void *rec1, const void *rec2);

int main(int argc, char *argv[]) {
    int os_ratio;             /* oversampling ratio */
    const char *filename;
    char splitter_filename[BUFSIZ];
    int rank, nprocs;
    int rc;
    splitter *splitters;
    int i;
    int fd;
    int random_seed;

    rc = MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, NULL);
    if(rc != MPI_SUCCESS) {
        fprintf(stderr, "failed to initialize MPI, error code %d\n", rc);
        exit(1);
    }

    /* command-line arguments */
    os_ratio = atoi(argv[1]);
    filename = argv[2];

    /* MPI state */
    rc = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rc != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Comm_rank failed, error code %d\n", rc);
        exit(1);
    }

    rc = MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    if(rc != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Comm_size failed, error code %d\n", rc);
        exit(1);
    }

    printf("I am proc %d of %d\n", rank, nprocs);

    /* seed PRNG differently for each proc */
    fd = open("/dev/urandom", O_RDONLY);
    read(fd, &random_seed, sizeof(random_seed));
    close(fd);

    srandom(random_seed * rank);

    /* do the stuff with the thing */
    splitters = (splitter *) calloc(os_ratio * nprocs, sizeof(splitter));
    calc_splitters(splitters, filename, rank, os_ratio);

    if(rank == 0) {
        gather_splitters(splitters, nprocs, os_ratio);
        qsort(splitters, nprocs * os_ratio, sizeof(splitter), rec_cmp);

        /* move chosen splitters to front of array */
        for(i=0; i<nprocs-1; i++) {
            *(splitters + i) = *(splitters + os_ratio * (i + 1));
        }

        /* last "splitter" is max int64_t */
        (splitters+nprocs-1)->key = INT64_MAX;
        (splitters+nprocs-1)->proc = INT64_MAX;
        (splitters+nprocs-1)->index = INT64_MAX;

        printf("\nchosen splitters:\n");
        for(i=0; i<nprocs; i++) {
            printf("%016lx == %ld (%ld, %ld)\n", (splitters + i)->key,
                    (splitters + i)->key, (splitters + i)->proc,
                    (splitters + i)->index);
        }

        distribute_splitters(splitters, nprocs);
    } else {
        scatter_splitters(splitters, os_ratio);
        accept_splitters(splitters, nprocs);
    }

    snprintf(splitter_filename, BUFSIZ, "splitters-%d", rank);
    write_splitters(splitters, nprocs, splitter_filename);

    rc = MPI_Finalize();
    if(rc != MPI_SUCCESS) {
        fprintf(stderr, "failed to finalize MPI, error code %d\n", rc);
        exit(1);
    }

    return 0;
}

void calc_splitters(splitter *splitters, const char *filename, int rank,
        int os_ratio)
{
    int rc;
    int fd;
    struct stat sbuf;
    long file_size;
    int nrecs;
    int i, idx;

    fd = open(filename, O_RDONLY);

    /* this *does* assume a single input file (is this valid?) */
    rc = fstat(fd, &sbuf);
    if(rc < 0) {
        printf("stat(%s) failed with error %d\n", filename, errno);
        exit(1);
    }

    file_size = sbuf.st_size;
    nrecs = file_size / reclen;

    for(i=0; i<os_ratio; i++) {
        idx = random() % nrecs;
        lseek(fd, idx * reclen, SEEK_SET);
        read(fd, &(splitters + i)->key, keylen);
        (splitters + i)->proc = rank;
        (splitters + i)->index = idx;
        /* printf("splitter %d: %ld (%016lx) @ %ld, %ld\n", i,
                (splitters + i)->key, (splitters + i)->key,
                (splitters + i)->proc, (splitters + i)->index); */
    }

    close(fd);

    printf("read %ld bytes (%d records) from %s\n", file_size, nrecs,
            filename);
}

void scatter_splitters(splitter *splitters, int os_ratio)
{
    int rc;

    rc = MPI_Send(splitters, os_ratio * sizeof(splitter), MPI_CHAR, 0,
            MPI_TAG_SPLITTERS, MPI_COMM_WORLD);
    if(rc != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Send failed, error code %d\n", rc);
        exit(1);
    }

    printf("sent splitters to head node\n");
}

void gather_splitters(splitter *splitters, int nprocs, int os_ratio)
{
    int i;
    int rc;
    MPI_Status status;

    for(i=1; i<nprocs; i++) {
        rc = MPI_Recv(splitters + i * os_ratio, os_ratio * sizeof(splitter),
                MPI_CHAR, MPI_ANY_SOURCE, MPI_TAG_SPLITTERS, MPI_COMM_WORLD,
                &status);
        if(rc != MPI_SUCCESS) {
            fprintf(stderr, "MPI_Send failed, error code %d\n", rc);
            exit(1);
        }

        printf("received splitters from proc %d\n", status.MPI_SOURCE);
    }

    printf("received splitters from all other nodes\n");
}

void distribute_splitters(splitter *splitters, int nprocs)
{
    int i;
    int rc;

    for(i=1; i<nprocs; i++) {
        rc = MPI_Send(splitters, nprocs * sizeof(splitter), MPI_CHAR, i,
                MPI_TAG_SPLITTER_DIST, MPI_COMM_WORLD);
        if(rc != MPI_SUCCESS) {
            fprintf(stderr, "MPI_Send failed, error code %d\n", rc);
            exit(1);
        }

        printf("splitters sent to proc %d\n", i);
    }

    printf("distributed splitters to all other nodes\n");
}

void accept_splitters(splitter *splitters, int nprocs)
{
    int rc;
    MPI_Status status;

    rc = MPI_Recv(splitters, nprocs * sizeof(splitter), MPI_CHAR,
            MPI_ANY_SOURCE, MPI_TAG_SPLITTER_DIST, MPI_COMM_WORLD, &status);
    if(rc != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Send failed, error code %d\n", rc);
        exit(1);
    }

    printf("received splitters from head node\n");
}

void write_splitters(splitter *splitters, int nprocs, const char *filename)
{
    int fd;
    int n;

    fd = open(filename, O_CREAT | O_WRONLY, 0644);
    n = write(fd, splitters, nprocs * sizeof(splitter));
    close(fd);

    if(n < nprocs  * sizeof(splitter)) {
        fprintf(stderr, "error %d writing splitter file\n", errno);
        exit(1);
    }

    printf("wrote splitters to file %s\n", filename);
}

int rec_cmp(const void *rec1, const void *rec2)
{
    splitter *s1, *s2;

    s1 = (splitter *) rec1;
    s2 = (splitter *) rec2;

    return (s1->key < s2->key) ? -1 : (s1->key > s2->key) ? 1 :
           (s1->proc < s2->proc) ? -1 : (s1->proc > s2->proc) ? 1 :
           (s1->index < s2->index) ? -1 : (s1->index > s2->index) ? 1 : 0;
}

