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

#define SIMPLE_FUNC "func"
#define SIMPLE_WRAP_FUNC "wrap_func"
#define SIMPLE_HOOK_FUNC "__func"

class BPatch_function;

class SimpleTest: public Test {
	private:
		BPatch_process *proc;
		std::string muttee_bin;
		const char **muttee_argv;
		int muttee_argc;

		CountUtil count;

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
		Dyninst::SymtabAPI::Symbol *findHookSymbol(BPatch_function *func_wrap);
};


#endif /* __LPROFILE_SIMPLE_H__ */
