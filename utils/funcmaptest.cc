#include <stdio.h>
#include <getopt.h>

#include "util.h"
#include "funcmap.h"
#include "test.h"

using namespace std;

FuncMapTest::FuncMapTest(void)
{
	elf_path = "";
	funcmap = NULL;
	force_update = false;
}

FuncMapTest::~FuncMapTest(void)
{
}

Test *FuncMapTest::construct(void)
{
	return new FuncMapTest();
}

bool FuncMapTest::init(void)
{
	FuncMap *fmap = new FuncMap(elf_path);

	if (!fmap) {
		LPROFILE_ERROR("Failed to create FuncMap instance");
		return false;
	}

	if (!fmap->load(force_update)) {
		LPROFILE_ERROR("Failed to load function map for %s",
						elf_path.c_str());
		delete fmap;
		return false;
	}

	funcmap = fmap;
	return true;
}

void FuncMapTest::staticUsage(void)
{
	fprintf(stdout, "./lprofile %s -e <path_to_elf> [OPTIONS]\n",
					FUNCMAP_CMD);
	fprintf(stdout, "  OPTIONS:\n");
	fprintf(stdout, "    -f       force updating the cache file\n");
}

void FuncMapTest::usage(void)
{
	staticUsage();
}

bool FuncMapTest::parseArgs(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "e:f")) != -1) {
		switch(c) {
			case 'e':
				if (access(optarg, F_OK) != 0) {
					LPROFILE_ERROR("File %s doesn't exist",
									optarg);
					return false;
				}
				elf_path = optarg;
				break;

			case 'f':
				force_update = true;
				break;

			default:
				LPROFILE_ERROR("Unknown option %c, usage:", c);
				usage();
				return false;
		}
	}

	if (elf_path.size() == 0) {
		LPROFILE_ERROR("No ELF file specified");
		return false;
	}

	return true;
}

bool FuncMapTest::process(void)
{
	funcmap->printAll();
	return true;
}

void FuncMapTest::destroy(void)
{
	if (funcmap) {
		delete funcmap;
		funcmap = NULL;
	}
}
