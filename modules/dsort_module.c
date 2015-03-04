/*
 * dsort_module.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <sys/stat.h>
#include <mpi.h>
#include <fcntl.h>
#include <unistd.h>

#include "fg_internal.h"
#include "pq.h"

#define DSORT_DATA 1
#define DSORT_SCATTER_DONE 2

/* WARNING: pretty much this entire file depends on keys being 64-bit signed
 * integers and records (including key) being 64 bytes */
enum {
    KEYLEN = 8,
    RECLEN = 64
};

/* WARNING; TACKY: this struct is duplicated from bin/dsort-pass0.c; changes
 * must be synchronized */
typedef struct {
    int64_t key;
    int64_t proc;
    int64_t index;
} splitter;

/* sort stage definition prototypes */
char sort_name[] = "sort";
char sort_doc[] = "sort doc";
int sort_func(FG_stage *stage);
FG_pin sort_pins[] = { { "data_in",  PIN_IN  },
                       { "data_out", PIN_OUT },
                       { NULL }
                     };

/* merge stage definition prototypes */
char merge_name[] = "merge";
char merge_doc[] = "merge doc";
int merge_init(FG_stage *stage);
int merge_func(FG_stage *stage);
FG_pin merge_pins[] = { { "buf_in",   PIN_IN        },
                        { "data_in",  PIN_ARRAY_IN, },
                        { "data_out", PIN_OUT       },
                        { "buf_out",  PIN_OUT       },
                        { NULL }
                      };

/* scatter stage definition prototypes */
char scatter_name[] = "dsort-scatter";
char scatter_doc[] = "scatter doc";
int scatter_init(FG_stage *stage);
int scatter_func(FG_stage *stage);
FG_pin scatter_pins[] = { { "data_in",  PIN_IN, },
                          { "buf_out",  PIN_OUT },
                          { NULL }
                        };
const char *scatter_params[] = { "splitter_filename",
                                 NULL
                               };

/* gather stage definition prototypes */
char gather_name[] = "dsort-gather";
char gather_doc[] = "gather doc";
int gather_func(FG_stage *stage);
FG_pin gather_pins[] = { { "buf_in",   PIN_IN  },
                         { "data_out", PIN_OUT },
                         { NULL }
                       };


/* module defs */
char *fg_module_name = "dsort operations";
FG_stage_def fg_module_export[] = {
    { sort_name, sort_doc, NULL, sort_func, NULL, sort_pins, NULL },
    { merge_name, merge_doc, merge_init, merge_func, NULL, merge_pins, NULL },
    { scatter_name, scatter_doc, scatter_init, scatter_func, NULL, scatter_pins, scatter_params },
    { gather_name, gather_doc, NULL, gather_func, NULL, gather_pins, NULL },
    { NULL }
};

/* compares two records; assumes that keys are a) 64 bits in length; b) come
 * first in each record */
int reccmp(const void *r1, const void *r2)
{
    int64_t *k1, *k2;

    k1 = (int64_t *) r1;
    k2 = (int64_t *) r2;

    return (*k1 < *k2) ? -1 : (*k1 > *k2) ? 1 : 0;
}

/* sort stage definition
 *************************************************************/
int sort_func(FG_stage *stage)
{
    FG_pin *pin;
    FG_buf *buf;

    pin = fg_stage_pin_get_by_name(stage, "data_in");
    buf = fg_pin_accept_buffer(pin);

    if(!buf) {
        return FG_STAGE_TERMINATE;
    }

    qsort(buf->data, buf->datalen / RECLEN, RECLEN, reccmp);

    pin = fg_stage_pin_get_by_name(stage, "data_out");
    fg_pin_convey_buffer(pin, buf);

    return FG_STAGE_SUCCESS;
}

/* merge stage definition
 *************************************************************/

struct merge_state {
    uint32_t num_inputs;
    pq *p;
};

struct pq_entry {
    FG_buf *buffer;
    uint32_t offset;
    uint32_t pin_index;
};

int merge_init(FG_stage *stage)
{
    struct merge_state *s;

    s = (struct merge_state *) malloc(sizeof(struct merge_state));
    stage->data = s;

    return 0;
}

