#include "libmuse.h"
#include "network.h"
#include "rpc.h"

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
	char value[32];
	sprintf(value,"%lu",tam);
	rpc_function_params params[] = {
									{ "uint32_t","tam", value }
								  };
	int result = rpc_client_call(master_socket,"alloc",1,params);
	return 0;
}

void muse_free(uint32_t dir)
{
	char value[32];
	sprintf(value,"%lu",dir);

	rpc_function_params params[]={
									{"uint32_t","dir",value}
								 };
	int result = rpc_client_call(master_socket,"free",1,params);
	return 0;
}

int muse_get(void* dst, uint32_t src, size_t n)
{
	uint32_t *dest = (uint32_t *)dst;
	char arg1[32];
	char arg2[32];
	char arg3[32];

	sprintf(arg1,"%lu",*dest);
	sprintf(arg2,"%lu",src);
	sprintf(arg3,"%zu",n);

	rpc_function_params params[]={
									{"void*","dst",arg1},
									{"uint32_t","src",arg2},
									{"size_t","n",arg3}
								};
	int result = rpc_client_call(master_socket,"get",3,params);
	return 0;
}

int muse_cpy(uint32_t dst, void* src, int n)
{
	uint32_t *source = (uint32_t)src;
	char arg1[32];
	char arg2[32];
	char arg3[32];

	sprintf(arg1,"%lu",dst);
	sprintf(arg2,"%lu",*source);
	sprintf(arg3,"%d",n);

	rpc_function_params params[]={
									{"uint32_t","dst",arg1},
									{"void*","src",arg2},
									{"int","n",arg3}
								};
	int result = rpc_client_call(master_socket,"cpy",3,params);
	
	return 0;
}

uint32_t muse_map(char *path, size_t length, int flags)
{
	char arg2[16];
	char arg3[16];

	sprintf(arg2,"%zu",length);
	sprintf(arg3,"%d",flags);

	rpc_function_params params[]={
									{"char*","path",path},
									{"size_t","length",arg2},
									{"int","flags",arg3}
								};
	int result = rpc_client_call(master_socket,"map",3,params);
	return 0;
}

int muse_sync(uint32_t addr, size_t len)
{
	char arg1[16];
	char arg2[32];

	sprintf(arg1,"%lu",addr);
	sprintf(arg2,"%zu",len);

	rpc_function_params params[]={
									{"uint32_t","addr",arg1},
									{"size_t","len",arg2}
								};
	int result = rpc_client_call(master_socket,"sync",2,params);
	return 0;
}

int muse_unmap(uint32_t dir)
{
	char arg1[sizeof(uint32_t)];
	sprintf(arg1,"%lu",dir);

	rpc_function_params params[]={
									{"uint32_t","dir",arg1}
								};
	int result = rpc_client_call(master_socket,"unmap",1,params);
	return 0;
}



