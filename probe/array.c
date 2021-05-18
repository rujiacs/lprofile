#include "util.h"
#include "array.h"

struct array *array__new(uint32_t elem_num, uint32_t elem_size)
{
	struct array *arr = NULL;

	arr = (struct array*)zalloc(sizeof(struct array)
					+ elem_num * elem_size);

	if (arr != NULL) {
		arr->elem_num = elem_num;
		arr->elem_size = elem_size;
	}

	return arr;
}

void array__delete(struct array *arr)
{
	free(arr);
}
