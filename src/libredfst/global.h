#ifndef GLOBAL_H
#define GLOBAL_H
#include <stdint.h>
#include <string.h>
#include "region.h"
#include "libredfst_config.h"
#include "region.h"
extern __thread int tRedfstCpu;
extern __thread int tRedfstPrevId;
extern int gFreq[2];
extern int gRedfstCurrentId[REDFSTLIB_MAX_THREADS];
extern int gRedfstCurrentFreq[REDFSTLIB_MAX_THREADS];
extern redfst_region_t gRedfstRegion[REDFSTLIB_MAX_THREADS][REDFSTLIB_MAX_REGIONS];
extern uint64_t gRedfstSlowRegions;
extern uint64_t gRedfstFastRegions;
extern int gRedfstThreadCount;

#define FREQ_HIGH gFreq[1]
#define FREQ_LOW  gFreq[0]

#endif