/* assumes that buffer sizes are an integer multiple of record size! */
int merge_func(FG_stage *stage)
{
    struct merge_state *s;
    FG_pin *data_in, *data_out, *buf_in, *buf_out;
    FG_buf *merged_buf = NULL;
    int i;
    struct pq_entry *pqes;
    struct pq_entry *pqe;
    int64_t key;

    s = (struct merge_state *) stage->data;

    data_in = fg_stage_pin_get_by_name(stage, "data_in");
    data_out = fg_stage_pin_get_by_name(stage, "data_out");
    buf_in = fg_stage_pin_get_by_name(stage, "buf_in");
    buf_out = fg_stage_pin_get_by_name(stage, "buf_out");

    s->num_inputs = fg_pin_array_get_width(data_in);
    s->p = pq_create(s->num_inputs);
    pqes = (struct pq_entry *) calloc(s->num_inputs, sizeof(struct pq_entry));

    printf("%s> input pin array width is %d\n", stage->name, s->num_inputs);

    /* before we can start, need to get a buffer from each input */
    for(i=0; i<s->num_inputs; i++) {
        pqe = pqes + i;
        pqe->offset = 0;
        pqe->pin_index = i;
        pqe->buffer = fg_pin_array_accept_buffer(data_in, i);

        if(pqe->buffer) {
            key = *((int64_t *) pqe->buffer->data);

            pq_insert(s->p, key, pqe);

            fg_log(FG_LOG_MODULE, "%s> accepted initial buffer from input %d\n",
                    stage->name, i);
        } else {
            fg_log(FG_LOG_MODULE, "%s> no initial buffer available from input %d\n",
                    stage->name, i);
        }
    }

    while((pqe = pq_pop(s->p, NULL)) != NULL) {
        /* if no output buffer, get one */
        if(!merged_buf) {
            merged_buf = fg_pin_accept_buffer(buf_in);
            merged_buf->datalen= 0;

            fg_log(FG_LOG_MODULE, "%s> accepted empty buffer to fill\n",
                    stage->name);
        }

        /* copy record to output buffer */
        memcpy(merged_buf->data + merged_buf->datalen,
                pqe->buffer->data + pqe->offset, RECLEN);
        merged_buf->datalen += RECLEN;
        pqe->offset += RECLEN;

        /* if input buffer is exhausted, discard it and get another */
        if(pqe->offset >= pqe->buffer->size) {
            fg_pin_convey_buffer(buf_out, pqe->buffer);
            fg_log(FG_LOG_MODULE, "%s> input buffer %i exhausted\n",
                    stage->name, pqe->pin_index);

            pqe->buffer = fg_pin_array_accept_buffer(data_in, pqe->pin_index);
            pqe->offset = 0;
        }

        /* only insert into priority queue if pin is still active and data
         * remains */
        if(pqe->buffer && pqe->buffer->datalen > pqe->offset) {
            key = *((int64_t *) (pqe->buffer->data + pqe->offset));
            pq_insert(s->p, key, pqe);
        }

        /* if output buffer full, convey */
        if(merged_buf->datalen >= merged_buf->size) {
            fg_pin_convey_buffer(data_out, merged_buf);
            fg_log(FG_LOG_MODULE, "%s> conveyed merged buffer\n", stage->name);
            merged_buf = NULL;
        }
    }

    /* might have left-over data to send */
    if(merged_buf) {
        fg_pin_convey_buffer(data_out, merged_buf);
    }

    return FG_STAGE_TERMINATE;
}

/* scatter stage definition
 *************************************************************/

struct scatter_state {
    int num_procs;
    int recnum;
    splitter *splitters;
};

int scatter_init(FG_stage *stage)
{
    int i;
    char *filename;
    int fd;
    struct scatter_state *s;

    s = (struct scatter_state *) malloc(sizeof(struct scatter_state));

    MPI_Comm_size(MPI_COMM_WORLD, &s->num_procs);
    s->recnum = 0;

    filename = fg_stage_get_param(stage, "splitter_filename");
    fd = open(filename, O_RDONLY);

    s->splitters = (splitter *) calloc(s->num_procs, sizeof(splitter));
    read(fd, s->splitters, sizeof(splitter) * s->num_procs);

    close(fd);

    for(i=0; i<s->num_procs; i++) {
        printf("%s> splitter %d = %016lx == %ld (%ld, %ld)\n",
                stage->name, i, (s->splitters + i)->key,
                (s->splitters + i)->key, (s->splitters + i)->proc,
                (s->splitters + i)->index);
    }

    stage->data = s;

    return 0;
}

