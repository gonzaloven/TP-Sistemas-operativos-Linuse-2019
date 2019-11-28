#include "sac_cliente.h"
#include <pthread.h>

pthread_mutex_t s_socket = PTHREAD_MUTEX_INITIALIZER;

int send_call(Function *f)
{
	Message* msg = malloc(sizeof(Message));
	MessageHeader header;

	create_message_header(&header,MESSAGE_CALL,1,sizeof(char) * tamDataFunction(*f));
	create_function_message(msg,&header,f);

	pthread_mutex_lock(&s_socket);
	int resultado = send_message(serverSocket,msg);
	pthread_mutex_unlock(&s_socket);

	free(msg);
	msg = NULL;

	return resultado;

}

int send_path(FuncType func_type, const char *path){
	Function f;

	f.args[0].type = VAR_CHAR_PTR;
	f.args[0].size = strlen(path) + 1;
	f.args[0].value.val_charptr = malloc(f.args[0].size);
	memcpy(f.args[0].value.val_charptr, path, f.args[0].size);

	f.type = func_type;
	f.num_args = 1;

	int resultado = send_call(&f);

	free(f.args[0].value.val_charptr);
	f.args[0].value.val_charptr = NULL;

	return resultado;
}

static int sac_getattr(const char *path, struct stat *stbuf) {
	Message msg;

	memset(stbuf, 0, sizeof(struct stat));

	log_info(logger,"Gettatr llamado -> Path: %s", path);

	if(send_path(FUNCTION_GETATTR, path) == -1){
		return EXIT_FAILURE;
	}

	pthread_mutex_lock(&s_socket);
	receive_message(serverSocket,&msg);
	pthread_mutex_unlock(&s_socket);

	Function* f = msg.data;

	if(f->type == FUNCTION_RTA_GETATTR_NODORAIZ){
		stbuf->st_mode = f->args[0].value.val_u32;
		stbuf->st_nlink = f->args[1].value.val_u32;

		free(msg.data);
		msg.data = NULL;
		log_info(logger,"Respuesta Getattr recibida -> Nodo raiz encontrado.");
		return EXIT_SUCCESS;
	}

	if(f->type != FUNCTION_RTA_GETATTR){
		free(msg.data);
		msg.data = NULL;
		log_error(logger,"Respuesta Getattr recibida -> El nodo buscado no existe.");
		return -ENOENT;
	}
	
	log_info(logger,"Respuesta Gettatr recibida -> Modo: %d, Links: %d, Size: %d",
					f->args[0].value.val_u32,
					f->args[1].value.val_u32,
					f->args[2].value.val_u32);

	stbuf->st_mode = f->args[0].value.val_u32; // fijarse que pegue int 32, sino agregarlo a menssage (junto con type etc) o usar char*
	stbuf->st_nlink = f->args[1].value.val_u32;
	stbuf->st_size = f->args[2].value.val_u32;

	struct timespec tiempo;
	tiempo.tv_sec = (uint64_t) f->args[3].value.val_u32;

	stbuf->st_mtim = tiempo;

	free(msg.data);
	msg.data = NULL;

	return EXIT_SUCCESS;
}


static int sac_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	(void) fi;
	(void) offset;
	Message msg;
	uint8_t tam_dir = 0;
	uint16_t off = 0;

	log_info(logger,"Readdir llamado -> Path: %s", path);

	if(send_path(FUNCTION_READDIR, path) == -1){
			return EXIT_FAILURE;
	}

	pthread_mutex_lock(&s_socket);
	receive_message(serverSocket,&msg);
	pthread_mutex_unlock(&s_socket);

	Function* f = msg.data;

	if(f->type != FUNCTION_RTA_READDIR){
		free(msg.data);
		msg.data = NULL;
		return -1;
	}

	int dimListaSpliteada;
	char **listaSpliteada;
	char* filename;

	//filler(buf, ".", NULL, 0);
	//filler(buf, "..", NULL, 0);

	if (f->args[0].value.val_charptr[0] == '\0')
	{
		free(f->args[0].value.val_charptr);
		f->args[0].value.val_charptr = NULL;
		free(msg.data);
		log_info(logger,"Respuesta Readdir recibida -> El directorio buscado esta vacio.");
		return 0;
	}

	listaSpliteada = string_split(f->args[0].value.val_charptr, "/");
	dimListaSpliteada = largoListaString(listaSpliteada);

	log_info(logger,"Respuesta Readdir recibida -> Nodos encontrados: %d", dimListaSpliteada);

	for(int i=0 ; i<dimListaSpliteada; i++){

		filename = listaSpliteada[i];
		filler(buf, filename, NULL, 0);
		free(listaSpliteada[i]);
		listaSpliteada[i] = NULL;
	}

	free(listaSpliteada);
	listaSpliteada = NULL;
	free(f->args[0].value.val_charptr);
	f->args[0].value.val_charptr = NULL;
	free(msg.data);
	msg.data = NULL;

	return EXIT_SUCCESS;
}

