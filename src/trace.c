#include "config.h"
#ifdef REDFST_TRACE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include "util.h"
#include "trace.h"
FILE * __redfstTraceFd;
uint64_t __redfstTraceT0;

static FILE * redfst_trace_get_fd(){
/*
	this function must be called before any files are opened (unless they're closed)
*/
	FILE *fd;
	if(getenv("REDFST_TRACEFILE")){
		fd = fopen(getenv("REDFST_TRACEFILE"),"a");
		if(!fd){
			fprintf(stderr, "ReDFST: Failed to open trace file \"%s\"\n", getenv("REDFST_TRACEFILE"));
			exit(1);
		}
		return fd;
	}
	if(0 <= fcntl(4, F_GETFL))
		return fdopen(4, "w");
	return stderr;
}

void redfst_trace_init(){
	__redfstTraceFd = redfst_trace_get_fd();
	__redfstTraceT0 = time_now();
	fprintf(__redfstTraceFd, "time, thread, region, frequency\n");
}
#endif
