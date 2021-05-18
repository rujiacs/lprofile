#ifndef _PROFILE_THREAD_MAP_H_
#define _PROFILE_THREAD_MAP_H_

#include <sys/types.h>
#include <stdio.h>

struct thread_map {
	int nr;
	pid_t pid[];
};

struct thread_map *thread_map__new_dummy(void);
struct thread_map *thread_map__new(pid_t pid);
void thread_map__free(struct thread_map *map);
void thread_map__dump(struct thread_map *map);

int thread_map__getindex(struct thread_map *map, int pid);


static inline int thread_map__nr(struct thread_map *map)
{
	return map ? map->nr : 1;
}

#define PID(map, thread)	\
		((map)->pid[thread])

#endif /* _PROFILE_THREAD_MAP_H_ */
