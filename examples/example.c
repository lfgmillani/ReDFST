#include <unistd.h>
#include "redfst.h"

int main(){
	int n = 1024*1024*1024;
	int i;
	int j;
	int x;
	redfst_reset(); // zero redfst counters - useful to skip initialization stuff
	redfst_region_all(1); // enter region 1
#pragma omp parallel for reduction(+:x)
	for(i=0;i<n/10;++i)
		for(j=0;j<10;++j)
			x=x*i+j; // nonsense
	redfst_print(); // print counters
	redfst_reset(); // zero counters
	redfst_region_all(2); // enter region 2
	usleep(3000000);
	redfst_print(); // print counters
	return x;
}
