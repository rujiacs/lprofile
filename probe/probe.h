#ifndef __LPROFILE_PROBE_H__
#define __LPROFILE_PROBE_H__

#ifdef __cplusplus
extern "C" {
#endif

void lprobe_func_entry_empty(unsigned int func_index);
void lprobe_func_exit_empty(unsigned int func_index);

void *lprobe_init_empty(void);
void lprobe_exit_empty(void);

int lprobe_empty_pthread_create(void *p1, void *p2, void *p3, void *p4);
void lprobe_empty_pthread_exit(void *p);
void lprobe_thread_init_empty(void *);
void lprobe_thread_exit_empty(void);

void *lprobe_init(char *evlist_str, char *logfile,
                unsigned min_id, unsigned max_id, unsigned freq);

void lprobe_exit(void);

void lprobe_thread_exit(void);
void lprobe_thread_init(void *);

int lprobe_pthread_create(void *p1, void *p2, void *p3, void *p4);
void lprobe_pthread_exit(void *p);
void lprobe_func_entry(unsigned int func_index);
void lprobe_func_exit(unsigned int func_index);

extern unsigned tid_offset;


void HOOK_pthread_exit(void *p);
int HOOK_pthread_create(void *p1, void *p2, void *p3, void *p4);

#ifdef __cplusplus
}
#endif

#endif /* __LPROFILE_PROBE_H__ */
