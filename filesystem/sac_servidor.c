#include <sys/mman.h>
#include "protocol.h"
#include "sac_servidor.h"
#include "network.h"
#include "rpc.h"
#include <commons/log.h>
#include <commons/config.h>
#include <signal.h>


t_log *sac_logger = NULL;
sac_configuration *sac_config = NULL;

void *handler(void *args);
sac_configuration *load_configuration(char *path);

int sac_start_service(ConnectionHandler connectionHandler)
{
	sac_config = load_configuration(SAC_CONFIG_PATH);
	sac_logger = log_create("../logs/sac_server.log","FUSE",true,LOG_LEVEL_TRACE);
	server_start(sac_config->listen_port,connectionHandler);
}

void sac_stop_service()
{
	log_info(sac_logger,"Received SIGINT signal shuting down!");

	log_destroy(sac_logger);
	server_stop();
}

void *handler(void *args)
{
	ConnectionArgs *conna = (ConnectionArgs *)args;
	char* payload;
	int n=0;
	tMessage tipoDeMensaje;
	int sock = conna->client_fd;
	struct sockaddr_in client_address = conna->client_addr;
	
	while((n=recieve_package(sock, &tipoDeMensaje, &payload, sac_logger, "Se recibe la operacion en el servidor")) > 0)
	{
		// este if no se como adaptarlo a lo nuestro
		//if((n = message_decode(buffer,n,&msg)) > 0)
		{
			//deberia fijarme si esto lo pase bien o si tengo que ponerle el &
			message_handler(tipoDeMensaje, payload, sock);
		}
	}	
	log_debug(sac_logger,"The client was disconnected!");
	close(sock);
	return (void*)NULL;
}

sac_configuration *load_configuration(char *path)
{
	t_config *config = config_create(path);
	sac_configuration *sc = (sac_configuration *)malloc(sizeof(sac_configuration));

	if(config == NULL)
	{	
		log_error(sac_logger,"Configuration couldn't be loaded.Quitting program!");
		free(sc);
		exit(-1);
	}
	
	sc->listen_port = config_get_int_value(config,"LISTEN_PORT");
	config_destroy(config);
	return sc;
}

void message_handler(tMessage tipoDeMensaje, char* payload, int sock)
{
	switch(tipoDeMensaje)
	{
		case FF_GETATTR:
			log_debug(sac_logger,"Getattr recibido!");
			sac_server_getattr(payload);
			break;
		case FF_READDIR:
			log_debug(sac_logger,"Readdir recibido!");
			sac_server_readdir(payload);
			break;
		case FF_READ:
			log_debug(sac_logger,"Read recibido!");
			sac_server_read(payload);
			break;
		case FF_MKNOD:
			log_debug(sac_logger,"Mknod recibido!");
			sac_server_mknod(payload);
			break;
		default:
			log_error(sac_logger,"Undefined message");
			break;
	}
	return;

}

int main(int argc,char *argv[])
{
	//When Ctrl-C is pressed stops SUSE and frees resources
	signal(SIGINT,sac_stop_service);

	sac_start_service(handler);
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
	//declaro una lista o un array no se como funciona asi que lo pongo en pseudocodigo por ahora
	//listaRespuesta es la que se envia con los nombres de los archivos y directorios
	t_Path* tipoPath = malloc(sizeof(t_Path));

	path_decode(payload, tipoPath);

	ptrGBloque nodoPadre = determine_node(tipoPath->path);

	while(i < MAX_NUMBER_OF_FILES){
		if(tablaDeNodos->state != 0)
		{
			if(tablaDeNodos->parent_dir_block == nodoPadre)
			{
				//esto lo copie de otro lado asi que no se si esta bien
				char * name = malloc(MAX_NAME_SIZE + 1 * sizeof(char)); // esto esta mal, deberia ser strlen fname + 1
				strcpy(name, tablaDeNodos->fname);
				*(name + MAX_NAME_SIZE + 1) = '\0';
				//listaRespuesta[PosicionActual] = name;
			}
		}
		tablaDeNodos++;
		//tambien incrementar el indice de la lista resultado
	}
	//finalmente tendria que enviar la lista resultado al sac cli para que use la funcion filler despues
}

/*int sac_server_mknod (char* payload){
	int currNode = 0;

	size_t diskSize = //sacar el tamanio del archivo del config en algun atributo de ahi y usar esta funcion para sacarlo -> getFileSize(filename);
	GBlock* disco = mmap(NULL, diskSize, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED,1,0);

	GFile* tablaDeNodos = disco + 3;

	while(tablaDeNodos[currNode].state != 0 && currNode < MAX_NAME_SIZE){
		currNode++;
	}

	msync(disco, diskSize, MS_SYNC);
	return 0;

}

	strcpy((char*) nodoVacio->fname, payload->path+1);
	nodoVacio->file_size = 0;
	nodoVacio->state = 1;

	msync(disco, diskSize, MS_SYNC);
	return 0;

}*/
