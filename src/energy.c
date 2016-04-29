#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include "config.h"
#include "util.h"
#include "msr.h"
#include "macros.h"
#include "control.h"
#include "hw.h"
#include "energy.h"

#define LOCK(x) __sync_val_compare_and_swap(&(x), 0, 1) // return 0 if it manages to obtain the __redfstMutex
#define UNLOCK(x) do{(x)=0;}while(0)

#ifdef REDFST_STATIC
uint64_t __redfstTime0; // timer is set to last reset
#endif

void (*__redfst_energy_update)();
void (*__redfst_energy_update_one)(cpu_t *c);
int  (*__redfst_energy_init)();
void (*__redfst_energy_end)();

static void dummy(){}
static void dummy_one(cpu_t *c){}

int __redfstNcpus;
cpu_t *__redfstCpu;
cpu_t **gCpuId2Cpu = 0;
volatile int __redfstMutex;
FILE *__redfst_fd;

static void safe_force_update(){
	while(unlikely(LOCK(__redfstMutex)))
		;
	__redfst_energy_update();
	UNLOCK(__redfstMutex);
}



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
	struct dirent *e;
	DIR *d;
	cpu_t *c;
	char *s;
	int ncpus;

	d = opendir("/dev/cpu/");
	if(!d){
		__redfstNcpus = 0;
		__redfstCpu   = 0;
		return;
	}
	ncpus = 0;
	c = malloc(1);

	while((e=readdir(d))){
		for(s=e->d_name; *s; ++s)
			if(*s<'0' || *s>'9')
				break;
		if(!*s){
			c = realloc(c, (ncpus+1) * sizeof(*c));
			memset(c+ncpus,0,sizeof(*c));
			c[ncpus++].id = atoi(e->d_name);
		}
	}
	closedir(d);
	if(!ncpus){
		free(c);
		c = 0;
	}
	__redfstNcpus = ncpus;
	__redfstCpu = c;
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
				if(a>b)
					a^=b; b^=a; a^=b;
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
	int j;
	if(gCpuId2Cpu)
		free(gCpuId2Cpu);
	for(i=j=0; i < __redfstNcpus; ++i)
		if(j < __redfstNcpus)
			j = __redfstNcpus;
	gCpuId2Cpu = malloc((j+1)*sizeof(*gCpuId2Cpu));
	for(i=j=0; i < __redfstNcpus; ++i)
		gCpuId2Cpu[__redfstCpu[i].id] = __redfstCpu+i;
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
		__redfst_energy_update     = redfst_msr_update;
		__redfst_energy_update_one = redfst_msr_update_one;
		__redfst_energy_init       = redfst_msr_init;
		__redfst_energy_end        = redfst_msr_end;
	}else{
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

#ifdef REDFST_CSV
	if(getenv("REDFST_HEADER")){
		for(i=0; i < __redfstNcpus; ++i)
			fprintf(__redfst_fd,"pkg.%d, pp0.%d, dram.%d, ", __redfstCpu[i].id, __redfstCpu[i].id, __redfstCpu[i].id);
		fprintf(__redfst_fd,"pkg, pp0, dram, time\n");
	}
#endif
}

