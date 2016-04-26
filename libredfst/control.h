#ifndef REDFST_CONTROL_H
#define REDFST_CONTROL_H
#include "redfst/config.h"

#ifndef REDFSTLIB_STATIC
/* reset energy counters */
void redfst_reset();
/* prints energy counters to file descriptor 3*/
void redfst_print();
/* writes into dst the values of pkg, pp0 and dram of core
   dst = {PKG.core, PP0.core, DRAM.core} */
void redfst_get(double *dst, int core);
/* writes into dst the values of pkg, pp0 and dram for each core
   dst = {PKG.0, PP0.0, DRAM.0, ..., PKG.N, PP0.N, DRAM.N} */
void redfst_get_all(double *dst);
#else
#include "redfst/control.c"
#endif
#endif
