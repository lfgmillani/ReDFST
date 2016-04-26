#ifndef REDFST_ENERGY_H
#define REDFST_ENERGY_H
#include <stdint.h>

typedef struct{
	uint64_t pkg, pp0, dram;
	uint32_t pkgPrev, pp0Prev, dramPrev;
	double unit;
	int id;
	int fd;
}core_t;

extern void (*__redfst_energy_update)();
extern void (*__redfst_energy_update_one)(core_t *c);
extern int __redfstNcores; // actually these are hardware threads, not cores. this is misnamed everywhere.
extern core_t *__redfstCore;
extern core_t **gCoreId2Core;
extern volatile int __redfstMutex;
extern FILE *__redfst_fd;

void redfst_energy_init();
#endif