#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <cpufreq.h>
#include <macros.h>
#include "global.h"
#include "perf.h"
#include "monitor.h"
#include "region.h"
#include "util.h"
#include "profile.h"
#include "redfst.h"
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

static void env2uint64(uint64_t *dst, const char *envname){
	char *s;
	uint64_t n;
	char c;
	char neg=0;
	s = getenv(envname);
	if(!s||!*s)
		return;
	if('-'==*s){
		neg = 1;
		++s;
	}
	for(c=*s,n=0; c; c=*(++s)){
		if(c < '0' || c>'9' || n>10*n){
			fprintf(stderr,"invalid value for %s: \"%s\"\n",envname,s);
			exit(1);
		}
		n = 10*n + c - '0';
	}
	if(neg)
		n = -n;
	*dst = n;
	return;
}

static void env2f(float *dst, const char *envname){
	char *s,*r;
	float x;
	s = getenv(envname);
	if(!s||!*s)
		return;
	x = strtof(s,&r);
	if(*r){
		fprintf(stderr,"invalid value for %s: \"%s\"\n",envname,s);
		exit(1);
	}
	*dst = x;
}


static void from_env(){
	const char *s;

	FREQ_LOW = FREQ_HIGH = 0;

	env2i(&FREQ_HIGH,"REDFST_HIGH");
	env2i(&FREQ_LOW,"REDFST_LOW");
	env2uint64(&gRedfstSlowRegions,"REDFST_SLOWREGIONS");
	env2uint64(&gRedfstFastRegions,"REDFST_FASTREGIONS");
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
	redfst_perf_init();
	redfst_omp_init();
	redfst_profile_load();
	if(gCfg.monitor)
		redfst_monitor_init();
}

void redfst_thread_init(int cpu){
/* must be called by every thread at the beginning of execution exactly once */
  uint64_t timeNow;
  timeNow = time_now();
  tRedfstCpu = cpu;
  tRedfstPrevId = 0;
#ifdef REDFSTLIB_FREQ_PER_CORE
  gRedfstCurrentFreq[cpu] = FREQ_HIGH;
	cpufreq_set_frequency(cpu, FREQ_HIGH);
#else
	if(REDFSTLIB_CPU0 == cpu || REDFSTLIB_CPU1 == cpu){
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
