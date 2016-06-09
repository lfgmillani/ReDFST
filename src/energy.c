#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include "config.h"
#ifdef REDFST_FUN_IN_H
#include "redfst-static.h"
#else
#include "redfst-default.h"
#endif
#include "util.h"
#include "msr.h"
#include "likwid.h"
#include "macros.h"
#include "control.h"
#include "hw.h"
#include "energy.h"

#define LOCK(x) __sync_val_compare_and_swap(&(x), 0, 1) // return 0 if it manages to obtain the __redfstMutex
#define UNLOCK(x) do{(x)=0;}while(0)

void (*__redfst_energy_update)();
void (*__redfst_energy_update_one)(cpu_t *c);
int  (*__redfst_energy_init)();
void (*__redfst_energy_end)();

static void dummy(){}
static void dummy_one(cpu_t *c){}

int redfstEnergySupport = 0; // if 0 reading energy is not supported
int __redfstNcpus;
cpu_t *__redfstCpu;
cpu_t **gCpuId2Cpu = 0;
volatile int __redfstMutex;
FILE *__redfst_fd;

#ifdef REDFST_FUN_IN_H
static inline
#endif
int redfst_support(){
	return redfstEnergySupport;
}

#ifdef REDFST_FUN_IN_H
static inline
#endif
int redfst_ncpus(){
	return __redfstNcpus;
}

static void safe_force_update(){
	while(REDFST_UNLIKELY(LOCK(__redfstMutex)))
		;
	__redfst_energy_update();
	UNLOCK(__redfstMutex);
}
#undef LOCK
#undef UNLOCK


static void * redfst_energy_loop(void *a){
	while(1){
		usleep(50000000);
		safe_force_update();
	}
	return 0;
}


static FILE * redfst_energy_get_fd(){
/*
	this function must be called before any files are opened (unless they're closed)
*/
	FILE *fd;
	int i;
	if(getenv("REDFST_OUT")){
		fd = fopen(getenv("REDFST_OUT"),"a");
		if(!fd){
			fprintf(stderr, "ReDFST: Failed to open output file \"%s\"\n", getenv("REDFST_OUT"));
			exit(1);
		}
		return fd;
	}
	for(i=3; i>=1; --i){
		if(0 <= fcntl(i, F_GETFL)){
			switch(i){
				case 3: return fdopen(i, "w");
				case 2: return stderr;
				case 1: return stdout;
			}
		}
	}
	return stderr;
}

static int cmpid(const void *a, const void *b){
	return ((cpu_t*)a)->id - ((cpu_t*)b)->id;
}

static void get_all_cpus(){
	int i;
	__redfstCpu = calloc(__redfstHwNcpus, sizeof(*__redfstCpu));
	for(i=0; i<__redfstHwNcpus; ++i)
		__redfstCpu[i].id = i;
	__redfstNcpus = __redfstHwNcpus;
}

static void get_default_cpus(){
	char *s;
	cpu_t *cpu;
	int alloc;
	int ncpus;
	int a,b;
	int *l;
	char c;
	char st;
	s = getenv("REDFST_CPUS");
	if(!s){
		get_all_cpus();
		return;
	}else if(!*s){
		__redfstNcpus = 0;
		__redfstCpu = 0;
		return;
	}
	alloc = 1024;
	ncpus = 0;
	cpu = malloc(alloc * sizeof(*cpu));

	st = 0;
#define APPEND(x) do{\
	if(x >= __redfstHwNcpus){\
		fprintf(__redfst_fd, "Failed to parse REDFST_CPUS: \"%s\" - \"%d\" is not a valid CPU\n", getenv("REDFST_CPUS"), x);\
		exit(1);\
	}\
	if(ncpus+1 == alloc){\
		alloc += 1024;\
		cpu = realloc(cpu, alloc * sizeof(cpu));\
	}\
	memset(cpu+ncpus,0,sizeof(*cpu));\
	cpu[ncpus++].id = (x);\
}while(0)
#define N (c>='0'&&c<='9')

	do{
		c = *s++;
		switch(st){
		case 0:
			if(N){
				a = c - '0';
				++st;
			}else if(c){
				goto FAIL;
			}
			break;
		case 1:
			if(N){
				a = 10 * a + c - '0';
			}else if(','==c || !c){
				APPEND(a);
				--st;
			}else if('-'==c || ':'==c){
				++st;
			}else
				goto FAIL;
			break;
		case 2:
			if(!N)
				goto FAIL;
			b = c - '0';
			++st;
			break;
		case 3:
			if(N){
				b = 10 * b + c - '0';
			}else if(','==c || !c){
				if(a>b){
					a^=b; b^=a; a^=b;
				}
				for(; a<=b; ++a)
					APPEND(a);
				st = 0;
			}else{
				goto FAIL;
			}
			break;
		}
	}while(c);