static int sac_open(const char *path, struct fuse_file_info *fi) {

	//chequeo si el archivo existe, ignoro permisos

	Message msg;

	log_info(logger,"Open llamado -> Path: %s", path);

	if(send_path(FUNCTION_OPEN, path) == -1){
		return EXIT_FAILURE;
	}

	pthread_mutex_lock(&s_socket);
	receive_message(serverSocket,&msg);
	pthread_mutex_unlock(&s_socket);

	Function* f = msg.data;

	if(f->type != FUNCTION_RTA_OPEN){
		free(msg.data);
		msg.data = NULL;
		log_error(logger,"Respuesta Open recibida -> No se ha encontrado el archivo.");
		return -ENOENT;
	}

	log_info(logger,"Respuesta Open recibida -> Se ha encontrado el archivo buscado.");

	int respuesta = f->args[0].value.val_u32;

	free(msg.data);
	msg.data = NULL;

	return respuesta;
}

static int sac_opendir(const char *path, struct fuse_file_info *fi){
	Message msg;

	log_info(logger,"Opendir llamado -> Path: %s", path);

	if(send_path(FUNCTION_OPENDIR, path) == -1){
		return EXIT_FAILURE;
	}

	pthread_mutex_lock(&s_socket);
	receive_message(serverSocket,&msg);
	pthread_mutex_unlock(&s_socket);

	Function* f = msg.data;

	if(f->type != FUNCTION_RTA_OPENDIR){
		free(msg.data);
		msg.data = NULL;
		log_error(logger,"Respuesta Opendir recibida -> No se ha encontrado el directorio.");
		return -ENOENT;
	}

	log_info(logger,"Respuesta Opendir recibida -> Se ha encontrado el directorio buscado.");

	int respuesta = f->args[0].value.val_u32;

	free(msg.data);
	msg.data = NULL;

	return respuesta;
}

static int sac_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

	Message msg;
	char *lectura;
	
	Function fsend;

	fsend.args[0].type = VAR_UINT32; // fijarse si pega con int32, sino agregarlo / usar char*
	fsend.args[0].size = sizeof(uint32_t);
	fsend.args[0].value.val_u32 = offset;

	fsend.args[1].type = VAR_CHAR_PTR;
	fsend.args[1].size = strlen(path) + 1;
	fsend.args[1].value.val_charptr = malloc(fsend.args[1].size);
	memcpy(fsend.args[1].value.val_charptr, path, fsend.args[1].size);

	fsend.args[2].type = VAR_UINT32; // fijarse si pega con int32, sino agregarlo / usar char*
	fsend.args[2].size = sizeof(uint32_t);
	fsend.args[2].value.val_u32 = size;

	fsend.type = FUNCTION_READ;
	fsend.num_args = 3;

	log_info(logger,"Read llamado -> Path: %s, Size: %d, Offset: %d", path, size, offset);

	if(send_call(&fsend) == -1){
		free(fsend.args[1].value.val_charptr);
		fsend.args[1].value.val_charptr = NULL;
		return EXIT_FAILURE;
	}

	free(fsend.args[1].value.val_charptr);
	fsend.args[1].value.val_charptr = NULL;

	pthread_mutex_lock(&s_socket);
	receive_message_var(serverSocket,&msg); // esto cambie
	pthread_mutex_unlock(&s_socket);

	Function* freceive = msg.data;

	if(freceive->type != FUNCTION_RTA_READ){
		free(msg.data);
		msg.data = NULL;
		log_error(logger,"Respuesta Read recibida -> No se ha podido leer el archivo.");
		return -ENOENT;
	}

	int tamanioLectura = freceive->args[0].size;

	if(tamanioLectura == 0){
		free(freceive->args[0].value.val_charptr);
		freceive->args[0].value.val_charptr = NULL;
		free(freceive);
		freceive = NULL;

		log_info(logger,"Respuesta Read recibida -> El archivo esta vacio.");
		return tamanioLectura;
	}

	log_info(logger,"Respuesta Read recibida -> Bytes leidos: %d", tamanioLectura);

	memcpy(buf, freceive->args[0].value.val_charptr, tamanioLectura);

	free(freceive->args[0].value.val_charptr);
	freceive->args[0].value.val_charptr = NULL;
	free(freceive);
	freceive = NULL;
  
  	return tamanioLectura;
}

