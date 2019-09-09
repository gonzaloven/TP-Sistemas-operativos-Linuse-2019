#include "libmuse.h"
#include "network.h"

int master_socket = 0;

int libmuse_init(char *ip,int port);

int muse_init(int id)
{
	//TODO: hardcoded change!!
	//the configuration should be loaded here ?
	if(libmuse_init("127.0.0.1",5003) <= 0)
	{
		return -1;
	}
	return 0;

}

int libmuse_init(char *ip,int port)
{
	master_socket = connect_to(ip,port);
	return master_socket; 
}

void muse_close()
{
	close(master_socket);
	return;
}

uint32_t muse_alloc(uint32_t tam)
{
	int result = rpc_client_call(master_socket,"alloc",1,tam);
	return 0;
}

void muse_free(uint32_t dir)
{
	//int result = rpc_client_call(master_socket,"free",1,dir);
	printf("free called\n");
	return 0;
}

int muse_get(void* dst, uint32_t src, size_t n)
{
	printf("get called\n");
	return 0;
}

int muse_cpy(uint32_t dst, void* src, int n)
{
	printf("cpy called\n");
	return 0;
}

uint32_t muse_map(char *path, size_t length, int flags)
{
	printf("map called\n");
	return 0;
}

int muse_sync(uint32_t addr, size_t len)
{
	printf("sync called\n");
	return 0;
}

int muse_unmap(uint32_t dir)
{
	printf("unmap called\n");
	return 0;
}



