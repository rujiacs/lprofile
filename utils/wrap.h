#ifndef __LPROFILE_WRAP_H__
#define __LPROFILE_WRAP_H__

#include <vector>
#include <string>

#include "BPatch_Vector.h"
#include "count.h"
#include "test.h"

class BPatch_process;

#define WRAP_CMD "wrap"

class BPatch_function;
class BPatch_object;
class TargetFunc;

class WrapTest: public Test {
	private:
		unsigned int mode;
		CountUtil count;
		BPatch_addressSpace *as;
		std::string muttee_bin;

		const char **muttee_argv;
		int muttee_argc;

		std::string output;

	public:
		WrapTest(void);
		~WrapTest(void);

		static void staticUsage(void);
		static Test *construct(void);
		bool parseArgs(int argc, char **argv);
		bool init(void);
		bool process(void);
		void destroy(void) {};

	private:
		Dyninst::SymtabAPI::Symbol *findHookSymbol(
						BPatch_function *func_wrap, unsigned int index);

		void handleElf(std::string elf, funclist_t &targets);
		void handleFunction(BPatch_object *libwrap, TargetFunc *func);
};













#endif
