#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <signal.h>
#include <sched.h>
#include <errno.h>
#include "energy.h"
#include "util.h"

#define MSR_RAPL_POWER_UNIT	   0x606
#define MSR_PKG_ENERGY_STATUS  0x611
#define MSR_DRAM_ENERGY_STATUS 0x619
#define MSR_PP0_ENERGY_STATUS  0x639
#define MSR_PP1_ENERGY_STATUS  0x641

/* the structs/enum below and the daemon launch function were mostly copied from likwid */

/* This naming with AccessType and AccessMode is admittedly a bit confusing */
typedef enum {
	DAEMON_AM_DIRECT = 0,
	DAEMON_AM_ACCESS_D
} AccessMode;

typedef enum {
	DAEMON_READ = 0,
	DAEMON_WRITE,
	DAEMON_EXIT
} AccessType;

typedef enum {
	DAEMON_AD_PCI_R3QPI_LINK_0 = 0,
	DAEMON_AD_PCI_R3QPI_LINK_1,
	DAEMON_AD_PCI_R2PCIE,
	DAEMON_AD_PCI_IMC_CH_0,
	DAEMON_AD_PCI_IMC_CH_1,
	DAEMON_AD_PCI_IMC_CH_2,
	DAEMON_AD_PCI_IMC_CH_3,
	DAEMON_AD_PCI_HA,
	DAEMON_AD_PCI_QPI_PORT_0,
	DAEMON_AD_PCI_QPI_PORT_1,
	DAEMON_AD_PCI_QPI_MASK_PORT_0,
	DAEMON_AD_PCI_QPI_MASK_PORT_1,
	DAEMON_AD_PCI_QPI_MISC_PORT_0,
	DAEMON_AD_PCI_QPI_MISC_PORT_1,
	DAEMON_AD_MSR
} AccessDevice;

typedef enum {
	ERR_NOERROR = 0,  /* no error */
	ERR_UNKNOWN,      /* unknown command */
	ERR_RESTREG,      /* attempt to access restricted MSR */
	ERR_OPENFAIL,     /* failure to open msr files */
	ERR_RWFAIL,       /* failure to read/write msr */
	ERR_DAEMONBUSY,   /* daemon already has another client */
	ERR_LOCKED,       /* access to HPM is locked */
	ERR_UNSUPPORTED,   /* unsupported processor */
	ERR_NODEV /* No such device */
} AccessErrorType;

typedef struct {
	uint32_t cpu;
	uint32_t reg;
	uint64_t data;
	AccessDevice device;
	AccessType type;
	AccessErrorType errorcode; /* Only in replies - 0 if no error. */
} AccessDataRecord;

static int lik_init(void)
{
	/* Check the function of the daemon here */
	struct sockaddr_un address;
	struct sigaction sa;
	uint64_t t0,t1;
	size_t address_length;
	pid_t pid;
	int socket_fd = -1;
	int i;

	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_NOCLDWAIT;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGCHLD,&sa,0);

	pid = fork();
	if(pid == 0){
		execlp("likwid-accessD", "likwid-accessD", NULL);
		_Exit(1);
	}else if (pid < 0){
		return -1;
	}

	if(0 > (socket_fd = socket(AF_LOCAL, SOCK_STREAM, 0))){
		kill(pid, SIGKILL);
		return -1;
	}

	fcntl(socket_fd, F_SETFL, O_NONBLOCK);
	address.sun_family = AF_LOCAL;
	address_length = sizeof(address);
	snprintf(address.sun_path, sizeof(address.sun_path), "/tmp/likwid-%d", pid);
	t0 = __redfst_time_now();
	for(i=0; connect(socket_fd, (struct sockaddr *) &address, address_length); ++i){
		if(i > 256){ // too many failures
			i = 0;
			t1 = __redfst_time_now();
			if(t1-t0 > 1e9){
				close(socket_fd);
				kill(pid, SIGKILL);
				return -1;
			}
		}
	}
	return socket_fd;
}

static void lik_data_init(AccessDataRecord *d, int cpu, int reg){
	d->cpu    = cpu;
	d->reg    = reg;
	d->type   = DAEMON_READ;
	d->device = DAEMON_AD_MSR;
}

