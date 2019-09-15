#include "libmuse.h"

int main(int argc,char *argv[])
{
	muse_init(1); //testing this shouldn't be here
	
	uint16_t r=muse_alloc(10);
	printf("%d\n",r);
	muse_free(r);
	muse_close();

	return 0;
}
