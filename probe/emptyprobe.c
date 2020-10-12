#include <stdio.h>

#include "probe.h"
#include "util.h"

void lprobe_func_entry_empty(
				LPROBE_UNUSED unsigned int func_index)
{
	LPROBE_DEBUG("func %d enter", func_index);
}

void lprobe_func_exit_empty(
				LPROBE_UNUSED unsigned int func_index)
{
	LPROBE_DEBUG("func %d exit", func_index);
}
