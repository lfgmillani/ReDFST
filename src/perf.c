#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <sched.h>
#include <macros.h>
#include "config.h"
#include "perf.h"
#include "monitor.h"

//#define CACHE_IS_L1 // not implemented
#ifndef CACHE_IS_L1
#define CACHE_IS_LL
#endif

static int const gRedfstPerfEventCode[REDFST_PERF_NUM_EVENTS] = {
#ifdef CACHE_IS_LL
	PERF_COUNT_HW_CACHE_REFERENCES,
	PERF_COUNT_HW_CACHE_MISSES,
#else // will break event_init if you're not careful
			// also this is not finished so right now it does not works
				(PERF_COUNT_HW_CACHE_L1D) | ((PERF_COUNT_HW_CACHE_OP_READ)<<8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS<<16),
				(PERF_COUNT_HW_CACHE_L1D) | ((PERF_COUNT_HW_CACHE_OP_READ)<<8) | (PERF_COUNT_HW_CACHE_RESULT_MISS  <<16),
#endif
//	PERF_COUNT_HW_STALLED_CYCLES_FRONTEND,
	PERF_COUNT_HW_CPU_CYCLES,
	PERF_COUNT_HW_INSTRUCTIONS,
};


// http://web.eece.maine.edu/~vweaver/projects/perf_events/perf_event_open.html
struct read_format{
	uint64_t nr;            /* The number of events */
//	uint64_t time_enabled;  /* if PERF_FORMAT_TOTAL_TIME_ENABLED */
//	uint64_t time_running;  /* if PERF_FORMAT_TOTAL_TIME_RUNNING */
	struct {
		uint64_t value;     /* The value of the event */
//		uint64_t id;        /* if PERF_FORMAT_ID */
	} values[REDFST_PERF_NUM_EVENTS]; // N events - WILL BREAK IF WE MONITOR MORE AND DON'T CHANGE THIS
};

static int gFd[REDFST_MAX_THREADS] = {0};

static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags){
	return syscall(__NR_perf_event_open, hw_event, pid, cpu,
	               group_fd, flags);
}

static void
event_init(struct perf_event_attr *p, uint64_t event){
	memset(p,0,sizeof(*p));
#ifdef CACHE_IS_LL // crappy hack
	if(event == gRedfstPerfEventCode[0] || event == gRedfstPerfEventCode[1])
		p->type = PERF_TYPE_HW_CACHE;
#else
	p->type = PERF_TYPE_HARDWARE;
#endif
	p->size = sizeof(*p);
	p->read_format = PERF_FORMAT_GROUP;
	p->config = event;
//	p->exclude_kernel = 1;
}

void redfst_perf_read(int cpu, redfst_perf_t *p){
	struct read_format r;
	int i;
	if(REDFST_UNLIKELY(!gFd[cpu])){
		for(i=0;i<REDFST_PERF_NUM_EVENTS;++i)
			p->events[i] = 0;
	}else{
		read(gFd[cpu], &r, sizeof(r));
		for(i=0;i<REDFST_PERF_NUM_EVENTS;++i)
			p->events[i] = r.values[i].value;
	}
}

float redfst_perf_get_miss_rate(redfst_perf_t *p){
	return ((float)p->events[1])/p->events[0];
}

void redfst_perf_init(){
	memset(gFd,-1,sizeof(gFd));
}

void redfst_perf_init_worker(){
	struct perf_event_attr events[REDFST_PERF_NUM_EVENTS];
	int fd[REDFST_PERF_NUM_EVENTS];
	int cpu = sched_getcpu();
	int i;
	for(i=0;i<REDFST_PERF_NUM_EVENTS;++i)
		event_init(events+i, gRedfstPerfEventCode[i]);
	events[0].disabled = 1;
	fd[0] = -1;
	for(i=0;i<REDFST_PERF_NUM_EVENTS;++i){
		if(0 > (fd[i] = perf_event_open(events+i, 0, cpu, fd[0], 0))){
			fprintf(stderr, "failed to open perf event\n");
			exit(1);
		}
		ioctl(fd[i], PERF_EVENT_IOC_RESET, 0);
	}
	ioctl(fd[0], PERF_EVENT_IOC_ENABLE, 0);
	gFd[cpu] = fd[0];
}

void redfst_perf_shutdown(){
	int i;
	for(i=0;i<REDFST_LEN(gFd);++i)
		if(gFd[i] > 0)
			close(gFd[i]);
}

