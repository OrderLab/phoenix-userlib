#include <phx.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>

typedef unsigned char u8;

#define RANGE_SIZE 0x1000

struct preserve_info {
    int *var1, *var2;
    u8 *start, *end;
};

int main(int argc, const char **argv, const char **envp) {

    u8 *range_start, *range_end;
    int *var1, *var2;
    struct preserve_info *info;

    puts("");

    // phx_get_preserved((void**)&info, (void**)&range_start, (void**)&range_end);
    info = phx_init(argc, argv, envp, NULL);
    printf("Get phx_init return %p.\n", info);

    assert((!!info) == phx_is_recovery_mode());

    if (info == NULL) {
        // first time running this program, normal initialization path
        puts("Start first time initialization.");

        // range_start to range_end can be considered as our custom managed heap
        range_start = mmap(NULL, RANGE_SIZE, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        range_end = range_start + RANGE_SIZE;

        // set var1 and var2 address
        // pretent as allocated from heap

        var1 = (int*)range_start;
        var2 = (int*)(range_start + sizeof(int));

        *var1 = 0;
        *var2 = 0;

        puts("Finished first time initialization.");
    } else {
        // we are in recovery path
        puts("Start recovering...");

        range_start = info->start;
        range_end = info->end;
        var1 = info->var1;
        var2 = info->var2;

        phx_finish_recovery();

        puts("Finished recovering.");
        printf("\trange_start=%p, range_end=%p.\n", range_start, range_end);
        printf("\tvar1=%p, var1=%p.\n", var1, var2);
        printf("\t*var1=%d, *var1=%d.\n", *var1, *var2);

        puts("Modifying vars.");

        *var1 += 1;
        *var2 -= 2;

        puts("Finished modifying in this round.");
        printf("\t*var1=%d, *var1=%d.\n", *var1, *var2);
    }

    info = (struct preserve_info*)(range_start + sizeof(int) * 2);
    info->var1 = var1;
    info->var2 = var2;
    info->start = range_start;
    info->end = range_end;

    printf("info = %p\n", info);
    printf("info->var1 = %p\n", info->var1);
    printf("info->var2 = %p\n", info->var2);
    printf("info->start = %p\n", info->start);
    printf("info->end = %p\n", info->end);

    // now still try to restart
    puts("Restarting in 2s... Ctrl-C if you want to stop");

    puts("Restarting...");
    phx_restart(info, range_start, range_end);

    return 0;
}
