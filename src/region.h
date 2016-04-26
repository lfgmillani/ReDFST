#ifndef REDFST_REGION_H
#define REDFST_REGION_H
#include <stdint.h>
#include "redfst/perf.h"
#include "redfst/config.h"
#include "redfst/region_types.h"
#ifndef REDFSTLIB_STATIC
void redfst_region_final();
#else
#include "redfst/region.c"
#endif
#endif
