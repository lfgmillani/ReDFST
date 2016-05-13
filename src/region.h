#ifndef REDFST_REGION_H
#define REDFST_REGION_H
#include <stdint.h>
#include "perf.h"
#include "config.h"
#include "region_types.h"
#ifndef REDFST_FUN_IN_H
void redfst_region(int id);
#else
#include "region.c"
#endif
#endif