static void readall(int fd, void *buf, int count){
	int n;
	while(count){
		n = read(fd, buf, count);
		if(n > 0){
			count -= n;
			buf += n;
		}
	}
}

static void writeall(int fd, const void *buf, int count){
	int n;
	while(count){
		n = write(fd, buf, count);
		if(n > 0){
			count -= n;
			buf += n;
		}
	}
}

static void update(uint64_t *total, uint32_t *prev, uint32_t now){
	*total += now - *prev;
	*prev = now;
}

static uint64_t lik_read(cpu_t *c, int reg){
	AccessDataRecord d;
	lik_data_init(&d, c->id, reg);
	writeall(c->fd, &d, sizeof(d));
	readall(c->fd, &d, sizeof(d));
	return d.data;
}

void redfst_likwid_update_one(cpu_t *c){
	AccessDataRecord d[3];
	lik_data_init(d+0, c->id, MSR_PKG_ENERGY_STATUS);
	lik_data_init(d+1, c->id, MSR_PP0_ENERGY_STATUS);
	lik_data_init(d+2, c->id, MSR_DRAM_ENERGY_STATUS);
	update(&c->pkg,  &c->pkgPrev,  d[0].data);
	update(&c->pp0,  &c->pp0Prev,  d[1].data);
	update(&c->dram, &c->dramPrev, d[2].data);
	writeall(*c->fd, d, sizeof(d));
	readall(*c->fd, d, sizeof(d));
}

void redfst_likwid_update(){
	AccessDataRecord d[3*__redfstNcpus];
	cpu_t *c;
	int i;
	for(i=0; i < __redfstNcpus; ++i){
		c = __redfstCpu+i;
		lik_data_init(d+3*i+0, c->id, MSR_PKG_ENERGY_STATUS);
		lik_data_init(d+3*i+1, c->id, MSR_PP0_ENERGY_STATUS);
		lik_data_init(d+3*i+2, c->id, MSR_DRAM_ENERGY_STATUS);
	}
	writeall(*__redfstCpu[0].fd, d, sizeof(d));
	readall(*__redfstCpu[0].fd, d, sizeof(d));
	for(i=0; i < __redfstNcpus; ++i){
		c = __redfstCpu+i;
		update(&c->pkg,  &c->pkgPrev,  d[3*i+0].data);
		update(&c->pp0,  &c->pp0Prev,  d[3*i+1].data);
		update(&c->dram, &c->dramPrev, d[3*i+2].data);
	}
}

static int redfst_likwid_safe_read(cpu_t *c, int reg){
	AccessDataRecord d;
	void *p;
	int count;
	int n;
	lik_data_init(&d, c->id, reg);
	// write
	count = sizeof(d);
	p = &d;
	while(count){
		n = write(*c->fd, p, count);
		if(n > 0){
			count -= n;
			p += n;
		}else{
			exit(1);
		}
	}
	// read
	count = sizeof(d);
	p = &d;
	while(count){
		n = read(*c->fd, p, count);
		if(n > 0){
			count -= n;
			p += n;
		}else if(0 == n){
			return 0;
		}else if(EAGAIN == errno || EWOULDBLOCK == errno){
			;
		}
	}
	return d.data;
}

int redfst_likwid_init(){
	cpu_t *c;
	uint64_t k;
	int fd;
	int i;
	fd = lik_init();
	if(0 > fd)
		return -1;
	for(i=0; i < __redfstNcpus; ++i){
		c = __redfstCpu + i;
		*c->fd = fd;
		k = redfst_likwid_safe_read(c, MSR_RAPL_POWER_UNIT);
		if(!k){
			while(i>=0)
				*__redfstCpu[i--].fd = 0;
			close(fd);
			return -1;
		}
		c->unit = pow(0.5,(double)((( k )>>8)&0x1f));
		redfst_likwid_update_one(c); // save current value - it doesn't start at zero
		c->pkg = c->pp0 = c->dram = 0;
	}
	return 0;
}

void redfst_likwid_end(){
	int i;
	kill(*__redfstCpu[0].fd,SIGKILL);
	for(i=0;i<__redfstNcpus;++i)
		*__redfstCpu[i].fd = 0;
}

