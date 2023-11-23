#include "phx.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/time.h>

#define SYS_PHX_RESTART 449
#define SYS_PHX_GET_PRESERVED 450

#define PHX_PRESERVE_LIMIT 64

#define dprintf(...) do {} while(0)
// #define dprintf(fmt, ...) do { fprintf(stderr, fmt, ##__VA_ARGS__); } while(0)

static long long ustime(void) {
    struct timeval tv;
    long long ust;

    gettimeofday(&tv, NULL);
    ust = ((long)tv.tv_sec)*1000000;
    ust += tv.tv_usec;
    return ust;
}

static int __phx_recovery_mode;

static long long init_duration = 0;
static long long duration = 0;
static int phx_mode = 0;
static int cnt = 0;
static struct ckpt_t {
	long long t;
	const char *name;
} ckpts[32] = { { 0, NULL, }, };

//enum { COUNTER_BASE = cnt };

#define CKPT 1

#define ckpt(s) \
	if(CKPT && phx_mode) { \
		if (cnt == 0) { \
			ckpts[0] = (struct ckpt_t) { \
				.t = ustime(), \
				.name = s, \
			}; \
		} else { \
			ckpts[cnt].t = ustime() - ckpts[0].t; \
			ckpts[cnt].name = s; \
		} \
		cnt++; \
	} 

static struct phx_saved_args {
    // FIXME: leak?
    // void *__pages;
    int argc;
    const char **argv;
    const char **envp;
} __phx_saved_args;

/* ====== Public API for recovery mode status ====== */

int phx_is_recovery_mode(void) {
    return __phx_recovery_mode;
}

void phx_finish_recovery(void) {
    __phx_recovery_mode = 0;
}

