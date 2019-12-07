#include "libmuse.h"

int main(int argc,char *argv[])
{
	int pi1 = muse_init(20);
	//int pi2 = muse_init(21);
	printf("Libmuse initialized!\n");
	printf("Comencing test!\n");
	sleep(2);
	//say("hola muse\n"); //say = send
	printf("Message sent!\n");

	int dir = muse_alloc(200);
	printf("La dirección de la memoria reservada es %d!\n",dir);

	printf("Press a button to continue\n");
	getchar();
	
	return 0;
}
