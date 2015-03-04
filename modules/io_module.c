/*
 * io_module.c
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "fg_internal.h"

/* read stage definition prototypes */
char read_name[] = "read-file";
char read_doc[] = "read doc";
int read_init(FG_stage *stage);
void file_io_fini(FG_stage *stage);
int read_func(FG_stage *stage);
FG_pin read_pins[] = { { "buf_in",   PIN_IN  },
                       { "data_out", PIN_OUT },
                       { NULL }
                     };
const char *read_params[] = { "filename",
                              NULL
                            };

/* write stage definition prototypes */
char write_name[] = "write-file";
char write_doc[] = "write doc";
int write_init(FG_stage *stage);
int write_func(FG_stage *stage);
FG_pin write_pins[] = { { "data_in", PIN_IN  },
                        { "buf_out", PIN_OUT },
                        { NULL }
                      };
const char *write_params[] = { "filename",
                               NULL
                             };

/* multiwrite stage definition prototypes */
char multiwrite_name[] = "multiwrite-file";
char multiwrite_doc[] = "writes each buffer accepted into separate file";
int multiwrite_init(FG_stage *stage);
int multiwrite_func(FG_stage *stage);
FG_pin multiwrite_pins[] = { { "data_in", PIN_IN  },
                             { "buf_out", PIN_OUT },
                             { NULL }
                           };
const char *multiwrite_params[] = { "filename_fmt",
                                    NULL
                                  };

/* combine stage definition prototypes */
char combine_name[] = "rr-combine";
char combine_doc[] = "round robin combine";
int combine_func(FG_stage *stage);
FG_pin combine_pins[] = { { "data_in",  PIN_ARRAY_IN },
                          { "data_out", PIN_OUT      },
                          { NULL }
                        };

/* module defs */
char *fg_module_name = "i/o operations";
FG_stage_def fg_module_export[] = {
    { read_name, read_doc, read_init, read_func, file_io_fini, read_pins, read_params },
    { write_name, write_doc, write_init, write_func, file_io_fini, write_pins, write_params },
    { multiwrite_name, multiwrite_doc, multiwrite_init, multiwrite_func, NULL, multiwrite_pins, multiwrite_params },
    { combine_name, combine_doc, NULL, combine_func, NULL, combine_pins, NULL },
    { NULL }
};

struct file_io_state {
    char *filename;
    FILE *file;
    int bytes_so_far;
};

/* read stage definition
 *************************************************************/
int read_init(FG_stage *stage)
{
    struct file_io_state *s;

    s = (struct file_io_state *) malloc(sizeof(struct file_io_state));

    s->filename = fg_stage_get_param(stage, "filename");
    s->file = fopen(s->filename, "r");
    s->bytes_so_far = 0;

    stage->data = s;

    printf("%s> opened %s for reading\n", stage->name, s->filename);

    return 0;
}

void file_io_fini(FG_stage *stage)
{
    struct file_io_state *s = (struct file_io_state *) stage->data;

    fclose(s->file);
    free(s->filename);
    free(s);

    printf("%s> closed file\n", stage->name);
}

int read_func(FG_stage *stage)
{
    struct file_io_state *s = (struct file_io_state *) stage->data;
    FG_pin *pin;
    FG_buf *buf;
    int n;
    int c;

    pin = fg_stage_pin_get_by_name(stage, "buf_in");
    buf = fg_pin_accept_buffer(pin);

    n = fread(buf->data, 1, buf->size, s->file);
    buf->datalen = n;
    s->bytes_so_far += n;
    printf("%s> read %d bytes (%d total)\n", stage->name, n, s->bytes_so_far);

    pin = fg_stage_pin_get_by_name(stage, "data_out");
    fg_pin_convey_buffer(pin, buf);

    c = fgetc(s->file);
    if(c == EOF) {
        printf("%s> EOF reached\n", stage->name);
        return FG_STAGE_TERMINATE;
    } else {
        ungetc(c, s->file);
    }

    return FG_STAGE_SUCCESS;
}

/* write stage definition
 ***************************************************************/

int write_init(FG_stage *stage)
{
    struct file_io_state *s;

    s = (struct file_io_state *) malloc(sizeof(struct file_io_state));

    s->filename = fg_stage_get_param(stage, "filename");
    s->file = fopen(s->filename, "w");
    s->bytes_so_far = 0;

    stage->data = s;

    printf("%s> opened %s for writing\n", stage->name, s->filename);

    return 0;
}

int write_func(FG_stage *stage)
{
    struct file_io_state *s = (struct file_io_state *) stage->data;
    FG_pin *pin;
    FG_buf *buf;
    int n;

    pin = fg_stage_pin_get_by_name(stage, "data_in");
    buf = fg_pin_accept_buffer(pin);

    if(!buf) {
        return FG_STAGE_TERMINATE;
    }

    n = fwrite(buf->data, 1, buf->datalen, s->file);
    s->bytes_so_far += n;
    printf("%s> wrote %d bytes (%d total)\n", stage->name, n,
            s->bytes_so_far);

    pin = fg_stage_pin_get_by_name(stage, "buf_out");
    fg_pin_convey_buffer(pin, buf);

    return FG_STAGE_SUCCESS;
}

/* multiwrite stage definition
 ***************************************************************/

struct multiwrite_state {
    char *filename_fmt;
    int i;
};

int multiwrite_init(FG_stage *stage)
{
    struct multiwrite_state *s;

    s = (struct multiwrite_state *) malloc(sizeof(struct multiwrite_state));
    if(!s)
        return -1;

    s->filename_fmt = fg_stage_get_param(stage, "filename_fmt");
    s->i = 0;

    stage->data = s;

    return 0;
}

int multiwrite_func(FG_stage *stage)
{
    FG_pin *pin;
    FG_buf *buf;
    struct multiwrite_state *s;
    FILE *f;
    char filename[BUFSIZ];

    s = (struct multiwrite_state *) stage->data;

    pin = fg_stage_pin_get_by_name(stage, "data_in");
    buf = fg_pin_accept_buffer(pin);

    if(!buf)
        return FG_STAGE_TERMINATE;

    snprintf(filename, BUFSIZ, s->filename_fmt, s->i);
    s->i++;

    f = fopen(filename, "w");
    fwrite(buf->data, buf->datalen, 1, f);
    fclose(f);

    printf("%s> wrote %d bytes to %s\n", stage->name, buf->datalen, filename);

    pin = fg_stage_pin_get_by_name(stage, "buf_out");
    fg_pin_convey_buffer(pin, buf);

    return FG_STAGE_SUCCESS;
}

/* combine stage definition
 ***************************************************************/

int combine_func(FG_stage *stage) {
    FG_pin *in_pin, *out_pin;
    FG_buf *buf;
    int i, n;
    int buf_count;

    in_pin = fg_stage_pin_get_by_name(stage, "data_in");
    n = fg_pin_array_get_width(in_pin);

    out_pin = fg_stage_pin_get_by_name(stage, "data_out");

    buf_count = 0;
    for(i=0; i<n; i++) {
        buf = fg_pin_array_accept_buffer(in_pin, i);
        if(!buf)
            continue;

        fg_pin_convey_buffer(out_pin, buf);
        buf_count++;
    }

    if(buf_count == 0)
        return FG_STAGE_TERMINATE;
    else
        return FG_STAGE_SUCCESS;
}

