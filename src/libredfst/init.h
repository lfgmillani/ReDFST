#ifndef INIT_H
#define INIT_H
void __attribute__((constructor))
redfst_init();
void redfst_thread_init(int cpu);
void __attribute__((destructor))
redfst_close();
#endif

