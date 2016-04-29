#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#include <time.h>
#include <stdint.h>
#include "redfst/util.h"

// hack to work with -std=C99
#if 0
//#if __STDC_VERSION__ == 199901L
#define CLOCK_MONOTONIC 1
struct timespec{
	time_t tv_sec;
	long   tv_nsec
};
#endif

#ifdef REDFST_STATIC
static
#endif
uint64_t time_now(){
	struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC, &spec);
	return ((uint64_t)1e9) * spec.tv_sec + spec.tv_nsec;
}
