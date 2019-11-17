#include "libmuse.h"
#include <commons/config.h>
#define LIBMUSE_CONFIG_PATH "../configs/libmuse.config"

int master_socket = 0;
char* SERVER_IP;
int SERVER_PORT;
t_config* config = config_create(LIBMUSE_CONFIG_PATH);

int muse_init(int id)
{
	//SERVER_IP = config_get_int_value(config,"SERVER_IP"); 
	//Se debería cambiar lo de abajo
	SERVER_IP = "127.0.0.1"
	SERVER_PORT = config_get_int_value(config,"SERVER_PORT");	
	master_socket = connect_to(SERVER_IP,SERVER_PORT);
	return master_socket; 
}

int call(Function *function)
{
	Message msg;
	MessageHeader header;
	
	create_message_header(&header,MESSAGE_CALL,1,sizeof(Function));
	create_function_message(&msg,&header,function);

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
	Function function;
	Arg arg; //argumento

	arg.type = VAR_UINT32;
	arg.size = sizeof(uint32_t);
	arg.value.val_u32 = tam;

	function.type = FUNCTION_MALLOC;
	function.num_args = 1;
	function.args[0] = arg;
	
	int result = call(&function);
	
	return result;
}

void muse_free(uint32_t dir)
{
	Function function;
	Arg arg;

	arg.type = VAR_UINT32;
	arg.size = sizeof(uint32_t);
	arg.value.val_u32 = dir;

	function.type = FUNCTION_FREE;
	function.num_args = 1;
	function.args[0] = arg;	

	call(&function);
	return;
}

int muse_get(void* dst, uint32_t src, size_t n)
{
	Function function;
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

	function.type = FUNCTION_GET;
	function.num_args = 3;
	function.args[0] = arg[0];
	function.args[1] = arg[1];
	function.args[2] = arg[2];

	*d = call(&function);
	return 0;
}

int muse_cpy(uint32_t dst, void* src, int n)
{
	Function function;
	Arg arg[3];	//argumento
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

	function.type = FUNCTION_COPY;
	function.num_args = 3;
	function.args[0] = arg[0];
	function.args[1] = arg[1];
	function.args[2] = arg[2];

	int result = call(&function);
	return result;
}


/*
	Ejemplo de uso
	uint32_t map = muse_map("hola.txt", filesize, MAP_PRIVATE);		
	opens the "hola.txt" file in RDONLY mode
	map is a position _mapped_ of pages of a given 'filesize' of the "hola.txt" file  
	muse_map basically is just putting the hola.txt file in memory 

	@Return
	On success, mmap() returns a pointer to the mapped area. On error,
	the value MAP_FAILED (that is, (void *) -1) is returned, and errno
	is set appropriately.
*/
uint32_t muse_map(char *path, size_t length, int flags)
{
   	int file_descriptor = open(*path, O_RDONLY, 0);
   	void* map = mmap(NULL, length, NULL, flags, file_descriptor, 0);
   	printf(map, length);   
	return *(int*) map;
}

int muse_sync(uint32_t addr, size_t len)
{
	int result=0;
	return result;
}

/*
	@Return
	On success, munmap() returns 0, on failure -1,
	and errno is set (probably to EINVAL).
*/
int muse_unmap(uint32_t dir)
{
	int unmap_result = munmap(dir, 1 << 10); //TODO: ¿¿ 1 << 10 ??
  	if (unmap_result == 0 ) {
		printf("Could not unmap");
		//log_error(muse_logger,"Could not unmap");
		return -1;
	}
	return 0;
}
