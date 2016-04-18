#include "global.h"
__thread int tRedfstCpu;
__thread int tRedfstPrevId;

int gFreq[2];
int gRedfstCurrentId[REDFSTLIB_MAX_THREADS];
int gRedfstCurrentFreq[REDFSTLIB_MAX_THREADS];
redfst_region_t gRedfstRegion[REDFSTLIB_MAX_THREADS][REDFSTLIB_MAX_REGIONS];
uint64_t gRedfstSlowRegions;
uint64_t gRedfstFastRegions;
int gRedfstThreadCount;
