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
// class Dyninst::SymtabAPI::Symbol;
class FuncMap;

enum {
	LPROFILE_MODE_PROC = 0,
	LPROFILE_MODE_EDIT,
};

// #define USE_FUNCCNT

#define LIBCNT "/home/jiaru/lprofile/build/probe/libprobe.so"
#define LIBTIDOFFSET "tid_offset"

#ifdef USE_FUNCCNT
#define FUNC_PRE "lprobe_func_entry_empty"
#define FUNC_POST "lprobe_func_exit_empty"
#define FUNC_INIT "lprobe_init_empty"
#define FUNC_EXIT "lprobe_exit_empty"
#define WRAP_TINIT "lprobe_empty_pthread_create"
#define WRAP_TEXIT "lprobe_empty_pthread_exit"
#define FUNCC_ARG "f:"
#else
#define FUNC_PRE "lprobe_func_entry"
#define FUNC_POST "lprobe_func_exit"
#define FUNC_INIT "lprobe_init"
#define FUNC_EXIT "lprobe_exit"
#define WRAP_TINIT "lprobe_pthread_create"
#define WRAP_TEXIT "lprobe_pthread_exit"
#define FUNCC_ARG "f:e:l:F:o:"
#endif /* ifdef USE_FUNCCNT */

#define PATTERN_ALL "(.*)"

class TargetFunc{
	public:
		BPatch_function *func;
		unsigned int index;
		std::string elf;
		bool is_user;

		TargetFunc(void);
		TargetFunc(BPatch_function *, unsigned int,
						std::string elfname, bool user);
};

typedef std::vector<size_t> funclist_t;
typedef std::map<std::string, vector<size_t>> elfmap_t;

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
#define EVLIST_DEF "l1d-read-misses"
		std::string evlist;
#define LOGFILE_DEF "profile.log"
		std::string logfile;
#define FREQ_DEF (0U)
		unsigned int freq;
#endif

		BPatch_addressSpace *as;

		BPatch_object *libcnt;
		BPatch_function *func_pre, *func_post;
		BPatch_function *func_init, *func_tinit;
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

		/* Get exit function */
		BPatch_function *getExit(void) { return func_exit; };

		/* Load all functions */
		bool loadFunctions(void);

		/* Get the list of target functions. */
		bool getTargetFuncs(bool is_wrap = false);
		/* Add target function to the list */
		void addTargetFunc(BPatch_function *func, unsigned idx,
				std::string obj, bool user);

		/* Insert counting functions into target functions */
		bool insertCount(void);

		// /* Insert exit functions into target program */
		// bool insertExit(std::string filter);

		/* Find a function in 'obj' based on its 'name' */
		BPatch_function *findFunction(BPatch_object *obj,
						std::string name);

		/* Construct the argument list for init function */
		void buildInitArgs(std::vector<BPatch_snippet *> &args);

		bool wrapFunction(std::string funcstr, BPatch_function *__wrap = NULL);

		bool wrapPthreadFunc(void);

		void clearSnippet(void);
	private:
		/* Get all objects and filter out system libraries and
		 * dyninst libraries */
		void getUserObjs(BPatch_Vector<BPatch_object *> &objs);

		/* Match the 'pattern' and the functions in 'fmap'. */
		void matchFuncs(BPatch_object *obj, FuncMap *fmap,
						std::string pattern);
		void matchFunc(BPatch_object *obj, FuncMap *fmap,
						std::string pattern);

		/* Calculate range of target function indices */
		void calculateRange(void);

		uint32_t findTIDOffset(void);

		Dyninst::SymtabAPI::Symbol *findHookSymbol(
						BPatch_function *func_wrap, string orgname);

};


#endif // __LPROFILE_PROF_H__
