#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <error.h>
#include <fcntl.h>
#include "macros.h"
#include "hw.h"
#include "energy.h"
#include "msr.h"

#define MSR_RAPL_POWER_UNIT    0x606
#define MSR_PKG_ENERGY_STATUS  0x611
#define MSR_DRAM_ENERGY_STATUS 0x619
#define MSR_PP0_ENERGY_STATUS  0x639
#define MSR_PP1_ENERGY_STATUS  0x641

static uint64_t msr_read(int fd, int which){
	uint64_t data;
	if(REDFST_UNLIKELY(pread(fd,&data,sizeof(data),which)!=sizeof(data))){
		perror("rdmsr:pread");
		exit(1);
	}
	return data;
}

static void redfst_msr_update_which(uint64_t *total, uint32_t *prev, int fd, int which){
	uint32_t r, delta;
	r = msr_read(fd, which);
	delta = r - *prev;
	*prev = r;
	*total += delta;
}

void redfst_msr_update_one(cpu_t *c){
		redfst_msr_update_which(&c->pkg,  (uint32_t*)&c->pkgPrev,  *c->fd, MSR_PKG_ENERGY_STATUS);
		redfst_msr_update_which(&c->pp0,  (uint32_t*)&c->pp0Prev,  *c->fd, MSR_PP0_ENERGY_STATUS);
		redfst_msr_update_which(&c->dram, (uint32_t*)&c->dramPrev, *c->fd, c->fd[1]); // dram or pp1
}

void redfst_msr_update(){
	cpu_t *c;
	int i;
	for(i=0; i < __redfstNcpus; ++i){
		c = __redfstCpu+i;
		redfst_msr_update_which(&c->pkg,  (uint32_t*)&c->pkgPrev,  *c->fd, MSR_PKG_ENERGY_STATUS);
		redfst_msr_update_which(&c->pp0,  (uint32_t*)&c->pp0Prev,  *c->fd, MSR_PP0_ENERGY_STATUS);
		redfst_msr_update_which(&c->dram, (uint32_t*)&c->dramPrev, *c->fd, c->fd[1]); // dram or pp1
	}
}

int redfst_msr_init(){
	char buf[32];
	cpu_t *c;
	int i;
	for(i=0; i < __redfstNcpus; ++i){
		c = __redfstCpu + i;
		sprintf(buf,"/dev/cpu/%d/msr",socket_to_logical_core(__redfstCpu[i].id));
		*c->fd = open(buf,O_RDONLY);
		if(*c->fd < 0){
			*__redfstCpu[i].fd = 0;
			while(i--){
				close(*__redfstCpu[i].fd);
				*__redfstCpu[i].fd = 0;
			}
			return -1;
		}
		if(0 < pread(*c->fd,buf,8,MSR_DRAM_ENERGY_STATUS)){
			c->fd[1] = MSR_DRAM_ENERGY_STATUS;
		}else{
			c->fd[1] = MSR_PP1_ENERGY_STATUS;
		}
		c->unit = pow(0.5,(double)((( msr_read(*c->fd, MSR_RAPL_POWER_UNIT) )>>8)&0x1f));
		redfst_msr_update_one(c); // save current counter value - it doesn't start at zero
		c->pkg = c->pp0 = c->dram = 0;
	}
	return 0;
}

void redfst_msr_end(){
	int i;
	for(i=0;i<__redfstNcpus;++i)
		close(*__redfstCpu[i].fd);
}
