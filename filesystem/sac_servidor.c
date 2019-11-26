#include "sac_servidor.h"
#include <sys/stat.h>
#include <fcntl.h>
#include "sac_handlers.h"

t_log *fuse_logger;
fuse_configuration *fuse_config;

Function fuse_invoke_function(Function *f);

void fuse_stop_service()
{
	log_info(fuse_logger,"SIGINT recibida. Servidor desconectado!");
	free(fuse_config->path_archivo);
	free(fuse_config);
	log_destroy(fuse_logger);
	server_stop();
}

fuse_configuration* load_configuration(char *path)
{
	t_config *config = config_create(path);
	fuse_configuration *fc = (fuse_configuration *)malloc(sizeof(fuse_configuration));
	if(config == NULL)
	{
		log_error(fuse_logger,"No se pudo cargar la configuracion. Apagando servidor...!");
		fuse_stop_service();
		exit(-1);
	}

	fc->path_archivo = malloc(strlen(config_get_string_value(config,"PATH_ARCHIVO")) + 1); //aca hay memory leaks

	fc->listen_port = config_get_int_value(config,"LISTEN_PORT");
	fc->disk_size = config_get_int_value(config,"DISK_SIZE");
	strcpy(fc->path_archivo,config_get_string_value(config,"PATH_ARCHIVO"));

	config_destroy(config);
	return fc;
}

void inicioTablaDeNodos(){
	int tamanioHeader = 1;
	double diskSize = (double) fuse_config->disk_size;
	double tamanioBloque = (double) BLOQUE_SIZE;
	double divisionUno = diskSize / tamanioBloque;
	double divisionDos = divisionUno / (double) 8;
	double divisionTres = divisionDos / tamanioBloque;
	int tamanioBitmapBloque = ceil(divisionTres);

	bloqueInicioTablaDeNodos = tamanioHeader + tamanioBitmapBloque;
}

int inicioBloquesDeDatos(){
	return bloqueInicioTablaDeNodos + MAX_NUMBER_OF_FILES;
}

/*int finBloquesDeDatos(){
	return inicioBloquesDeDatos + ;
}*/

void configurar_server(){
	int fileDescriptor = 0;
	diskSize = fuse_config->disk_size;
	char *pathArchivo = fuse_config->path_archivo;
	fileDescriptor = open(pathArchivo, O_RDWR, 0);

	disco = (GBlock*) mmap(NULL, diskSize, PROT_READ|PROT_WRITE, MAP_SHARED,fileDescriptor,0);

	inicioTablaDeNodos();

	tablaDeNodos = (GFile*) (disco + bloqueInicioTablaDeNodos);

	printf("\033[0;34m");
	printf("-----------Contenido de la tabla de nodos-----------\n");
	printf("\033[0m");

	//es para monitorear los archivos que hay en el filesystem, solo para testing
	for(int i=0; i<100; i++){

		printf("Nodo:%d Bloque:%d Estado:%d Nombre:%s Bloque padre:%d\n",
						i,
						bloqueInicioTablaDeNodos + i,
						tablaDeNodos[i].state,
						tablaDeNodos[i].fname,
						tablaDeNodos[i].parent_dir_block);
		printf("------------------------------------------------------------\n");

	}

	printf("\033[0;34m");
	printf("-----------Fin tabla de nodos-----------\n");
	printf("\033[0m");

	bitmap = bitarray_create_with_mode((char *)(disco + 1), BLOQUE_SIZE, LSB_FIRST);
}

void fuse_start_service(ConnectionHandler ch)
{
	fuse_config = load_configuration(SAC_CONFIG_PATH);
	fuse_logger = log_create("/home/utnso/tp-2019-2c-Los-Trapitos/logs/fuse.log","FUSE",true,LOG_LEVEL_TRACE);
	configurar_server();
	//fuse_logger = log_create("../logs/fuse.log","FUSE",true,LOG_LEVEL_TRACE);
	server_start(fuse_config->listen_port,ch);
}

