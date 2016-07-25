#ifndef REDFST_H
#define REDFST_H
#ifdef __cplusplus
extern "C"{
#endif
void redfst_init();
void redfst_monitor_set_status(int n);
void redfst_region(int id);
void redfst_region_all(int id);
void redfst_reset(void);
void __redfst_print(void);
void redfst_get(double *dst, int cpu);
void redfst_get_all(double *dst);
int  redfst_ncpus();
#include <stdio.h>
extern FILE *__redfst_fd;
#define redfst_print() __redfst_print()
#define redfst_printf(...) do{fprintf(__redfst_fd, __VA_ARGS__);__redfst_print();}while(0)
#ifdef __cplusplus
};
#endif
#endif
