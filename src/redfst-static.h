#ifndef REDFST_H
#define REDFST_H
#ifdef __cplusplus
extern "C"{
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#include <time.h>
#include <stdint.h>
#ifndef CONFIG_H
#define CONFIG_H
/*
	maximum number of code regions.
	as 64bit integers are used with this, it cannot be higher than 64.
*/
#define REDFST_MAX_REGIONS 64

/*
	maximum number of threads supported
*/
#define REDFST_MAX_THREADS 32

/*
	polling frequency for the monitor
*/
#define REDFST_MONITOR_FREQUENCY 100

/*
	if defined, the library will target OpenMP. at the moment that's the only
	option.
*/
#define REDFST_OMP

/*
	if defined, the library will define small functions in the header
	files. otherwise, the implementations will reside in the lib.
*/
//#define REDFST_FUN_IN_H static inline

/*
	if defined, frequency is decided on a per-core basis. otherwise, its per-cpu.
*/
#define REDFST_FREQ_PER_CORE
/*
	if REDFST_FREQ_PER_CORE was not defined, these variables define which cores will
	have their frequencies changed. all other cores will be kept at the low
	frequency.
*/
#define REDFST_CPU0 0
#define REDFST_CPU1 1

/*
	if defined, all calls to redfst_region(_all) are traced
*/
//#define REDFST_TRACE

#endif

#ifdef REDFST_FUN_IN_H
static inline
#endif
uint64_t __redfst_time_now(){
	struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC, &spec);
	return ((uint64_t)1e9) * spec.tv_sec + spec.tv_nsec;
}
#ifndef REDFST_CONTROL_C
#define REDFST_CONTROL_C
#include <stdio.h>
#include <stdlib.h>
#ifndef UTIL_H
#define UTIL_H
#include <stdint.h>
#ifdef REDFST_FUN_IN_H
#else
uint64_t __redfst_time_now();
#endif
#endif
#ifndef MACROS_H
#define MACROS_H
/*
	counts number of arguments; supports 0 args
	not exactly standard-compliant
 */
#define VA_NUM_ARGS(...) VA_NUM_ARGS_IMPL(, ##__VA_ARGS__, 10,9,8,7,6,5,4,3,2,1,0)
#define VA_NUM_ARGS_IMPL(_0,_1,_2,_3,_4,_5,_6,_7,_8_,_9,_10,N,...) N



/*
	gets the length of an array
*/
/* 0[x] guards against [] overloading, the division against using with a pointer */
#define LEN(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))



/*
	compile-time assert
*/
#define CASSERT(condition) ((void)sizeof(char[1 - 2*!(condition)]))



#define likely(x)    __builtin_expect (!!(x), 1)
#define unlikely(x)  __builtin_expect (!!(x), 0)

#endif
#ifndef REDFST_ENERGY_H
#define REDFST_ENERGY_H
#include <stdint.h>
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
#endif
#ifndef REDFST_CONTROL_H
#define REDFST_CONTROL_H

#ifndef REDFST_FUN_IN_H
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
#endif
#endif

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
#ifndef REDFST_REGION_C
#define REDFST_REGION_C
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <cpufreq.h>
#ifndef REDFST_REGION_H
#define REDFST_REGION_H
#include <stdint.h>
#ifndef REDFST_PERF_H
#define REDFST_PERF_H
#include <stdint.h>

#define REDFST_PERF_NUM_EVENTS 4

