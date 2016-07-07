#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <unistd.h>
#include <cpufreq.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <macros.h>
#include "global.h"
#include "perf.h"
#include "monitor.h"
#include "region.h"
#include "util.h"
#include "profile.h"
#include "energy.h"
#include "redfst_omp.h"
#include "trace.h"

typedef struct{
	int monitor;
}cfg_t;
cfg_t gCfg = {0,};

static char gInitStatus = 0;

static int * str_list_to_int_array(const char *s){
/*
	Given a list of natural numbers 's' in the format '1,2-3', where ','
	separates two numbers and 'a-b' means all numbers from 'a' to 'b'
	(inclusive; a<=b), return a new array with all those numbers in the order they
	appear. The last element of the array is -1. Return 0 on empty list or error.
*/
	int *a;
	int alloc;
	int count;
	int l, r;
	char c;
	char st;

	if(!s||!*s)
		return 0;

	count = 0;
	alloc = 32;
	a = malloc(alloc*sizeof(*a));

	st = 0;
	do{
		c = *s++;
		switch(st){
		case 0:
			if(c >= '0' && c <= '9'){
				l = c - '0';
				++st;
			}else{
				free(a);
				return 0;
			}
			break;
		case 1:
			if(c >= '0' && c <= '9'){
				l = 10 * l + c - '0';
			}else if(','==c || !c){
				if(count == alloc){
					alloc += 32;
					a = realloc(a, alloc * sizeof(*a));
				}
				a[count++] = l;
				--st;
			}else if('-'==c){
				++st;
			}else{
				free(a);
				return 0;
			}
			break;
		case 2:
			if(c >= '0' && c <= '9'){
				r = c - '0';
				++st;
			}else{
				free(a);
				return 0;
			}
			break;
		case 3:
			if(c >= '0' && c <= '9'){
				r = 10 * r + c - '0';
			}else if(','==c || !c){
				if(l > r){
					free(a);
					return 0;
				}
				do{
					if(count == alloc){
						alloc += 32;
						a = realloc(a, alloc*sizeof(*a));
					}
					a[count++] = l;
				}while(++l<=r);
				st = 0;
			}else{
				free(a);
				return 0;
			}
			break;
		}
	}while(c);
	if(count == alloc)
		a = realloc(a, ++alloc*sizeof(*a));
	a[count++] = -1;
	return a;
}


static void from_env_region_freq_mapping(){
	extern char **environ;
	char **e,*s;
	int *a;
	int i;
	int nfreq;
	int freq;
	uint8_t id;
	REDFST_CASSERT(REDFST_MAX_NFREQS >= 0 && REDFST_MAX_NFREQS <= 0xff);
	gRedfstMinFreq = INT_MAX;
	gRedfstMaxFreq = 0;

	nfreq = 1; // start at 1 as gRedfstFreq[0] is 0 (that is, no freq change)
	for(e=environ; *e; ++e){
		if(strncmp("REDFST_",*e,7))
			continue;
		freq = 0;
		for(s = *e + 7; *s && *s!='='; ++s){
			if('0' > *s || '9' < *s)
				break;
			freq = 10 * freq + *s - '0';
		}
		if('=' != *s++)
			continue;
		for(id = 0; id < nfreq && gRedfstFreq[id] != freq; ++id)
			;
		if(id == nfreq){ // new freq
			if(nfreq == REDFST_MAX_NFREQS - 1){
				fprintf(stderr, "REDFST: number of different frequencies used is above the limit of %d\n", REDFST_MAX_NFREQS);
				exit(1);
			}
			gRedfstFreq[nfreq++] = freq;
			if(freq && freq < gRedfstMinFreq)
				gRedfstMinFreq = freq;
			if(freq && freq > gRedfstMaxFreq)
				gRedfstMaxFreq = freq;
		}
		if(s&&*s){
			a = str_list_to_int_array(s);
			if(!a){
				fprintf(stderr, "REDFST: Failed to parse %s\n", *e);
				exit(1);
			}
			for(i=0; a[i] >= 0; ++i){
				if(gRedfstRegionFreqId[a[i]]){
					fprintf(stderr, "REDFST: Region %d has two frequencies: %d and %d\n", a[i],
					        gRedfstFreq[id], gRedfstFreq[gRedfstRegionFreqId[a[i]]]);
					exit(1);
				}
				gRedfstRegionFreqId[a[i]] = id;
			}
			free(a);
		}
	}
}

