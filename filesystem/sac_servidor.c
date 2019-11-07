#include <sys/mman.h>
#include "sac_servidor.h"
#include "network.h"
#include <commons/log.h>
#include <commons/config.h>
#include <signal.h>
#include <libgen.h>

t_log *fuse_logger = NULL;
fuse_configuration *fuse_config = NULL;
GBlock* disco = NULL;
GFile* tablaDeNodos = NULL;
t_bitarray* bitmap = NULL;
size_t diskSize = NULL;

uint32_t fuse_invoke_function(Function *f,uint32_t pid);

int fuse_start_service(ConnectionHandler ch)
{
	fuse_config = load_configuration(SAC_CONFIG_PATH);
	fuse_logger = log_create("../logs/fuse.log","FUSE",true,LOG_LEVEL_TRACE);
	server_start(fuse_config->listen_port,ch);
}

void fuse_stop_service()
{
	log_info(fuse_logger,"SIGINT received.Shuting down!");
	free(fuse_config);
	log_destroy(fuse_logger);
	server_stop();
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

	//cambiar esto
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

fuse_configuration *load_configuration(char *path)
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
	config_destroy(config);
	return fc;
}

/*
void message_handler(Message *m,int sock)
{
	uint32_t res= 0;
	Message msg;
	MessageHeader head;
	switch(m->header.message_type)
	{
		case MESSAGE_CALL:
			res = fuse_invoke_function((Function *)m->data,m->header.caller_id);
			log_trace(fuse_logger,"Call received!");
			message_free_data(m);
			
			create_message_header(&head,MESSAGE_FUNCTION_RET,2,sizeof(uint32_t));
			create_response_message(&msg,&head,res);
			send_message(sock,&msg);
			message_free_data(&msg);
			break;
		default:
			log_error(fuse_logger,"Undefined message");
			break;
	}
	return;

}

uint32_t fuse_invoke_function(Function *f,uint32_t pid) 
{
	uint32_t func_ret = 0;
	switch(f->type)
	{
		case FUNCTION_MALLOC:
			log_debug(muse_logger,"Malloc called with args ->%d",f->args[0].value.val_u32);
			func_ret = muse_malloc(f->args[0].value.val_u32,pid);//TODO put the caller_id in func
			break;
		case FUNCTION_FREE:
			log_debug(muse_logger,"Free called");
			func_ret = muse_free(f->args[0].value.val_u32,pid);
			break;
		case FUNCTION_GET:
			log_debug(muse_logger,"Get called");
			func_ret = muse_get(f->args[0].value.val_voidptr,
								f->args[1].value.val_u32,f->args[2].value.val_sizet,pid);
			break;
		case FUNCTION_COPY:
			log_debug(muse_logger,"Copy called with args -> arg[0] %d  arg[1] %d arg[2] %d",f->args[0].value.val_u32,f->args[1].value.val_u32,f->args[2].value.val_u32);
			func_ret = muse_cpy(f->args[0].value.val_u32,
								&f->args[1].value.val_u32,f->args[2].value.val_u32,pid);
			break;
		case FUNCTION_MAP:
			log_debug(muse_logger,"Map called");
			func_ret = muse_map(f->args[0].value.val_charptr,
								f->args[1].value.val_sizet,f->args[2].value.val_u32,pid);
			break;
		case FUNCTION_SYNC:
			log_debug(muse_logger,"Sync called");
			func_ret = muse_sync(f->args[0].value.val_u32,f->args[1].value.val_sizet,pid);
			break;
		case FUNCTION_UNMAP:
			log_debug(muse_logger,"Unmap called");
			func_ret = muse_unmap(f->args[0].value.val_u32,pid);
			break;
		default:
			log_error(muse_logger,"Unknown function");
			func_ret = 0;
			break;
	}
	return func_ret;currNode

}
*/

int main(int argc,char *argv[])
{
	signal(SIGINT,fuse_stop_service);
	fuse_start_service(handler); 
	&disco = mmap(NULL, diskSize, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED,1,0);
	tablaDeNodos = disco + 2;
	bitmap = bitarray_create_with_mode(disco + 1, BLOQUE_SIZE, LSB_FIRST);
	diskSize = fuse_config->disk_size;
	return 0;
}

