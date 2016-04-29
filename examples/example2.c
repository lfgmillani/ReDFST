#include <stdlib.h>
#include <unistd.h>
#include <redfst.h>

int main(int argc, char **argv){
	int n;
	int x;
	int i;
	int j;

	n = argc > 1 ? atoi(argv[1]) : 1024*1024;

	redfst_reset(); // clear the energy counters to skip initialization
#pragma omp parallel for reduction(+:x)
	for(i = 0; i < n; ++i){
		redfst_region(1); // set the code region of the current thread to 1
		for(j = 0; j < n; ++j){
			x ^= i^j;
		}
	}
	redfst_print(); // print the energy counters

	redfst_reset(); // reset the energy counters
	redfst_region_all(2); // set the code region of all threads to 2
	usleep(3000000);
	redfst_print(); // print the energy counters

	return x;
}

