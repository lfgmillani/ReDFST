#include <stdio.h>
#include <stdlib.h>
#include "hw.h"
int *__redfstHwSocketToLogicalCore = 0;
int __redfstHwNcpus = 0;

void __redfst_hw_init(){
	char buf[64];
	FILE *fp;
	int ncpus=0;
	int i;
	int cpu;
	__redfstHwSocketToLogicalCore = malloc(1);
	for(i=0;;++i){
		sprintf(buf,"/sys/devices/system/cpu/cpu%d/topology/physical_package_id",i);
		fp = fopen(buf, "rt");
		if(!fp)
			break;
		fread(buf,1,sizeof(buf),fp);
		fclose(fp);
		cpu = atoi(buf);
		if(cpu>ncpus){
			fprintf(stderr, "failed to read topology\n");
			exit(1);
		}
		if(cpu==ncpus){
			__redfstHwSocketToLogicalCore = realloc(__redfstHwSocketToLogicalCore, (++ncpus)*sizeof(*__redfstHwSocketToLogicalCore));
			__redfstHwSocketToLogicalCore[cpu] = i;
		}
	}
	if(!ncpus){
		fprintf(stderr, "failed to read topology\n");
		exit(1);
	}
	__redfstHwNcpus = ncpus;
}
