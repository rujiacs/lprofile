#include <stdio.h>
#include <getopt.h>

#include "util.h"
#include "report.h"
#include "test.h"

using namespace std;

bool ReportTest::processRecord(struct prof_record *record)
{
	struct prof_record stack[32];
	int index = 0;

	if (is_count) {
		if (record->pos == PROF_RECORD_POS_PRE) {
			stack[index].ts = record->ts;
			stack[index].func_idx = record->func_idx;
			stack[index].pos = record->pos;
			stack[index].ev_idx = record->ev_idx;
			stack[index].count = record->count;
			index ++;
		}
		else {
			if (index < 1 ) {
				LPROFILE_ERROR("Cannot find previous record");
				return false;
			}
			if (stack[index-1].func_idx == record->func_idx ||
					stack[index-1].pos == PROF_RECORD_POS_PRE) {
				fprintf(stdout, "%u %lu %lu\n", record->func_idx,
					(record->ts - stack[index-1].ts),
					(record->count - stack[index-1].count));
			}
		}
	}
	else {
		fprintf(stdout, "%lu %u %u %u %lu\n",record->ts,
				record->func_idx, record->pos,
				record->ev_idx, record->count);
	}
	return true;
}


ReportTest::ReportTest(void)
{
	datafile = "";
	is_count = false;
}

ReportTest::~ReportTest(void)
{
}

Test *ReportTest::construct(void)
{
	return new ReportTest();
}

bool ReportTest::init(void)
{
	return true;
}

void ReportTest::staticUsage(void)
{
	fprintf(stdout, "./lprofile %s -f <datafile> [OPTIONS]\n",
					REPORT_CMD);
	fprintf(stdout, "  OPTIONS:\n");
	// fprintf(stdout, "    -f       force updating the cache file\n");
}

void ReportTest::usage(void)
{
	staticUsage();
}

bool ReportTest::parseArgs(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "f:c")) != -1) {
		switch(c) {
			case 'f':
				if (access(optarg, F_OK) != 0) {
					LPROFILE_ERROR("File %s doesn't exist",
									optarg);
					return false;
				}
				datafile = optarg;
				break;
			
			case 'c':
				is_count = true;

			default:
				LPROFILE_ERROR("Unknown option %c, usage:", c);
				usage();
				return false;
		}
	}

	if (datafile.size() == 0) {
		LPROFILE_ERROR("No data file specified");
		return false;
	}

	return true;
}

bool ReportTest::process(void)
{
	FILE *fp = NULL;
	struct prof_record record;
	size_t nread = 0;
	int nrecord = 0;

	fp = fopen(datafile.c_str(), "r");
	if (!fp) {
		LPROFILE_ERROR("Failed to open datafile %s", datafile.c_str());
		return false;
	}

	while (!feof(fp)) {
		nread = fread(&record, sizeof(struct prof_record), 1, fp);
		if (nread < 1) {
			if (!feof(fp)) {
				LPROFILE_ERROR("Failed to read record from %s",
					datafile.c_str());
			}
			break;
		}
		if (!processRecord(&record)) {
			LPROFILE_ERROR("Failed to process %d-nd record", nread);
			break;
		}
		nrecord ++;
	}

	LPROFILE_DEBUG("Total %d records", nrecord);
	fclose(fp);
	
//	Report->printAll();
	return true;
}

void ReportTest::destroy(void)
{
	
}
