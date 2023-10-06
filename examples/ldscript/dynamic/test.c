#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <phx.h>
#include <unistd.h>
#include <sys/mman.h>
#include "data.h"

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

int main(int argc, const char *argv[], const char *envp[]) {
    void *old = phx_init(argc, argv, envp, NULL);

    char buf[50];
    sprintf(buf, "cat /proc/%d/maps", getpid());
    system(buf);

#if 0
    so_data_bss++;
    printf("so_data_bss is %p val %d\n", &so_data_bss, so_data_bss);

    //if (so_data_bss == 2)
    if (old == (void*)1)
        sleep(100000);
#elif 0
    so_data_static++;
    printf("so_data_static is %p val %d\n", &so_data_static, so_data_static);

    //if (so_data_static == 2)
    if (old == (void*)1)
        sleep(100000);
#else
    libdata_incr_counter();

    //if (so_data_static == 2)
    if (old == (void*)1)
        sleep(100000);
#endif

    struct phx_range *ranges;
    size_t len = 0;
    ranges = phx_get_ranges(&len);

    unsigned long start_arr[len];
    unsigned long end_arr[len];

    printf("Got num ranges %lu\n", len);
    for (size_t i = 0; i < len; ++i) {
        start_arr[i] = ranges[i].start;
        end_arr[i] = ranges[i].start + ranges[i].length;
        printf("Range %lu: 0x%lx size 0x%lx\n", i, ranges[i].start, ranges[i].length);
    }

    printf("bss  so_data_bss %p\n", &so_data_bss);
    printf("data so_data_static %p\n", &so_data_static);

    /* void *map = mmap(NULL, 4096, PROT_NONE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0); */
    /* printf("map %p\n", map); */

    // read range from file generated from ld
    phx_restart_multi((void*)1, start_arr, end_arr, len);

    /* printf("%p\n", dlerror()); */
}
