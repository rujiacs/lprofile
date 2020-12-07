#ifndef __LPROFILE_COUNT_H__
#define __LPROFILE_COUNT_H__

#include <vector>
#include <map>
#include <string>

#include "BPatch_Vector.h"


//#include "test.h"

class BPatch_addressSpace;
class BPatch_function;
class BPatch_object;
class BPatch_snippet;

class FuncMap;

enum {
	LPROFILE_MODE_PROC = 0,
	LPROFILE_MODE_EDIT,
};

#define USE_FUNCCNT

#define LIBCNT "/home/jr/profile/lprofile/build/probe/libprobe.so"

#ifdef USE_FUNCCNT
#define FUNC_PRE "lprobe_func_entry_empty"
#define FUNC_POST "lprobe_func_exit_empty"
#define FUNCC_ARG "f:"
#else
#define FUNC_PRE "prof_count_pre"
#define FUNC_POST "prof_count_post"
#define FUNC_INIT "prof_init"
#define FUNC_EXIT "prof_exit"
#define FUNC_TEXIT "prof_thread_exit"
#define FUNCC_ARG "f:e:l:F:o:"
#endif /* ifdef USE_FUNCCNT */

#define PATTERN_ALL "(.*)"

class TargetFunc{
	public:
		BPatch_function *func;
		unsigned int index;
		std::string elf;

		TargetFunc(void);
		TargetFunc(BPatch_function *, unsigned int, std::string elfname);
};

typedef std::vector<TargetFunc *> funclist_t;
typedef std::map<std::string, funclist_t> elfmap_t;

struct range {
	unsigned int min;
	unsigned int max;
};

class CountUtil {
	private:
		/* Command-line arguments */
		std::string pattern;
#ifndef USE_FUNCCNT
		std::string output;
#define OUTPUT_DEF "profile.data"
#define EVLIST_DEF "cpu-cycles"
		std::string evlist;
#define LOGFILE_DEF "profile.log"
		std::string logfile;
#define FREQ_DEF (0U)
		unsigned int freq;
#endif

		BPatch_addressSpace *as;

		BPatch_function *func_pre, *func_post;
		BPatch_function *func_init;
		BPatch_function *func_exit, *func_texit;

		struct range func_id_range;

		std::vector<TargetFunc> target_funcs;
		elfmap_t elf_targets;

	public:
		/* Get option string for parsing */
		static std::string getOptStr(void);
		/* Get usage string */
		static std::string getUsageStr(void);

		CountUtil(void) : pattern(PATTERN_ALL) {};

		/* Parse command-line options */
		bool parseOption(int opt, char *optarg);

		std::vector<TargetFunc> &getTargets(void) { return target_funcs; }
		elfmap_t& getTargetLists(void) { return elf_targets; }

		/* Set address space (for convenience) */
		void setAS(BPatch_addressSpace *addr) { as = addr; };

		/* Get function pattern */
		const std::string getPattern(void) { return pattern; };
		/* Set function pattern */
		void setPattern(const char *str) { pattern = str; }

		/* Get init function */
		BPatch_function *getInit(void) { return func_init; };

		/* Load all functions */
		bool loadFunctions(void);

		/* Get the list of target functions. */
		bool getTargetFuncs(bool is_wrap = false);
		/* Add target function to the list */
		void addTargetFunc(BPatch_function *func, unsigned idx, std::string obj);

		/* Insert counting functions into target functions */
		bool insertCount(void);

		/* Insert exit functions into target program */
		bool insertExit(std::string filter);

		/* Find a function in 'obj' based on its 'name' */
		BPatch_function *findFunction(BPatch_object *obj,
						std::string name);

		/* Construct the argument list for init function */
		void buildInitArgs(std::vector<BPatch_snippet *> &args);
	private:
		/* Get all objects and filter out system libraries and
		 * dyninst libraries */
		void getUserObjs(BPatch_Vector<BPatch_object *> &objs);

		/* Match the 'pattern' and the functions in 'fmap'. */
		void matchFuncs(BPatch_object *obj, FuncMap *fmap,
						std::string pattern);

		/* Calculate range of target function indices */
		void calculateRange(void);
};


#endif // __LPROFILE_PROF_H__
