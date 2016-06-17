#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <macros.h>
#include "perf.h"
#include "global.h"
#include "config.h"
#include "monitor.h"

#define REDFST_MONITOR_PERIOD (1000000 / REDFST_MONITOR_FREQUENCY)

typedef struct{
	redfst_perf_t events[REDFST_MAX_THREADS];
	uint64_t freq;
	uint8_t region[REDFST_MAX_REGIONS];
	int status;
}monitor_t;

static char * gRedfstMonitorEventName[REDFST_PERF_NUM_EVENTS] = {
	"refs",
	"misses",
//	"stalled",
	"cycles",
	"instr",
};
static monitor_t *gMon = (void*)0xdeadbabe;
static int gI = 0;
static int gLen = 0;
static char gLoop = 1;
static int gRedfstMonitorStatus = 0;



void redfst_monitor_set_status(int n){
	gRedfstMonitorStatus = n;
}

static void freq_encode(uint64_t *dst, int *src){
	uint64_t n = 0;
	int i;
	REDFST_CASSERT(REDFST_LEN(gFreq)==2);
	REDFST_CASSERT(REDFST_MAX_THREADS<=64);
	for(i=0;i<REDFST_MAX_THREADS;++i){
		n |= (src[i] == FREQ_HIGH) << i;
	}
	*dst = n;
}

static void freq_decode(int *dst, uint64_t src){
	int i;
	for(i=0;i<REDFST_MAX_THREADS;++i){
		dst[i] = (src&(1LL<<i)) ? FREQ_HIGH : FREQ_LOW;
	}
}

static inline void monitor_poll(){
	int i;
	if(REDFST_UNLIKELY(gI == gLen)){
		gLen = gLen << 1;
		gMon = realloc(gMon, gLen * sizeof(*gMon));
	}
	gMon[gI].status = gRedfstMonitorStatus;
	for(i=0;i<gRedfstThreadCount;++i){
		redfst_perf_read(i, gMon[gI].events+i);
		gMon[gI].region[i] = gRedfstCurrentId[i];
	}
	freq_encode(&gMon[gI].freq, gRedfstCurrentFreq);
	++gI;
}

void * redfst_monitor_loop(void *dummy){
	while(REDFST_LIKELY(gLoop)){
		monitor_poll();
		usleep(REDFST_MONITOR_PERIOD);
	}
	return 0;
}

void redfst_monitor_init(){
	pthread_t tid;
	gLen = 512 * REDFST_MONITOR_FREQUENCY;
	gI = 0;
	gLoop = 1;
	gMon = malloc(gLen * sizeof(*gMon));
	if((REDFST_UNLIKELY(pthread_create(&tid,0,redfst_monitor_loop,0)))){
		fprintf(stderr,"Failed to create monitor thread\n");
		exit(1);
	}
}

void redfst_monitor_end(){
	gLoop = 0;
}

void redfst_monitor_show(){
	int freq[REDFST_MAX_THREADS];
	FILE *f;
	monitor_t *m;
	int i,j,k;

	f = fopen("redfst-monitor.csv","wt");
	if(!f){
		return;
	}
	// header
	fprintf(f,"time;status");
	for(j=0; j<gRedfstThreadCount; ++j)
		fprintf(f,";freq%d",j);
	for(j=0; j<gRedfstThreadCount; ++j)
		fprintf(f,";region%d",j);
	for(j=0; j<gRedfstThreadCount; ++j)
		for(k=0; k < REDFST_PERF_NUM_EVENTS; ++k)
			fprintf(f,";%s%d",gRedfstMonitorEventName[k],j);

	for(i=0; i < gI; ++i){
		m = gMon + i;
		fprintf(f,"\n");
		fprintf(f,"%d", i * REDFST_MONITOR_PERIOD);
		fprintf(f,";%d",m->status);
		freq_decode(freq, m->freq);
		for(j = 0; j < gRedfstThreadCount; ++j)
			fprintf(f,";%d",freq[j]);
		for(j = 0; j < gRedfstThreadCount; ++j)
			fprintf(f,";%d",m->region[j]);
		for(j=0; j<gRedfstThreadCount; ++j)
			for(k=0; k < REDFST_PERF_NUM_EVENTS; ++k)
				fprintf(f,";%"PRIu64,m->events[j].events[k]);
	}
	fclose(f);
}
