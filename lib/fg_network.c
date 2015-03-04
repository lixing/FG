/*
 * fg_network.c
 */

#define _GNU_SOURCE /* for asprintf() */
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>

#include "fg_internal.h"

FG_network *fg_network_create(const char *name, uint32_t default_bufcount,
        uint32_t default_bufsize)
{
    FG_network *nw;

    nw = (FG_network *) malloc (sizeof(FG_network));
    if(nw == NULL)
        return NULL;

    fg_log(FG_LOG_NETWORK, "new network %s\n", name);

    nw->name = strdup(name);

    /* HACK: magic number! */
    nw->stages = (FG_stage **) calloc(401, sizeof(FG_stage *));
    nw->stage_count = 0;

    nw->default_bufcount = default_bufcount;
    nw->default_bufsize = default_bufsize;

    /* HACK: magic number! */
    nw->params = (FG_param_rename **) calloc(11, sizeof(FG_param_rename *));

    return nw;
}

void fg_network_set_default_bufcount(FG_network *nw,
        uint32_t default_bufcount)
{
    if(!nw)
        return;

    nw->default_bufcount = default_bufcount;
}

void fg_network_set_default_bufsize(FG_network *nw, uint32_t default_bufsize)
{
    if(!nw)
        return;

    nw->default_bufsize = default_bufsize;
}

void fg_network_destroy(FG_network *nw)
{
    FG_stage **s;
    FG_param_rename **pr;

    if(nw) {
        fg_log(FG_LOG_NETWORK, "destroying network %s\n", nw->name);

        for(s=nw->stages; *s; s++)
            fg_stage_destroy(*s);
        free(nw->stages);

        for(pr=nw->params; *pr; pr++) {
            free((*pr)->name);
            free((*pr)->local_name);
            free(*pr);
        }
        free(nw->params);

        free(nw->name);
        free(nw);
    }
}

FG_stage *fg_network_get_stage_by_name(FG_network *nw, const char *stage_name)
{
    FG_stage **stage;

    if(!nw || !(nw->stages))
        return NULL;

    for(stage = nw->stages; *stage; stage++) {
        if(strcmp(stage_name, (*stage)->name) == 0)
            return *stage;
    }

    return NULL;
}

int fg_network_rename_param(FG_network *nw, const char *stage_name,
        const char *param_name, const char *new_name)
{
    FG_stage *s;
    const char **p;
    char **v;
    FG_param_rename **pr;

    s = fg_network_get_stage_by_name(nw, stage_name);
    if(!s)
        return -1;

    /* check if param_name is valid */
    if(!s->sd || !s->sd->params)
        return -1;
    for(p = s->sd->params, v = s->param_vals; *p; p++, v++) {
        if(strcmp(param_name, *p) == 0)
            break;
    }
    if(!*p)
        return -1;

    /* find end of param rename list and add new entry */
    for(pr = nw->params; *pr; pr++) {     }
    *pr = (FG_param_rename *) malloc(sizeof(FG_param_rename));
    if(!*pr)
        return -1;

    (*pr)->name = strdup(new_name);
    (*pr)->stage = s;
    (*pr)->local_name = strdup(param_name);

    return 0;
}

/* TODO: "param" is not const because we use strtok on it---bad idea? */
int fg_network_set_param(FG_network *nw, char *param, const char *value)
{
    const char *stage_name;
    FG_stage *stage;
    char *param_name;
    FG_param_rename **pr;

    if(!nw)
        return -1;

    /* first check if param name is in lookup table */
    if(nw->params) {
        for(pr = nw->params; *pr; pr++) {
            if(strcmp(param, (*pr)->name) == 0) {
                stage = (*pr)->stage;
                param_name = (*pr)->local_name;
                fg_stage_set_param(stage, param_name, value);
                return 0;
            }
        }
    }

    /* otherwise assume it's of form <stage name>.<param name> */
    stage_name = param;
    param_name = strtok(param, ".");
    stage = fg_network_get_stage_by_name(nw, stage_name);
    fg_stage_set_param(stage, param_name, value);

    return 0;
}

char *fg_network_get_param(FG_network *nw, const char *param)
{
    return NULL;
}

int fg_network_stage_add(FG_network *nw, FG_stage *stage)
{
    nw->stages[nw->stage_count] = stage;
    stage->nw = nw;
    nw->stage_count++;

    return 0;
}

int fg_network_fix(FG_network *nw)
{
    FG_stage **stage;
    FG_pin **pin;
    FG_buf *buf;
    int i;
    int buf_id;
    uint32_t bufsize;
    uint32_t bufcount;
    int rc;
    const char **p;
    char **v;

    fg_log(FG_LOG_NETWORK, "Fixing network %s\n", nw->name);
    fg_log(FG_LOG_NETWORK, "found %d stages\n", nw->stage_count);

    /* initialize stages */
    fg_log(FG_LOG_NETWORK, "initializing stages\n");
    for(stage = nw->stages; *stage; stage++) {
        fg_log(FG_LOG_STAGE, "initializing stage %s with params:\n",
                (*stage)->name);
        if((*stage)->sd->params) {
            for(p = (*stage)->sd->params, v = (*stage)->param_vals; *p;
                    p++, v++) {
                fg_log(FG_LOG_STAGE, "    %s = %s\n", *p, *v);
            }
        }

        if((*stage)->sd->init) {
            rc = (*stage)->sd->init(*stage);
            if(rc < 0) {
                fprintf(stderr, "%s> stage initialization failed\n", (*stage)->name);
                fg_stage_destroy(*stage);
                return -1;
            }
        }
    }

    /* for each unconnected source pin, create and add buffers to queue */
    fg_log(FG_LOG_NETWORK, "finding source pins:\n");
    buf_id = 0;
    for(stage = nw->stages; *stage; stage++) {
        for(pin = (*stage)->pins; *pin && (*pin)->name; pin++) {
            if((*pin)->direction == PIN_IN && (*pin)->queue == NULL) {
                (*pin)->queue = fg_queue_create();
                (*pin)->queue->reader = *pin;

                bufsize = (*pin)->bufsize ? (*pin)->bufsize : nw->default_bufsize;
                bufcount = (*pin)->bufcount ? (*pin)->bufcount : nw->default_bufcount;

                /* allocate buffers, add to queue */
                for(i=0; i<bufcount; i++) {
                    buf = fg_buffer_create(buf_id++, bufsize);
                    fg_queue_write((*pin)->queue, buf);
                    buf->origin = *pin;
                }

                fg_log(FG_LOG_NETWORK, "  %s.%s: %d x %db buffers\n",
                        (*pin)->stage->name, (*pin)->name, bufcount, bufsize);
            }
        }
    }

    return 0;
}

