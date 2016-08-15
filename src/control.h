#ifndef REDFST_CONTROL_H
#define REDFST_CONTROL_H
#include "config.h"
#include "energy.h"

#define redfst_print() __redfst_print()
#define redfst_printf(...) do{fprintf(__redfst_fd, __VA_ARGS__);__redfst_print();}while(0)

#ifndef REDFST_FUN_IN_H
/* reset energy counters */
void redfst_reset();
/* prints energy counters to file descriptor 3*/
void __redfst_print();
/* writes into dst the values of pkg, pp0 and dram of the given cpu
   dst = {cpu.PKG, cpu.PP0, cpu.DRAM} */
void redfst_get_legacy(double *dst, int cpu);
/* writes into dst the values of pkg, pp0 and dram for each cpu, followed
   by the total energy for pkg, pp0 and dram, and execution time in seconds
   dst = {cpu0.PKG, cpu0.PP0, cpu0.DRAM,
          ...,
          cpuN.PKG, cpuN.PP0, cpuN.DRAM,
          TIME} */
void redfst_get_all_legacy(double *dst);
redfst_dev_t * redfst_get(redfst_dev_t *dev);
redfst_dev_t * redfst_dev_init(redfst_dev_t *dev);
#endif
#endif
