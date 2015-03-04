/*
 * sort-verify.c
 *
 * Verifies files are sorted.  Filenames provided, in order, as command-line
 * arguments.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>

/* record parameters */
#define keylen 8
#define reclen 64       /* including key */

#define nrecs  10240    /* number of records to read at a time */

int main(int argc, char *argv[])
{
    char **filename;
    char buffer[(reclen * nrecs)];
    int fd;
    int i, n;
    int64_t cur_key, new_key;

    cur_key = INT64_MIN;
    for(filename=argv+1; *filename; filename++) {
        fd = open(*filename, O_RDONLY);

        while((n = read(fd, buffer, sizeof(buffer))) > 0) {
            for(i=0; i<n; i+=reclen) {
                new_key = *((int64_t *) (buffer + i));
                if(new_key < cur_key) {
                    fprintf(stderr, "ERROR @ %d of %s\n", i, *filename);
                    fprintf(stderr, "cur_key: %0lx (%ld)\n", cur_key, cur_key);
                    fprintf(stderr, "new_key: %0lx (%ld)\n", new_key, new_key);
                    exit(1);
                }

                /* printf("%0lx : %ld\n", cur_key, cur_key); */

                cur_key = new_key;
            }
        }

        close(fd);
    }

    return 0;
}

