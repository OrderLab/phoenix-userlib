#pragma CODE_SECTION(".phxd")

#define attr __attribute__ ((section (".mydata")))

int so_data_bss  = 0;
int so_data_static = 100;
