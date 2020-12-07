#ifndef __LPROFILE_REPLACE_H__
#define __LPROFILE_REPLACE_H__

#include <vector>
#include <string>

#include "BPatch_Vector.h"
#include "count.h"
#include "test.h"

class BPatch_process;

#define REPLACE_CMD "replace"

class BPatch_function;
class BPatch_object;
class TargetFunc;

class ReplaceTest: public Test {
	private:
		BPatch_addressSpace *as;
		std::string muttee_bin;
		unsigned int mode;

		const char **muttee_argv;
		int muttee_argc;

		std::string output;

		CountUtil count;

	public:
		ReplaceTest(void);
		~ReplaceTest(void);

		static void staticUsage(void);
		static Test *construct(void);
		bool parseArgs(int argc, char **argv);
		bool init(void);
		bool process(void);
		void destroy(void) {};

	private:

		void handleElf(std::string elf, funclist_t &targets);
		void handleFunction(BPatch_object *libwrap, TargetFunc *func);
};













#endif
