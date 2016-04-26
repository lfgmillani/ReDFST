#ifndef REDFST_REGION_H
#define REDFST_REGION_H
#include <stdint.h>
#include "redfst/perf.h"
#include "redfst/config.h"
typedef struct{
	int next[REDFSTLIB_MAX_REGIONS];
	redfst_perf_t perf;
	uint64_t time,timeStarted;
} __attribute__((aligned(4),packed)) redfst_region_t;
#ifndef REDFSTLIB_STATIC
void redfst_region_final();
#endif
#endif