typedef union{
	struct{
		uint64_t refs;
		uint64_t miss;
		uint64_t cycles;
    uint64_t instr;
	};
	uint64_t events[REDFST_PERF_NUM_EVENTS];
}redfst_perf_t;
void redfst_perf_read(int cpu, redfst_perf_t *p);
float redfst_perf_get_miss_rate(redfst_perf_t *p);
void redfst_perf_init();
void redfst_perf_init_worker();
void redfst_perf_read(int cpu, redfst_perf_t *p);
#endif
#ifndef REDFST_REGION_TYPES_H
#define REDFST_REGION_TYPES_H
#include <stdint.h>
typedef struct{
	int next[REDFST_MAX_REGIONS];
	redfst_perf_t perf;
	uint64_t time,timeStarted;
} __attribute__((aligned(4),packed)) redfst_region_t;
#endif
#ifndef REDFST_FUN_IN_H
void redfst_region(int id);
#else
#endif
#endif
#ifndef GLOBAL_H
#define GLOBAL_H
#include <stdint.h>
extern __thread int tRedfstCpu;
extern __thread int tRedfstPrevId;
extern int gFreq[2];
extern int gRedfstCurrentId[REDFST_MAX_THREADS];
extern int gRedfstCurrentFreq[REDFST_MAX_THREADS];
extern redfst_region_t gRedfstRegion[REDFST_MAX_THREADS][REDFST_MAX_REGIONS];
extern uint64_t gRedfstSlowRegions;
extern uint64_t gRedfstFastRegions;
extern uint64_t __redfstTime0;
extern int gRedfstThreadCount;
#define FREQ_HIGH gFreq[1]
#define FREQ_LOW  gFreq[0]

#endif

#ifdef REDFST_TRACE
extern FILE * __redfstTraceFd;
extern uint64_t __redfstTraceT0;
#endif

static inline redfst_region_t * region_get(int cpu, int id){
	return &gRedfstRegion[cpu][id];
}

#define REDFST_ID_IS_FAST(id) (gRedfstFastRegions&(1LL<<(id)))
#define REDFST_ID_IS_SLOW(id) (gRedfstSlowRegions&(1LL<<(id)))
static int redfst_freq_get(redfst_region_t *m, int id){
	if(REDFST_ID_IS_FAST(id))
		return FREQ_HIGH;
	else if(REDFST_ID_IS_SLOW(id))
		return FREQ_LOW;
	return 0;
}
#undef REDFST_IS_FAST
#undef REDFST_IS_SLOW


static void redfst_region_impl(int id, int cpu){
	redfst_region_t *m;
	uint64_t timeNow;
	int freq;
//	if(tRedfstCpu!=0&&tRedfstCpu!=1) return; // might make sense if freq is per cpu
	m = region_get(cpu,tRedfstPrevId);
	++m->next[id];

	// don't waste too many cycles when called inside loops
	if(likely(id == tRedfstPrevId)){
		return;
	}
	tRedfstPrevId = id;
	timeNow = __redfst_time_now();
	m->time += timeNow - m->timeStarted;
	region_get(cpu,id)->timeStarted = timeNow;

	// for the execution monitor
	gRedfstCurrentId[cpu] = id;

	redfst_perf_read(cpu, &m->perf);

#ifdef REDFST_TRACE
	freq = redfst_freq_get(region_get(cpu,id), id);
	fprintf(__redfstTraceFd, "%"PRIu64",%d,%d,%d\n", timeNow - __redfstTraceT0, cpu,
	        id, freq ? freq : gRedfstCurrentFreq[cpu]);
#endif

#ifndef REDFST_FREQ_PER_CORE
	if(REDFST_CPU0 != cpu && REDFST_CPU1 != cpu)
		return;
#endif
#ifndef REDFST_TRACE
	freq = redfst_freq_get(region_get(cpu,id), id);
#endif
	if(unlikely(freq && freq != gRedfstCurrentFreq[cpu])){
		gRedfstCurrentFreq[cpu] = freq;
		cpufreq_set_frequency(cpu, freq);
	}
}

#ifdef REDFST_FUN_IN_H
static inline
#endif
void redfst_region(int id){
	redfst_region_impl(id, tRedfstCpu);
}
#endif
#ifndef REDFST_REGION_ALL_H
#define REDFST_REGION_ALL_H
#ifdef REDFST_OMP
void redfst_region_all(int id);
#endif
#endif
#ifdef __cplusplus
};
#endif
#endif
