#include <string.h>
#include <stdint.h>
#include "region.h"
#include "config.h"
#include "global.h"
__thread int tRedfstCpu;
__thread int tRedfstPrevId;
char *__redfstPrintBuf=0;
int gFreq[2];
int gRedfstCurrentId[REDFST_MAX_THREADS];
int gRedfstCurrentFreq[REDFST_MAX_THREADS];
redfst_region_t gRedfstRegion[REDFST_MAX_THREADS][REDFST_MAX_REGIONS];
uint64_t gRedfstSlowRegions;
uint64_t gRedfstFastRegions;
#ifdef REDFST_FUN_IN_H
uint64_t __redfstTime0;
#endif
int gRedfstThreadCount;
