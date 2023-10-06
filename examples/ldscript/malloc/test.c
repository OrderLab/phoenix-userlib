#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <phx.h>
#include <unistd.h>
#include <sys/mman.h>

#if 0
#define sec_attr __attribute__ ((section (".mydata")))
#else
#define sec_attr
#endif

/* extern void *_dl_phx_get_ranges(void); */
/* extern void *phx_get_ranges(void); */
/* extern void *__phx_get_ranges(void); */
/* extern void *dlerror(void); */
/* extern void *__phx_get_ranges(void) __attribute__ ((visibility ("hidden"))); */

struct phx_range {
    size_t start, length;
};
extern struct phx_range *phx_get_ranges(size_t *len);

struct mydata {
    int counter;
};

int main(int argc, const char *argv[], const char *envp[]) {
    struct mydata *old = phx_init(argc, argv, envp, NULL);
    int verbose = argc > 1 && !strcmp(argv[1], "-v");

    if (verbose) {
        char buf[50];
        sprintf(buf, "cat /proc/%d/maps", getpid());
        system(buf);
    }

    if (!old)
        old = malloc(sizeof(*old));
    assert(old);

    old->counter++;
    printf("old->counter is %d\n", old->counter);

    if (phx_is_recovery_mode())
        sleep(100000);

    struct phx_range *ranges;
    size_t len = 0;
    ranges = phx_get_ranges(&len);

    // unsigned long start_arr[len];
    // unsigned long end_arr[len];
    unsigned long *start_arr = malloc(len * sizeof(unsigned long));
    unsigned long *end_arr = malloc(len * sizeof(unsigned long));

    printf("Got num ranges %lu\n", len);
    for (size_t i = 0; i < len; ++i) {
        start_arr[i] = ranges[i].start;
        end_arr[i] = ranges[i].start + ranges[i].length;
        printf("Range %lu: 0x%lx size 0x%lx\n", i, ranges[i].start, ranges[i].length);
    }

    printf("&old->counter %p\n", &old->counter);

    /* void *map = mmap(NULL, 4096, PROT_NONE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0); */
    /* printf("map %p\n", map); */

    // read range from file generated from ld
    phx_restart_multi(old, start_arr, end_arr, len);

    /* printf("%p\n", dlerror()); */
}
