/*
 * mpi_module.c
 */

#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "fg_internal.h"

#define MPI_TAG_TX_END 1

/* recv stage definition prototypes */
char recv_name[] = "mpi-recv-prev";
char recv_doc[] = "mpi-recv-prev doc";
int recv_func(FG_stage *stage);
FG_pin recv_pins[] = { { "buf_in",   PIN_IN  },
                       { "data_out", PIN_OUT },
                       { NULL }
                     };

/* send stage definition prototypes */
char send_name[] = "mpi-send-next";
char send_doc[] = "mpi-send-next doc";
int send_func(FG_stage *stage);
FG_pin send_pins[] = { { "data_in", PIN_IN  },
                       { "buf_out", PIN_OUT },
                       { NULL }
                     };

/* module defs */
char *fg_module_name = "mpi i/o operations";
FG_stage_def fg_module_export[] = {
    { recv_name, recv_doc, NULL, recv_func, NULL, recv_pins },
    { send_name, send_doc, NULL, send_func, NULL, send_pins },
    { NULL }
};

/* recv stage definition
 *************************************************************/
int recv_func(FG_stage *stage)
{
    FG_pin *pin;
    FG_buf *buf;
    MPI_Status status;
    int rc;

    pin = fg_stage_pin_get_by_name(stage, "buf_in");
    buf = fg_pin_accept_buffer(pin);

    rc = MPI_Recv(buf->data, buf->size, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG,
            MPI_COMM_WORLD, &status);
    if(rc != MPI_SUCCESS)
        return FG_STAGE_TERMINATE;

    printf("%s> received %d bytes from %d (tag: %d)\n",
            stage->name, status.count, status.MPI_SOURCE, status.MPI_TAG);

    pin = fg_stage_pin_get_by_name(stage, "data_out");

    if(status.MPI_TAG == MPI_TAG_TX_END) {
        buf->datalen = 0;
        printf("%s> received tx end\n", stage->name);
    } else {
        buf->datalen = status.count;
    }

    fg_pin_convey_buffer(pin, buf);

    return (status.MPI_TAG == MPI_TAG_TX_END)
        ? FG_STAGE_TERMINATE : FG_STAGE_SUCCESS;
}

/* send stage definition
 ***************************************************************/
int send_func(FG_stage *stage)
{
    FG_pin *pin;
    FG_buf *buf;
    int rc;
    int dst, rank, size;
    char blah[1];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    pin = fg_stage_pin_get_by_name(stage, "data_in");
    buf = fg_pin_accept_buffer(pin);

    /* send to next node by rank */
    dst = (rank + 1) % size;

    if(buf) {
        rc = MPI_Send(buf->data, buf->datalen, MPI_CHAR, dst, 0, MPI_COMM_WORLD);
        if(rc != MPI_SUCCESS)
            return FG_STAGE_TERMINATE;

        printf("%s> sent %d bytes to %d\n", stage->name, buf->datalen, dst);

        pin = fg_stage_pin_get_by_name(stage, "buf_out");
        fg_pin_convey_buffer(pin, buf);

        return FG_STAGE_SUCCESS;
    } else {
        rc = MPI_Send(blah, 0, MPI_CHAR, dst, MPI_TAG_TX_END, MPI_COMM_WORLD);
        printf("%s> sent tx end msg to %d\n", stage->name, dst);
        return FG_STAGE_TERMINATE;
    }
}

