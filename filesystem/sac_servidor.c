#include "sac_servidor.h"
#include <sys/stat.h>
#include <fcntl.h>
#include "sac_handlers.h"

t_log *fuse_logger;
fuse_configuration *fuse_config;
size_t diskSize;

Function fuse_invoke_function(Function *f);

void fuse_stop_service()
{
	log_info(fuse_logger,"SIGINT received.Shuting down!");
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
		log_error(fuse_logger,"Configuration couldn't be loaded.Quitting program!");
		fuse_stop_service();
		exit(-1);
	}

	fc->listen_port = config_get_int_value(config,"LISTEN_PORT");
	fc->disk_size = config_get_int_value(config,"DISK_SIZE");
	fc->path_archivo = config_get_string_value(config,"PATH_ARCHIVO");

	config_destroy(config);
	return fc;
}

void fuse_start_service(ConnectionHandler ch)
{
	fuse_config = load_configuration(SAC_CONFIG_PATH);
	fuse_logger = log_create("/home/utnso/tp-2019-2c-Los-Trapitos/logs/fuse.log","FUSE",true,LOG_LEVEL_TRACE);
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
			log_trace(fuse_logger,"Call received!");
			message_free_data(m->data);
			create_message_header(&head,MESSAGE_CALL,1,sizeof(Function));
			create_function_message(&msg,&head,&frespuesta);
			send_message(sock,&msg);
			message_free_data(msg.data);
			break;
		default:
			log_error(fuse_logger,"Undefined message");
			break;
	}
	return;

}

void* handler(void *args)
{
	ConnectionArgs *conna = (ConnectionArgs *)args;
	Message msg;
	char buffer[1024];
	int n=0;
	int sock = conna->client_fd;
	struct sockaddr_in client_address = conna->client_addr;

	printf("A client has connected!\n");

	while((n=receive_packet(sock,buffer,1024)) > 0)
	{
		if((n = message_decode(buffer,n,&msg)) > 0)
		{
			message_handler(&msg,sock);
			memset(buffer,'\0',1024);
		}
	}	
	log_debug(fuse_logger,"The client was disconnected!");
	close(sock);
	return (void*)NULL;
}

Function fuse_invoke_function(Function *f)
{
	Function func_ret;
	switch(f->type)
	{
		case FUNCTION_GETATTR:
			log_debug(fuse_logger,"Getattr called");
			func_ret = sac_server_getattr(f->args[0].value.val_charptr);
			break;
		case FUNCTION_READDIR:
			log_debug(fuse_logger,"Readdir called");
			func_ret = sac_server_readdir(f->args[0].value.val_charptr);
			break;
		case FUNCTION_OPEN:
			log_debug(fuse_logger,"Open called");
			func_ret = sac_server_open(f->args[0].value.val_charptr);
			break;
		case FUNCTION_READ:
			log_debug(fuse_logger,"Read called with args -> arg[0] %d  arg[1] %d arg[2] %d", f->args[0].value.val_charptr, f->args[1].value.val_sizet, f->args[2].value.val_u32);
			func_ret = sac_server_read(f->args[0].value.val_charptr, f->args[1].value.val_sizet, f->args[2].value.val_u32);
			break;
		case FUNCTION_OPENDIR:
			log_debug(fuse_logger,"Opendir called");
			func_ret = sac_server_opendir(f->args[0].value.val_charptr);
			break;
		case FUNCTION_MKNOD:
			log_debug(fuse_logger,"Mknod called");
			func_ret = sac_server_mknod(f->args[0].value.val_charptr);
			break;
		case FUNCTION_WRITE:
			log_debug(fuse_logger,"Write called");
			func_ret = sac_server_write(f->args[0].value.val_charptr, f->args[1].value.val_charptr, f->args[2].value.val_sizet, f->args[3].value.val_u32);
			break;
		case FUNCTION_UNLINK:
			log_debug(fuse_logger,"Unlink called");
			func_ret = sac_server_unlink(f->args[0].value.val_charptr);
			break;
		case FUNCTION_MKDIR:
			log_debug(fuse_logger,"Mkdir called");
			func_ret = sac_server_mkdir(f->args[0].value.val_charptr);
			break;
		case FUNCTION_RMDIR:
			log_debug(fuse_logger,"Rmdir called");
			func_ret = sac_server_rmdir(f->args[0].value.val_charptr);
			break;
		default:
			log_error(fuse_logger,"Unknown function");
			break;
	}
	return func_ret;

}

void configurar_server(){
	int fileDescriptor;
	diskSize = fuse_config->disk_size;
	char *pathArchivo = fuse_config->path_archivo;
	fileDescriptor = open(pathArchivo, O_RDONLY);

	disco = (GBlock*) mmap(NULL, diskSize, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED,fileDescriptor,0);

	//el 2 en realidad deberia estar calculado, porque depende el tamanio del archivo, el bitmap mide distinto
	tablaDeNodos = (GFile*) (disco + 2);

	bitmap = bitarray_create_with_mode((char *)(disco + 1), BLOQUE_SIZE, LSB_FIRST);
}