static const char **save_arg_array(const char *argv[], size_t *save_cnt) {
    const char **e = argv;
    size_t count = 0, str_len = 0;

    while (*e) {
        str_len += strlen(*e) + 1;
        // dprintf("\"%s\"\n", *e);
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

void *phx_init(int argc, const char *argv[], const char *envp[], void (*handler)(int)) {
    dprintf("In new process phx_init\n");
    clock_t t10 = clock();
    phx_mode = 1;
    ckpt("Entering phx_init");
   
    //fprintf(stderr, "Entering Phxinit, t10 = %lf\n", (double)t10);
    if (SIG_ERR == signal(SIGSEGV, handler))
        dprintf("Warning: segfault handler could not be set: %s\n", strerror(errno));
    dprintf("t10 = %lf", (double)t10);
    save_args(argc, argv, envp);
    clock_t t11 = clock();
    dprintf("t11 = %lf", (double)t11);

    unsigned int len = 0;
    void *data, *preserved_start = NULL, *preserved_end=NULL;
    phx_get_preserved_multi(&data, &preserved_start, &preserved_end,&len);
    dprintf("phx_get_preserved got data=%p, start=%p, end=%p, with len=%d.\n",
            data, preserved_start, preserved_end, len);
    if (preserved_start!=NULL && preserved_end!=NULL){
        dprintf("actual data: start=%lu, end=%lu.\n",
                *(unsigned long *)preserved_start,
                *(unsigned long *)preserved_end);
    }
    preserved_start = (void*)((unsigned long)preserved_start & ~0xfffUL);
    preserved_end = (void*)(((unsigned long)preserved_end + 4096 - 1) & ~0xfffUL);

    clock_t t1 = clock();
    //fprintf(stderr, "PHXINIT Done, t1 = %lf\n", (double)t1);
    if (data == NULL) {
        cnt = 0;
	phx_mode = 0;
	return NULL;
    }

    __phx_recovery_mode = 1;
    assert(preserved_start && preserved_end);

    ckpt("Phx init done");

    if (CKPT && phx_mode) {
                int total = cnt;

                ckpts[0].t = 0;
                size_t i;
                for (i = 1; i < total; ++i) {
                        init_duration += (ckpts[i].t - ckpts[i - 1].t);
                }
                phx_mode = 0;
                cnt = 0;
        }
    memcpy(data+sizeof(long long), &init_duration, sizeof(long long));

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

void phx_restart_multi(void *data, void *start_arr, void *end_arr,
                       unsigned int len) {
    dprintf("Phoenix preserving multi range...\n");

    phx_mode = 1;
    ckpt("Restart begins");
    // Append glibc malloc ranges to the user data ranges
    unsigned int raw_len = len;

    clock_t t0 = clock();
    //fprintf(stderr, "before get malloc ranges, t0 = %lf\n", (double)t0);
    struct allocator_info **allocator_list = phx_get_malloc_ranges();
    
    clock_t t3 = clock();
    //fprintf(stderr, "After get malloc ranges, t3 = %lf\n", (double)t3);
    dprintf("list addr in phx.c = %p\n", allocator_list);
    struct allocator_info **node = (allocator_list == NULL) ? NULL : &allocator_list[0];

    unsigned int count = 0;
    dprintf("start addr = %p, node ptr = %p\n", node, *node);
    while (node != NULL && *node != NULL){
        count += 1;
        len += 1;
        node = &allocator_list[count];
        dprintf("new node addr = %p", node);
    }
    dprintf("raw len = %u, len = %u\n", raw_len, len);
    // Malloc new arrays to store the new ranges
    void *new_startarr = malloc(len * sizeof(unsigned long));
    void *new_endarr = malloc(len * sizeof(unsigned long));
    dprintf("malloc start_arr = %p\n", start_arr);
    dprintf("count = %u\n", count);
    for (unsigned int i = 0; i < count; i++) {
	dprintf("phoenix start preserving malloc range\n");
	unsigned long start = ((unsigned long)allocator_list[i]->start) & ~0xfff;
        unsigned long end = ((unsigned long)allocator_list[i]->end + 4096 - 1) & ~0xfff;
	dprintf("half\n");
	((unsigned long *)new_startarr)[raw_len + i] = start;
        ((unsigned long *)new_endarr)[raw_len + i] = end;
	dprintf("Phoenix preserving malloc range %d: start=%p, end=%p, size is %lu\n", i,
                (void *)start, (void *)end, (size_t)((uintptr_t)end-(uintptr_t)start));
	// free(allocator_list[i]);
    }

    if (len > PHX_PRESERVE_LIMIT){
        dprintf("exceed limit.\n"); 
    }

    for (unsigned int i = 0; i < raw_len; i++) {
        unsigned long start = (((unsigned long *)start_arr)[i]) & ~0xfff;
        unsigned long end = (((unsigned long *)end_arr)[i] + 4096 - 1) & ~0xfff;
        dprintf("Phoenix preserving range %d: start=%p, end=%p.\n", i,
                (void *)start, (void *)end);
        
        ((unsigned long *)new_startarr)[i] = start;
        ((unsigned long *)new_endarr)[i] = end;
    }

    struct user_phx_args_multi args = {
        .filename = "/proc/self/exe",
        .argv = __phx_saved_args.argv,
        .envp = __phx_saved_args.envp,
        .data_arr = data,
        .start_arr = new_startarr,
        .end_arr = new_endarr,
        .len = len,
    };

    dprintf("before malloc preserve meta\n");
    // Phx preserve meta
    clock_t t4 = clock();      
    ckpt("Before preserve meta");
    //fprintf(stderr, "Before malloc preserve meta, t4 = %lf\n", (double)t4);
    phx_malloc_preserve_meta(); 
    clock_t t5 = clock();             
    //fprintf(stderr, "After malloc preserve meta, t5 = %lf\n", (double)t5);
    ckpt("Before entering restart syscall");

    if (CKPT && phx_mode) {
                int total = cnt;

                ckpts[0].t = 0;
                size_t i;
                for (i = 1; i < total; ++i) {
                	duration += (ckpts[i].t - ckpts[i - 1].t);
		}
                phx_mode = 0;
                cnt = 0;
        }
    memcpy(data, &duration, sizeof(long long));

    dprintf("before system call\n");
    int ret = syscall(SYS_PHX_RESTART, &args);
    if (ret){
        dprintf("phx_get_preserved_multi did not copy enough data.\n");
    	perror("Error");
	dprintf("errno: %d\n", errno);
    }
    dprintf("Phoenix preserved multi range.\n");
}


void phx_get_preserved_multi(void **data, void **start_arr, void **end_arr,
                             unsigned int *len) {
    // allocate memory for start_arr, end_arr
    int ret = 0;
    clock_t t9 = clock();
    //fprintf(stderr, "t9 = %lf", (double)t9);
    
    *start_arr = malloc(sizeof(unsigned long) * PHX_PRESERVE_LIMIT);
    *end_arr = malloc(sizeof(unsigned long) * PHX_PRESERVE_LIMIT);
    dprintf("first address of start_arr is %p\n",*start_arr);
    clock_t t7 = clock();
    ckpt("Before syscall phx get preserved");
    //fprintf(stderr, "t7 = %lf", (double)t7);
    ret = syscall(SYS_PHX_GET_PRESERVED, data, start_arr, end_arr, len);
    clock_t t8 = clock();
    //fprintf(stderr, "t8 = %lf\n", (double)t8);
    if (ret)
        dprintf("phx_get_preserved_multi did not copy enough data.\n");
    dprintf("phx_get_preserved_multi got data=%p, start=%p, end=%p, with len=%d.\n",
        *data, *start_arr, *end_arr, *len);
    if (*start_arr != NULL && *end_arr != NULL && *len > 0)
        dprintf("with actual data: data0=%p, start=%lx, end=%lx.\n",
                *data, *(unsigned long *)*start_arr, *(unsigned long *)*end_arr);
    if (*len == 0) {
        free(*start_arr);
        free(*end_arr);
        *start_arr = NULL;
        *end_arr = NULL;
    }
}

