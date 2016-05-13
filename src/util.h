#ifndef UTIL_H
#define UTIL_H
#include <stdint.h>
#include "config.h"
#ifdef REDFST_FUN_IN_H
#include "util.c"
#else
uint64_t __redfst_time_now();
#endif
#endif
