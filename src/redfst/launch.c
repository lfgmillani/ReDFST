#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "launch.h"

int launch(char **cmd, int n){
	int pid = fork();
	if(!pid){
		char *args[n+1];
		memcpy(args,cmd,n*sizeof(*args));
		args[n] = 0;
		while(strchr(args[0],'/'))
			args[0] = strchr(args[0],'/')+1;
		execvp(*cmd, args);
		fprintf(stderr, "failed to run %s", cmd[0]);
		exit(1);
	}
	return pid;
}
