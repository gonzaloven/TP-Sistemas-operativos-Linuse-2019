#include "../libs/Hilolay/hilolay.h"

int main(int argc,char *argv[])
{
	libhilolay_init("127.0.0.1",8000);
	printf("LibHilolay initialized!\n");
	printf("Comencing test!\n");
	sleep(2);
	say("hola suse!");
	printf("Message sent!\n");
	printf("Press a button to continue\n");
	getchar();
	return 0;
}