void message_handler(Message *m,int sock)
{
	Function frespuesta;
	Message msg;
	MessageHeader head;
	switch(m->header.message_type)
	{
		case MESSAGE_CALL:
			frespuesta = fuse_invoke_function((Function *)m->data);
			log_trace(fuse_logger,"Generando respuesta...");
			create_message_header(&head,MESSAGE_CALL,1,sizeof(Function));
			create_function_message(&msg,&head,&frespuesta);
			send_message(sock,&msg);

			liberarMemoria(&frespuesta);

			liberarMemoria((Function *)m->data);
			free(m->data);
			m->data = NULL;

			break;
		default:
			log_error(fuse_logger,"Undefined message");
			break;
	}
}

void* handler(void *args)
{
	ConnectionArgs *conna = (ConnectionArgs *)args;
	Message msg;
	char buffer[1024];
	int n=0;
	int sock = conna->client_fd;
	struct sockaddr_in client_address = conna->client_addr;

	printf("Un cliente se ha conectado!\n");

	while((n=receive_packet(sock,buffer,1024)) > 0)
	{
		if((n = message_decode(buffer,n,&msg)) > 0)
		{
			message_handler(&msg,sock);
			memset(buffer,'\0',1024);
		}
		else{
			liberarMemoria((Function *)&msg);
			free(msg.data);
			msg.data = NULL;
		}
	}	
	log_debug(fuse_logger,"El cliente se desconecto!");
	close(sock);
	return (void*)NULL;
}

Function fuse_invoke_function(Function *f)
{
	Function func_ret;
	switch(f->type)
	{
		case FUNCTION_GETATTR:
			log_debug(fuse_logger,"Getattr llamado");
			func_ret = sac_server_getattr(f->args[0].value.val_charptr);
			break;
		case FUNCTION_READDIR:
			log_debug(fuse_logger,"Readdir llamado");
			func_ret = sac_server_readdir(f->args[0].value.val_charptr);
			break;
		case FUNCTION_OPEN:
			log_debug(fuse_logger,"Open llamado");
			func_ret = sac_server_open(f->args[0].value.val_charptr);
			break;
		case FUNCTION_READ:
			log_debug(fuse_logger,"Read llamado with -> Path: %s  Size: %d Offset: %d", f->args[2].value.val_charptr, f->args[0].value.val_sizet, f->args[1].value.val_u32);
			func_ret = sac_server_read(f->args[2].value.val_charptr, f->args[0].value.val_sizet, f->args[1].value.val_u32);
			break;
		case FUNCTION_OPENDIR:
			log_debug(fuse_logger,"Opendir llamado");
			func_ret = sac_server_opendir(f->args[0].value.val_charptr);
			break;
		case FUNCTION_MKNOD:
			log_debug(fuse_logger,"Mknod llamado");
			func_ret = sac_server_mknod(f->args[0].value.val_charptr);
			break;
		case FUNCTION_WRITE:
			log_debug(fuse_logger,"Write llamado");
			func_ret = sac_server_write(f->args[0].value.val_charptr, f->args[1].value.val_charptr, f->args[2].value.val_sizet, f->args[3].value.val_u32);
			break;
		case FUNCTION_UNLINK:
			log_debug(fuse_logger,"Unlink llamado");
			func_ret = sac_server_unlink(f->args[0].value.val_charptr);
			break;
		case FUNCTION_MKDIR:
			log_debug(fuse_logger,"Mkdir llamado");
			func_ret = sac_server_mkdir(f->args[0].value.val_charptr);
			break;
		case FUNCTION_RMDIR:
			log_debug(fuse_logger,"Rmdir llamado");
			func_ret = sac_server_rmdir(f->args[0].value.val_charptr);
			break;
		default:
			log_error(fuse_logger,"Funcion desconocida");
			break;
	}
	return func_ret;

}

int main(int argc,char *argv[])
{
	signal(SIGINT,fuse_stop_service);

	fuse_start_service(handler);

	return 0;
}

Function sac_server_open(char* path){
	return validarSiExiste(path, FUNCTION_RTA_OPEN);
}

Function sac_server_opendir(char* path){
	return validarSiExiste(path, FUNCTION_RTA_OPENDIR);
}

