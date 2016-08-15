#include <unistd.h>
#include "config.h"
#ifdef REDFST_FUN_IN_H
#include <redfst-static.h>
#else
#include <redfst-default.h>
#endif

int main(){
	redfst_init();
	redfst_reset();
	usleep(300000);
	redfst_print(); // .3
	redfst_print(); // .3
	usleep(300000);
	redfst_print(); // .6
	redfst_reset();
	usleep(300000);
	redfst_print(); // .3
	return 0;
}
