#ifndef GLOBAL_H
#define GLOBAL_H
#include <stdint.h>
#include "redfst/region_types.h"
#include "redfst/config.h"
extern __thread int tRedfstCpu;
extern __thread int tRedfstPrevId;
extern int gFreq[2];
extern int gRedfstCurrentId[REDFST_MAX_THREADS];
extern int gRedfstCurrentFreq[REDFST_MAX_THREADS];
extern redfst_region_t gRedfstRegion[REDFST_MAX_THREADS][REDFST_MAX_REGIONS];
extern uint64_t gRedfstSlowRegions;
extern uint64_t gRedfstFastRegions;
extern int gRedfstThreadCount;

#define FREQ_HIGH gFreq[1]
#define FREQ_LOW  gFreq[0]

#endif
