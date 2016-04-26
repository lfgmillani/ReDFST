#include <unistd.h>
#include <redfst.h>

int main(){
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
