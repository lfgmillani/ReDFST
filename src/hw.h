#ifndef HW_H
#define HW_H
extern int *__redfstHwSocketToLogicalCore;
extern int __redfstHwNcpus;
void __redfst_hw_init();
#define socket_to_logical_core(x) __redfstHwSocketToLogicalCore[x]
#endif
