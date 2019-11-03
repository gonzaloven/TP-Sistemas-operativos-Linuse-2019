#include "libmuse.h"

int main(int argc,char *argv[])
{
	muse_init(1); //testing this shouldn't be here
	
	uint16_t r=muse_alloc(10);
	printf("%d\n",r);
	int x = 20;
	int *y = malloc(4);
	muse_cpy(r,&x,4);
	muse_get(y,r,4);
	printf("%d\n",*y);
	muse_free(r);
	free(y);
	muse_close();

	return 0;
}
