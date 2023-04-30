#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#if 0
#define sec_attr __attribute__ ((section (".mydata")))
#else
#define sec_attr
#endif

// this will be in .bss
static int global_data sec_attr = 0;
// this will be in .data
static int global_data2 sec_attr = 100;

/* extern void *_dl_phx_get_ranges(void); */
/* extern void *phx_get_ranges(void); */
/* extern void *__phx_get_ranges(void); */
/* extern void *dlerror(void); */
/* extern void *__phx_get_ranges(void) __attribute__ ((visibility ("hidden"))); */

int main() {
    char buf[50];
    sprintf(buf, "cat /proc/%d/maps", getpid());
    system(buf);
    global_data++;
    printf("global_data is %d\n", global_data);
    sleep(100000);

    /* printf("%p\n", __phx_get_ranges()); */
    /* printf("%p\n", dlerror()); */
}
