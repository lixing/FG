/*
 * fg.c
 */

#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "fg_internal.h"

/* local-use funcs */
static void *get_sym(void *handle, const char *name);

static struct {
    int log_settings;           /* should this be associated with a network? */

    FG_module **modules;        /* NULL_terminated list of loaded modules */
    int module_count;

    FG_stage_def **stage_defs;  /* NULL-terminated list of stage definitions */
    int stage_def_count;
} fg_context;

int fg_init(int *argc, char **argv[])
{
    fg_context.log_settings = FG_LOG_NETWORK | FG_LOG_STAGE;

    fg_log(FG_LOG_MAIN, "initializing FG\n");

    /* HACK: magic number! */
    fg_context.modules = (FG_module **) calloc(101, sizeof(FG_module *));
    fg_context.module_count = 0;

    /* HACK: magic number! */
    fg_context.stage_defs = (FG_stage_def **) calloc(101,
            sizeof(FG_stage_def *));
    fg_context.stage_def_count = 0;

    /* Eventually, this will search a predefined list of paths and add all
     * modules found therein */
    fg_log(FG_LOG_MAIN, "  loading modules\n");
    fg_module_load("io_module.so");
    fg_module_load("dsort_module.so");
    fg_module_load("mpi_module.so");

    return 0;
}

void fg_log(int log_domain, const char *s, ...)
{
    va_list ap;

    if(log_domain & fg_context.log_settings) {
        va_start(ap, s);
        vprintf(s, ap);
        va_end(ap);
    }
}

int fg_fini(void)
{
    FG_module **module;

    fg_log(FG_LOG_MAIN, "shutting down FG\n");

    fg_log(FG_LOG_MAIN, "  unloading modules\n");
    free(fg_context.stage_defs);
    for(module=fg_context.modules; *module; module++) {
        fg_log(FG_LOG_MODULE, "    %s\n", (*module)->name);
        fg_module_unload(*module);
    }
    free(fg_context.modules);

    fg_log(FG_LOG_MAIN, "FG shutdown complete\n");

    return 0;
}

int fg_module_load(const char *path)
{
    void *handle;
    int i;
    FG_module *module;
    FG_stage_def *sd;

    fg_log(FG_LOG_MODULE, "Loading module %s\n", path);
    handle = dlopen(path, RTLD_LAZY);
    if(!handle) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        exit(1);
    }

    module = (FG_module *) malloc(sizeof(FG_module));
    module->path = strdup(path);
    module->handle = handle;
    module->stage_defs = (FG_stage_def *) get_sym(handle, "fg_module_export");

    module->name = (char *) get_sym(handle, "fg_module_name");

    /* NOTE: this implies a flat stage namespace */
    for(sd=module->stage_defs, i=0; sd->name; sd++, i++) {
        fg_log(FG_LOG_MODULE, "    %s\n", sd->name);
        *(fg_context.stage_defs + fg_context.stage_def_count) = sd;
        fg_context.stage_def_count++;
    }

    /* stuff it into the global list of loaded modules */
    *(fg_context.modules + fg_context.module_count) = module;
    fg_context.module_count++;

    return 0;
}

void fg_module_unload(FG_module *module)
{
    char *s;

    if(!module)
        return;

    free(module);

    (void) dlerror();       /* clear error flag */
    dlclose(module->handle);
    s = dlerror();
    if(s) {
        fprintf(stderr, "dlclose failed: %s\n", s);
        exit(1);
    }
}

void fg_print_stage_defs(void)
{
    FG_stage_def **sd;
    FG_pin *pin;

    for(sd = fg_context.stage_defs; *sd; sd++) {
        printf("stage: %s\n", (*sd)->name);

        printf("  pins:\n");
        for(pin = (*sd)->pins; pin->name; pin++) {
            printf("    %s\n", fg_pin_as_string(pin));
        }
    }
}

FG_stage_def *fg_stage_def_get_by_name(const char *name)
{
    FG_stage_def **sd;

    for(sd = fg_context.stage_defs; *sd; sd++) {
        if(strcmp(name, (*sd)->name) == 0)
            return *sd;
    }

    return NULL;
}

void *get_sym(void *handle, const char *name)
{
    void *sym;
    char *s;

    (void) dlerror();       /* clear error flag */
    sym = dlsym(handle, name);

    s = dlerror();
    if(s) {
        fprintf(stderr, "dlsym failed: %s\n", s);
        exit(1);
    }

    return sym;
}

void print_stages(FG_network *nw) {
    FG_stage **stage;

    for(stage = nw->stages; *stage; stage++) {
        printf("%s\n", fg_stage_as_string(*stage));
    }
}

FG_stage_def **fg_get_stage_defs(void) {
    return fg_context.stage_defs;
}

