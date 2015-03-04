/*
 * fg_internal.h
 */

#ifndef __FG_INTERNAL_H
#define __FG_INTERNAL_H

#include <stdarg.h>
#include <pthread.h>

#include "FG.h"

enum fg_stage_result {
    FG_STAGE_SUCCESS,
    FG_STAGE_TERMINATE
};

enum fg_log_domain {
    FG_LOG_MAIN = 0x1,
    FG_LOG_QUEUE = 0x2,
    FG_LOG_BUFFER = 0x4,
    FG_LOG_PIN = 0x8,
    FG_LOG_STAGE = 0x10,
    FG_LOG_NETWORK = 0x20,
    FG_LOG_MODULE = 0x40
};

struct _FG_module {
    char *path;
    char *name;
    FG_stage_def *stage_defs;
    void *handle;
};

struct _FG_param_rename {
    char *name;
    FG_stage *stage;
    char *local_name;
};
typedef struct _FG_param_rename FG_param_rename;

struct _FG_network {
    char *name;
    int stage_count;
    FG_stage **stages;
    unsigned int default_bufsize;
    unsigned int default_bufcount;
    FG_param_rename **params;
};

struct _FG_stage_def {
    const char *name;
    const char *doc;
    int (*init)(FG_stage *stage);
    int (*func)(FG_stage *stage);
    void (*fini)(FG_stage *stage);
    FG_pin *pins;
    const char **params;
};

struct _FG_stage {
    char *name;
    void *data;
    pthread_t thread;
    FG_pin **pins;
    FG_stage_def *sd;
    FG_network *nw;
    char **param_vals;
};

struct _FG_pin {
    char *name;
    int direction;
    FG_stage *stage;
    FG_queue *queue;        /* could be made */
    FG_queue **queues;      /* into a union */
    uint32_t queue_count;
    uint32_t bufcount;
    uint32_t bufsize;
    uint32_t cur_round_num;
};

struct _FG_buf {
    unsigned int id;
    unsigned int round_num;
    FG_pin *origin;
    char *data;
    unsigned int size;
    unsigned int datalen;       /* if data doesn't occupy whole buffer... */
    FG_buf *next;
};

struct _FG_queue {
    pthread_mutex_t mutex;
    pthread_cond_t read_cv;
    FG_pin *writer;
    FG_pin *reader;
    FG_buf *head;
    FG_buf *tail;
    unsigned int occupancy;
    int is_active;
};

/* FG context */
int fg_module_load(const char *path);
void fg_module_unload(FG_module *module);
FG_stage_def *fg_stage_def_get_by_name(const char *name);
void fg_log(int log_domain, const char *s, ...);
FG_stage_def **fg_get_stage_defs(void);

/* networks */
int fg_network_stage_add(FG_network *nw, FG_stage *stage);
void fg_network_halt(FG_network *nw);

/* stages */

/* pins */
enum pindir { PIN_IN,
              PIN_ARRAY_IN,
              PIN_OUT,
              PIN_ARRAY_OUT };
FG_pin *fg_pin_create(const char *name, FG_stage *stage, int direction);
void fg_pin_destroy(FG_pin *pin);
void fg_pin_disconnect(FG_pin *pin);
FG_buf *fg_pin_accept_buffer(FG_pin *pin);
void fg_pin_convey_buffer(FG_pin *pin, FG_buf *buf);

int fg_pin_array_get_width(FG_pin *pin);
FG_buf *fg_pin_array_accept_buffer(FG_pin *pin, int n);

/* queues */
FG_queue *fg_queue_create(void);
void fg_queue_destroy(FG_queue *q);
int fg_queue_write(FG_queue *q, FG_buf *buffer);
FG_buf *fg_queue_read(FG_queue *q);
void fg_queue_deactivate(FG_queue *q);

/* buffers */
FG_buf *fg_buffer_create(int id, int size);
void fg_buffer_destroy(FG_buf *buf);

/* helpers */
char *fg_pin_as_string(FG_pin *pin);
char *fg_stage_as_string(FG_stage *stage);
void print_stages(FG_network *nw);
void *fg_stage_handler(void *data);

#endif /* __FG_INTERNAL_H */

