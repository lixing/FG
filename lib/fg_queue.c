/*
 * fg_queue.c
 *
 * Note: by design, the capacity of the queue is equal to the maximum number
 * of buffers in the system, so overflow cannot occur (theoretically).
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <pthread.h>

#include "fg_internal.h"

/* q->is_active could eventually be counter of active writers, which upon
 * reaching zero signals okay to shut down */

FG_queue *fg_queue_create(void)
{
    FG_queue *q;

    q = (FG_queue *) malloc(sizeof(FG_queue));
    if(!q)
        return NULL;

    pthread_mutex_init(&(q->mutex), NULL);
    pthread_cond_init(&(q->read_cv), NULL);
    q->head = NULL;
    q->tail = NULL;
    q->occupancy = 0;
    q->is_active = 1;
    q->reader = NULL;
    q->writer = NULL;

    return q;
}

void fg_queue_destroy(FG_queue *q)
{
    if(q) {
        free(q);
    }
}

/* on deactivation, the queue will accept no more writes, but will fulfill
 * reads until empty, at which point read will return NULL */
void fg_queue_deactivate(FG_queue *q)
{
    if(!q)
        return;

    pthread_mutex_lock(&(q->mutex));

    q->is_active = 0;
    fg_log(FG_LOG_QUEUE, "%s> queue deactivated: %s\n", q->writer->stage->name,
            q->writer->name);

    pthread_cond_signal(&(q->read_cv));
    pthread_mutex_unlock(&(q->mutex));
}

int fg_queue_write(FG_queue *q, FG_buf *buf)
{
    pthread_mutex_lock(&(q->mutex));

    if(q->is_active == 0) {
        pthread_mutex_unlock(&(q->mutex));
        return -1;
    }

    if(q->tail) {
        q->tail->next = buf;
        q->tail = buf;
        buf->next = q->head;
    } else {
        q->head = buf;
        q->tail = buf;
        buf->next = buf;
    }

    if(q->writer) {
        fg_log(FG_LOG_QUEUE, "%s> wrote buffer %d (round %d)\n",
                q->writer->stage->name, buf->id, buf->round_num);
    } else {
        buf->round_num = q->reader->cur_round_num;
        q->reader->cur_round_num++;

        fg_log(FG_LOG_QUEUE, "NULL> wrote buffer %d (round %d)\n",
                buf->id, buf->round_num);
    }

    q->occupancy++;

    pthread_cond_signal(&(q->read_cv));
    pthread_mutex_unlock(&(q->mutex));

    return 0;
}

/* blocking read: returns to pointer to buffer; if queue has been deactivated
 * and is empty, returns NULL */
FG_buf *fg_queue_read(FG_queue *q)
{
    FG_buf *buf;

/*
    fg_log(FG_LOG_QUEUE, "%d> %s/%s locking\n", q->read_pin->stage->id,
            q->read_pin->stage->name, q->read_pin->name);
*/
    pthread_mutex_lock(&(q->mutex));

    while(q->occupancy == 0) {
        if(! q->is_active) {
            pthread_mutex_unlock(&(q->mutex));
            return NULL;
        }

        pthread_cond_wait(&(q->read_cv), &(q->mutex));
    }

    buf = q->head;
    q->occupancy--;

    if(q->occupancy == 0) {
        q->head = NULL;
        q->tail = NULL;
    } else {
        q->head = q->head->next;
    }

    fg_log(FG_LOG_QUEUE, "%s> read buffer %d (round %d)\n",
            q->reader->stage->name, buf->id, buf->round_num);

    pthread_mutex_unlock(&(q->mutex));

    return buf;
}

