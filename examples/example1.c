#include <stdlib.h>
#include <unistd.h>
#include <redfst.h>

int main(int argc, char **argv){
	int n;
	int x;
	int i;
	int j;

	n = argc > 1 ? atoi(argv[1]) : 1024*1024;

	redfst_cpus(0); // only measure the energy consumption of cpu 0. without this call, all cpus are used.

	redfst_reset(); // zero energy counters to skip initialization code
#pragma omp parallel for reduction(+:x)
	for(i = 0; i < n; ++i){
		for(j = 0; j < n; ++j){
			x ^= i^j;
		}
	}
	redfst_print(); // print energy counters

	redfst_reset(); // reset energy counters so we can measure something else
	usleep(3000000);
	redfst_print(); // print energy counters

	return x;
}
