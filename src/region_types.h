#ifndef REDFST_REGION_TYPES_H
#define REDFST_REGION_TYPES_H
#include <stdint.h>
#include "perf.h"
#include "config.h"
typedef struct{
	int next[REDFST_MAX_REGIONS];
	redfst_perf_t perf;
	uint64_t time,timeStarted;
} __attribute__((aligned(4),packed)) redfst_region_t;
#endif