int main(int argc,char *argv[])
{
	signal(SIGINT,fuse_stop_service);


	fuse_start_service(handler);
	configurar_server();

	return 0;
}

Function validarSiExiste(char* path, FuncType tipoFuncion){
	Message msg;
	Function fsend;
	Arg arg[1];

	ptrGBloque nodoBuscado = determine_nodo(path);

	if(nodoBuscado == -1){
		arg[0].type = VAR_UINT32;
		arg[0].size = sizeof(uint32_t);
		arg[0].value.val_u32 = -ENOENT;
	}else{
		arg[0].type = VAR_UINT32;
		arg[0].size = sizeof(uint32_t);
		arg[0].value.val_u32 = 0;
	}

	fsend.type = tipoFuncion;
	fsend.num_args = 1;
	fsend.args[0] = arg[0];

	return fsend;
}

Function sac_server_open(char* path){
	return validarSiExiste(path, FUNCTION_RTA_OPEN);
}

Function sac_server_opendir(char* path){
	return validarSiExiste(path, FUNCTION_RTA_OPENDIR);
}

Function sac_server_write(char* path, char* buf, size_t size, uint32_t offset){
	//TODO
	Function f;
	return f;
}

Function sac_server_getattr(char* path){
	Message msg;
	Function fsend;
	Arg arg[3];

	uint32_t modo;
	uint32_t nlink_t;
	uint32_t total_size;

	ptrGBloque nodoBuscadoPosicion = determine_nodo(path);

	bool esArchivo = tablaDeNodos[nodoBuscadoPosicion].state == 1;

	modo = esArchivo == 1 ? (S_IFREG | 0777) : (S_IFDIR | 0755);
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

Function sac_server_read(char* path, size_t size, uint32_t offset){
	/*
	//t_Getatrr* p_param = getatrr_decode(payload);
	log_info(logger, "Reading: Path: %s - Size: %d - Offset %d",
	//		p_param->path, p_param->size, p_param->offset);
	(void) fi;
	unsigned int node_n, bloque_punteros, num_bloque_datos;
	unsigned int bloque_a_buscar; // Estructura auxiliar para no dejar choclos
	struct sac_server_gfile *node;
	ptrGBloque *pointer_block;
	char *data_block;
	size_t tam = p_param->size;
	int res;

	//node_n = determine_node(p_param->path);

	if (node_n == -1) return -ENOENT; // contemplo que exista el nodo

	node = node_table_start;

	// Ubica el nodo correspondiente al archivo
	node = &(node[node_n]);

	pthread_rwlock_rdlock(&rwlock); //Toma un lock de lectura.
	log_lock_trace(logger, "Read: Toma lock lectura. Cantidad de lectores: %d", rwlock.__data.__nr_readers);

	if(node->file_size <= p_param->offset){
		log_error(logger, "Se intenta leer un offset mayor o igual que el tamanio de archivo. Se retorna size 0. File: %s, Size: %d", p_param->path, node->file_size);
		// TODO enviar error al cliente
		res = 0;
		goto finalizar;
	} else if (node->file_size <= (p_param->offset + size)){
		tam = size = ((node->file_size)-(offset));
		// TODO enviar error al cliente
		log_error(logger, "Se intenta leer una posicion mayor o igual que el tamanio de archivo. Se retornaran %d bytes. File: %s, Size: %d", p_param->size, p_param->path, node->file_size);
	}
	// Recorre todos los punteros en el bloque de la tabla de nodos
	for (bloque_punteros = 0; bloque_punteros < BLKINDIRECT; bloque_punteros++){

		// Chequea el offset y lo acomoda para leer lo que realmente necesita
		if (offset > BLOCKSIZE * 1024){
			offset -= (BLOCKSIZE * 1024);
			continue;
		}

		bloque_a_buscar = (node->blk_indirect)[bloque_punteros];	// Ubica el nodo de punteros a nodos de datos, es relativo al nodo 0: Header.
		bloque_a_buscar -= (GFILEBYBLOCK + BITMAP_BLOCK_SIZE + NODE_TABLE_SIZE);	// Acomoda el nodo de punteros a nodos de datos, es relativo al bloque de datos.
		pointer_block =(ptrGBloque *) &(data_block_start[bloque_a_buscar]);		// Apunta al nodo antes ubicado. Lo utiliza para saber de donde leer los datos.
		// Recorre el bloque de punteros correspondiente.
		for (num_bloque_datos = 0; num_bloque_datos < 1024; num_bloque_datos++){
		// Chequea el offset y lo acomoda para leer lo que realmente necesita
			if (offset >= BLOCKSIZE){
				offset -= BLOCKSIZE;
				continue;
			}

			bloque_a_buscar = pointer_block[num_bloque_datos]; 	// Ubica el nodo de datos correspondiente. Relativo al nodo 0: Header.
			bloque_a_buscar -= (GFILEBYBLOCK + BITMAP_BLOCK_SIZE + NODE_TABLE_SIZE);	// Acomoda el nodo, haciendolo relativo al bloque de datos.
			data_block = (char *) &(data_block_start[bloque_a_buscar]);

			// Corre el offset hasta donde sea necesario para poder leer lo que quiere.
			if (offset > 0){
				data_block += offset;
				offset = 0;
			}

			if (tam < BLOCKSIZE){
				memcpy(buf, data_block, tam);
				buf = &(buf[tam]);
				tam = 0;
				break;
			} else {
				memcpy(buf, data_block, BLOCKSIZE);
				tam -= BLOCKSIZE;
				buf = &(buf[BLOCKSIZE]);
				if (tam == 0) break;
			}

		}

		if (tam == 0) break;
	}
	res = size;

	finalizar:
	pthread_rwlock_unlock(&rwlock); //Devuelve el lock de lectura.
	log_lock_trace(logger, "Read: Libera lock lectura. Cantidad de lectores: %d", rwlock.__data.__nr_readers);
	log_trace(logger, "Terminada lectura.");
	return res;
	*/
	Function f;
	return f;
}

Function sac_server_readdir (char* path) {
	Message msg;
	Function fsend;
	Arg arg[1];

	t_list* listaDeArchivos = list_create();
	int i = 0;
	ptrGBloque nodoPadre = determine_nodo(path);

	while(i < MAX_NUMBER_OF_FILES){
		if(tablaDeNodos->state != 0)
		{
			if(tablaDeNodos->parent_dir_block == nodoPadre)
			{
				char * name = malloc(MAX_NAME_SIZE + 1 * (strlen(tablaDeNodos->fname)+1));
				strcpy(name, tablaDeNodos->fname);
				*(name + MAX_NAME_SIZE + 1) = '\0';
				list_add(listaDeArchivos, tablaDeNodos->fname);
				free(name);
			}
		}
		tablaDeNodos++;
	}

	//Aca hay que hacer que la serializacion acepte poner un tipo de dato que sea t_list, capaz usandola como un char*
	//arg[0].type = VAR_CHAR_PTR;
	//arg[0].size = strlen(listaDeArchivos) + 1;
	//strcpy(arg[0].value.val_charptr,listaDeArchivos);

	fsend.type = FUNCTION_RTA_READDIR;
	fsend.num_args = 1;
	fsend.args[0] = arg[0];

	list_destroy(listaDeArchivos);

	return fsend;
}

/*int crear_nuevo_nodo (char* path, int tipoDeArchivo){
	int currNode = 0;
	char *parentDirPath;
	char *fileName;

	while(tablaDeNodos[currNode].state != 0 && currNode < MAX_NUMBER_OF_FILES){
		currNode++;
	}

	if (currNode >= MAX_NUMBER_OF_FILES) 
	{
		return EDQUOT;
	}

	parentDirPath = dirname(path);
	ptrGBloque nodoPadre = determine_nodo(parentDirPath);

	fileName = basename(path);

	GFile *nodoVacio = tablaDeNodos + currNode;

	strcpy((char*) nodoVacio->fname, fileName+1);
	nodoVacio->file_size = 0;
	nodoVacio->parent_dir_block = nodoPadre;
	nodoVacio->state = tipoDeArchivo;
	// nodoVacio->create_date
	// nodoVacio->modify_date

	msync(disco, diskSize, MS_SYNC);
	return 0;
}*/

Function sac_server_mknod (char* path){
	/*
	return crear_nuevo_nodo(path, 1);
	*/
	Function f;
	return f;
}

Function sac_server_mkdir (char* path){
	/*
	return crear_nuevo_nodo(path, 2);
	*/
	Function f;
	return f;
}

void borrar_directorio (ptrGBloque nodoPosicion){
	/*
	int currNode = 0;

	GFile *nodoPadre = tablaDeNodos + nodoPosicion;
	borrar_archivo(nodoPadre, nodoPosicion);

	while(currNode < MAX_NUMBER_OF_FILES){
		if(tablaDeNodos[currNode].state != 0)
		{
			if(tablaDeNodos[currNode].parent_dir_block == nodoPosicion)
			{
				GFile *nodoABorrar = tablaDeNodos + currNode;
				if(tablaDeNodos[currNode].state == 1){
					borrar_archivo(nodoABorrar, currNode);
				}else{
					borrar_directorio(currNode);
				}
			}
		}
		currNode++;
	}
	*/
}

void borrar_archivo(GFile* nodo, ptrGBloque nodoPosicion){
	/*
	nodo->state = 0;

	aca tengo que hacer el borrado de todos los bloques de datos que tenga el archivo
	*/
}

Function sac_server_unlink (char* path){
	/*
	ptrGBloque nodoPath =
	//tengo que validar si no existe el nodo para ese path
	GFile* nodoABorrar = tablaDeNodos + nodoPath;

	borrar_archivo(nodoABorrar, nodoPath);
	*/
	Function f;
	return f;
}

Function sac_server_rmdir (char* path){
	/*
	ptrGBloque nodoPadre = determine_nodo(path);
	//validar que no sea el directorio raiz

	borrar_directorio(nodoPadre);
	*/
	Function f;
	return f;
}