static int sac_mknod(const char* path, mode_t mode, dev_t rdev){
	Message msg;

	log_info(logger,"Mknod llamado -> Path: %s", path);

	if(send_path(FUNCTION_MKNOD, path) == -1){
		return EXIT_FAILURE;
	}

	pthread_mutex_lock(&s_socket);
	receive_message(serverSocket,&msg);
	pthread_mutex_unlock(&s_socket);

	Function* f = msg.data;

	if(f->type != FUNCTION_RTA_MKNOD){
		free(msg.data);
		msg.data = NULL;
		log_error(logger,"Respuesta Mknod recibida -> No se ha podido crear el archivo.");
		return -1;
	}

	log_info(logger,"Respuesta Mknod recibida -> Se ha creado el archivo correctamente.");

	int respuesta = f->args[0].value.val_u32;

	free(msg.data);
	msg.data = NULL;

	return respuesta;
}

static int sac_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){

	Message msg;

	Function f;

	f.type = FUNCTION_WRITE;
	f.num_args = 3;

	f.args[1].type = VAR_CHAR_PTR;
	f.args[1].size = strlen(path) + 1;
	f.args[1].value.val_charptr = malloc(f.args[1].size);
	memcpy(f.args[1].value.val_charptr, path, f.args[1].size);

	f.args[2].type = VAR_CHAR_PTR;
	f.args[2].size = size;
	f.args[2].value.val_charptr = malloc(f.args[2].size);
	memcpy(f.args[2].value.val_charptr, buf, f.args[2].size);

	f.args[0].type = VAR_UINT32; // fijarse si pega con int32, sino agregarlo / usar char*
	f.args[0].size = sizeof(uint32_t);
	f.args[0].value.val_u32 = offset;

	log_info(logger,"Write llamado -> Path: %s, Pide Escribir: %s, Size: %d, Offset: %d", path, buf, size, offset);

	if(send_call(&f) == -1){
		free(f.args[1].value.val_charptr);
		f.args[1].value.val_charptr = NULL;
		free(f.args[2].value.val_charptr);
		f.args[2].value.val_charptr = NULL;
		return EXIT_FAILURE;
	}

	free(f.args[1].value.val_charptr);
	f.args[1].value.val_charptr = NULL;
	free(f.args[2].value.val_charptr);
	f.args[2].value.val_charptr = NULL;

	pthread_mutex_lock(&s_socket);
	receive_message_var(serverSocket,&msg);
	pthread_mutex_unlock(&s_socket);

	Function* fresp = msg.data;

	if(fresp->type != FUNCTION_RTA_WRITE){
		free(msg.data);
		msg.data = NULL;
		log_error(logger,"Respuesta Write recibida -> No se ha podido escribir el archivo.");
		return -ENOENT;
	}

	int bytesEscritos = fresp->args[0].value.val_u32;

	log_info(logger,"Respuesta Write recibida -> Bytes escritos: %d", bytesEscritos);

	free(msg.data);
	msg.data = NULL;

	return bytesEscritos; // podria verificar que sean los mismos que los pedidos o sino error
}


