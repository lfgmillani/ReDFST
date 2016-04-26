#ifndef CONFIG_H
#define CONFIG_H
/*
	maximum number of code regions.
	as 64bit integers are used with this, it cannot be higher than 64.
*/
#define REDFSTLIB_MAX_REGIONS 64

/*
	maximum number of threads supported
*/
#define REDFSTLIB_MAX_THREADS 32

/*
	polling frequency for the monitor
*/
#define REDFSTLIB_MONITOR_FREQUENCY 100

/*
	if defined, the library will target OpenMP. at the moment that's the only
	option.
*/
#define REDFSTLIB_OMP

/*
	if defined as static, the library will implement some functions in the header
	files. otherwise, the implementations will reside in the lib.
*/
//#define REDFST_STATIC static

/*
	if defined, frequency is decided on a per-core basis. otherwise, its per-cpu.
*/
#define REDFSTLIB_FREQ_PER_CORE
/*
	if REDFSTLIB_FREQ_PER_CORE was not defined, these variables define which cores will
	have their frequencies changed. all other cores will be kept at the low
	frequency.
*/
#define REDFSTLIB_CPU0 0
#define REDFSTLIB_CPU1 1

/* if defined, energy is printed as a table */
#define REDFST_CSV

#endif
