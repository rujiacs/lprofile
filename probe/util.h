#ifndef __LPROFILE_PROBE_UTIL_H__
#define __LPROFILE_PROBE_UTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

//#define LPROBE_DEBUG_PRINT

#define LPROBE_UNUSED __attribute__((unused))

#define LPROBE_ERROR(format, ...) \
		fprintf(stderr, "[ERROR] %s %d: " format "\n", \
						__FILE__, __LINE__, ##__VA_ARGS__);

#define LPROBE_INFO(format, ...) \
		fprintf(stdout, "[INFO] %s %d: " format "\n", \
						__FILE__, __LINE__, ##__VA_ARGS__);

#ifdef LPROBE_DEBUG_PRINT
#define LPROBE_DEBUG(format, ...) \
		fprintf(stderr, "[DEBUG] %s %d: " format "\n", \
						__FILE__, __LINE__, ##__VA_ARGS__);
#else
#define LPROBE_DEBUG(format, ...)
#endif /* LPROBE_DEBUG_PRINT */

#ifdef __cplusplus
}
#endif

#endif /* __LPROFILE_PROBE_UTIL_H__ */
