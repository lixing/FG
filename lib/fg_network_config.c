/*
 * fg_network_config.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "fg_internal.h"

int process_cmd(FG_network *nw, char *s);
char *substitute_dollar(char *s, int n);

FG_network *fg_network_from_config(const char *name, const char *filename)
{
    int linenum;
    FILE *f;
    FG_network *nw;
    char line[1024];

    nw = fg_network_create(name, 4, 256);

    f = fopen(filename, "r");
    if(!f) {
        fprintf(stderr, "Error opening %s: %s\n", filename, strerror(errno));
        exit(1);
    }

    linenum = 1;
    while(fgets(line, sizeof(line) - 1, f)) {
        if(process_cmd(nw, line) != 0) {
            fprintf(stderr, "error on line %d\n", linenum);
            exit(1);
        }

        linenum++;
    }

    fclose(f);

    return nw;
}

int process_cmd(FG_network *nw, char *s)
{
    FG_stage *stage0, *stage1;
    FG_pin *pin0;
    char *cmd, *a, *b;
    char *stage0_name, *stage1_name;
    char *pin0_name, *pin1_name;
    char *param_name, *param_value;
    int i, n;

    cmd = strtok(s, " ");
    a = strtok(NULL, " ");
    b = strtok(NULL, "\n");

    if(strcmp(cmd, "stage") == 0) {
        fg_stage_create(nw, a, b);
    } else if(strcmp(cmd, "set") == 0) {
        stage0_name = strtok(a, ".");
        param_name = strtok(NULL, "\n");
        param_value = b;

        stage0 = fg_network_get_stage_by_name(nw, stage0_name);
        if(!stage0) {
            fprintf(stderr, "stage %s not found\n", stage0_name);
            return -1;
        }
        fg_stage_set_param(stage0, param_name, param_value);
    } else if(strcmp(cmd, "connect") == 0) {
        stage0_name = strtok(a, ".");
        pin0_name = strtok(NULL, "\n");
        stage1_name = strtok(b, ".");
        pin1_name = strtok(NULL, "\n");

        stage0 = fg_network_get_stage_by_name(nw, stage0_name);
        if(!stage0) {
            fprintf(stderr, "stage %s not found\n", stage0_name);
            return -1;
        }
        stage1 = fg_network_get_stage_by_name(nw, stage1_name);
        if(!stage1) {
            fprintf(stderr, "stage %s not found\n", stage1_name);
            return -1;
        }

        fg_pin_connect(stage0, pin0_name, stage1, pin1_name);
    } else if(strcmp(cmd, "set_bufcount") == 0) {
        if(strcmp(a, "default") == 0) {
            nw->default_bufcount = atoi(b);
        } else {
            stage0_name = strtok(a, ".");
            pin0_name = strtok(NULL, "\n");

            stage0 = fg_network_get_stage_by_name(nw, stage0_name);
            if(!stage0) {
                fprintf(stderr, "stage %s not found\n", stage0_name);
                return -1;
            }
            pin0 = fg_stage_pin_get_by_name(stage0, pin0_name);
            if(!pin0) {
                fprintf(stderr, "pin %s not found in stage %s\n", pin0_name,
                        stage0_name);
                return -1;
            }
            pin0->bufcount = atoi(b);
        }
    } else if(strcmp(cmd, "set_bufsize") == 0) {
        if(strcmp(a, "default") == 0) {
            nw->default_bufsize = atoi(b);
        } else {
            stage0_name = strtok(a, ".");
            pin0_name = strtok(NULL, "\n");

            stage0 = fg_network_get_stage_by_name(nw, stage0_name);
            if(!stage0) {
                fprintf(stderr, "stage %s not found\n", stage0_name);
                return -1;
            }
            pin0 = fg_stage_pin_get_by_name(stage0, pin0_name);
            if(!pin0) {
                fprintf(stderr, "pin %s not found in stage %s\n", pin0_name,
                        stage0_name);
                return -1;
            }
            pin0->bufsize = atoi(b);
        }
    } else if(strcmp(cmd, "loop") == 0) {
        n = atoi(a);

        if(n <= 0) {
            fprintf(stderr, "invalid loop size %d\n", n);
            return -1;
        }

        for(i=0; i<n; i++) {
            process_cmd(nw, substitute_dollar(b, i));
        }
    } else {

    }

    return 0;
}

/* there is almost certainly a better way to do this, but this seems the most
 * expedient for now */
char *substitute_dollar(char *s, int n)
{
    static char new_s[1024];
    char *c;

    for(c = new_s; *s; c++, s++) {
        if(*s == '$') {
            /* cat on the int, then advance to end of string */
            sprintf(c, "%d", n);
            while(*(c+1)) c++;
        } else {
            *c = *s;
        }
    }
    *c = '\0';

    return new_s;
}

