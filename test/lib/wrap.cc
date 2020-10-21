#include <stdio.h>

void * __func(void *, void *, void *, void *, void *, void *, void *, void *);

void * wrap_func(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6, void *p7, void *p8) {
	void *ret;
	fprintf(stdout, "enter wrap_func\n");
	ret = __func(p1, p2, p3, p4, p5, p6, p7, p8);
	fprintf(stdout, "exit wrap_func\n");
	return ret;
}