static int sac_unlink(const char* path){
	Message msg;

	log_info(logger,"Unlink llamado -> Path: %s", path);

	if(send_path(FUNCTION_UNLINK, path) == -1){
		return EXIT_FAILURE;
	}

	pthread_mutex_lock(&s_socket);
	receive_message(serverSocket,&msg);
	pthread_mutex_unlock(&s_socket);

	Function* fresp = msg.data;

	if(fresp->type != FUNCTION_RTA_UNLINK){
		free(msg.data);
		msg.data = NULL;
		log_error(logger,"Respuesta Unlink recibida -> No se ha podido borrar el archivo.");
		return -1;
	}

	int respuesta = fresp->args[0].value.val_u32;

	log_info(logger,"Respuesta Unlink recibida -> Se ha borrado el archivo correctamente.");

	free(msg.data);
	msg.data = NULL;

	return respuesta; //Devuelve 0 si todo bien, -1 si todo mal
}

static int sac_truncate(const char* path, off_t size){
	Message msg;
	char *lectura;

	Function fsend;

	fsend.args[0].type = VAR_UINT32; // fijarse si pega con int32, sino agregarlo / usar char*
	fsend.args[0].size = sizeof(uint32_t);
	fsend.args[0].value.val_u32 = size;

	fsend.args[1].type = VAR_CHAR_PTR;
	fsend.args[1].size = strlen(path) + 1;
	fsend.args[1].value.val_charptr = malloc(fsend.args[1].size);
	memcpy(fsend.args[1].value.val_charptr, path, fsend.args[1].size);

	fsend.type = FUNCTION_TRUNCATE;
	fsend.num_args = 2;

	log_info(logger,"Truncate llamado -> Path: %s, Size: %d", path, size);

	if(send_call(&fsend) == -1){
		free(fsend.args[1].value.val_charptr);
		fsend.args[1].value.val_charptr = NULL;
		return EXIT_FAILURE;
	}

	free(fsend.args[1].value.val_charptr);
	fsend.args[1].value.val_charptr = NULL;

	pthread_mutex_lock(&s_socket);
	receive_message_var(serverSocket,&msg);
	pthread_mutex_unlock(&s_socket);

	Function* freceive = msg.data;

	if(freceive->type != FUNCTION_RTA_TRUNCATE){
		free(msg.data);
		msg.data = NULL;
		log_error(logger,"Respuesta Truncate recibida -> No se ha podido cambiar el size del archivo.");
		return -ENOENT;
	}

	int respuesta = freceive->args[0].value.val_u32;

	log_info(logger,"Respuesta Truncate recibida -> Se ha cambiado el size del archivo correctamente");

	free(msg.data);
	msg.data = NULL;

	return respuesta;
}


static int sac_mkdir(const char* path, mode_t mode){
	Message msg;

	log_info(logger,"Mkdir llamado -> Path: %s", path);

	if(send_path(FUNCTION_MKDIR, path) == -1){
		return EXIT_FAILURE;
	}

	pthread_mutex_lock(&s_socket);
	receive_message(serverSocket,&msg);
	pthread_mutex_unlock(&s_socket);

	Function* fresp = msg.data;

	if(fresp->type != FUNCTION_RTA_MKDIR){
		free(msg.data);
		msg.data = NULL;
		log_error(logger,"Respuesta Mkdir recibida -> No se ha podido crear el directorio.");
		return -1;
	}

	int respuesta = fresp->args[0].value.val_u32;

	log_info(logger,"Respuesta Mkdir recibida -> Se ha creado el directorio correctamente.");

	free(msg.data);
	msg.data = NULL;

	return respuesta;
}

