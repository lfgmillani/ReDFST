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
	meximum number of different cpu frequencies
*/
#define REDFST_MAX_NFREQS 32

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
