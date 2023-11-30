#include <phx.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <sys/mman.h>
#include <cstring>
#include <cassert>
#include <chrono>
#include <iostream>
#include <fstream>
typedef unsigned char u8;

#define RANGE_SIZE 0x1000
#define DATA_SIZE 10

struct worker_info {
    int id;
    bool status;
    char job[7];
};

struct preserve_info {
    int *var1;
    int *var2;	
    u8 **start_array;
    u8 **end_array;
    //std::chrono::time_point<std::chrono::_V2::system_clock, std::chrono::duration<long int, std::ratio<1, 1000000000> > > t1;
    struct worker_info *worker;
    size_t size;
};

void usage(const char *prog_name, int exit_val) {
    fprintf(stderr, "Usage: %s <size>\n", prog_name);
    exit(exit_val);
}

int main(int argc, const char **argv, const char **envp) {
    //std::ofstream csv("snap.csv", std::ios::app);
    //printf("Oprn csv\n"); 
    u8 *range_start, *range_end, *last_range_start = NULL;
    u8 **start_array, **end_array;
    int *var1, *var2;
    struct preserve_info *info;
    struct worker_info *worker;
    size_t size;

    //puts("");
    if (sscanf(argv[1], "%ld", &size) != 1)
        usage(argv[0], 1);

    info = (preserve_info *)phx_init(argc, argv, envp, NULL);
    //auto t2 = std::chrono::high_resolution_clock::now();
    //if (info != NULL) {
    //	long long duration = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - info->t1).count();
	//csv << size << "," << duration << "\n"; 
	//printf("time result = %lu,%lld\n", size, duration);
    //}
  
    printf("Get phx_init return %p.\n", info);

    assert((!!info) == phx_is_recovery_mode());

    start_array = (u8 **)malloc(sizeof(u8 *) * DATA_SIZE);
    end_array = (u8 **)malloc(sizeof(u8 *) * DATA_SIZE);

    if (info == NULL) {
	puts("Start first time initialization.");

        worker = (worker_info *)malloc(sizeof(struct worker_info) * size);
	for (size_t i = 0; i < size; i++) {
	    worker[i].id = 20 + i;
	    worker[i].status = i;
	    strcpy(worker[i].job, "feeder\0");
	}
	printf("initialize workers starting from %p with size = %u\n", worker, sizeof(struct worker_info) * size);

	for (int i = 0; i < DATA_SIZE; i++) {
            // range_start to range_end can be considered as our custom managed
            // heap

            if (last_range_start == NULL) {
                range_start = (u8 *)mmap((void*)0x12345678, RANGE_SIZE, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            } else {
                range_start = (u8 *)mmap(last_range_start - 0x5000, RANGE_SIZE, PROT_READ | PROT_WRITE,
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

	var1 = (int *)start_array[0];
	var2 = (int *)(start_array[0] + sizeof(int));

        puts("Finished first time initialization.");
    }
    else{
	// we are in recovery path
        puts("Start recovering...");

	phx_finish_recovery();
	printf("1\n");
	start_array = info->start_array;
	end_array = info->end_array;
	printf("2\n");

	// prove that the worker is still valid
	//printf("worker status = %d\n", info->worker->status);
	//printf("worker job = %s\n", info->worker->job);
	for (size_t i = 0; i < size; i++) {
            printf("worker %u id = %d, job = %s\n", i, info->worker[i].id, info->worker[i].job);
        }
	printf("size = %u, info size = %u\n", size, info->size);
	printf("Recover workers starting from %p with size = %u\n", info->worker, sizeof(struct worker_info) * size);

	puts("Modifying vars.");

	// test malloc
	worker_info *victim = (worker_info *)malloc(sizeof(worker_info) * 80000);
	victim[79999].id = 79999;
	printf("test int = %d\n", victim[79999].id);
	free(victim);

	var1 = info->var1;
	var2 = info->var2;
	printf("Old var1 = %d\n", *var1);

	*var1 += 1;
	*var2 -= 2;
    }

    info = (preserve_info *)malloc(sizeof(struct preserve_info));
    info->var1 = var1;
    info->var2 = var2;
    info->worker = worker;
    info->start_array = start_array;
    info->end_array = end_array;
    info->size = size;

    printf("info var1 = %d, var2 = %d\n", *(info->var1), *(info->var2));

    if (*(info->var1) == 101 && *(info->var2) == 98) {
	puts("Stop restarting");
	sleep(0.01);
	return 0;
    }

    puts("Restarting in 2s... Ctrl-C if you want to stop");

    sleep(0.02);

    puts("Restarting...");

    
    phx_restart_multi(info, start_array, end_array, DATA_SIZE);

    return 0;
}
