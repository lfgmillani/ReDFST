#ifndef UTIL_H
#define UTIL_H
#include <stdint.h>
#ifdef REDFST_STATIC
#include "redfst/util.c"
#else
uint64_t time_now();
#endif
#endif
