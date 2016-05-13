#ifndef REDFST_H
#define REDFST_H
#ifdef __cplusplus
extern "C"{
#endif
void redfst_monitor_set_status(int n);
void redfst_region(int id);
void redfst_region_all(int id);
void redfst_reset(void);
void __redfst_print(void);
void redfst_get(double *dst, int cpu);
void redfst_get_all(double *dst);
/*
	allow redfst_print to be called with no arguments or with n arguments.
	with no arguments, __redfst_print is called.
	otherwise, the arguments are printed to the same stream __redfst_print uses
	and then __redfst_print is called.
*/
#include <stdio.h>
extern FILE *__redfst_fd;
#define REDFST_IS_EMPTY(...) REDFST_IS_EMPTY_(, ##__VA_ARGS__, 0,0,0,0,0,0,0,0,0,0,1)
#define REDFST_IS_EMPTY_(_0,_1,_2,_3,_4,_5,_6,_7,_8_,_9,_10,N,...) N
#define redfst_print(...) redfst_print_(REDFST_IS_EMPTY(__VA_ARGS__),__VA_ARGS__)
#define redfst_print_(empty,...) redfst_print__(empty, __VA_ARGS__)
#define redfst_print__(empty,...) redfst_print ## empty (__VA_ARGS__)
#define redfst_print1(...) __redfst_print()
#define redfst_print0(...) do{fprintf(__redfst_fd, __VA_ARGS__);__redfst_print();}while(0)

#ifdef __cplusplus
};
#endif
#endif
