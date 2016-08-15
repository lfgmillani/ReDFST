#ifndef REDFST_H
#define REDFST_H
#ifdef __cplusplus
extern "C"{
#endif

typedef struct{
	char **name;
	float *energy;
	double time;
	int count;
}redfst_dev_t;

typedef enum {REDFST_NONE, REDFST_POWERCAP, REDFST_MSR, REDFST_LIKWID} redfst_support_t;

void redfst_init();
void redfst_monitor_set_status(int n);
void redfst_region(int id);
void redfst_region_all(int id);
void redfst_reset(void);
redfst_support_t redfst_support(void);
void __redfst_print(void);
void redfst_get_legacy(double *dst, int cpu);
void redfst_get_all_legacy(double *dst);
redfst_dev_t * redfst_get(redfst_dev_t *dev);
int  redfst_ncpus();
#include <stdio.h>
extern FILE *__redfst_fd;
#define redfst_print() __redfst_print()
#define redfst_printf(...) do{fprintf(__redfst_fd, __VA_ARGS__);__redfst_print();}while(0)
#ifdef __cplusplus
};
#endif
#endif
