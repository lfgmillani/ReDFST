#ifndef GLOBAL_H
#define GLOBAL_H
#include <stdint.h>
#include "region_types.h"
#include "config.h"
extern __thread int tRedfstCpu;
extern __thread int tRedfstPrevId;
extern int gFreq[2];
extern int gRedfstCurrentId[REDFST_MAX_THREADS];
extern int gRedfstCurrentFreq[REDFST_MAX_THREADS];
extern redfst_region_t gRedfstRegion[REDFST_MAX_THREADS][REDFST_MAX_REGIONS];
extern uint64_t gRedfstSlowRegions;
extern uint64_t gRedfstFastRegions;
extern uint64_t __redfstTime0;
extern int gRedfstThreadCount;
#define FREQ_HIGH gFreq[1]
#define FREQ_LOW  gFreq[0]

#endif
