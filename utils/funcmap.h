#ifndef __FUNC_MAP_H__
#define __FUNC_MAP_H__

#include <cstdlib>
#include <cstdint>
#include <unistd.h>
#include <cstdio>

#include <string>
#include <map>
#include <vector>

#include "test.h"

#define FUNCMAP_DIR "/home/jr/profile/lprofile/lprofile_cache/funcmap/"
#define FUNCMAP_PREFIX "LPROFILE_WRAP_";

#define FUNC_NAME_MAX 256

class BPatch_object;
class BPatch_function;

enum {
	// Cache is up-to-date. Load directly from cache file
	FUNCMAP_STATE_CACHED = 0,
	// Cache is out-of-date, need updating.
	FUNCMAP_STATE_UPDATE,
	// ERROR: ELF file is not found.
	FUNCMAP_STATE_ENOELF,
	// ERROR: failed to check cache file
	FUNCMAP_STATE_ECACHE,
};

enum {
	FUNCMAP_SYMBOL_HOOK = 0,
	FUNCMAP_SYMBOL_WRAP
};

enum {
	FUNCMAP_FILE_SRC = 0,
	FUNCMAP_FILE_LIB
};

class FuncMap {
	private:
		// The name (and path) of execuable/library
		std::string elf_name;
		std::string elf_path;
		// BPatch_object instance for this elf
		BPatch_object *obj;
		// List of monitroable functions
		std::vector<std::string> funcs;
		// Map between functions' name and index
		std::map<std::string, unsigned int> func_indices;
		// wrapper source file
		FILE *wrapper;
		bool is_wrap;

		// check the state of cache file
		uint8_t checkState(void);

		// add funtion to function map
		unsigned addFunction(const char *func);

		// generate wrapper
		bool generateWrapper(BPatch_function *func, unsigned index);

		std::string generateWrapDef(unsigned index, unsigned type,
						bool is_proto, unsigned nb_params, bool has_ret);
		std::string generateWrapParam(bool is_proto, unsigned nb_params);
		
		bool compileWrapper(void);

		// build directories for cache file
		bool buildDir(void);

		// load functions from cache file
		bool loadFromCache(void);
		// load functions from ELF file
		bool loadFromELF(void);

		// update cache file
		bool updateCache(void);

	public:
		FuncMap(std::string name, std::string path, bool wrap);
		FuncMap(std::string path, bool wrap);
		FuncMap(BPatch_object *target, bool wrap);
		~FuncMap(void);

		/* load function map
		 * @param force_update
		 *		Whether force updating the cache
		 */
		bool load(bool force_update);

		// get function ID by name
		// return UINT16_MAX on failure
		unsigned int getFunctionID(std::string func);
		unsigned int getFunctionID(const char *func);

		// print all functions
		void printAll(void);

		static std::string getWrapFilePath(std::string elf, unsigned type);
		static std::string getWrapSymbolName(unsigned index, unsigned type);
};

#define FUNCMAP_CMD "funcmap"
class FuncMapTest: public Test {
	private:
		std::string elf_path;
		bool force_update;
		FuncMap *funcmap;

	public:
		FuncMapTest(void);
		~FuncMapTest(void);

		static void staticUsage(void);
		static Test *construct(void);

		void usage(void);
		bool parseArgs(int argc, char **argv);
		bool init(void);
		bool process(void);
		void destroy(void);
};

#endif // __FUNC_MAP_H__
