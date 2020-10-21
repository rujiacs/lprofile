#include <stdio.h>

//#define TEST_WRAP

#ifdef TEST_WRAP
#include <dlfcn.h>

#define LIBWRAP_PATH "/home/rujia/project/profile/lprofile/test/lib/libtest.so"

typedef (void *) (*func_type)(void *, void *, void *, void *);
#endif

struct test_struct {
	int sa;
	int sb;
};

class test_class {
	public:
		int ca;
		int cb;
};

int func(int i1, int i2, int i3, int i4, int i5, int arr[4], struct test_struct *ts, test_class &tc) {
	int j = 10;

	j = i1 + i2 + i3 + i4 + i5
			+ ts->sa + ts->sb + tc.ca + tc.cb
			+ arr[0] + arr[1] + arr[2] + arr[3];

	return j;
}

int main()
{
	int i = 1, ret = 0;
	int arr[4];
	struct test_struct ts;
	test_class tc;

	arr[0] = arr[1] = arr[2] = arr[3] = 1;
	tc.ca = tc.cb = 1;
	ts.sa = ts.sb = 1;

#ifdef TEST_WRAP
	void *libwrap = NULL;
   	func_type wrap = NULL;

	libwrap = dlopen(LIBWRAP_PATH, RTLD_NOW);
	if (!libwrap) {
		fprintf(stderr, "Failed to open %s, %s\n",
						LIBWRAP_PATH, dlerror());
		return 1;
	}

	wrap = dlsym(libwrap, "wrap_func");
	if (!wrap) {
		fprintf(stderr, "Failed to open func wrap_func\n");
		dlclose(libwrap);
		libwrap = NULL;
		return 1;
	}

	ret = wrap(i);
	fprintf(stdout, "%d\n", ret);
#endif

	ret = func(i, 1, 1, 1, 1, arr, &ts, tc);

	fprintf(stdout, "%d\n", ret);
	
	return 0;
}
