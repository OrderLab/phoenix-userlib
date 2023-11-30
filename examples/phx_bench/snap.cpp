#include "phx.h"
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#include <algorithm>
#include <thread>
#include <chrono>
#include <sys/syscall.h>

using std::min;
using namespace std::chrono;

#define SYS_PHX_GET_DURATION 455

#define PGSIZE 4096UL
#define KB 1024UL
#define MB (1024UL * KB)
#define GB (1024UL * MB)

// Force assert regardless of NDEBUG matter
#define assert(expr) do { \
		if (!(expr)) \
			fprintf(stderr, #expr "failed"); \
	} while (0)

extern long long _gduration;

unsigned long empty(void *store, void *argbuf) {
	(void)store;
	(void)argbuf;
	std::this_thread::sleep_for(milliseconds(5));
	return 0;
}

// Make pages dirty
void dirty_pages(int **ptr, size_t size) {
	for (size_t i = 0; i < size/PGSIZE; i++)
	{
		*ptr[i] = 25;
	}
} 

struct ckpt_t {
        long long t;
        const char *name;
};

struct phx_recovery_info {
    //ckpt_t ckpts[32];
	long long duration;
	long long init_duration;
	time_point<high_resolution_clock> t1;
	time_point<high_resolution_clock> t2;
	int **array;
};
struct phx_recovery_info *__phx_recovery_info;

void phx_one(size_t size) {
	//auto t1 = high_resolution_clock::now();
	__phx_recovery_info->t1 = high_resolution_clock::now();
	//printf("%lu,%lld,", size, duration_cast<nanoseconds>((high_resolution_clock::now()).time_since_epoch()).count());
	
	//fflush(stdout);
	phx_restart_multi(__phx_recovery_info, nullptr, nullptr, 0);
	//auto t2 = high_resolution_clock::now();
	//assert(ret == 0);

	//return duration_cast<nanoseconds>(t2 - t1).count();
}

void bench_snap(size_t size, bool csv) {
	if (!csv)
		printf("Testing snapshot size %lu\n", size);

	int** array = (int**)malloc(sizeof(int*)*(size/PGSIZE));
	
	for (size_t i = 0; i < size/PGSIZE; i++) {
		array[i] = (int *)malloc(PGSIZE);
		//fprintf(stderr, "ptr = %p\n", array[i]);
	}
	//fprintf(stderr, "ptr = %p\n", array);
	assert(array);
	//assert(size == pool->used);
	dirty_pages(array, size);
	//fprintf(stderr, "access %p, %d\n", array[524200], *array[524200]);
	__phx_recovery_info->array = array;
	//long long duration = phx_one(size);
	phx_one(size);

	/*if (csv)
		printf("%lu,%lld\n", size, duration);
	else
		printf("Snap %lu takes %lld ns.\n", size, duration);
	*/
}

void usage(const char *prog_name, int exit_val) {
	fprintf(stderr, "Usage: %s <size>\n", prog_name);
	exit(exit_val);
}

#define HP_TIMING_NOW(Var) \
  (__extension__ ({				\
    unsigned int __aux;				\
    (Var) = __builtin_ia32_rdtscp (&__aux);	\
  }))

#define HP_TIMING_DIFF(Diff, Start, End)	((Diff) = (End) - (Start))

typedef uint64_t hp_timing_t;

void rtld_timer_stop (hp_timing_t *var, hp_timing_t start)
{
  hp_timing_t stop;
  HP_TIMING_NOW (stop);
  HP_TIMING_DIFF (*var, start, stop);
}

extern "C" hp_timing_t phx_get_time();
	
int main (int argc, char **argv, char **envp) {
	__phx_recovery_info = (struct phx_recovery_info *)phx_init(argc, (const char **)argv, (const char **)envp, nullptr);
	hp_timing_t mytime = 0;
	rtld_timer_stop(&mytime, phx_get_time());
	if(!__phx_recovery_info) {
		__phx_recovery_info = (struct phx_recovery_info *)malloc(sizeof(*__phx_recovery_info));
	}
	else {
		//printf("%lld\n", duration_cast<nanoseconds>((high_resolution_clock::now()).time_since_epoch()).count());
		//fflush(stdout);
		__phx_recovery_info->t2 = high_resolution_clock::now();
		//printf("size = %s, t1 = %lld, t2 = %lld\n", argv[1],  __phx_recovery_info->t1, __phx_recovery_info->t2);
		printf("%s, %lld\n", argv[1], (duration_cast<microseconds>(__phx_recovery_info->t2 - __phx_recovery_info->t1)).count());
		//printf("------------------------------------\n");
		//printf("size = %s, Total time = %llu us\n", argv[1], (duration_cast<microseconds>(__phx_recovery_info->t2 - __phx_recovery_info->t1)).count());
		// Print out syscall duration
		// void *data;
		uint64_t duration = syscall(SYS_PHX_GET_DURATION);
		/*if (ret){
        		printf("syscall duration = %10llu \n", (uint64_t)data);
        		perror("Error");
        		printf("errno: %d\n", errno);
	    	}*/ 
		printf("syscall duration = %llu, %llu us\n", duration/1000, mytime/2200);

		// Print out userlib duration (from fault handler to before syscall)
		printf("userlib duration = %lld us\n", __phx_recovery_info->duration);
	
		// Print out libc start main time
		printf("libc start main duration = %lld us\n", _gduration);

		// Print out init duration
		printf("After restart, phx_init duration = %lld us\n", __phx_recovery_info->init_duration);
		
	}

	assert(__phx_recovery_info != nullptr);
	
	if (argc != 2) 
		usage(argv[0], 1);

	size_t size;
	if (sscanf(argv[1], "%ld", &size) != 1) 
		usage(argv[0], 1);
	
	if (!phx_is_recovery_mode()) {
		bench_snap(size, true);
	} 
	else {
		for (size_t i = 0; i < size/PGSIZE; i++) {
			//fprintf(stderr, "access res = %d\n", *(__phx_recovery_info->array[i]));
		}
		//for (size_t i = 0; i < size/PGSIZE; i++) {
		//	fprintf(stderr, "access %p, %d\n", array[i], *array[i]);
		//}
		// Cleanup
                for (size_t i = 0; i < size/PGSIZE; i++) {
                        free(__phx_recovery_info->array[i]);
                }
	}

	phx_finish_recovery();

	return 0;
}
