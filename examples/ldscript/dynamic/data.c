#include <stdio.h>

int so_data_bss = 0;
int so_data_static = 100;

void libdata_incr_counter(void) {
    so_data_bss++;
    printf("so_data_bss is %p val %d\n", &so_data_bss, so_data_bss);
    so_data_static++;
    printf("so_data_static is %p val %d\n", &so_data_static, so_data_static);
}
