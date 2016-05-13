#include "region.h"
#include "config.h"
#ifdef REDFST_OMP
#include <omp.h>
void redfst_region_all(int id){
	int nthreads;
	int i;
	nthreads = omp_get_max_threads();
#pragma omp parallel for num_threads(nthreads)
	for(i=0; i < nthreads; ++i)
		redfst_region(id);
}
#endif
