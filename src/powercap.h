#ifndef POWERCAP_H
#define POWERCAP_H
#include "energy.h"
void redfst_powercap_update_one(cpu_t *c);
void redfst_powercap_update();
int redfst_powercap_init();
void redfst_powercap_end();
#endif
