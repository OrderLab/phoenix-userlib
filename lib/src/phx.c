#include "phx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>

#define SYS_PHX_RESTART 449
#define SYS_PHX_GET_PRESERVED 450

static bool __phx_recovery_mode;

static struct phx_saved_args {
    // FIXME: leak?
    // void *__pages;
    int argc;
    const char **argv;
    const char **envp;
} __phx_saved_args;

/* ====== Public API for recovery mode status ====== */

bool phx_is_recovery_mode(void) {
    return __phx_recovery_mode;
}

void phx_finish_recovery(void) {
    __phx_recovery_mode = false;
}

static const char **save_arg_array(const char *argv[], size_t *save_cnt) {
    const char **e = argv;
    size_t count = 0, str_len = 0;

    while (*e) {
        str_len += strlen(*e) + 1;
        // printf("\"%s\"\n", *e);
        ++e;
        ++count;
    }

    void *pages = malloc((count + 1) * sizeof(char*) + str_len);
    assert(pages);
    char **saved = pages;
    char *p = (char*)pages + (count + 1) * sizeof(char*);

    for (size_t i = 0; i < count; ++i) {
        size_t len = strlen(argv[i]);
        saved[i] = p;
        strcpy(p, argv[i]);
        p += len + 1;
    }
    assert((size_t)(p - (char*)pages) == (count + 1) * sizeof(char*) + str_len);
    saved[count] = NULL;

    if (save_cnt)
        *save_cnt = count;

    return (const char **)saved;
}

static void save_args(int argc, const char *argv[], const char *envp[]) {
    size_t count;
    __phx_saved_args.argc = argc;
    __phx_saved_args.argv = save_arg_array(argv, &count);
    __phx_saved_args.envp = save_arg_array(envp, NULL);
    assert(count == (size_t)argc);
}

void *phx_init(int argc, const char *argv[], const char *envp[], sighandler_t handler) {
    fprintf(stderr, "In new process phx_init\n");

    if (SIG_ERR == signal(SIGSEGV, handler))
        fprintf(stderr, "Warning: segfault handler could not be set: %s\n", strerror(errno));

    save_args(argc, argv, envp);

    void *data, *preserved_start, *preserved_end;
    phx_get_preserved(&data, &preserved_start, &preserved_end);
    fprintf(stderr, "phx_get_preserved got data=%p, start=%p, end=%p.\n",
            data, preserved_start, preserved_end);
    preserved_start = (void*)((unsigned long)preserved_start & ~0xfffUL);
    preserved_end = (void*)(((unsigned long)preserved_end + 4096 - 1) & ~0xfffUL);

    if (data == NULL)
        return NULL;

    __phx_recovery_mode = true;
    assert(preserved_start && preserved_end);
    return data;
}

/* ====== Low-level APIs ====== */

struct user_phx_args {
    const char *filename;
    const char *const *argv;
    const char *const *envp;
    void *data;
    void *start;
    void *end;
};

void phx_restart(void *data, void *start, void *end) {
    fprintf(stderr, "Phoenix preserving...\n");
    start = (void*)((unsigned long)start & ~0xfff);
    end = (void*)(((unsigned long)end + 4096 - 1) & ~0xfff);

    struct user_phx_args args = {
        .filename = "/proc/self/exe",
        .argv = __phx_saved_args.argv,
        .envp = __phx_saved_args.envp,
        .data = data,
        .start = start,
        .end = end,
    };

    syscall(SYS_PHX_RESTART, &args);
    fprintf(stderr, "Phoenix preserved.\n");
}

void phx_get_preserved(void **data, void **start, void **end) {
    syscall(SYS_PHX_GET_PRESERVED, data, start, end);
}
