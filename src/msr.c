#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <error.h>
#include <fcntl.h>
#include "macros.h"
#include "energy.h"

#define MSR_RAPL_POWER_UNIT    0x606
#define MSR_PKG_ENERGY_STATUS  0x611
#define MSR_DRAM_ENERGY_STATUS 0x619
#define MSR_PP0_ENERGY_STATUS  0x639
#define MSR_PP1_ENERGY_STATUS  0x641

extern int __redfstNcores;
extern core_t *__redfstCore;

static uint64_t msr_read(int fd, int which){
	uint64_t data;
	if(unlikely(pread(fd,&data,sizeof(data),which)!=sizeof(data))){
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

void redfst_msr_update_one(core_t *c){
		redfst_msr_update_which(&c->pkg,  &c->pkgPrev,  c->fd, MSR_PKG_ENERGY_STATUS);
		redfst_msr_update_which(&c->pp0,  &c->pp0Prev,  c->fd, MSR_PP0_ENERGY_STATUS);
		redfst_msr_update_which(&c->dram, &c->dramPrev, c->fd, MSR_DRAM_ENERGY_STATUS);
}

void redfst_msr_update(){
	core_t *c;
	int i;
	for(i=0; i < __redfstNcores; ++i){
		c = __redfstCore+i;
		redfst_msr_update_which(&c->pkg,  &c->pkgPrev,  c->fd, MSR_PKG_ENERGY_STATUS);
		redfst_msr_update_which(&c->pp0,  &c->pp0Prev,  c->fd, MSR_PP0_ENERGY_STATUS);
		redfst_msr_update_which(&c->dram, &c->dramPrev, c->fd, MSR_DRAM_ENERGY_STATUS);
	}
}

int redfst_msr_init(){
	char buf[32];
	core_t *c;
	int i;
	for(i=0; i < __redfstNcores; ++i){
		c = __redfstCore + i;
		sprintf(buf,"/dev/cpu/%d/msr",__redfstCore[i].id);
		c->fd = open(buf,O_RDONLY);
		if(c->fd < 0){
			fprintf(__redfst_fd, "Failed to open %s\n",buf);
			__redfstCore[i].fd = 0;
			while(i--){
				close(__redfstCore[i].fd);
				__redfstCore[i].fd = 0;
			}
			return -1;
		}
		c->unit = pow(0.5,(double)((( msr_read(c->fd, MSR_RAPL_POWER_UNIT) )>>8)&0x1f));
		redfst_msr_update_one(c); // save current counter value - it doesn't start at zero
		c->pkg = c->pp0 = c->dram = 0;
	}
	return 0;
}

