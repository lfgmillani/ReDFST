#ifndef REGION_H
#define REGION_H
#include <stdint.h>
#include "libredfst_config.h"
#include "perf.h"
typedef struct{
	int next[REDFSTLIB_MAX_REGIONS];
	redfst_perf_t perf;
	uint64_t time,timeStarted;
} __attribute__((aligned(4),packed)) redfst_region_t;
#ifndef REDFSTLIB_STATIC
void redfst_region_final();
#endif
#endif
