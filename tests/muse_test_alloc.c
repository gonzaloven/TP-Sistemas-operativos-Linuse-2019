#include "libmuse.h"

int main(int argc,char *argv[])
{
	muse_init(1);
	int r=muse_alloc(1);
	muse_free(1);
	muse_close();
	return 0;
}