int sac_server_getattr(char* payload){
	t_Rta_Getattr metadata;

	ptrGBloque nodoBuscadoPosicion = determine_node(payload->path);

	bool esArchivo = tablaDeNodos[nodoBuscadoPosicion].state == 1;

	metadata.modo = esArchivo == 1 ? S_IFREG | 0444 : S_IFDIR | 0755; //permisos default para directorios y para archivos respectivamente
	metadata.nlink_t = 1; //esto esta hardcodeado en otros tps porque no podemos crear hardlinks nosotros, asi que no tenemos como calcular cuantos tiene
	metadata.total_size = tablaDeNodos[nodoBuscadoPosicion].file_size;

	//Deberia completarse el envio del mensaje y seria eso

}

int sac_server_read(char* payload, int fd){
	t_Getatrr* p_param = getatrr_decode(payload);
	log_info(logger, "Reading: Path: %s - Size: %d - Offset %d",
			p_param->path, p_param->size, p_param->offset);
	(void) fi;
	unsigned int node_n, bloque_punteros, num_bloque_datos;
	unsigned int bloque_a_buscar; // Estructura auxiliar para no dejar choclos
	struct sac_server_gfile *node;
	ptrGBloque *pointer_block;
	char *data_block;
	size_t tam = p_param->size;
	int res;

	node_n = determine_node(p_param->path);

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

}

int sac_server_readdir (char* payload) {
	t_list* listaDeArchivos = list_create();
	int i = 0;
	ptrGBloque nodoPadre = get_nodo_path_payload(payload);

	while(i < MAX_NUMBER_OF_FILES){
		if(tablaDeNodos->state != 0)
		{
			if(tablaDeNodos->parent_dir_block == nodoPadre)
			{
				char * name = malloc(MAX_NAME_SIZE + 1 * (strlen(fname)+1));
				strcpy(name, tablaDeNodos->fname);
				*(name + MAX_NAME_SIZE + 1) = '\0';
				list_add(listaDeArchivos, fname);
				free(name);
			}
		}
		tablaDeNodos++;
	}

	//aca deberia enviarse el paquete con la lista serializada

	list_destroy(listaDeArchivos);
	return 0;
}

int sac_server_mknod (char* payload){
	crear_nuevo_nodo(payload, 1);
	return 0;
}

int sac_server_mkdir (char* payload){
	crear_nuevo_nodo(payload, 2);
	return 0;
}

int sac_server_unlink (char* payload){
	
	ptrGBloque nodoPath = get_nodo_path_payload(payload);
	//tengo que validar si no existe el nodo para ese path
	GFile* nodoABorrar = tablaDeNodos + nodoPath;

	borrar_archivo(nodoABorrar, nodoPath);

	return 0;
}

int sac_server_rmdir (char* payload){
	ptrGBloque nodoPadre = get_nodo_path_payload(payload);
	//validar que no sea el directorio raiz

	borrar_directorio(nodoPadre);

	return 0;
}

void borrar_directorio (ptrGBloque nodoPosicion){
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
}

void borrar_archivo(GFile* nodo, ptrGBloque nodoPosicion){
	nodo->state = 0;

	bitarray_clean_bit(bitmap, nodoPosicion + 2);
}

ptrGBloque get_nodo_path_payload(char* payload){
	t_Path* tipoPath = malloc(sizeof(t_Path));

	path_decode(payload, tipoPath);
	return determine_node(tipoPath->path);
}

void crear_nuevo_nodo (char* payload, int tipoDeArchivo){
	int currNode = 0;
	t_Path *tipoPath = malloc(sizeof(t_Path));
	char *parentDirPath;
	char *fileName;

	while(tablaDeNodos[currNode].state != 0 && currNode < MAX_NUMBER_OF_FILES){
		currNode++;
	}

	if (currNode >= MAX_NUMBER_OF_FILES) 
	{
		return EDQUOT;
	}

	path_decode(payload, tipoPath);
	parentDirPath = dirname(tipoPath->path);
	ptrGBloque nodoPadre = determine_node(parentDirPath);

	fileName = basename(payload->path);

	GFile *nodoVacio = tablaDeNodos + currNode;

	strcpy((char*) nodoVacio->fname, fileName+1);
	nodoVacio->file_size = 0;
	nodoVacio->parent_dir_block = nodoPadre;
	nodoVacio->state = tipoDeArchivo;
	// nodoVacio->create_date
	// nodoVacio->modify_date

	bitarray_set_bit(bitmap, currNode + 2);

	msync(disco, diskSize, MS_SYNC);
}
