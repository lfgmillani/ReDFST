#include <string.h>
#include <stdint.h>
#include "region.h"
#include "config.h"
#include "global.h"
__thread int tRedfstCpu;
__thread int tRedfstPrevId;
char *__redfstPrintBuf=0;
int gRedfstCurrentId[REDFST_MAX_THREADS];
int gRedfstCurrentFreq[REDFST_MAX_THREADS];
int gRedfstMinFreq;
int gRedfstMaxFreq;
int gRedfstFreq[REDFST_MAX_NFREQS];
uint8_t  gRedfstRegionFreqId[REDFST_MAX_REGIONS];
redfst_region_t gRedfstRegion[REDFST_MAX_THREADS][REDFST_MAX_REGIONS];
uint64_t __redfstTime0;
int gRedfstThreadCount;
