#ifndef REDFST_H
#define REDFST_H
#ifdef __cplusplus
extern "C"{
#endif
#include "redfst/config.h"
void redfst_monitor_set_status(int n);

#ifndef REDFST_STATIC
void redfst_region(int id);
void redfst_region_all(int id);
void redfst_reset(void);
void redfst_print(void);
void redfst_get(double *dst, int cpu);
void redfst_get_all(double *dst);
#else
#include "redfst/region.c"
#include "redfst/control.c"
#endif
#ifdef __cplusplus
};
#endif
#endif