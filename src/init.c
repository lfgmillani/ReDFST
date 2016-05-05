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

typedef struct{
	int monitor;
}cfg_t;
cfg_t gCfg = {0,};

static void env2i(int *dst, const char *envname){
	char *s,*r;
	long x;
	s = getenv(envname);            
	if(!s||!*s)
		return;
	x = strtol(s,&r,10); 
	if(*r || x > INT_MAX){
		fprintf(stderr,"invalid value for %s: \"%s\"\n",envname,s);
		exit(1);
	}
	*dst = (int)x;
}

static int get_freq(int isHigh){
	char s[128];
	FILE *f;
	sprintf(s, "/sys/devices/system/cpu/cpufreq/policy0/scaling_%s_freq", isHigh?"max":"min");
	f = fopen(s, "rt");
	if(!f){
		sprintf(s, "/sys/devices/system/cpu/cpu0/cpufreq/scaling_%s_freq", isHigh?"max":"min");
		f = fopen(s, "rt");
	}
	if(!f){
		fprintf(stderr,"ReDFST: Failed to get %s cpu frequency. Please specify it with REDFST_%s\n",
		        isHigh?"max":"min", isHigh?"HIGH":"LOW");
		exit(1);
	}
	fread(s, 1, sizeof(s), f);
	fclose(f);
	return atoi(s);
}

static uint64_t list_to_bitmask(const char *envname){
	char *s;
	uint64_t mask;
	uint64_t n;
	char c;
	char st = 0;
	s = getenv(envname);
	if(!s||!*s)
		return 0;
	mask = 0;
	st = 0;
	do{
		c=*s++;
		switch(st){
		case 0:
			if(c >= '0' && c <= '9'){
				n = c - '0';
				++st;
			}else{
				fprintf(stderr, "invalid value for %s: \"%s\"\n", envname, s);
				exit(1);
			}
			break;
		case 1:
			if(c >= '0' && c <= '9'){
				n = 10 * n + c - '0';
				if(n > REDFST_MAX_REGIONS-1){
					fprintf(stderr, "invalid value for %s: the maximum region number is %d\n", envname, REDFST_MAX_REGIONS-1);
					exit(1);
				}
			}else if(c==','||!c){
				n = 1ULL << n;
				if(mask & n){
					fprintf(stderr, "invalid value for %s: repeated regions found\n", envname);
					exit(1);
				}
				mask |= n;
				--st;
			}else{
				fprintf(stderr, "invalid value for %s: \"%s\"\n", envname, s);
				exit(1);
			}
		}
	}while(c);
	return mask;
}

static void from_env(){
	const char *s;

	FREQ_LOW = FREQ_HIGH = 0;

	env2i(&FREQ_HIGH,"REDFST_HIGH");
	env2i(&FREQ_LOW,"REDFST_LOW");
	if(!FREQ_HIGH)
		FREQ_HIGH = get_freq(1);
	if(!FREQ_LOW)
		FREQ_LOW = get_freq(0);

	gRedfstSlowRegions = list_to_bitmask("REDFST_SLOWREGIONS");
	gRedfstFastRegions = list_to_bitmask("REDFST_FASTREGIONS");
	if(gRedfstSlowRegions&gRedfstFastRegions){
		fprintf(stderr, "The same region cannot be slow and fast. Verify REDFST_SLOWREGIONS and REDFST_FASTREGIONS.\n");
	}
	if((s=getenv("REDFST_MONITOR"))){
		if(*s&&*s!='0'&&*s!='F'&&*s!='f'){
			gCfg.monitor = 1;
		}
	}
}

void __attribute__((constructor))
redfst_init(){
/* must be called exactly once at the beginning of execution and before redfst_thread_init */	
	memset(gRedfstCurrentId,0,sizeof(gRedfstCurrentId));
	memset(gRedfstCurrentFreq,0,sizeof(gRedfstCurrentFreq));
	memset(gRedfstRegion,0,sizeof(gRedfstRegion));
	gRedfstSlowRegions = 0;
	gRedfstFastRegions = 0;
	gRedfstThreadCount = 0;
	gFreq[0] = gFreq[1] = 0;
	from_env();
	redfst_energy_init();
	redfst_perf_init();
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
  timeNow = time_now();
  tRedfstCpu = cpu;
  tRedfstPrevId = 0;
#ifdef REDFST_FREQ_PER_CORE
  gRedfstCurrentFreq[cpu] = FREQ_HIGH;
	cpufreq_set_frequency(cpu, FREQ_HIGH);
#else
	if(REDFST_CPU0 == cpu || REDFST_CPU1 == cpu){
  	gRedfstCurrentFreq[cpu] = FREQ_HIGH;
		cpufreq_set_frequency(cpu, FREQ_HIGH);
	}else{
  	gRedfstCurrentFreq[cpu] = FREQ_LOW;
		cpufreq_set_frequency(cpu, FREQ_LOW);
	}
#endif
	gRedfstRegion[cpu][0].timeStarted = timeNow;
	__sync_fetch_and_add(&gRedfstThreadCount, 1);
  redfst_perf_init_worker();
}

void __attribute__((destructor))
redfst_close(){
/* must be called at the end of execution */
	redfst_region_final();

	if(gCfg.monitor){
		redfst_monitor_end();
		redfst_monitor_show();
	}

	redfst_profile_save();
	redfst_profile_graph_save();
}
