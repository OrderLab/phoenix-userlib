#ifndef _PHX_H_
#define _PHX_H_

/* ====== recovery mode status API ====== */
int phx_is_recovery_mode(void);
void phx_finish_recovery(void);

/* Main public API */
void *phx_init(int argc, const char *argv[], const char *envp[], void (*handler)(int));

/* Low-level building block API */
// currently only support one range
void phx_restart(void *data, void *start, void *end);
void phx_get_preserved(void **data, void **start, void **end);

#endif /* _PHX_H_ */
