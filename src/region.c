#ifndef REDFST_REGION_C
#define REDFST_REGION_C
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <cpufreq.h>
#include "redfst/macros.h"
#include "redfst/region.h"
#include "redfst/perf.h"
#include "redfst/util.h"
#include "redfst/global.h"
#include "redfst/config.h"

#ifdef REDFST_TRACE
extern FILE * __redfstTraceFd;
extern uint64_t __redfstTraceT0;
#endif

static inline redfst_region_t * region_get(int cpu, int id){
	return &gRedfstRegion[cpu][id];
}

#define id_is_fast(id) (gRedfstFastRegions&(1LL<<(id)))
#define id_is_slow(id) (gRedfstSlowRegions&(1LL<<(id)))
static int redfst_freq_get(redfst_region_t *m, int id){
	if(id_is_fast(id))
		return FREQ_HIGH;
	else if(id_is_slow(id))
		return FREQ_LOW;
	return 0;
}


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
	timeNow = time_now();
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
	freq = redfst_freq_get(region_get(cpu,id), id);
	if(unlikely(freq && freq != gRedfstCurrentFreq[cpu])){
		gRedfstCurrentFreq[cpu] = freq;
		cpufreq_set_frequency(cpu, freq);
	}
}

#ifdef REDFST_STATIC
static
#endif
void redfst_region(int id){
	redfst_region_impl(id, tRedfstCpu);
}

#ifdef REDFST_STATIC
static
#endif
void redfst_region_final(){
	int cpu;
	for(cpu=0; cpu < gRedfstThreadCount;++cpu){
		redfst_region_impl(REDFST_MAX_REGIONS-1, cpu);
	}
}

#ifdef REDFST_OMP
#include <omp.h>
#ifdef REDFST_STATIC
static
#endif
void redfst_region_all(int id){
	int nthreads;
	int i;
	nthreads = omp_get_max_threads();
#pragma omp parallel for num_threads(nthreads)
	for(i=0; i < nthreads; ++i)
		redfst_region(id);
}
#endif
#endif
