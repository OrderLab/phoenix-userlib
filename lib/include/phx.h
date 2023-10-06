#ifndef _PHX_H_
#define _PHX_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ====== recovery mode status API ====== */
int phx_is_recovery_mode(void);
void phx_finish_recovery(void);

/* Main public API */
void *phx_init(int argc, const char *argv[], const char *envp[], void (*handler)(int));

/* Low-level building block API */
// for multi-range support
void phx_restart_multi(void *data, void *start_arr, void *end_arr,
                    unsigned int len);
void phx_get_preserved_multi(void **data, void **start_arr, void **end_arr,
                            unsigned int* len);

#ifdef __cplusplus
}
#endif

#endif /* _PHX_H_ */
