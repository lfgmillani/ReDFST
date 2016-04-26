#ifndef REDFST_CONTROL_C
#define REDFST_CONTROL_C
#include <stdio.h>
#include <stdlib.h>
#include "redfst/config.h"
#include "redfst/util.h"
#include "redfst/macros.h"
#include "redfst/energy.h"
#include "redfst/control.h"

#define REDFST_LOCK(x) __sync_val_compare_and_swap(&(x), 0, 1) // return 0 if it manages to obtain the mutex
#define REDFST_UNLOCK(x) do{(x)=0;}while(0)

#ifdef REDFSTLIB_STATIC // static is not accidentally swapped here
extern uint64_t __redfstTime0; // timer is set to last reset
#else
static uint64_t __redfstTime0; // timer is set to last reset
#endif

static void __redfst_safe_update(){
/*
	Update energy counters if no update is underway.
	Otherwise, wait for update to finish before returning.
*/
	if(likely(!REDFST_LOCK(__redfstMutex)))
		__redfst_energy_update();
	else
		while(REDFST_LOCK(__redfstMutex))
			;
	REDFST_UNLOCK(__redfstMutex);
}

static void __redfst_safe_update_one(cpu_t *c){
/*
	Update a single energy counter if no update is underway.
	Otherwise, wait for update to finish before returning.
*/
	if(likely(!REDFST_LOCK(__redfstMutex)))
		__redfst_energy_update_one(c);
	else
		while(REDFST_LOCK(__redfstMutex))
			;
	REDFST_UNLOCK(__redfstMutex);
}



#ifdef REDFSTLIB_STATIC
static
#endif
void redfst_reset(){
/* zeroes energy counters */
	cpu_t *c;
	int i;
	while(unlikely(REDFST_LOCK(__redfstMutex)))
		;
	for(i=0; i < __redfstNcpus; ++i){
		c = __redfstCpu+i;
		c->pkg = c->pp0 = c->dram = 0;
	}
	REDFST_UNLOCK(__redfstMutex);
	__redfstTime0 = time_now();
}



#ifdef REDFSTLIB_STATIC
static
#endif
void redfst_print(){
/* print current value of energy counters */
	static char *buf = 0;
	cpu_t *c;
	double pkg, pp0, dram;
	double totalPkg, totalPp0, totalDram;
	double t;
	int n;
	int i;
	totalPkg = totalPp0 = totalDram = 0;
	__redfst_safe_update();
	t = (time_now() - __redfstTime0) * 1e-9;
	if(!buf){
		buf = malloc((__redfstNcpus+1) * 3 * 32);
	}
	n = 0;
	for(i=0; i < __redfstNcpus; ++i){
		c = __redfstCpu+i;
		pkg = c->pkg * c->unit;
		pp0 = c->pp0 * c->unit;
		dram = c->dram * c->unit;
		totalPkg += pkg;
		totalPp0 += pp0;
		totalDram += dram;
#ifdef REDFST_CSV
		n += sprintf(buf+n,"%lf, %lf, %lf, ",pkg,pp0,dram);
#else
		n += sprintf(buf+n,"pkg.%d  %lf\npp0.%d  %lf\ndram.%d  %lf\n",
		       c->id, pkg, c->id, pp0, c->id, dram);
#endif
	}
#ifdef REDFST_CSV
	sprintf(buf+n,"%lf, %lf, %lf, %lf\n",totalPkg,totalPp0,totalDram,t);
#else
	sprintf(buf+n,,"pkg    %lf\npp0    %lf\ndram    %lf\ntime   %lf\n",totalPkg,totalPp0,totalDram,t);
#endif
	fprintf(__redfst_fd,buf);
}



#ifdef REDFSTLIB_STATIC
static
#endif
void redfst_get(double *dst, int cpu){
/*
	write into dst the energy consumed by each core:
	dst[0] = pkg.core
	dst[1] = pp0.core
	dst[2] = dram.core
*/
	cpu_t *c;
	c = gCpuId2Cpu[cpu];
	__redfst_safe_update_one(c);
	dst[0] = c->pkg * c->unit;
	dst[1] = c->pp0 * c->unit;
	dst[2] = c->dram * c->unit;
}



#ifdef REDFSTLIB_STATIC
static
#endif
void redfst_get_all(double *dst){
/*
	write into dst the energy consumed by each core:
	dst[0+3*core] = pkg.core
	dst[1+3*core] = pp0.core
	dst[2+3*core] = dram.core
*/
	cpu_t *c;
	int i;
	__redfst_safe_update();
	for(i=0; i<__redfstNcpus; ++i){
		c = __redfstCpu + i;
		*dst++ = c->pkg * c->unit;
		*dst++ = c->pp0 * c->unit;
		*dst++ = c->dram * c->unit;
	}
}

#undef REDFST_LOCK
#undef REDFST_UNLOCK
#endif
