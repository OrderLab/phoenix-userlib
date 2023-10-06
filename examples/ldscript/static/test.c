#include <stdlib.h>
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

// this will be in .bss
static int global_bss sec_attr = 0;
// this will be in .data
static int global_data sec_attr = 100;

/* extern void *_dl_phx_get_ranges(void); */
/* extern void *phx_get_ranges(void); */
/* extern void *__phx_get_ranges(void); */
/* extern void *dlerror(void); */
/* extern void *__phx_get_ranges(void) __attribute__ ((visibility ("hidden"))); */

struct phx_range {
    size_t start, length;
};
extern struct phx_range *phx_get_ranges(size_t *len);

int main(int argc, const char *argv[], const char *envp[]) {
    void *old = phx_init(argc, argv, envp, NULL);
    int verbose = argc > 1 && !strcmp(argv[1], "-v");

    if (verbose) {
        char buf[50];
        sprintf(buf, "cat /proc/%d/maps", getpid());
        system(buf);
    }
    global_bss++;
    global_data++;
    printf("global_bss is %d\n", global_bss);
    printf("global_data is %d\n", global_data);

    //if (global_bss == 2)
    if (old == (void*)1)
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

    printf("global_bss %p\n", &global_bss);
    printf("global_data %p\n", &global_data);

    /* void *map = mmap(NULL, 4096, PROT_NONE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0); */
    /* printf("map %p\n", map); */

    // read range from file generated from ld
    phx_restart_multi((void*)1, start_arr, end_arr, len);

    /* printf("%p\n", dlerror()); */
}
