#include "hilolay.h"

void *test()
{
	printf("test\n");
}

int main(int argc,char *argv[])
{
	th_create(test);
	return 0;
}