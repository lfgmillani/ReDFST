#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <macros.h>
#include "config.h"
#include "init.h"
#include "global.h"
#include "redfst_omp.h"
#ifdef REDFST_OMP
static void redfst_omp_init_check(){
	// if we're not changing the frequency, threads are free to move around
	if(!gRedfstFreq[1])
		return;
	if(omp_proc_bind_true != omp_get_proc_bind()){
		fprintf(stderr,"You must set OMP_PROC_BIND=TRUE to use libredfst. Aborting.\n");
		exit(1);
	}
}

void redfst_omp_init(){
	int *ok;
	int n;
	int i,j;
	redfst_omp_init_check();
	n = omp_get_max_threads();
	if(n > REDFST_MAX_THREADS){
		fprintf(stderr,"Number of threads should be no greater than REDFST_MAX_THREADS(%d). Aborting.\n",
		        REDFST_MAX_THREADS);
		exit(1);
	}
	ok = malloc(n * sizeof(*ok));
#pragma omp parallel for num_threads(n)
	for(i=0;i<n;++i){
		ok[i] = omp_get_thread_num();
		redfst_thread_init(ok[i]);
	}
	i=0;
	while(i<n){
		j = ok[i];
		if(0 > j){
			++i;
		}else if(-1 == ok[j]){
			fprintf(stderr, "redfst_init failed. Not all threads initialized.\n");
			exit(1);
		}else if(i == j){
			ok[i] = -1;
			++i;
		}else{
			ok[i] = ok[j];
			ok[j] = -1;
		}
	}
	free(ok);
}
#endif
