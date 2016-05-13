#ifndef REDFST_CONTROL_H
#define REDFST_CONTROL_H
#include "config.h"
#include "energy.h"

/*
	allow redfst_print to be called with no arguments or with n arguments.
	with no arguments, __redfst_print is called.
	otherwise, the arguments are printed to the same stream __redfst_print uses
	and then __redfst_print is called.
*/
#define REDFST_IS_EMPTY(...) REDFST_IS_EMPTY_(, ##__VA_ARGS__, 0,0,0,0,0,0,0,0,0,0,1)
#define REDFST_IS_EMPTY_(_0,_1,_2,_3,_4,_5,_6,_7,_8_,_9,_10,N,...) N
#define redfst_print(...) redfst_print_(REDFST_IS_EMPTY(__VA_ARGS__),__VA_ARGS__)
#define redfst_print_(empty,...) redfst_print__(empty, __VA_ARGS__)
#define redfst_print__(empty,...) redfst_print ## empty (__VA_ARGS__)
#define redfst_print1(...) __redfst_print()
#define redfst_print0(...) do{fprintf(__redfst_fd, __VA_ARGS__);__redfst_print();}while(0)


#ifndef REDFST_FUN_IN_H
/* reset energy counters */
void redfst_reset();
/* prints energy counters to file descriptor 3*/
void __redfst_print();
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
#endif
#endif
