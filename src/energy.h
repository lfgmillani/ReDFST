#ifndef REDFST_ENERGY_H
#define REDFST_ENERGY_H
#include <stdint.h>
#include "config.h"
typedef struct{
	uint64_t pkg, pp0, dram;
	uint32_t pkgPrev, pp0Prev, dramPrev;
	double unit;
	int id;
	int fd;
}cpu_t;

extern void (*__redfst_energy_update)();
extern void (*__redfst_energy_update_one)(cpu_t *c);
extern int __redfstNcpus;
extern cpu_t *__redfstCpu;
extern cpu_t **gCpuId2Cpu;
extern volatile int __redfstMutex;
extern FILE *__redfst_fd;
extern int redfstEnergySupport;

void redfst_energy_init();
int redfst_cpus(const int *cpus);
#ifdef REDFST_STATIC
static int redfst_support(){ return redfstEnergySupport; }
#else
int redfst_support();
#endif
#endif
