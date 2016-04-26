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
#include "energy.h"

#define LOCK(x) __sync_val_compare_and_swap(&(x), 0, 1) // return 0 if it manages to obtain the __redfstMutex
#define UNLOCK(x) do{(x)=0;}while(0)

#ifdef REDFSTLIB_STATIC
uint64_t __redfstTime0; // timer is set to last reset
#endif

void (*__redfst_energy_update)();
void (*__redfst_energy_update_one)(core_t *c);

static void dummy(){}
static void dummy_one(core_t *c){}

int __redfstNcores; // actually these are hardware threads, not cores. this is misnamed everywhere.
core_t *__redfstCore;
core_t **gCoreId2Core;
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


static FILE * __redfst_energy_get_fd(){
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
	return ((core_t*)a)->id - ((core_t*)b)->id;
}

static void get_all_sensors(){
	struct dirent *e;
	DIR *d;
	core_t *c;
	char *s;
	int ncores;

	d = opendir("/dev/cpu/");
	if(!d){
		__redfstNcores = 0;
		__redfstCore   = 0;
		return;
	}
	ncores = 0;
	c = malloc(1);

	while((e=readdir(d))){
		for(s=e->d_name; *s; ++s)
			if(*s<'0' || *s>'9')
				break;
		if(!*s){
			c = realloc(c, (ncores+1) * sizeof(*c));
			memset(c+ncores,0,sizeof(*c));
			c[ncores++].id = atoi(e->d_name);
		}
	}
	closedir(d);
	if(!ncores){
		free(c);
		c = 0;
	}
	__redfstNcores = ncores;
	__redfstCore = c;
}

static void get_sensors(){
	char *s;
	core_t *core;
	int alloc;
	int ncores;
	int a,b;
	int *l;
	char c;
	char st;
	s = getenv("REDFST_CPU");
	if(!s){
		get_all_sensors();
		return;
	}else if(!*s){
		__redfstNcores = 0;
		__redfstCore = 0;
		return;
	}
	alloc = 1024;
	ncores = 0;
	core = malloc(alloc * sizeof(*core));

	st = 0;
#define APPEND(x) do{\
	if(ncores+1 == alloc){\
		alloc += 1024;\
		core = realloc(core, alloc * sizeof(core));\
	}\
	memset(core+ncores,0,sizeof(*core));\
	core[ncores++].id = (x);\
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
	if(ncores){
		core = realloc(core, ncores * sizeof(*core));
		l = calloc(ncores, sizeof(*l));
		for(a=0;a<ncores;++a){
			if(l[core[a].id]++){
				fprintf(__redfst_fd, "No repeated elements allowed in REDFST_CPU: \"%s\". %d appears twice.\n", getenv("REDFST_CPU"),core[a].id);
				goto FAIL;
			}
		}
		free(l);
	}else{
		free(core);
		core = 0;
	}
	__redfstNcores = ncores;
	__redfstCore = core;
	return;
FAIL:
	fprintf(__redfst_fd, "Failed to parse REDFST_CPU: \"%s\"\n", getenv("REDFST_CPU"));
	exit(1);
}

void redfst_energy_init(){
	pthread_t t;
	int i,j;

	__redfst_fd = __redfst_energy_get_fd();

	__redfstMutex = 0;

	// what should we monitor?
	get_sensors();
	if(__redfstNcores)
		qsort(__redfstCore, __redfstNcores, sizeof(*__redfstCore), cmpid);
#if 0
	for(i=0;i<__redfstNcores;++i)
		printf("%d ",__redfstCore[i].id);
	printf("\n");
#endif

	// create core mapping
	for(i=j=0; i < __redfstNcores; ++i)
		if(j < __redfstNcores)
			j = __redfstNcores;
	gCoreId2Core = malloc((j+1)*sizeof(*gCoreId2Core));
	for(i=j=0; i < __redfstNcores; ++i)
		gCoreId2Core[__redfstCore[i].id] = __redfstCore+i;

	// initialize measurement functions
	if(!redfst_msr_init()){
		__redfst_energy_update = redfst_msr_update;
		__redfst_energy_update_one = redfst_msr_update_one;
	}else{
		__redfst_energy_update = dummy;
		__redfst_energy_update_one = dummy_one;
		__redfstNcores = 0;
		free(__redfstCore);
		__redfstCore = 0;
	}

	redfst_reset();

	pthread_create(&t,0,redfst_energy_loop,0);

#ifdef REDFST_CSV
	if(getenv("REDFST_HEADER")){
		for(i=0; i < __redfstNcores; ++i)
			fprintf(__redfst_fd,"pkg.%d, pp0.%d, dram.%d, ", __redfstCore[i].id, __redfstCore[i].id, __redfstCore[i].id);
		fprintf(__redfst_fd,"pkg, pp0, dram, time\n");
	}
#endif
}
