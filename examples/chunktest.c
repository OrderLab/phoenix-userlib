#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <phx.h>
extern void phx_marked_used(void*);
extern void phx_cleanup(void);
struct preserve_data {
    int* a, *b;
};
struct preserve_data *info;
struct phx_range {
    size_t start, length;
};
extern struct phx_range *phx_get_ranges(size_t *len);


void *thread_function(void * arg){
    int *a = calloc(3, sizeof(int));
    int *b = calloc(4, sizeof(int));
    a[0] = 10;
    b[0] = 5;
    info->a = a;
    info->b = b;
    fprintf(stderr, "in new thread, a: %p, b: %p\n", a, b);
    phx_marked_used(a);
    return NULL;
}

int main(int argc, char** argv, char** envp) {
    fprintf(stderr, "new main\n");
    info = phx_init(argc,argv,envp,NULL);
    if (phx_is_recovery_mode()){
        fprintf(stderr, "before cleanup\n");
        phx_cleanup();
        fprintf(stderr, "a[0]=%d \n",(info->a)[0]);
        fprintf(stderr, "b[0]=%d\n", (info->b)[0]);
        exit(0);
    } else {
        info = (struct preserve_data*)malloc(sizeof(*info));
    }
    int *a = calloc(1,sizeof(int));
    int *b = calloc(2, sizeof(int));
    fprintf(stderr, "a: %p, b: %p\n", a, b);
    phx_marked_used(a);
    phx_marked_used(b);
    info->a = a;
    info->b = b;
    pthread_t thread_id;
    if(pthread_create(&thread_id, NULL, thread_function, NULL) !=0){
        fprintf(stderr, "create fail\n");
        exit(0);
    }
    pthread_join(thread_id, NULL);
    
    // restart
    struct phx_range *ranges;
    size_t len = 0;
    ranges = phx_get_ranges(&len);
 
    unsigned long *start_arr = (long unsigned int*)malloc(len * sizeof(unsigned long));
    unsigned long *end_arr = (long unsigned int*)malloc(len * sizeof(unsigned long));
 
    printf("Got num ranges %lu\n", len);
    for (size_t i = 0; i < len; ++i) {
        start_arr[i] = ranges[i].start;
        end_arr[i] = ranges[i].start + ranges[i].length;
        printf("Range %lu: 0x%lx size 0x%lx\n", i, ranges[i].start, ranges[i].length);
    }
    printf("restart in 2s\n");
    phx_cleanup();
    phx_restart_multi(info,start_arr,end_arr,len);
    //phx_restart_multi(info,NULL,NULL,0);


    return 0;
}
