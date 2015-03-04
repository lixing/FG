/*
 * fg_stage.c
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "fg_internal.h"

FG_stage *fg_stage_create(FG_network *nw, const char *stage_def_name,
        const char *stage_name)
{
    FG_stage *stage;
    FG_stage_def *stage_def;
    FG_pin *sd_pin;
    FG_pin *pin;
    int pin_count = 0;

    fg_log(FG_LOG_STAGE, "new stage %s (%s)\n", stage_name, stage_def_name);

    stage_def = fg_stage_def_get_by_name(stage_def_name);
    if(!stage_def) {
        fprintf(stderr, "Definition for stage \"%s\" not found\n",
                stage_def_name);
        return NULL;
    }

    stage = (FG_stage *) malloc(sizeof(FG_stage));
    stage->name = strdup(stage_name);
    stage->sd = stage_def;
    stage->nw = NULL;

    /* HACK: magic number! */
    stage->pins = (FG_pin **) calloc(10, sizeof(FG_pin *));
    /* HACK: magic number! (should tell from number of params in sd) */
    stage->param_vals = (char **) calloc(10, sizeof(char *));

    /* instantiate pins */
    for(sd_pin = stage_def->pins; sd_pin->name; sd_pin++) {
        pin = fg_pin_create(sd_pin->name, stage, sd_pin->direction);
        stage->pins[pin_count] = pin;
        pin_count++;

        fg_log(FG_LOG_STAGE, "    pin %s\n", pin->name);
    }

    fg_network_stage_add(nw, stage);

    return stage;
}

void fg_stage_destroy(FG_stage *s)
{
    FG_pin **pin;

    if(s) {
        for(pin=s->pins; *pin; pin++)
            fg_pin_destroy(*pin);

        free(s->pins);
        free(s->name);
        free(s);
    }
}

int fg_stage_set_param(FG_stage *stage, const char *param, const char *value) {
    const char **p;
    char **v;

    if(stage && stage->sd && stage->sd->params) {
        for(p = stage->sd->params, v = stage->param_vals; *p; p++, v++) {
            if(strcmp(param, *p) == 0) {
                *v = strdup(value);
                return 0;
            }
        }
    }

    return -1;
}

char *fg_stage_get_param(FG_stage *stage, const char *param) {
    const char **p;
    char **v;

    if(stage && stage->sd && stage->sd->params) {
        for(p = stage->sd->params, v = stage->param_vals; *p; p++, v++) {
            if(strcmp(param, *p) == 0) {
                return *v;
            }
        }
    }

    return NULL;
}

void *fg_stage_handler(void *data) {
    FG_stage *stage;
    FG_pin **pin;
    int rc = FG_STAGE_SUCCESS;

    stage = (FG_stage *) data;

    fg_log(FG_LOG_STAGE, "%s> HANDLER STARTING\n", stage->name);

    while(rc == FG_STAGE_SUCCESS) {
        /* printf("%d> handler for stage %s starting\n", stage->id, stage->name); */
        rc = stage->sd->func(stage);
        /* printf("%d> handler for stage %s complete\n", stage->id, stage->name); */
    }

    fg_log(FG_LOG_STAGE, "%s> stage complete\n", stage->name);

    /* deactivate all outgoing queues */
    for(pin = stage->pins; *pin; pin++) {
        if((*pin)->direction == PIN_OUT) {
            fg_queue_deactivate((*pin)->queue);
        }
    }

    return stage;
}

char *fg_stage_as_string(FG_stage *stage)
{
    static char s[128];
    FG_pin **pin;

    for(pin = stage->pins; *pin; pin++)
        printf("  pin: %s\n", fg_pin_as_string(*pin));

    return s;
}

FG_pin *fg_stage_pin_get_by_name(FG_stage *stage, const char *name)
{
    FG_pin **pin;

    for(pin = stage->pins; *pin; pin++) {
        if(strcmp((*pin)->name, name) == 0) {
            return *pin;
        }
    }

    return NULL;
}

