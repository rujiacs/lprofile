#ifndef __LPROFILE_PROF_H__
#define __LPROFILE_PROF_H__

#include <vector>
#include <string>

#include "BPatch_Vector.h"
#include "count.h"
#include "test.h"

class BPatch_process;
class BPatch_binaryEdit;
class BPatch_module;
#define PROF_CMD "prof"

class ProfTest: public Test {
	private:
		BPatch_addressSpace *as;

		std::string muttee_bin;
		// for binary editor
		std::string output;

		// for process
		const char **muttee_argv;
		int muttee_argc;

		unsigned int mode;
		CountUtil count;

	public:
		ProfTest(void);
		~ProfTest(void);

		static void staticUsage(void);
		static Test *construct(void);
		bool parseArgs(int argc, char **argv);
		bool init(void);
		bool process(void);
		void destroy(void) {};

	private:
		bool insertInit(void);
		bool insertExit(void);
		BPatch_module *findMainModule(void);
};


#endif /* __LPROFILE_PROF_H__ */
