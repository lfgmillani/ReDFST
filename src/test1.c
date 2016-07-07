#include <unistd.h>
#include <cpufreq.h>
#include "config.h"
#ifdef REDFST_FUN_IN_H
#include <redfst-static.h>
#else
#include <redfst-default.h>
#endif

int main(){
	int i;
	for(i=0;i<64;++i){
		redfst_region(i);
		printf("%lu\n", cpufreq_get_freq_kernel(0));
	}
	return 0;
}