Function sac_server_getattr(char* path){
	Message msg;
	Function fsend;
	Arg arg[3];

	uint32_t modo;
	uint32_t nlink_t;
	uint32_t total_size;

	if(!strcmp(path, "/")){
		Arg argNodoRaiz[2];
		argNodoRaiz[0].type = VAR_UINT32;
		argNodoRaiz[0].size = sizeof(uint32_t);
		argNodoRaiz[0].value.val_u32 = (S_IFDIR | 0777);

		argNodoRaiz[1].type = VAR_UINT32;
		argNodoRaiz[1].size = sizeof(uint32_t);
		argNodoRaiz[1].value.val_u32 = 1;

		fsend.type = FUNCTION_RTA_GETATTR_NODORAIZ;
		fsend.num_args = 2;
		fsend.args[0] = argNodoRaiz[0];
		fsend.args[1] = argNodoRaiz[1];
		return fsend;
	}

	int nodoBuscadoPosicion = determine_nodo(path);

	if(nodoBuscadoPosicion == -1){
		return retornar_error(fsend);
	}

	bool esArchivo = tablaDeNodos[nodoBuscadoPosicion].state == 1;

	modo = esArchivo == 1 ? (S_IFREG | 0777) : (S_IFDIR | 0777);
	nlink_t = 1;
	total_size = tablaDeNodos[nodoBuscadoPosicion].file_size;

	arg[0].type = VAR_UINT32;
	arg[0].size = sizeof(uint32_t);
	arg[0].value.val_u32 = modo;

	arg[1].type = VAR_UINT32;
	arg[1].size = sizeof(uint32_t);
	arg[1].value.val_u32 = nlink_t;

	arg[2].type = VAR_UINT32;
	arg[2].size = sizeof(uint32_t);
	arg[2].value.val_u32 = total_size;

	fsend.type = FUNCTION_RTA_GETATTR;
	fsend.num_args = 3;
	fsend.args[0] = arg[0];
	fsend.args[1] = arg[1];
	fsend.args[2] = arg[2];

	return fsend;
}

/*void leer_primer_puntero_completo(int nodo, size_t size, uint32_t offset){
	int punteroInicial = ceil(offset / (PUNTEROS_A_BLOQUES_DATOS * BLOQUE_SIZE)) - 1;
	int punteroABloqueInicial = ceil((offset % (PUNTEROS_A_BLOQUES_DATOS * BLOQUE_SIZE)) / BLOQUE_SIZE) - 1;
	int byteDelBloqueInicial = ((offset % (PUNTEROS_A_BLOQUES_DATOS * BLOQUE_SIZE)) % BLOQUE_SIZE) - 1;

	ptrGBloque bloqueDePunterosPosicionInicial = tablaDeNodos[nodo].indirect_blocks_array[punteroInicial];
	punterosBloquesDatos *bloqueDePunterosDatosInicial = (punterosBloquesDatos *) (disco + bloqueDePunterosPosicionInicial);

	for(int i = punteroABloqueInicial; i < PUNTEROS_A_BLOQUES_DATOS; i++){

		ptrGBloque bloqueDeDatosPosicionInicial = bloqueDePunterosDatosInicial->punteros_a_bloques[i];

		GBlock *bloque = (GBlock *) (disco + bloqueDeDatosPosicionInicial);

		for(int i = byteDelBloqueInicial; i < )
	}
}*/

/*int leer_archivo(int nodo, size_t size, uint32_t offset){

	int punteroFinal = ceil((offset + size) / (PUNTEROS_A_BLOQUES_DATOS * BLOQUE_SIZE)) - 1;
	int punteroABloqueFinal = ceil(((offset + size) % (PUNTEROS_A_BLOQUES_DATOS * BLOQUE_SIZE)) / BLOQUE_SIZE) - 1;
	int byteDelBloqueFinal = (((offset + size) % (PUNTEROS_A_BLOQUES_DATOS * BLOQUE_SIZE)) % BLOQUE_SIZE) - 1;

	//validar que el tamanio no sea mas grande que lo que tiene el archivo

}*/

Function sac_server_read(char* path, size_t size, uint32_t offset){
	Message msg;
	Function fsend;
	int sizeRespuesta;

	char* buffer[size];
	memset(buffer, 0, size);

	sizeRespuesta = leer_archivo(*buffer, path, size, offset);

	fsend.type = FUNCTION_RTA_READ;
	fsend.num_args = 1;
	fsend.args[0].type = VAR_CHAR_PTR;
	fsend.args[0].size = sizeRespuesta;
	fsend.args[0].value.val_charptr = malloc(fsend.args[0].size);
	memcpy(fsend.args[0].value.val_charptr, buffer, fsend.args[0].size);

	return fsend;
}

