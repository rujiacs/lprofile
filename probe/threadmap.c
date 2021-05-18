#include <dirent.h>
#include <limits.h>

#include "util.h"
#include "threadmap.h"

/* Skip "." and ".." directories */
static int filter(const struct dirent *dir)
{
	if (dir->d_name[0] == '.')
		return 0;
	else
		return 1;
}

static struct thread_map *_thread_map__alloc(int nr)
{
	struct thread_map *map = NULL;
	size_t size = 0;

	size = sizeof(struct thread_map)
			+ sizeof(pid_t) * nr;
	map = zalloc(size);
	if (map == NULL) {
		LOG_ERROR("Failed to allocate %u bytes memory for thread_map",
						(unsigned int)size);
		return NULL;
	}
	return map;
}

struct thread_map *thread_map__new_dummy(void)
{
	struct thread_map *map = NULL;

	map = _thread_map__alloc(1);
	if (map != NULL) {
		map->nr = 1;
		PID(map, 0) = -1;
	}
	return map;
}

struct thread_map *thread_map__new(pid_t pid)
{
	struct thread_map *map = NULL;
	char name[256] = {'\0'};
	int items;
	struct dirent **namelist = NULL;
	int i;

	sprintf(name, "/proc/%d/task", pid);
	items = scandir(name, &namelist, filter, NULL);
	if (items <= 0)
		return NULL;

	map = _thread_map__alloc(items);
	if (map != NULL) {
		map->nr = items;
		for (i = 0; i < items; i++) {
			PID(map, i) = atoi(namelist[i]->d_name);
//			LOG_INFO("pid[%d] %d(%d)", i, thread_map__pid(map, i),
//							atoi(namelist[i]->d_name));
		}
	}

	for (i = 0; i < items; i++)
		free(namelist[i]);
	free(namelist);

	return map;
}

void thread_map__free(struct thread_map *map)
{
	if (map != NULL)
		free(map);
	map = NULL;
}

void thread_map__dump(struct thread_map *map)
{
	int nthread = 0, i = 0;

	nthread = thread_map__nr(map);
	for (i = 0; i < nthread; i++) {
		LOG_INFO("thread %d", PID(map, i));
	}
}

int thread_map__getindex(struct thread_map *map, int pid)
{
	int i = 0, nthread = 0;

	nthread = thread_map__nr(map);
	for (i = 0; i < nthread; i++) {
		if (pid == PID(map, i))
			return i;
	}
	return -1;
}