void fg_network_run(FG_network *nw)
{
    FG_stage **stage;

    /* create one thread for each stage and watch 'em go! */
    fg_log(FG_LOG_NETWORK, "Creating threads\n");
    for(stage = nw->stages; *stage; stage++) {
        fg_log(FG_LOG_NETWORK, "  %s @ %p\n", (*stage)->name, *stage);
        pthread_create(&((*stage)->thread), NULL, fg_stage_handler, *stage);
    }
    fg_log(FG_LOG_NETWORK, "Creating threads complete\n");

    /* join threads once they're done */
    for(stage = nw->stages; *stage; stage++) {
        pthread_join((*stage)->thread, NULL);
        fg_log(FG_LOG_NETWORK, "joined thread %s @ %p\n", (*stage)->name, *stage);
    }

    /* and let the stages clean themselves up */
    for(stage = nw->stages; *stage; stage++) {
        fg_log(FG_LOG_NETWORK, "finalizing stage %s\n", (*stage)->name);
        if((*stage)->sd->fini)
            (*stage)->sd->fini(*stage);
    }
}

void fg_network_halt(FG_network *nw)
{
    FG_stage **stage;

    fg_log(FG_LOG_NETWORK, "halting network %s\n", nw->name);

    for(stage = nw->stages; *stage; stage++) {
        fg_log(FG_LOG_NETWORK, "canceling stage %s\n", (*stage)->name);
        pthread_cancel((*stage)->thread);
    }
}

FG_network *fg_network_copy(FG_network *nw, const char *name)
{
    FG_network *new_nw;
    FG_stage **s;
    FG_stage *new_stage, *new_stage2;
    const char **param;
    char **value;
    FG_pin **pin;
    FG_param_rename **pr;

    new_nw = fg_network_create(name, nw->default_bufcount, nw->default_bufsize);
    if(!new_nw)
        return NULL;

    /* create stages and populate parameters, if set */
    for(s=nw->stages; *s; s++) {
        new_stage = fg_stage_create(new_nw, (*s)->sd->name, (*s)->name);
        if(new_stage->sd->params) {
            for(param=new_stage->sd->params, value=(*s)->param_vals;
                    *param; param++, value++) {
                if(*value)
                    fg_stage_set_param(new_stage, *param, *value);
            }
        }
    }

    /* duplicate pin connections */
    for(s=nw->stages; *s; s++) {
        for(pin=(*s)->pins; *pin; pin++) {
            if((*pin)->direction == PIN_IN && (*pin)->queue) {
                new_stage = fg_network_get_stage_by_name(new_nw, (*s)->name);
                new_stage2 = fg_network_get_stage_by_name(new_nw,
                        (*pin)->queue->writer->stage->name);
                fg_pin_connect(new_stage2, (*pin)->queue->writer->name,
                        new_stage, (*pin)->name);
            }
        }
    }

    /* duplicate parameter renaming */
    for(pr=nw->params; *pr; pr++) {
        fg_network_rename_param(new_nw, (*pr)->stage->name, (*pr)->name,
                (*pr)->local_name);
    }

    return new_nw;
}

int fg_network_merge(FG_network *nw1, FG_network *nw2)
{
    FG_stage **s;
    FG_param_rename **pr;
    char *tmp;

    printf("merging %s into %s\n", nw2->name, nw1->name);

    /* merge stages */
    for(s=nw2->stages; *s; s++) {
        nw1->stages[nw1->stage_count] = *s;
        tmp = (*s)->name;
        asprintf(&((*s)->name), "%s.%s", nw2->name, (*s)->name);
        nw1->stage_count++;

        /* reclaim memory from the old name and remove stage from nw2's list
         * of stages---this lets us destroy nw2 without taking the merged
         * stages with it */
        free(tmp);
        *s = NULL;
    }

    /* merge renamed parameters */
    for(pr=nw2->params; *pr; pr++) {
        asprintf(&tmp, "%s.%s", nw2->name, (*pr)->name);
        fg_network_rename_param(nw1, (*pr)->stage->name, (*pr)->local_name,
                tmp);
        /* no need to free(tmp) because fg_network_destroy will do it */
    }

    fg_network_destroy(nw2);

    return 0;
}

void fg_network_print(FG_network *nw)
{
    FG_stage **s;
    FG_param_rename **pr;

    if(!nw)
        return;

    printf("network %s:\n", nw->name);

    for(s=nw->stages; *s; s++) {
        printf("  stage %s (instance of %s)\n", (*s)->name, (*s)->sd->name);
    }

    for(pr=nw->params; *pr; pr++) {
        printf("  param rename: %s.%s -> %s\n", (*pr)->name, (*pr)->local_name,
                (*pr)->name);
    }
}

