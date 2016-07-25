#include <stdlib.h>
#include <unistd.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "energy.h"
#include "powercap.h"

static int powercap_open(int cpu, int which){
	static const char *subsystem[] = {"",":0",":1"}; // package, core, dram/uncore
	char buf[64];
	sprintf(buf, "/sys/class/powercap/intel-rapl:%d%s/energy_uj", cpu, subsystem[which]);
	return open(buf, O_RDONLY);
}

static void redfst_powercap_update_which(uint64_t *total, uint64_t *prev, int fd){
	char buf[32];
	uint64_t r, delta;
	lseek(fd, 0, SEEK_SET);
	buf[read(fd, buf, sizeof(buf)-1)] = 0;
	r = atoll(buf);
	delta = r - *prev;
	*prev = r;
	*total += delta;
}

void redfst_powercap_update_one(cpu_t *c){
		redfst_powercap_update_which(&c->pkg,  &c->pkgPrev,  c->fd[0]);
		redfst_powercap_update_which(&c->pp0,  &c->pp0Prev,  c->fd[1]);
		redfst_powercap_update_which(&c->dram, &c->dramPrev, c->fd[2]);
}

void redfst_powercap_update(){
	cpu_t *c;
	int i;
	for(i=0; i < __redfstNcpus; ++i){
		c = __redfstCpu+i;
		redfst_powercap_update_which(&c->pkg,  &c->pkgPrev,  c->fd[0]);
		redfst_powercap_update_which(&c->pp0,  &c->pp0Prev,  c->fd[1]);
		redfst_powercap_update_which(&c->dram, &c->dramPrev, c->fd[2]);
	}
}

int redfst_powercap_init(){
	cpu_t *c;
	int i;
	for(i=0; i < __redfstNcpus; ++i){
		c = __redfstCpu + i;
		c->fd[0] = powercap_open(c->id, 0);
		c->fd[1] = powercap_open(c->id, 1);
		c->fd[2] = powercap_open(c->id, 2);
		if(0 > c->fd[0] || 0 > c->fd[1] || 0 > c->fd[2]){
			for(;i>=0;--i){
				c = __redfstCpu + i;
				if(0 <= c->fd[0]) close(c->fd[0]);
				if(0 <= c->fd[1]) close(c->fd[1]);
				if(0 <= c->fd[2]) close(c->fd[2]);
				c->fd[0] = c->fd[1] = c->fd[2] = 0;
			}
			return -1;
		}
		c->unit = 1e-6; // uJ
		redfst_powercap_update_one(c); // save current counter value - it doesn't start at zero although we could zero it
		c->pkg = c->pp0 = c->dram = 0;
		if(0 == c->pkgPrev || 0 == c->pp0Prev || 0 == c->dramPrev)
			return -1;
	}
	return 0;
}

void redfst_powercap_end(){
	cpu_t *c;
	int i;
	for(i=0; i < __redfstNcpus; ++i){
		c = __redfstCpu + i;
		close(c->fd[0]);
		close(c->fd[1]);
		close(c->fd[2]);
		c->fd[0] = c->fd[1] = c->fd[2] = 0;
	}
}
