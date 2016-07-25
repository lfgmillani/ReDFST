#ifndef REDFST_ENERGY_H
#define REDFST_ENERGY_H
#include <stdint.h>
#include "config.h"
typedef struct{
	uint64_t pkg, pp0, dram;
	uint32_t pkgPrev, pp0Prev, dramPrev;
	double unit;
	int id;
	int fd[3];
}cpu_t;

extern void (*__redfst_energy_update)();
extern void (*__redfst_energy_update_one)(cpu_t *c);
extern int __redfstNcpus;
extern cpu_t *__redfstCpu;
extern cpu_t **gCpuId2Cpu;
extern volatile int __redfstMutex;
extern FILE *__redfst_fd;
typedef enum {REDFST_NONE, REDFST_POWERCAP, REDFST_MSR, REDFST_LIKWID} redfst_support_t;
extern redfst_support_t redfstEnergySupport;

void redfst_energy_init();
int redfst_cpus(const int *cpus);
#endif
