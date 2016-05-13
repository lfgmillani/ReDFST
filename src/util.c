#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#include <time.h>
#include <stdint.h>
#include "redfst/util.h"

#ifdef REDFST_STATIC
static
#endif
uint64_t __redfst_time_now(){
	struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC, &spec);
	return ((uint64_t)1e9) * spec.tv_sec + spec.tv_nsec;
}
