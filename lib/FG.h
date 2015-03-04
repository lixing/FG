/*
 * FG.h
 *
 * Entities visible to applications using FG.
 */

#ifndef __FG_H
#define __FG_H

#include <stdint.h>

typedef struct _FG_network FG_network;
typedef struct _FG_stage FG_stage;
typedef struct _FG_pin FG_pin;
typedef struct _FG_queue FG_queue;
typedef struct _FG_buf FG_buf;

typedef struct _FG_module FG_module;
typedef struct _FG_stage_def FG_stage_def;

/* FG context */
int fg_init(int *argc, char **argv[]);
int fg_fini(void);
void fg_print_stage_defs(void);

/* networks */
FG_network *fg_network_create(const char *name, uint32_t default_bufcount,
        uint32_t default_bufsize);
void fg_network_set_default_bufcount(FG_network *nw,
        uint32_t default_bufcount);
void fg_network_set_default_bufsize(FG_network *nw, uint32_t default_bufsize);
void fg_network_destroy(FG_network *nw);
int fg_network_fix(FG_network *nw);
void fg_network_run(FG_network *nw);
void fg_network_print(FG_network *nw);
FG_stage *fg_network_get_stage_by_name(FG_network *nw, const char *stage_name);
int fg_network_rename_param(FG_network *nw, const char *stage_name,
        const char *param_name, const char *new_name);
FG_network *fg_network_copy(FG_network *nw, const char *name);
int fg_network_merge(FG_network *nw1, FG_network *nw2);

int fg_network_set_param(FG_network *nw, char *param, const char *value);
char *fg_network_get_param(FG_network *nw, const char *param);

/* fg_network_config.c */
FG_network *fg_network_from_config(const char *name, const char *filename);

/* stages */
FG_stage *fg_stage_create(FG_network *nw, const char *stage_def_name,
        const char *stage_name);
int fg_stage_set_param(FG_stage *stage, const char *param, const char *value);
char *fg_stage_get_param(FG_stage *stage, const char *param);
void fg_stage_destroy(FG_stage *stage);  /* here or internal? */
FG_pin *fg_stage_pin_get_by_name(FG_stage *stage, const char *name);

/* pins */
int fg_pin_connect(FG_stage *ins, const char *inp, FG_stage *outs,
        const char *outp);
void fg_pin_set_buffer_size(FG_pin *pin, uint32_t size);
void fg_pin_set_buffer_count(FG_pin *pin, uint32_t count);

#endif /* __FG_H */

