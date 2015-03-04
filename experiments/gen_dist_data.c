/*
 * gen_dist_data.c
 * 
 * Generates records whose keys are distributed according to either a normal
 * or Poisson distribution.  Keys are always 8-byte signed integers, though
 * record size can be tweaked with a command-line option.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

void usage(char *progname);
int64_t gauss_rand(double mean, double stddev);
int64_t uniform_rand(void);
int64_t poisson_rand(double lambda);

enum {
    normal,
    poisson
};

int key_len = 8;
int recs_per_block = 8;

int main(int argc, char *argv[])
{
    int64_t nrecs, rec_len;
    double mean, stddev, lambda;
    int8_t *block;
    int block_size;     /* num octets */
    int i;
    int64_t n;
    int mode = normal;
    unsigned int random_seed;
    int fd;

    if(argc < 2) {
        usage(argv[0]);
        exit(1);
    } else if(strcmp(argv[1], "-p") == 0) {
        mode = poisson;
    } else if(strcmp(argv[1], "-n") == 0) {
        mode = normal;
    } else {
        usage(argv[0]);
        exit(0);
    }

    if(mode == normal) {
        if(argc < 6) {
            usage(argv[0]);
            exit(1);
        }

        nrecs = atol(argv[2]);
        rec_len = atoi(argv[3]);
        mean = atof(argv[4]);
        stddev = atof(argv[5]);
    } else {
        if(argc < 5) {
            usage(argv[0]);
            exit(1);
        }

        nrecs = atol(argv[2]);
        rec_len = atoi(argv[3]);
        lambda = atof(argv[4]);
    }

    /* randomly seed the PRNG */
    fd = open("/dev/urandom", O_RDONLY);
    read(fd, &random_seed, sizeof(random_seed));
    close(fd);
    srandom(random_seed);

    block_size = recs_per_block * (key_len + rec_len);
    block = (int8_t *) calloc(block_size, sizeof(int8_t));

    /* to minimize disk operations, this loop fills up a block of records in
     * memory before pushing them out */

    i = 0;
    while(nrecs > 0) {
        switch(mode) {
            case normal:  n = gauss_rand(mean, stddev); break;
            case poisson: n = poisson_rand(lambda);     break;
        }

        *((int64_t *) (block + i)) = n;
        i = (i + key_len + rec_len) % block_size;
        nrecs--;

        if(i == 0) {
            write(1, block, block_size);
        }
    }

    return 0;
}

void usage(char *progname)
{
    printf("normal:  %s -n nrecs rec_len mean stddev\n", progname);
    printf("poisson: %s -p nrecs rec_len lambda\n", progname);
}

/* Algorithm used is due to Knuth: http://c-faq.com/lib/gaussian.html */
int64_t gauss_rand(double mean, double stddev)
{
    static double V1, V2, S;
    static int phase = 0;
    double X;

    if(phase == 0) {
        do {
            double U1 = (double)random() / RAND_MAX;
            double U2 = (double)random() / RAND_MAX;

            V1 = 2 * U1 - 1;
            V2 = 2 * U2 - 1;
            S = V1 * V1 + V2 * V2;
        } while(S >= 1 || S == 0);

        X = V1 * sqrt(-2 * log(S) / S);
    } else {
        X = V2 * sqrt(-2 * log(S) / S);
    }

    phase = 1 - phase;

    return (int64_t) (X * stddev + mean);
}

/* algorithm used due to Knuth:
 * http://en.wikipedia.org/wiki/Poisson_distribution */
int64_t poisson_rand(double lambda)
{
    double L = exp(-1.0 * lambda);
    int64_t k = 0;
    double p = 1.0;

    do {
        k = k + 1;
        p = p * (double) random() / (double) RAND_MAX;
    } while (p > L);

    return k - 1;
}