static void from_env(){
	char *s;
	from_env_region_freq_mapping();

	if((s=getenv("REDFST_MONITOR"))){
		if(*s&&*s!='0'&&*s!='F'&&*s!='f'){
			gCfg.monitor = 1;
		}
	}
}

void __attribute__((constructor))
redfst_init(){
/* must be called exactly once at the beginning of execution and before redfst_thread_init */	
	if(1 == gInitStatus)
		return;
	gInitStatus = 1;
	memset(gRedfstCurrentId,0,sizeof(gRedfstCurrentId));
	memset(gRedfstCurrentFreq,0,sizeof(gRedfstCurrentFreq));
	memset(gRedfstRegion,0,sizeof(gRedfstRegion));
	memset(gRedfstRegionFreqId,0,sizeof(gRedfstRegionFreqId));
	memset(gRedfstFreq,0,sizeof(gRedfstFreq));
	gRedfstMinFreq = 0;
	gRedfstMaxFreq = 0;
	gRedfstThreadCount = 0;
	from_env();
	redfst_energy_init();
	redfst_perf_init();
#ifdef REDFST_TRACE
	redfst_trace_init();
#endif
#ifdef REDFST_OMP
	redfst_omp_init();
#endif
	redfst_profile_load();
	if(gCfg.monitor)
		redfst_monitor_init();
}

void redfst_thread_init(int cpu){
/* must be called by every thread at the beginning of execution exactly once */
	uint64_t timeNow;
#ifndef REDFST_OMP
	cpu_set_t set;

	CPU_ZERO(&set);
	CPU_SET(cpu, &set);
	sched_setaffinity(syscall(SYS_gettid), sizeof(set), &set);
#endif
	timeNow = __redfst_time_now();
	tRedfstCpu = cpu;
	tRedfstPrevId = 0;
#ifdef REDFST_FREQ_PER_CORE
	gRedfstCurrentFreq[cpu] = gRedfstMaxFreq;
	cpufreq_set_frequency(cpu, gRedfstMaxFreq);
#else
	if(REDFST_CPU0 == cpu || REDFST_CPU1 == cpu){
		gRedfstCurrentFreq[cpu] = gRedfstMaxFreq;
		cpufreq_set_frequency(cpu, gRedfstMaxFreq);
	}else{
		gRedfstCurrentFreq[cpu] = gRedfstMinFreq;
		cpufreq_set_frequency(cpu, gRedfstMinFreq);
	}
#endif
	gRedfstRegion[cpu][0].timeStarted = timeNow;
	__sync_fetch_and_add(&gRedfstThreadCount, 1);
	redfst_perf_init_worker();
}

#ifdef REDFST_OMP
#include <omp.h>
static void redfst_region_final(){
	int nthreads;
	int i;
	nthreads = omp_get_max_threads();
#pragma omp parallel for num_threads(nthreads)
	for(i=0; i < nthreads; ++i)
		redfst_region(REDFST_MAX_REGIONS-1);
}
#endif

void __attribute__((destructor))
redfst_close(){
/* must be called at the end of execution */
	if(1 != gInitStatus)
		return;
	gInitStatus = 2;
	redfst_region_final();

	if(gCfg.monitor){
		redfst_monitor_end();
		redfst_monitor_show();
	}

	redfst_profile_save();
	redfst_profile_graph_save();
}
