#ifdef __cplusplus
#include <cstdbool>
#include <csignal>
#else
#include <stdbool.h>
#define _GNU_SOURCE
#include <signal.h>
#endif

/* ====== recovery mode status API ====== */
bool phx_is_recovery_mode(void);
void phx_finish_recovery(void);

/* Main public API */
void *phx_init(int argc, const char *argv[], const char *envp[], sighandler_t handler);

/* Low-level building block API */
// currently only support one range
void phx_restart(void *data, void *start, void *end);
void phx_get_preserved(void **data, void **start, void **end);

// for multi-range support
void phx_restart_multi(void *data_arr, void *start_arr, void *end_arr,
                       const unsigned int len);
void phx_get_preserved_multi(void **data_arr, void **start_arr, void **end_arr,
                             const unsigned int len);