static int sac_rename(const char *path, const char *nuevoPath){
	Message msg;
	char *lectura;

	Function fsend;

	fsend.args[0].type = VAR_CHAR_PTR;
	fsend.args[0].size = strlen(path) + 1;
	fsend.args[0].value.val_charptr = malloc(fsend.args[0].size);
	memcpy(fsend.args[0].value.val_charptr, path, fsend.args[0].size);

	fsend.args[1].type = VAR_CHAR_PTR;
	fsend.args[1].size = strlen(nuevoPath) + 1;
	fsend.args[1].value.val_charptr = malloc(fsend.args[1].size);
	memcpy(fsend.args[1].value.val_charptr, nuevoPath, fsend.args[1].size);

	fsend.type = FUNCTION_RENAME;
	fsend.num_args = 2;

	log_info(logger,"Rename llamado -> Path viejo: %s, Path nuevo: %s", path, nuevoPath);

	if(send_call(&fsend) == -1){
		free(fsend.args[1].value.val_charptr);
		fsend.args[1].value.val_charptr = NULL;
		free(fsend.args[0].value.val_charptr);
		fsend.args[0].value.val_charptr = NULL;
		return EXIT_FAILURE;
	}

	free(fsend.args[1].value.val_charptr);
	fsend.args[1].value.val_charptr = NULL;
	free(fsend.args[0].value.val_charptr);
	fsend.args[0].value.val_charptr = NULL;

	pthread_mutex_lock(&s_socket);
	receive_message_var(serverSocket,&msg);
	pthread_mutex_unlock(&s_socket);

	Function* freceive = msg.data;

	if(freceive->type != FUNCTION_RTA_RENAME){
		free(msg.data);
		msg.data = NULL;
		log_error(logger,"Respuesta Rename recibida -> Ya hay otro nodo con ese nombre en esa direccion.");
		return -EEXIST;
	}

	int respuesta = freceive->args[0].value.val_u32;

	log_info(logger,"Respuesta Rename recibida -> Se ha renombrado el archivo correctamente");

	free(msg.data);
	msg.data = NULL;

	return respuesta;
}

static int sac_rmdir(const char* path){
	Message msg;

	log_info(logger,"Rmdir llamado -> Path: %s", path);

	if(send_path(FUNCTION_RMDIR, path) == -1){
		return EXIT_FAILURE;
	}

	pthread_mutex_lock(&s_socket);
	receive_message(serverSocket,&msg);
	pthread_mutex_unlock(&s_socket);

	Function* fresp = msg.data;

	if(fresp->type != FUNCTION_RTA_RMDIR){
		free(msg.data);
		msg.data = NULL;
		log_error(logger,"Respuesta Rmdir recibida -> No se ha podido borrar el directorio.");
		return -1;
	}

	int respuesta = fresp->args[0].value.val_u32;

	log_info(logger,"Respuesta Rmdir recibida -> El directorio se ha borrado correctamente.");

	free(msg.data);
	msg.data = NULL;

	return respuesta; // retorna 0 si existe.
}

static struct fuse_operations sac_oper = {
		.getattr = sac_getattr,
		.read = sac_read,
		.readdir = sac_readdir,
		.open = sac_open,
		.opendir = sac_opendir,
		.mkdir = sac_mkdir,
		.rmdir = sac_rmdir,
		.write = sac_write,
		.mknod = sac_mknod,
		.unlink = sac_unlink,
		.truncate = sac_truncate,
		.rename = sac_rename
};

fuse_configuration* load_configuration(char *path)
{
	t_config *config = config_create(path);
	fuse_configuration *fc = (fuse_configuration *)malloc(sizeof(fuse_configuration));
	if(config == NULL)
	{
		log_error(fuse_logger,"No se pudo cargar la configuracion.");
		exit(-1);
	}

	fc->ip_cliente = malloc(strlen(config_get_string_value(config,"IP_CLIENTE")));

	strcpy(fc->ip_cliente,config_get_string_value(config,"IP_CLIENTE"));
	fc->listen_port = config_get_int_value(config,"LISTEN_PORT");

	config_destroy(config);
	return fc;
}

// Dentro de los argumentos que recibe nuestro programa obligatoriamente
// debe estar el path al directorio donde vamos a montar nuestro FS
int main(int argc, char *argv[]){
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	////////////////////////////////no se donde hacer el close del socket////////////////////////////////////////

	//Cargando Configuraciones
	fuse_config = load_configuration(SAC_CONFIG_PATH);
	//Inicializando el logger
	logger = log_create("/home/utnso/tp-2019-2c-Los-Trapitos/logs/sac_cliente.log","SAC_CLIENTE",true,LOG_LEVEL_TRACE);
	//Conectando al servidor
	serverSocket = connect_to(fuse_config->ip_cliente,fuse_config->listen_port);

	free(fuse_config->ip_cliente);

	// Esta es la funcion principal de FUSE, es la que se encarga
	// de realizar el montaje, comuniscarse con el kernel, delegar todo
	// en varios threads

	fuse_main(args.argc, args.argv, &sac_oper, NULL);

	log_info(logger,"SIGINT recibida. Cliente desconectado!");
	log_destroy(logger);

	return 0;
}