#undef N
#undef APPEND
	if(ncpus){
		cpu = realloc(cpu, ncpus * sizeof(*cpu));
		l = calloc(ncpus, sizeof(*l));
		for(a=0;a<ncpus;++a){
			if(l[cpu[a].id]++){
				fprintf(__redfst_fd, "No repeated elements allowed in REDFST_CPUS: \"%s\". %d appears twice.\n", getenv("REDFST_CPUS"),cpu[a].id);
				goto FAIL;
			}
		}
		free(l);
	}else{
		free(cpu);
		cpu = 0;
	}
	__redfstNcpus = ncpus;
	__redfstCpu = cpu;
	return;
FAIL:
	fprintf(__redfst_fd, "Failed to parse REDFST_CPUS: \"%s\"\n", getenv("REDFST_CPUS"));
	exit(1);
}

static void create_cpu_mapping(){
	int i;
	int maxCpu;
	if(gCpuId2Cpu)
		free(gCpuId2Cpu);
	for(i=maxCpu=0; i < __redfstNcpus; ++i)
		if(maxCpu < __redfstCpu[i].id)
			maxCpu = __redfstCpu[i].id;
	gCpuId2Cpu = malloc((maxCpu+1) * sizeof(*gCpuId2Cpu));
	for(i=0; i <= maxCpu; ++i)
		gCpuId2Cpu[i] = (void*)0xdeadbabe;
	for(i=0; i < __redfstNcpus; ++i)
		gCpuId2Cpu[__redfstCpu[i].id] = __redfstCpu+i;
}

int redfst_cpus(const int *cpus){
	int cpu;
	int alloc = 1024;
	__redfst_energy_end();
	if(__redfstCpu)
		free(__redfstCpu);
	__redfstNcpus = 0;
	__redfstCpu = malloc(alloc*sizeof(*__redfstCpu));
	for(cpu=*cpus; -1!=*cpus++;){
		if(0 > cpu || __redfstHwNcpus <= cpu){
			free(__redfstCpu);
			__redfstNcpus = 0;
			return -1;
		}
		if(__redfstNcpus==alloc){
			alloc += 1024;
			__redfstCpu = realloc(__redfstCpu, alloc * sizeof(*__redfstCpu));
		}
		memset(__redfstCpu+__redfstNcpus, 0, sizeof(*__redfstCpu));
		__redfstCpu[__redfstNcpus++].id = cpu;
	}
	if(__redfstNcpus)
		qsort(__redfstCpu, __redfstNcpus, sizeof(*__redfstCpu), cmpid);
	__redfstCpu = realloc(__redfstCpu, __redfstNcpus * sizeof(*__redfstCpu));
	create_cpu_mapping();
	__redfst_energy_init();
	return 0;
}

void redfst_energy_init(){
	pthread_t t;
	int i;

	__redfst_fd = redfst_energy_get_fd();
	__redfst_hw_init();

	__redfstMutex = 0;

	// what cpus should we monitor?
	get_default_cpus();
	if(__redfstNcpus)
		qsort(__redfstCpu, __redfstNcpus, sizeof(*__redfstCpu), cmpid);

	create_cpu_mapping();

	// initialize measurement functions
	if(!redfst_msr_init()){
		redfstEnergySupport = 1;
		__redfst_energy_update     = redfst_msr_update;
		__redfst_energy_update_one = redfst_msr_update_one;
		__redfst_energy_init       = redfst_msr_init;
		__redfst_energy_end        = redfst_msr_end;
#if 1
	}else if(!redfst_likwid_init()){
		redfstEnergySupport = 2;
		__redfst_energy_update     = redfst_likwid_update;
		__redfst_energy_update_one = redfst_likwid_update_one;
		__redfst_energy_init       = redfst_likwid_init;
		__redfst_energy_end        = redfst_likwid_end;
#endif
	}else{
		redfstEnergySupport = 0;
		__redfst_energy_update     = dummy;
		__redfst_energy_update_one = dummy_one;
		__redfst_energy_init       = (void*)dummy;
		__redfst_energy_end        = dummy;
		__redfstNcpus = 0;
		if(__redfstCpu){
			free(__redfstCpu);
			__redfstCpu = 0;
		}
	}

	redfst_reset();

	pthread_create(&t,0,redfst_energy_loop,0);

	if(getenv("REDFST_HEADER")){
		for(i=0; i < __redfstNcpus; ++i)
			fprintf(__redfst_fd,"pkg.%d, pp0.%d, dram.%d, ", __redfstCpu[i].id, __redfstCpu[i].id, __redfstCpu[i].id);
		fprintf(__redfst_fd,"pkg, pp0, dram, time\n");
	}
}

