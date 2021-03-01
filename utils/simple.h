#ifndef __LPROFILE_SIMPLE_H__
#define __LPROFILE_SIMPLE_H__

#include <vector>
#include <string>

#include "BPatch_Vector.h"
#include "count.h"
#include "test.h"

class BPatch_process;

#define SIMPLE_CMD "simple"

#define SIMPLE_MUTTEE "/home/rujia/project/profile/lprofile/test/test"
#define SIMPLE_LIBWRAP "/home/rujia/project/profile/lprofile/test/lib/libtest.so"

#define SIMPLE_FUNC "libfunc,func"
//#define SIMPLE_WRAP_FUNC "wrap_func"
//#define SIMPLE_HOOK_FUNC "__func"

class BPatch_function;

class MutteeFunc {
	public:
		BPatch_function *func;
		BPatch_object *obj;
		unsigned int index;
		bool is_shared;

		MutteeFunc(BPatch_function *f = NULL, BPatch_object *o = NULL,
						unsigned i = UINT16_MAX);
};

class SimpleTest: public Test {
	private:
		BPatch_process *proc;
		std::string muttee_bin;
		const char **muttee_argv;
		int muttee_argc;

		vector<MutteeFunc> targets;

	public:
		SimpleTest(void);
		~SimpleTest(void);

		static void staticUsage(void);
		static Test *construct(void);
		bool parseArgs(int argc, char **argv);
		bool init(void);
		bool process(void);
		void destroy(void) {};

	private:
		Dyninst::SymtabAPI::Symbol *findHookSymbol(BPatch_function *func_wrap, std::string orgname);

		bool findTargets(void);
		void findTarget(const char *);

		bool isSystemLib(BPatch_object *);

		void findCaller(MutteeFunc *tfunc);
};


#endif /* __LPROFILE_SIMPLE_H__ */
