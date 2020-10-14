#ifndef __LPROFILE_UTIL_H__
#define __LPROFILE_UTIL_H__

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LPROFILE_DEBUG_PRINT

#define LPROFILE_UNUSED __attribute__((unused))

#define LPROFILE_ERROR(format, ...) \
		fprintf(stderr, "[ERROR] %s %d: " format "\n", \
						__FILE__, __LINE__, ##__VA_ARGS__);

#define LPROFILE_INFO(format, ...) \
		fprintf(stdout, "[INFO] %s %d: " format "\n", \
						__FILE__, __LINE__, ##__VA_ARGS__);

#ifdef LPROFILE_DEBUG_PRINT
#define LPROFILE_DEBUG(format, ...) \
		fprintf(stderr, "[DEBUG] %s %d: " format "\n", \
						__FILE__, __LINE__, ##__VA_ARGS__);
#else
#define LPROFILE_DEBUG(format, ...)
#endif /* LPROFILE_DEBUG_PRINT */

#define zfree(p) do {	\
	if (p) {			\
		free(p);		\
		p = NULL;		\
	} } while (0)

#ifdef __cplusplus
}
#endif

#endif /* __LPROFILE_PROBE_UTIL_H__ */
