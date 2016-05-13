#ifndef REDFST_CONTROL_C
#define REDFST_CONTROL_C
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "util.h"
#include "macros.h"
#include "energy.h"
#include "control.h"

#define REDFST_LOCK(x) __sync_val_compare_and_swap(&(x), 0, 1) // return 0 if it manages to obtain the mutex
#define REDFST_UNLOCK(x) do{(x)=0;}while(0)

#ifdef REDFST_FUN_IN_H // static is not accidentally swapped here
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



#ifdef REDFST_FUN_IN_H
static inline
#endif
void redfst_reset(){
/* zeroes energy counters */
	cpu_t *c;
	int i;
	while(unlikely(REDFST_LOCK(__redfstMutex)))
		;
	__redfst_energy_update();
	for(i=0; i < __redfstNcpus; ++i){
		c = __redfstCpu+i;
		c->pkg = c->pp0 = c->dram = 0;
	}
	REDFST_UNLOCK(__redfstMutex);
	__redfstTime0 = __redfst_time_now();
}



#ifdef REDFST_FUN_IN_H
static inline
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
	t = (__redfst_time_now() - __redfstTime0) * 1e-9;
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
		n += sprintf(buf+n,"%lf, %lf, %lf, ",pkg,pp0,dram);
	}
	sprintf(buf+n,"%lf, %lf, %lf, %lf\n",totalPkg,totalPp0,totalDram,t);
	fprintf(__redfst_fd,buf);
}



#ifdef REDFST_FUN_IN_H
static inline
#endif
void redfst_get(double *dst, int cpu){
/*
	write into dst the energy consumed by each cpu:
	dst[0] = cpu.pkg
	dst[1] = cpu.pp0
	dst[2] = cpu.dram
*/
	cpu_t *c;
	c = gCpuId2Cpu[cpu];
	__redfst_safe_update_one(c);
	dst[0] = c->pkg * c->unit;
	dst[1] = c->pp0 * c->unit;
	dst[2] = c->dram * c->unit;
}



#ifdef REDFST_FUN_IN_H
static inline
#endif
void redfst_get_all(double *dst){
/*
	write into dst the energy consumed by each cpu:
	dst[0+3*cpu]   = pkg.cpu
	dst[1+3*cpu]   = pp0.cpu
	dst[2+3*cpu]   = dram.cpu
	dst[3*ncpus+1] = execution time in seconds
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
	*dst = (__redfst_time_now() - __redfstTime0) * 1e-9;
}

#undef REDFST_LOCK
#undef REDFST_UNLOCK
#endif
