#include "libmuse.h"
#include "network.h"

int master_socket = 0;

int libmuse_init(char *ip,int port);
int call(Function *f);

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

int call(Function *f)
{
	Message msg;
	MessageHeader header;

	
	create_message_header(&header,MESSAGE_CALL,1,sizeof(Function));
	create_function_message(&msg,&header,f);

	send_message(master_socket,&msg);
	receive_message(master_socket,&msg);
	int *response = msg.data;
	return *response;
}

void muse_close()
{
	close(master_socket);
	return;
}

uint32_t muse_alloc(uint32_t tam)
{ 
	Function f;
	Arg arg;

	arg.type = VAR_UINT32;
	arg.size = sizeof(uint32_t);
	arg.value.val_u32 = tam;

	f.type = FUNCTION_MALLOC;
	f.num_args = 1;
	f.args[0] = arg;
	
	int result = call(&f);
	
	return result;
}

void muse_free(uint32_t dir)
{
	Function f;
	Arg arg;

	arg.type = VAR_UINT32;
	arg.size = sizeof(uint32_t);
	arg.value.val_u32 = dir;

	f.type = FUNCTION_FREE;
	f.num_args = 1;
	f.args[0] = arg;	

	call(&f);
	return;
}

int muse_get(void* dst, uint32_t src, size_t n)
{
	Function f;
	Arg arg[3];
	uint32_t *d = (uint32_t *)dst;

	arg[0].type = VAR_VOID_PTR;
	arg[0].size = sizeof(uint32_t);
	arg[0].value.val_u32 = *d;

	arg[1].type = VAR_UINT32;
	arg[1].size = sizeof(uint32_t);
	arg[1].value.val_u32 = src;

	arg[2].type = VAR_SIZE_T;
	arg[2].size = sizeof(size_t);
	arg[2].value.val_sizet = n;

	f.type = FUNCTION_GET;
	f.num_args = 3;
	f.args[0] = arg[0];
	f.args[1] = arg[1];
	f.args[2] = arg[2];

	*d = call(&f);
	return 0;
}

int muse_cpy(uint32_t dst, void* src, int n)
{
	Function f;
	Arg arg[3];
	uint32_t *s = (uint32_t *)src;

	arg[0].type = VAR_UINT32;
	arg[0].size = sizeof(uint32_t);
	arg[0].value.val_u32 = dst;

	arg[1].type = VAR_VOID_PTR;
	arg[1].size = sizeof(uint32_t);
	arg[1].value.val_u32 = *s;

	arg[2].type = VAR_UINT32;
	arg[2].size = sizeof(uint32_t);
	arg[2].value.val_u32 = n;

	f.type = FUNCTION_COPY;
	f.num_args = 3;
	f.args[0] = arg[0];
	f.args[1] = arg[1];
	f.args[2] = arg[2];

	int result = call(&f);
	return result;
}

uint32_t muse_map(char *path, size_t length, int flags)
{
	int result=0;
	return result;
}

int muse_sync(uint32_t addr, size_t len)
{
	int result=0;
	return result;
}

int muse_unmap(uint32_t dir)
{
	int result=0;
	return result;
}
