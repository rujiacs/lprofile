#include <stdio.h>
#include "../../probe/probe.h"

void * __func(void *, void *, void *, void *, void *, void *, void *);

void * wrap_func(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6, void *p7) {
	void *ret;
//	lprobe_func_entry_empty(1);
	fprintf(stdout, "enter func[0] func\n");
	ret = __func(p1, p2, p3, p4, p5, p6, p7);
//	lprobe_func_exit_empty(1);
	fprintf(stdout, "exit func[0] func\n");
	return ret;
}

void * __libfunc(void *);
void * wrap_libfunc(void *p1) {
	void *ret;
	fprintf(stdout, "enter func[1] libfunc\n");
	ret = __libfunc(p1);
	fprintf(stdout, "exit func[1] libfunc\n");
	return ret;
}