Function sac_server_write(char* path, char* buf, size_t size, uint32_t offset){
	//TODO
	Function f;
	return f;
}

Function sac_server_readdir (char* path) {
	Message msg;
	Function fsend;
	char* listaNombres;

	t_list* listaDeArchivos = list_create();

	if(!strcmp(path, "/")){
		completar_lista_con_fileNames(listaDeArchivos, 0);
	}else{
		int nodoPadre = determine_nodo(path);
		completar_lista_con_fileNames(listaDeArchivos, nodoPadre + bloqueInicioTablaDeNodos);
	}

	if(list_size(listaDeArchivos) == 0){
		char* stringVacia[1];
		stringVacia[0] = '\0';

		fsend.type = FUNCTION_RTA_READDIR;
		fsend.num_args = 1;
		fsend.args[0].type = VAR_CHAR_PTR;
		fsend.args[0].size = 1;
		fsend.args[0].value.val_charptr = malloc(fsend.args[0].size); //Aca hay memory leaks
		memcpy(fsend.args[0].value.val_charptr, stringVacia, fsend.args[0].size);

		list_destroy(listaDeArchivos);

		return fsend;
	}

	//Trate de sacar factor comun y delegar este cacho de codigo que se repite
	//Pero me tira warnings y no logre hacerlo, rarisimo

	lista_a_string(listaDeArchivos, &listaNombres);

	fsend.type = FUNCTION_RTA_READDIR;
	fsend.num_args = 1;
	fsend.args[0].type = VAR_CHAR_PTR;
	fsend.args[0].size = strlen(listaNombres) + 1;
	fsend.args[0].value.val_charptr = malloc(fsend.args[0].size);
	memcpy(fsend.args[0].value.val_charptr, listaNombres, fsend.args[0].size);

	list_destroy(listaDeArchivos);
	free(listaNombres);

	return fsend;
}

Function sac_server_mknod (char* path){
	int respuesta = crear_nuevo_nodo(path, 1);
	return enviar_respuesta_basica(respuesta, FUNCTION_RTA_MKNOD);
}

Function sac_server_mkdir (char* path){
	int respuesta = crear_nuevo_nodo(path, 2);
	return enviar_respuesta_basica(respuesta, FUNCTION_RTA_MKDIR);

}

Function sac_server_unlink (char* path){
	int nodoABorrarPosicion = determine_nodo(path);
	GFile *nodoABorrar = tablaDeNodos + nodoABorrarPosicion;

	int respuesta = borrar_archivo(nodoABorrar, nodoABorrarPosicion);
	return enviar_respuesta_basica(respuesta, FUNCTION_RTA_UNLINK);
}

Function sac_server_rmdir (char* path){
	int nodoABorrarPosicion = determine_nodo(path);
	GFile *nodoABorrar = tablaDeNodos + nodoABorrarPosicion;

	int respuesta = borrar_directorio(nodoABorrar, nodoABorrarPosicion);
	return enviar_respuesta_basica(respuesta, FUNCTION_RTA_RMDIR);
}

void liberarMemoria(Function* f){
	switch(f->type){
		case FUNCTION_GETATTR:
		case FUNCTION_READDIR:
		case FUNCTION_OPEN:
		case FUNCTION_OPENDIR:
		case FUNCTION_MKNOD:
		case FUNCTION_UNLINK:
		case FUNCTION_MKDIR:
		case FUNCTION_RMDIR:
		case FUNCTION_RTA_READ:
		case FUNCTION_RTA_READDIR:
			free(f->args[0].value.val_charptr);
			f->args[0].value.val_charptr = NULL;
			break;
		case FUNCTION_READ:
			free(f->args[2].value.val_charptr);
			f->args[2].value.val_charptr = NULL;
			break;
		case FUNCTION_WRITE:
			free(f->args[0].value.val_charptr);
			f->args[0].value.val_charptr = NULL;
			break;
			free(f->args[1].value.val_charptr);
			f->args[1].value.val_charptr = NULL;
			break;
		default:
			break;
	}
}
