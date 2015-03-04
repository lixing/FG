/*
 * fg_pin.c
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "fg_internal.h"

FG_pin *fg_pin_create(const char *name, FG_stage *stage, int direction)
{
    FG_pin *p;

    p = (FG_pin *) malloc(sizeof(FG_pin));
    p->name = strdup(name);
    p->stage = stage;
    p->direction = direction;
    p->queue = NULL;
    p->cur_round_num = 0;
    p->bufsize = 0;         /* default to network setting */
    p->bufcount = 0;        /* default to network setting */

    /* HACK: magic number! */
    if(p->direction == PIN_ARRAY_IN) {
        p->queues = (FG_queue **) calloc(201, sizeof(FG_queue *));
        p->queue_count = 0;
    }

    return p;
}

void fg_pin_destroy(FG_pin *pin)
{
    if(pin) {
        if(pin->direction == PIN_IN)
            fg_queue_destroy(pin->queue);
        fg_pin_disconnect(pin);
        free(pin->name);
        free(pin);
    }
}

void fg_pin_set_buffer_size(FG_pin *pin, uint32_t size)
{
    if(pin)
        pin->bufsize = size;
}

void fg_pin_set_buffer_count(FG_pin *pin, uint32_t count)
{
    if(pin)
        pin->bufcount = count;
}

int fg_pin_connect(FG_stage *outs, const char *outp, FG_stage *ins,
        const char *inp)
{
    FG_pin *in_pin, *out_pin;
    FG_queue *q;

    out_pin = fg_stage_pin_get_by_name(outs, outp);
    in_pin = fg_stage_pin_get_by_name(ins, inp);

    if(!out_pin) {
        fprintf(stderr, "error: output pin %s.%s does not exist\n", outs->name, outp);
        exit(1);
    }

    if(out_pin->direction != PIN_OUT && out_pin->direction != PIN_ARRAY_OUT) {
        fprintf(stderr, "error: %s.%s is not an output pin\n", outs->name,
                out_pin->name);
        exit(1);
    }

    if(!in_pin) {
        fprintf(stderr, "error: input pin %s.%s does not exist\n", ins->name, inp);
        exit(1);
    }

    if(in_pin->direction != PIN_IN && in_pin->direction != PIN_ARRAY_IN) {
        fprintf(stderr, "error: %s.%s is not an input pin\n", ins->name,
                in_pin->name);
        exit(1);
    }

    if(in_pin->direction == PIN_ARRAY_IN) {
        q = fg_queue_create();
        *(in_pin->queues + in_pin->queue_count) = q;
        out_pin->queue = q;
        q->reader = in_pin;
        q->writer = out_pin;

        fg_log(FG_LOG_PIN, "connected %s.%s -> %s.%s[%d]\n",
                out_pin->stage->name, out_pin->name, in_pin->stage->name,
                in_pin->name, in_pin->queue_count);

        in_pin->queue_count++;
    } else {
        q = fg_queue_create();
        in_pin->queue = q;
        q->reader = in_pin;
        out_pin->queue = q;
        q->writer = out_pin;

        fg_log(FG_LOG_PIN, "connected %s.%s -> %s.%s\n", out_pin->stage->name,
                out_pin->name, in_pin->stage->name, in_pin->name);
    }

    return 0;
}

void fg_pin_disconnect(FG_pin *pin)
{
    /* is this a no-op?  should at least set pin->queue pointer to NULL. */
}

FG_buf *fg_pin_accept_buffer(FG_pin *pin) {
    return fg_queue_read(pin->queue);
}

void fg_pin_convey_buffer(FG_pin *pin, FG_buf *buf) {
    FG_pin *dst;

    if(pin->queue) {
        fg_queue_write(pin->queue, buf);

        dst = pin->queue->reader;
        fg_log(FG_LOG_BUFFER, "buffer %d, round %d passed from %s.%s to %s.%s\n",
                buf->id, buf->round_num, pin->stage->name, pin->name,
                dst->stage->name, dst->name);
    } else {
        fg_queue_write(buf->origin->queue, buf);

        dst = buf->origin;
        fg_log(FG_LOG_BUFFER, "buffer %d, round %d passed from %s.%s to %s.%s\n",
                buf->id, buf->round_num, pin->stage->name, pin->name,
                dst->stage->name, dst->name);
    }
}

FG_buf *fg_pin_array_accept_buffer(FG_pin *pin, int i) {
    FG_queue *q;

    if(!pin)
        return NULL;

    q = *(pin->queues + i);

    if(!q)
        return NULL;

    return fg_queue_read(q);
}

int fg_pin_array_get_width(FG_pin *pin)
{
    return pin->queue_count;
}

char *fg_pin_as_string(FG_pin *pin)
{
    static char s[64];

    if(!pin)
        return NULL;

/*
    snprintf(s, 64, "%s %s, connected to %s.%s", pin->stage->name, pin->name,
            pin->direction == PIN_IN ? "PIN_IN" :
            pin->direction == PIN_OUT ? "PIN_OUT" : "UNDEF",
            pin->other_end ? pin->other_end->name : "nothing");
*/

    return s;
}

