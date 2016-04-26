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
