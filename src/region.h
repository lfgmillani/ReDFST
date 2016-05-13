#ifndef REDFST_REGION_H
#define REDFST_REGION_H
#include <stdint.h>
#include "perf.h"
#include "config.h"
#include "region_types.h"
#ifndef REDFST_STATIC
void redfst_region_final();
#else
#include "region.c"
#endif
#endif
