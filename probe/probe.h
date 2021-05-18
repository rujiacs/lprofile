#ifndef __LPROFILE_PROBE_H__
#define __LPROFILE_PROBE_H__

#ifdef __cplusplus
extern "C" {
#endif

void lprobe_func_entry_empty(unsigned int func_index);
void lprobe_func_exit_empty(unsigned int func_index);

void *lprobe_init_empty(void);
void lprobe_exit_empty(void);

void lprobe_thread_init_empty(unsigned long);
void lprobe_thread_exit_empty(void);

void *lprobe_init(char *evlist_str, char *logfile,
                unsigned min_id, unsigned max_id, unsigned freq);

void lprobe_exit(void);

void lprobe_thread_exit(void);
void lprobe_thread_init(void);

void lprobe_func_entry(unsigned int func_index);
void lprobe_func_exit(unsigned int func_index);


#ifdef __cplusplus
}
#endif

#endif /* __LPROFILE_PROBE_H__ */
