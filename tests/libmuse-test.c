#include "libmuse.h"

int main(int argc,char *argv[])
{
	libmuse_init("127.0.0.1",8000);
	printf("Libmuse initialized!\n");
	printf("Comencing test!\n");
	sleep(2);
	say("hola libmuse!");
	printf("Message sent!\n");
	printf("Press a button to continue\n");
	getchar();
	
	return 0;
}
