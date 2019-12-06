#include "libmuse.h"

int MASTER_SOCKET;
int PROCESS_ID;

int muse_init(int id)
{
	config = config_create(LIBMUSE_CONFIG_PATH);

	if(config == NULL)
	{
		printf("Configuration couldn't be loaded.Quitting program!\n");
		return(-1);
	}

	char* SERVER_IP = config_get_string_value(config, "SERVER_IP");
	int SERVER_PORT = config_get_int_value(config, "SERVER_PORT");
	
	MASTER_SOCKET = connect_to(SERVER_IP,SERVER_PORT);

	PROCESS_ID = id;

	/*
	TODO: importante, en una parte de acá, debería mandarle a muse mi p_id, ya que ahora todos 
	los programas libMuse que se están corriendo tiene p_id = 1, independientemente del socket que se conecten
	*/

	config_destroy(config);

	return 0; 
}

int call(Function *function)
{
	Message msg;
	MessageHeader header;	

	create_message_header(&header,MESSAGE_CALL,PROCESS_ID,sizeof(Function));
	create_function_message(&msg,&header,function);

	send_message(MASTER_SOCKET,&msg);
	receive_message(MASTER_SOCKET,&msg);
	int *response = msg.data;
	return *response;
}

void muse_close()
{
//	Function function;
//	Arg arg; //argumento
//
//	arg.type = VAR_UINT32;
//	arg.size = sizeof(uint32_t);
//	arg.value.val_u32 = 1;
//
//	function.type = FUNCTION_MUSE_CLOSE;
//	function.num_args = 1;
//	function.args[0] = arg;
//
//	///Hay que hacer que libere todo tambien de ese programa
//	int result = call(&function);

	close(MASTER_SOCKET);
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
	uint32_t *direccion = (uint32_t *)dst;
	void* lo_que_habia_adentro_de_muse_en_src;

	arg[0].type = VAR_VOID_PTR;
	arg[0].size = sizeof(uint32_t);
	arg[0].value.val_u32 = *direccion;

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

	//*direccion = call(&function);

	lo_que_habia_adentro_de_muse_en_src = (void*) call(&function);

	//memcpy(dst, lo_que_habia_adentro_de_muse_en_src, n);

	return 0;
}

int muse_cpy(uint32_t dst, void* src, int n)
{
	Function function;
	Arg arg[3];	//argumento

	arg[0].type = VAR_UINT32;
	arg[0].size = sizeof(uint32_t);
	arg[0].value.val_u32 = dst;

	function.args[1].type = VAR_VOID_PTR;
	function.args[1].size = n;
	function.args[1].value.val_voidptr = malloc(n);
	memcpy(function.args[1].value.val_voidptr, src, n);

	arg[2].type = VAR_UINT32;
	arg[2].size = sizeof(uint32_t);
	arg[2].value.val_u32 = n;

	function.type = FUNCTION_COPY;
	function.num_args = 3;
	function.args[0] = arg[0];
	function.args[2] = arg[2];

	int result = call(&function);
	return result;
}

uint32_t muse_map(char *path, size_t length, int flags)
{
	Function function;
	Arg arg[3];	//argumento
	char *s = path;

	function.args[0].type = VAR_CHAR_PTR;
	function.args[0].size = strlen(path) + 1;
	function.args[0].value.val_charptr = malloc(function.args[0].size);
	memcpy(function.args[0].value.val_charptr, path, function.args[0].size);

	arg[1].type = VAR_SIZE_T;
	arg[1].size = sizeof(size_t);
	arg[1].value.val_u32 = length;

	arg[2].type = VAR_UINT32;
	arg[2].size = sizeof(uint32_t);
	arg[2].value.val_u32 = flags;

	function.type = FUNCTION_MAP;
	function.num_args = 3;
	function.args[1] = arg[1];
	function.args[2] = arg[2];

	uint32_t result = call(&function);
	return result;
}

int muse_sync(uint32_t addr, size_t len)
{
	Function function;
	Arg arg[2];	//argumento

	arg[0].type = VAR_UINT32;
	arg[0].size = sizeof(uint32_t);
	arg[0].value.val_u32 = addr;

	arg[1].type = VAR_SIZE_T;
	arg[1].size = sizeof(size_t);
	arg[1].value.val_u32 = len;

	function.type = FUNCTION_SYNC;
	function.num_args = 2;
	function.args[0] = arg[0];
	function.args[1] = arg[1];

	int result = call(&function);
	return result;
}

int muse_unmap(uint32_t dir)
{
	Function function;
	Arg arg;

	arg.type = VAR_UINT32;
	arg.size = sizeof(uint32_t);
	arg.value.val_u32 = dir;

	function.type = FUNCTION_UNMAP;
	function.num_args = 1;
	function.args[0] = arg;	

	int result = call(&function);

	if(result == -1){
		printf("\nSegmentation fault (Esto fue escrito con un printf, cambiar por un log error)\n");
	}
	return result;
}
