#include "libmuse.h"


int main(int argc,char *argv[])
{
	muse_init(1);
	printf("Libmuse initialized!\n");
	printf("Comencing test!\n");
	sleep(2);
	//say("hola muse\n"); //say = send
	printf("Message sent!\n");
	printf("Press a button to continue\n");
	getchar();
	
	return 0;
}
