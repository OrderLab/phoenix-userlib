#include "phx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>

#define SYS_PHX_RESTART 449
#define SYS_PHX_GET_PRESERVED 450

#define PHX_PRESERVE_LIMIT 64

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

    unsigned int len = 0;
    void *data, *preserved_start = NULL, *preserved_end=NULL;
    phx_get_preserved_multi(&data, &preserved_start, &preserved_end,&len);
    fprintf(stderr,
            "phx_get_preserved got data=%p, start=%p, end=%p, with len=%d.\n",
            data, preserved_start, preserved_end, len);
    if (preserved_start!=NULL && preserved_end!=NULL){
        fprintf(stderr, "actual data: start=%lu, end=%lu.\n",
                *(unsigned long *)preserved_start,
                *(unsigned long *)preserved_end);
    }
    preserved_start = (void*)((unsigned long)preserved_start & ~0xfffUL);
    preserved_end = (void*)(((unsigned long)preserved_end + 4096 - 1) & ~0xfffUL);

    if (data == NULL)
        return NULL;

    __phx_recovery_mode = true;
    assert(preserved_start && preserved_end);

    return data;
}

/* ====== Low-level APIs ====== */

struct user_phx_args_multi {
    const char *filename;
    const char *const *argv;
    const char *const *envp;
    void *data_arr;
    void *start_arr;
    void *end_arr;
    unsigned int len; // len should be at least 1
};

extern void *phx_get_malloc_ranges();
// extern void *phx_get_malloc_meta();
extern void phx_malloc_preserve_meta();

struct allocator_info {
    void *start;
    void *end;
};

/* struct phx_malloc_meta {
    malloc_state *main_arena;
    malloc_par *mp_;
    struct malloc_state *next
}; */

void phx_restart_multi(void *data_arr, void *start_arr, void *end_arr,
                       unsigned int len) {
    fprintf(stderr, "Phoenix preserving multi range...\n");

    // Append glibc malloc ranges to the user data ranges
    unsigned int raw_len = len;
    struct allocator_info **allocator_list = phx_get_malloc_ranges();
    struct allocator_info *node = (allocator_list == NULL) ? NULL : allocator_list[0];
    unsigned int count = 0;
    while (node != NULL){
        count += 1;
        len += 1;
        node = (struct allocator_info *)((uintptr_t)node + sizeof(unsigned long));
    }
    start_arr = realloc(start_arr, len);
    for (unsigned int i = 0; i < count; i++) {
        ((unsigned long *)start_arr)[raw_len + count] = (unsigned long)allocator_list[i]->start;
        ((unsigned long *)end_arr)[raw_len + count] = (unsigned long)allocator_list[i]->end;

        free(allocator_list[i]);
    }

    if (len > PHX_PRESERVE_LIMIT){
        fprintf(stderr, "exceed limit.\n"); 
    }

    for (unsigned int i = 0; i < raw_len; i++) {
        unsigned long start = (((unsigned long *)start_arr)[i]) & ~0xfff;
        unsigned long end = (((unsigned long *)end_arr)[i] + 4096 - 1) & ~0xfff;
        fprintf(stderr, "Phoenix preserving range %d: start=%p, end=%p.\n", i,
                (void *)start, (void *)end);
        
        ((unsigned long *)start_arr)[i] = start;
        ((unsigned long *)end_arr)[i] = end;
    }

    struct user_phx_args_multi args = {
        .filename = "/proc/self/exe",
        .argv = __phx_saved_args.argv,
        .envp = __phx_saved_args.envp,
        .data_arr = data_arr,
        .start_arr = start_arr,
        .end_arr = end_arr,
        .len = len,
    };

    // Phx preserve meta
    phx_malloc_preserve_meta(); 

    fprintf(stderr, "before system call");
    syscall(SYS_PHX_RESTART, &args);
    fprintf(stderr, "Phoenix preserved multi range.\n");
}


void phx_get_preserved_multi(void **data_arr, void **start_arr, void **end_arr,
                             unsigned int *len) {
    // allocate memory for start_arr, end_arr
    int ret = 0;
    
    *data_arr = malloc(sizeof(unsigned long) * PHX_PRESERVE_LIMIT);
    *start_arr = malloc(sizeof(unsigned long) * PHX_PRESERVE_LIMIT);
    *end_arr = malloc(sizeof(unsigned long) * PHX_PRESERVE_LIMIT);
    fprintf(stderr,"first address of start_arr is %p\n",*start_arr);
    ret = syscall(SYS_PHX_GET_PRESERVED, data_arr, start_arr, end_arr, len);
    if (ret)
        fprintf(stderr, "phx_get_preserved_multi did not copy enough data.\n");
    fprintf(
        stderr,
        "phx_get_preserved_multi got data=%p, start=%p, end=%p, with len=%d.\n",
        *data_arr, *start_arr, *end_arr, *len);
    if (*start_arr != NULL && *end_arr != NULL && *len > 0)
        fprintf(stderr, "with actual data: data0=%lx, start=%lu, end=%lu.\n", *(unsigned long *)*data_arr,
                *(unsigned long *)*start_arr, *(unsigned long *)*end_arr);
    if (*len == 0) {
        free(*data_arr);
        free(*start_arr);
        free(*end_arr);
        *data_arr = NULL;
        *start_arr = NULL;
        *end_arr = NULL;
    }
}

