#include <phx.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>

typedef unsigned char u8;

#define RANGE_SIZE 0x1000
#define DATA_SIZE 10

struct preserve_info {
    int *var1, *var2;
    u8 *start, *end;
};

int main(int argc, const char **argv, const char **envp) {

    u8 *range_start, *range_end, *last_range_start = NULL;
    u8 **start_array, **end_array;
    int *var1, *var2;
    struct preserve_info **info_array;
    struct preserve_info *info;

    puts("");
    // phx_get_preserved((void**)&info, (void**)&range_start, (void**)&range_end);
    info_array = phx_init(argc, argv, envp, NULL);
    printf("Get phx_init return %p.\n", info_array);

    assert((!!info_array) == phx_is_recovery_mode());


    start_array = malloc(sizeof(u8 *) * DATA_SIZE);
    end_array = malloc(sizeof(u8 *) * DATA_SIZE);

    if (info_array == NULL) {
        // first time running this program, normal initialization path
        puts("Start first time initialization.");

        for (int i = 0; i < DATA_SIZE; i++) {
            // range_start to range_end can be considered as our custom managed
            // heap

            if (last_range_start == NULL) {
                range_start = mmap(NULL, RANGE_SIZE, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            } else {
                range_start = mmap(last_range_start - 0x5000, RANGE_SIZE, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            }
            range_end = range_start + RANGE_SIZE;
            last_range_start = range_start;

            // set var1 and var2 address
            // pretent as allocated from heap

            var1 = (int*)range_start;
            var2 = (int*)(range_start + sizeof(int));

            *var1 = 100 + i;
            *var2 = 100 + i;

            start_array[i] = range_start;
            end_array[i] = range_end;
        }

        puts("Finished first time initialization.");
    } else {
        // we are in recovery path
        puts("Start recovering...");

        for (int i = 0; i < DATA_SIZE; i++) {
            info = info_array[i];

            printf("Recovering for data%d, with data pointer: %p\n", i, info);

            range_start = info->start;
            range_end = info->end;
            var1 = info->var1;
            var2 = info->var2;
            
            phx_finish_recovery();

            printf("Finished recovering for data%d.\n", i);
            printf("\trange_start=%p, range_end=%p.\n", range_start, range_end);
            printf("\tvar1=%p, var2=%p.\n", var1, var2);
            printf("\t*var1=%d, *var2=%d.\n", *var1, *var2);
            

            puts("Modifying vars.");

            *var1 += 1;
            *var2 -= 2;

            start_array[i] = range_start;
            end_array[i] = range_end;

            printf("Finished modifying for data%d in this round.", i);
            printf("\t*var1=%d, *var2=%d.\n", *var1, *var2);
        }
    }

    info_array = malloc(sizeof(struct preserve_info *) * DATA_SIZE);

    for (int i = 0; i < DATA_SIZE; i++) {
        info = (struct preserve_info*)(start_array[i] + sizeof(int) * 2);
        info->var1 = (int*)start_array[i];
        info->var2 = (int*)start_array[i] + sizeof(int);
        info->start = start_array[i];
        info->end = end_array[i];
        info_array[i] = info;
        printf("info = %p\n", info_array[i]);
        printf("info->var1 = %p\n", info_array[i]->var1);
        printf("info->var2 = %p\n", info_array[i]->var2);
        printf("info->start = %p\n", info_array[i]->start);
        printf("info->end = %p\n", info_array[i]->end);
    }
    


    // now still try to restart
    puts("Restarting in 2s... Ctrl-C if you want to stop");

    puts("Restarting...");
    phx_restart_multi(info_array, start_array, end_array,DATA_SIZE);

    return 0;
}
