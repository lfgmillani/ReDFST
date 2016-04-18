/*
	code inspired by http://web.eece.maine.edu/~vweaver/projects/rapl/rapl-read.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "redfst_config.h"
#include "launch.h"

#define MSR_RAPL_POWER_UNIT    0x606
#define MSR_PKG_ENERGY_STATUS  0x611
#define MSR_DRAM_ENERGY_STATUS 0x619
#define MSR_PP0_ENERGY_STATUS  0x639
#define MSR_PP1_ENERGY_STATUS  0x641

#define err(...) do{ fprintf(stderr,"%s:%d ",__FILE__,__LINE__); fprintf(stderr,__VA_ARGS__); }while(0);

typedef struct{
	double pkg,pp0,pp1,dram;
	uint32_t pkgPrev,pp0Prev,pp1Prev,dramPrev;
	int fd;
	int id;
}core_t;

int gRetval = -1;
int gPid = 0;
uint8_t gRun = 1;

static uint64_t time_now(){
	struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC, &spec);
	return ((uint64_t)1e9) * spec.tv_sec + spec.tv_nsec;
}

void app_dies(int signum){
	int status;
	waitpid(gPid,&status,0);
	gRetval = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
	gRun = 0;
}

void signal_callback_handler(int signum){
	switch(signum){
		case SIGINT:
			gRun = 0;
			gRetval = 0;
			break;
		case SIGQUIT:
		case SIGTERM:
			exit(signum);
	}
}

static uint64_t read_msr(int fd, int which){
	uint64_t data;
	if(pread(fd,&data,sizeof(data),which) != sizeof(data)){
		perror("rdmsr:pread");
		exit(127);
	}
	return data;
}

#define CPU_SANDYBRIDGE    42
#define CPU_SANDYBRIDGE_EP 45
#define CPU_IVYBRIDGE      58
#define CPU_IVYBRIDGE_EP   62
#define CPU_HASWELL        60
int detect_cpu(void) {
	FILE *fff;

	int family,model=-1;
	char buffer[BUFSIZ],*result;
	char vendor[BUFSIZ];

	fff=fopen("/proc/cpuinfo","r");
	if (fff==NULL) return -1;

	while(1) {
		result=fgets(buffer,BUFSIZ,fff);
		if (result==NULL) break;

		if (!strncmp(result,"vendor_id",8)) {
			sscanf(result,"%*s%*s%s",vendor);

			if
				(strncmp(vendor,"GenuineIntel",12))
				{
					err("%s not an Intel chip\n",vendor);
					return -1;
				}
		}

		if (!strncmp(result,"cpu family",10)) {
			sscanf(result,"%*s%*s%*s%d",&family);
			if (family!=6) {
				err("Wrong CPU family: %d\n",family);
				return -1;
			}
		}

		if (!strncmp(result,"model",5)) {
			sscanf(result,"%*s%*s%d",&model);
		}

	}

	fclose(fff);

	switch(model) {
		case CPU_SANDYBRIDGE:
			break;
		case CPU_SANDYBRIDGE_EP:
			break;
		case CPU_IVYBRIDGE:
			break;
		case CPU_IVYBRIDGE_EP:
			break;
		case CPU_HASWELL:
			break;
		default:
			model=-1;
			break;
	}

	return model;
}

static inline void energy_update_one(double *total,uint32_t *prev,int fd,int which){
	uint32_t r, delta;
	r = read_msr(fd, which);
	delta = r - *prev;
	*prev = r;
	*total += delta;
}

static void energy_update(core_t *core){
	energy_update_one(&core->pkg, &core->pkgPrev, core->fd, MSR_PKG_ENERGY_STATUS);
	energy_update_one(&core->pp0, &core->pp0Prev, core->fd, MSR_PP0_ENERGY_STATUS);
#if 0
	energy_update_one(&core->pp1, &core->pp1Prev, core->fd, MSR_PP1_ENERGY_STATUS);
#endif
	energy_update_one(&core->dram, &core->dramPrev, core->fd, MSR_DRAM_ENERGY_STATUS);
}

static void energy_reset(core_t *core){
	core->pkg = core->pp0 = core->pp1 = core->dram = 0;
}

static void energy_init(core_t *core, int n){
	char buf[32];
	int fd;
	sprintf(buf,"/dev/cpu/%d/msr",n);
	fd = open(buf,O_RDONLY);
	if(fd < 0){
		err("Failed to open %s\n",buf);
		exit(1);
	}
	memset(core,0,sizeof(*core));
	core->fd = fd;
	core->id = n;
	energy_update(core);
	energy_reset(core);
}

#define show_usage() printf("Usage: %s [-c CORE...] [PROGRAM] [ARGS]\n"\
                            "  Outputs energy consumption for the pkg, pp0 and dram of each of the given COREs.\n"\
                            "\n"\
                            "  The following commands are accepted from the stdin:\n"\
                            "    reset  clears the counters\n"\
                            "    print  writes the counters to stderr\n"\
                            "    exit   writes the counters to stderr and terminates execution\n"\
                            "\n"\
                            "Arguments:\n"\
                            "  -c CORE  measures energy consumption of CORE. Defaults to 0.\n"\
                            "  PROGRAM  if given, executes program with ARGS and terminates execution\n"\
                            "           when PROGRAM finishes\n",*argv)

static void print(core_t *cores, int ncores, uint64_t deltat){
	double pkg=0,pp0=0,dram=0;
	double unit;
	uint64_t r;
	int i;
	for(i=0;i<ncores;++i){
		r=read_msr(cores[i].fd,MSR_RAPL_POWER_UNIT);
		unit = pow(0.5,(double)((r>>8)&0x1f));
		fprintf(stderr,"pkg.%d  %lf\n",cores[i].id,cores[i].pkg*unit);
		fprintf(stderr,"pp0.%d  %lf\n",cores[i].id,cores[i].pp0*unit);
#if 0
		if(cpuModel==CPU_SANDYBRIDGE || cpuModel==CPU_IVYBRIDGE
		|| cpuModel==CPU_HASWELL){
			printf("pp1.%d  %lf\n",cores[i].id,cores[i].pp1*unit);
		}
#endif
		fprintf(stderr,"dram.%d %lf\n",cores[i].id,cores[i].dram*unit);
		pkg += cores[i].pkg *unit;
		pp0 += cores[i].pp0 *unit;
		dram+= cores[i].dram*unit;
	}
	fprintf(stderr,"pkg    %lf\n"
	        "pp0    %lf\n"
	        "dram   %lf\n"
					"time   %lf\n",
	        pkg,pp0,dram,deltat/1e9f);
}

int main(int argc, char **argv){
	uint64_t t0;
	core_t cores[REDFSTBIN_MAX_CORES];
	const useconds_t pollingPeriod = 1000000 / REDFSTBIN_POLLING_FREQUENCY;
	int ncores = 0;
	int i;
	int cpuModel;

	cpuModel = detect_cpu();
	if(cpuModel < 0){
		err("CPU not supported\n");
		return 1;
	}

	if(signal(SIGCHLD,app_dies) || signal(SIGINT,signal_callback_handler)){
		err("failed to set sigaction\n");
		return 1;
	}

	for(i=1;i<argc;++i){
		char *a = argv[i];
		if(a[0]=='-'&&((a[1]=='-'&&!a[2])||!a[1])){
			++i;
			break;
		}else if(a[0]=='-'&&a[1]=='c'){
			if(a[2]){
				char *s;
				int n;
				s = argv[i] + ('='==a[2]?3:2);
				for(n=0; *s; n = 10*n + *s++ - '0')
					if(*s<'0'||*s>'9'){
						show_usage();
						exit(1);
					}
				energy_init(cores+ncores++,n);
			}else{
				if(i+1==argc){
					show_usage();
					exit(1);
				}
				energy_init(cores+ncores++,atoi(argv[++i]));
			}
		}else if(a[0]=='-'){
			show_usage();
			exit(1);
		}else{
			break;
		}
	}
	if(!ncores){
		ncores = 1;
		energy_init(cores, 0);
	}
	t0 = time_now();
	if(argc-i>=1)
		gPid = launch(argv+i,argc-i);

	if(gPid){
		while(gRun){
			usleep(pollingPeriod);
			for(i=0;i<ncores;++i){
				energy_update(cores+i);
			}
		}
	}else{
		int n;
		int i;
		int st = 0;
		char c;
		fcntl(0,F_SETFL,fcntl(0,F_GETFL)|O_NONBLOCK);
		while(gRun){
			n = read(0,&c,1);
			if(n == 0){
				gRun = 0;
				gRetval = 0;
				break;
			}else if(n < 0){
				usleep(pollingPeriod);
			}else{
#define N(x) if(c==(x)) ++st; else st=1; break
#define P() if(c!='\n'){st=1;break;}
				switch(st){
				case   0:
					if      ('e'==c) st = 100;
					else if ('p'==c) st = 200;
					else if ('r'==c) st = 300;
					else if('\n'!=c) st =   1;
					break;
				case   1: if('\n'==c) st=0; break;
				          // e
				case 100: N('x');
				case 101: N('i');
				case 102: N('t');
				case 103: P();
					gRun = 0;
					gRetval = 0;
					st=0;
					break;
				          // p
				case 200: N('r');
				case 201: N('i');
				case 202: N('n');
				case 203: N('t');
				case 204: P();
					for(i=0;i<ncores;++i)
						energy_update(cores+i);
					print(cores,ncores,time_now()-t0);
					st = 0;
					break;
				          // r
				case 300: N('e');
				case 301: N('s');
				case 302: N('e');
				case 303: N('t');
				case 304: P();
					t0 = time_now();
					for(i=0;i<ncores;++i)
						energy_reset(cores+i);
					st = 0;
					break;
				}
#undef N
#undef P
			}
			for(i=0;i<ncores;++i){
				energy_update(cores+i);
			}
		}
	}
	print(cores,ncores,time_now()-t0);
	return gRetval;
}
