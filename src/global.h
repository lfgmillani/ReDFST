#ifndef GLOBAL_H
#define GLOBAL_H
#include <stdint.h>
#include "region_types.h"
#include "config.h"
extern __thread int tRedfstCpu;
extern __thread int tRedfstPrevId;
extern char *__redfstPrintBuf;
extern int gRedfstCurrentId[REDFST_MAX_THREADS];
extern int gRedfstCurrentFreq[REDFST_MAX_THREADS];
extern int gRedfstMinFreq;
extern int gRedfstMaxFreq;
extern int gRedfstFreq[REDFST_MAX_NFREQS];
extern uint8_t gRedfstRegionFreqId[REDFST_MAX_REGIONS];
extern redfst_region_t gRedfstRegion[REDFST_MAX_THREADS][REDFST_MAX_REGIONS];
extern uint64_t __redfstTime0;
extern int gRedfstThreadCount;
#endif
