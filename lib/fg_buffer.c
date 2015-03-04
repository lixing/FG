/*
 * fg_buffer.c
 */

#include <stdio.h>
#include <malloc.h>
#include <openssl/sha.h>

#include "fg_internal.h"

FG_buf *fg_buffer_create(int id, int size)
{
    FG_buf *buf;

    buf = (FG_buf *) malloc(sizeof(FG_buf));

    buf->id = id;
    buf->size = size;
    buf->data = (char *) malloc(size);

    return buf;
}

void fg_buffer_destroy(FG_buf *buf)
{
    if(buf) {
        free(buf->data);
        free(buf);
    }
}

