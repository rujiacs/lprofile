#ifndef __LPROFILE_PROBE_H__
#define __LPROFILE_PROBE_H__

#ifdef __cplusplus
extern "C" {
#endif

void lprobe_func_entry_empty(unsigned int func_index);
void lprobe_func_exit_empty(unsigned int func_index);

#ifdef __cplusplus
}
#endif

#endif /* __LPROFILE_PROBE_H__ */
