#ifndef REDFST_H
#define REDFST_H
#ifdef __cplusplus
extern "C"{
#endif
void redfst_monitor_set_status(int n);
void redfst_region(int id);
void redfst_region_all(int id);
void redfst_reset(void);
void redfst_print(void);
void redfst_get(double *dst, int cpu);
void redfst_get_all(double *dst);
#ifdef __cplusplus
};
#endif
#endif