int scatter_func(FG_stage *stage)
{
    int i, n;
    int rank;
    int rc;
    int64_t *key;
    uint8_t *data = NULL;
    FG_pin *pin;
    FG_buf *buf;
    struct scatter_state *s;
    int bytes_per_round;

    s = (struct scatter_state *) stage->data;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    pin = fg_stage_pin_get_by_name(stage, "data_in");
    buf = fg_pin_accept_buffer(pin);

    if(!buf) {
        fg_log(FG_LOG_MODULE, "%s> scatter done %d\n", stage->name, rank);
        for(i=0; i<s->num_procs; i++) {
            rc = MPI_Send(data, 0, MPI_CHAR, i, DSORT_SCATTER_DONE,
                    MPI_COMM_WORLD);
            if(rc != MPI_SUCCESS) {
                printf("%s> MPI_Send failed\n", stage->name);
                return FG_STAGE_TERMINATE;
            }
        }
        return FG_STAGE_TERMINATE;
    }

    data = (uint8_t *) buf->data;

    bytes_per_round = 0;
    for(i=0; i<s->num_procs; i++) {
        n = 0;
        while(1) {
            if(data + n >= ((uint8_t *) buf->data) + buf->datalen) {
                fg_log(FG_LOG_MODULE, "%s> reached end of buffer (%d bytes)\n",
                        stage->name, n);
                break;
            }

            key = (int64_t *) (data + n);
            if(*key > (s->splitters + i)->key)
                break;
            else if(*key == (s->splitters + i)->key \
                  && rank > (s->splitters + i)->proc)
                break;
            else if(*key == (s->splitters + i)->key \
                  && rank == (s->splitters + i)->proc \
                  && s->recnum > (s->splitters + i)->index)
                break;

            n += RECLEN;
            s->recnum++;
        }

        if(n > 0) {
            rc = MPI_Send(data, n, MPI_CHAR, i, DSORT_DATA, MPI_COMM_WORLD);
            if(rc != MPI_SUCCESS)
                return FG_STAGE_TERMINATE;
        }
        fg_log(FG_LOG_MODULE, "%s> sent %d bytes to %d\n", stage->name, n, i);

        data += n;
        bytes_per_round += n;
    }

    fg_log(FG_LOG_MODULE, "%s> sent %d bytes total this round\n", stage->name,
            bytes_per_round);

    pin = fg_stage_pin_get_by_name(stage, "buf_out");
    fg_pin_convey_buffer(pin, buf);

    return FG_STAGE_SUCCESS;
}

/* gather stage definition
 *************************************************************/

/* assumes that buffer coming in on 'buf_in' pin is no smaller than single MPI
 * message */
int gather_func(FG_stage *stage)
{
    FG_pin *in_pin, *out_pin;
    FG_buf *buf;
    char *mpi_buf;
    MPI_Status status;
    int rc;
    int num_procs, num_done;
    int k, l;

    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    num_done = 0;

    in_pin = fg_stage_pin_get_by_name(stage, "buf_in");
    out_pin = fg_stage_pin_get_by_name(stage, "data_out");

    buf = fg_pin_accept_buffer(in_pin);
    buf->datalen = 0;

    mpi_buf = (char *) malloc(buf->size);

    /* I'm not convinced this is the best loop structure here */
    while(num_done < num_procs) {
        /* get an MPI message */
        rc = MPI_Recv(mpi_buf, buf->size, MPI_CHAR, MPI_ANY_SOURCE,
                MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if(rc != MPI_SUCCESS)
            return FG_STAGE_TERMINATE;

        /* if it's not a data msg, handle it accordingly */
        if(status.MPI_TAG == DSORT_SCATTER_DONE) {
            fg_log(FG_LOG_MODULE, "%s> received dsort scatter done from %d\n",
                    stage->name, status.MPI_SOURCE);
            num_done++;
            continue;
        }

        fg_log(FG_LOG_MODULE, "%s> received %d data bytes from %d\n",
                stage->name, status.count, status.MPI_SOURCE);

        /* either the received data will all fit in the current buffer */
        if(status.count < buf->size - buf->datalen) {
            memcpy(buf->data + buf->datalen, mpi_buf, status.count);
            buf->datalen += status.count;
            /* printf("%s> stuffed all %d bytes into current buffer\n",
                    stage->name, status.count); */
        }
        /* or it will have to be split */
        else {
            k = buf->size - buf->datalen;
            l = status.count - k;

            memcpy(buf->data + buf->datalen, mpi_buf, k);
            buf->datalen += k;
            /* printf("%s> stuffed %d of %d bytes into current buffer\n",
                    stage->name, k, status.count); */
            fg_pin_convey_buffer(out_pin, buf);

            buf = fg_pin_accept_buffer(in_pin);
            memcpy(buf->data, mpi_buf + k, l);
            /* printf("%s> stuffed remaining %d of %d bytes into new buffer\n",
                    stage->name, l, status.count); */
            buf->datalen = l;
        }

        /* if we've filled the output buffer, pass it on and get another */
        if(buf->datalen >= buf->size) {
            fg_pin_convey_buffer(out_pin, buf);
            buf = fg_pin_accept_buffer(in_pin);
            buf->datalen = 0;
        }

        /* printf("%s> %d bytes remaining in output buffer\n", stage->name,
                buf->size - buf->datalen); */
    }

    if(buf->datalen >= 0) {
        fg_pin_convey_buffer(out_pin, buf);
    }

    return FG_STAGE_TERMINATE;
}

