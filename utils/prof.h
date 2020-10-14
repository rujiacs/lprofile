#ifndef __LPROFILE_PROF_H__
#define __LPROFILE_PROF_H__

#include <vector>
#include <string>

#include "BPatch_Vector.h"
#include "count.h"
#include "test.h"

class BPatch_process;

#define PROF_CMD "prof"

class ProfTest: public Test {
	private:
		BPatch_process *proc;
		std::string muttee_bin;
		const char **muttee_argv;
		int muttee_argc;

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
#ifndef USE_FUNCCNT
		bool insertInit(void);
#endif
};


#endif /* __LPROFILE_PROF_H__ */
