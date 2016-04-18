#ifndef REDFST_H
#define REDFST_H
#ifdef __cplusplus
extern "C"{
#endif
#include "libredfst_config.h"
void redfst_monitor_set_status(int n);

void redfst_region(int id);
void redfst_region_all(int id);
void redfst_reset(void);
void redfst_print(void);
void redfst_exit(void);

#ifdef __cplusplus
};
#endif
#endif
