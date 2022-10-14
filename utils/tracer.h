#ifndef __TRACER_H__
#define __TRACER_H__

#include <vector>
#include <map>
#include <string>

#include "BPatch_Vector.h"
#include "count.h"
#include "test.h"

class BPatch_process;
class BPatch_function;
class BPatch_object;

class FuncMap;

#define TRACER_CMD "tracer"

// class TracedFunc{
// 	public:
// 		BPatch_function *func;
// 		unsigned int index;

// 		TracedFunc(void);
// 		TracedFunc(BPatch_function *, unsigned int);
// };

class TracerTest: public Test {

	private:
		int pid;
		BPatch_process *proc;

		CountUtil count;

		// void getTraceFunctions(BPatch_object *obj, FuncMap *fmap);
		// void getUserObjects(BPatch_Vector<BPatch_object *> &objs);
		// BPatch_function *findFunction(
		// 				BPatch_object *obj, std::string name);
		bool callInit(void);
		bool insertExit(void);

		void clearSnippets(void);

		BPatch_module *findMainModule(void);

	public:
		TracerTest(void);
		// TracerTest(int pid, const char *pattern);
		~TracerTest(void);

		static void staticUsage(void);
		static Test *construct(void);

		bool parseArgs(int argc, char **argv);
		bool init(void);
		bool process(void);
		void destroy(void);
};






#endif // __TRACER_H__
