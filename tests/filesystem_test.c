#include <stdio.h>

int main(void)
{
	FILE *f = fopen("/tmp/exm/file","r");
	if(f != NULL)
	{
		printf("file opened");
		fclose(f);
	}
	return 0;
}
