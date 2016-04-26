#ifndef REDFST_CONTROL_H
#define REDFST_CONTROL_H
#include "redfst/config.h"

#ifndef REDFST_STATIC
/* reset energy counters */
void redfst_reset();
/* prints energy counters to file descriptor 3*/
void redfst_print();
/* writes into dst the values of pkg, pp0 and dram of the given cpu
   dst = {cpu.PKG, cpu.PP0, cpu.DRAM} */
void redfst_get(double *dst, int cpu);
/* writes into dst the values of pkg, pp0 and dram for each cpu, followed
   by the total energy for pkg, pp0 and dram, and execution time in seconds
   dst = {cpu0.PKG, cpu0.PP0, cpu0.DRAM,
          ...,
          cpuN.PKG, cpuN.PP0, cpuN.DRAM,
          TIME} */
void redfst_get_all(double *dst);
#else
#include "redfst/control.c"
#endif
#endif
