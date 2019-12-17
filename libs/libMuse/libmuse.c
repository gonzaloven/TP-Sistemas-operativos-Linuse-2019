#include "libmuse.h"

int MASTER_SOCKET;
int PROCESS_ID;

pthread_mutex_t s_socket = PTHREAD_MUTEX_INITIALIZER;

t_log *logger;

int tamDataFunction(Function f){
	int tamano = 0;
	tamano+= sizeof(uint8_t);
	tamano+= sizeof(uint8_t);
	for(int y=0; y < f.num_args; y++){
		tamano+= sizeof(uint8_t);
		tamano+= sizeof(uint32_t);
		tamano+= f.args[y].size;
	}
	return tamano;
}

int muse_init(int id, char* ip, int puerto)
{
	config = config_create(LIBMUSE_CONFIG_PATH);
	logger = log_create("/home/utnso/workspace/tp-2019-2c-Los-Trapitos/logs/libmuse.log", "LIBMUSE", true, LOG_LEVEL_TRACE);

	if(config == NULL)
	{
		printf("Configuration couldn't be loaded.Quitting program!\n");
		return(-1);
	}
	
	MASTER_SOCKET = connect_to(ip,puerto);

	PROCESS_ID = id;

	config_destroy(config);

	return 0; 
}

int call(Function *function)
{
	Message msg;
	MessageHeader header;	

	create_message_header(&header,MESSAGE_CALL,PROCESS_ID,sizeof(Function));
	create_function_message(&msg,&header,function);

	pthread_mutex_lock(&s_socket);
	send_message(MASTER_SOCKET,&msg);
	pthread_mutex_unlock(&s_socket);

	pthread_mutex_lock(&s_socket);
	receive_message(MASTER_SOCKET,&msg);
	pthread_mutex_unlock(&s_socket);

	int *response = msg.data;
	return *response;
}

void muse_close()
{
	pthread_mutex_lock(&s_socket);
	close(MASTER_SOCKET);
	pthread_mutex_unlock(&s_socket);
	log_destroy(logger);
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
	
	log_info(logger,"Alloc llamado -> Size: %d", tam);

	int result = call(&function);

	if (result == -1)
		raise(SIGSEGV);
	
	log_info(logger,"Respuesta Alloc recibida -> se alloco correctamente", tam);

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

	log_info(logger,"Free llamado -> Direccion: %d", dir);

	call(&function);

	log_info(logger,"Respuesta Free recibida -> Se ha liberado la direccion de memoria correctamente");

	return;
}

int muse_get(void* dst, uint32_t src, size_t n)
{
	Function function;
	Arg arg[3];

	function.args[0].type = VAR_VOID_PTR;
	function.args[0].size = n;
	function.args[0].value.val_voidptr = malloc(n);
	memcpy(function.args[0].value.val_voidptr, dst, n);

	function.args[1].type = VAR_UINT32;
	function.args[1].size = sizeof(uint32_t);
	function.args[1].value.val_u32 = src;

	function.args[2].type = VAR_SIZE_T;
	function.args[2].size = sizeof(size_t);
	function.args[2].value.val_sizet = n;

	log_info(logger,"Get llamado -> Direccion origen: %d y Size: %d", src, n);

	function.type = FUNCTION_GET;
	function.num_args = 3;

	Message msg;
	MessageHeader header;

	create_message_header(&header, MESSAGE_CALL, PROCESS_ID, tamDataFunction(function));
	create_function_message(&msg, &header, &function);

	pthread_mutex_lock(&s_socket);
	if(send_message(MASTER_SOCKET,&msg) == -1){
		//error no se pudo enviar
		return -1;
	}
	pthread_mutex_unlock(&s_socket);

	free(function.args[0].value.val_voidptr);

	pthread_mutex_lock(&s_socket);
	receive_message_var(MASTER_SOCKET, &msg);
	pthread_mutex_unlock(&s_socket);

	Function* f = msg.data;

	if(f->type != RTA_FUNCTION_GET && f->type != RTA_FUNCTION_GET_ERROR){
		log_error(logger,"Respuesta Get recibida -> No se ha podido recibir el contenido de esa direccion");
		return -1;
	}

	if(f->type == RTA_FUNCTION_GET_ERROR){
		if(f->args[0].value.val_u32 == -1)
			log_error(logger,"Se trato de acceder a un archivo unmmaped");
		if(f->args[0].value.val_u32 == -2)
			log_error(logger, "El archivo mmap es privado y el programa no tiene permisos");
		if(f->args[0].value.val_u32 == -3)
			log_error(logger, "Estas tratando de leer mas del limite del segmento o el mismo no existe");

		log_info(logger,"Respuesta Get recibida -> ERROR");

		raise(SIGSEGV);
	}
	else{
		memcpy(dst, f->args[0].value.val_voidptr, f->args[0].size);
		free(f->args[0].value.val_voidptr);
		log_info(logger,"El tamaño de lo recibido es: %d", f->args[0].size);
		log_info(logger,"Respuesta Get recibida -> Se ha recibido el contenido de la direccion correctamente");
	}


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

	log_info(logger,"Cpy llamado -> Direccion destino: %d y Size: %d", dst, n);

	function.type = FUNCTION_COPY;
	function.num_args = 3;
	function.args[0] = arg[0];
	function.args[2] = arg[2];

	int result = call(&function);

	if(result == -1){
		log_error(logger, "Se trata de acceder a una direccion unmapped");
		raise(SIGSEGV);
	}

	if(result == -2){
		log_error(logger, "El archivo mmap es privado y el programa no tiene permisos");
		raise(SIGSEGV);
	}

	if(result == -3){
		log_error(logger, "Se busco el segmento, pero no se encontro");
		raise(SIGSEGV);
	}

	log_info(logger,"Respuesta Cpy recibida -> Se copiaron %d bytes en la direccion %d", n, dst);

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

	log_info(logger,"Map llamado -> Path: %s, Length: %d, Flag: %d", path, length, flags);

	int result = call(&function);

	if(result == -1){
		log_error(logger,"Respuesta Map recibida -> No se ha podido realizar el mapeo");
	}

	log_info(logger,"Respuesta Map recibida -> Se ha realizado el mapeo correctamente");

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

	log_info(logger,"Sync llamado -> Direccion: %d, Length: %d", addr, len);

	int result = call(&function);
	if (result == -2){
		log_error(logger,"Respuesta Sync recibida -> No se ha podido realizar la sincronizacion");
		return -1;
	}

	log_info(logger,"Respuesta Sync recibida -> Se ha realizado la sincronizacion correctamente");

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

	log_info(logger,"Unmap llamado -> Direccion: %d", dir);

	int result = call(&function);

	if(result == -1){
		raise(SIGSEGV);
	}

	log_info(logger,"Respuesta Unmap recibida -> Se ha realizado la operacion correctamente");

	return result;
}
