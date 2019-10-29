#include "sac_cliente.h"
#include "protocol.h"
#include <stddef.h>
#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

int sac_cliente_getattr(const char *path, struct stat *stbuf) {
	t_GetAttrResp* attr = malloc(sizeof(t_GetAttrResp));
	tPaquete* paquete = malloc(sizeof(tPaquete));

	memset(stbuf, 0, sizeof(struct stat));

	path_encode(FF_GETATTR, path, paquete);

	send_package(serverSocket, paquete, logger, "Se envia el path del cual se necesita la metadata");

	free(paquete);

	tMensaje tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe la estructura con la metadata");

	if(tipoDeMensaje != RTA_GETATTR){
		return -ENOENT;
	}

	deserializar_Gettattr_Resp(payload, attr);

	stbuf->st_mode = attr.modo;
	stbuf->st_nlink = attr.nlink;
	stbuf->st_size = attr.total_size;
	
	free(attr);

	return 0;
}


//quede aca en la correcion con el cambio en el protocolo
int sac_cliente_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	(void) offset;
	(void) fi;

	tPaquete paquete;
	char* path_;
	DesReaddir_resp respuesta;
	uint8_t tam_dir = 0;
	uint16_t off = 0;

	int pathSize = strlen(path) + 1;
	path_ = malloc(pathSize);
	memcpy(path_, path, pathSize);

	paquete.type = FF_READDIR;
	paquete.length = pathSize;
	memcpy(paquete.payload, path_, pathSize);

	send_package(master_socket, &paquete, logger, "Se envia el path del cual se necesita la lista de archivos que contiene");

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(master_socket, &tipoDeMensaje, &payload, logger, "Se recibe la estructura con la lista");


	if(tipoDeMensaje != FF_READDIR){
		free(path_);
		return -ENOENT;
	}

	//No se si deberia ser asi
	respuesta = deserializar_Readdir_Rta(tamano_paquete, payload);

	while(off < respuesta.tamano){

		memcpy(&tam_dir, respuesta.lista_nombres + off, sizeof(uint8_t));
		off = off + sizeof(uint8_t);
		mandar = malloc(tam_dir);
		memcpy(mandar, respuesta.lista_nombres+ off, tam_dir);
		off = off + tam_dir;
		filler(buf, mandar, NULL, 0);
		free(mandar);

	};

	free(path_);

	return 0;
}

int sac_cliente_open(const char *path, struct fuse_file_info *fi) {
	tPaquete paquete;

	path_encode(FF_OPEN, path, &paquete);

	send_package(serverSocket, &paquete, logger, "Se envia el path a abrir");

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe si ha funcionado correctamente o no la operacion");

	int respuesta = deserializar_Resultado_Resp(payload);
	
	return respuesta;
}

int sac_cliente_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

	tPaquete paquete;
	t_Read parametros;
	char *lectura;
	
	parametros.path = path;
	parametros.offset = offset;
	parametros.size = size;

	read_encode(FF_READ, parametros, &paquete);

	send_package(serverSocket, &paquete, logger, "Se envia el path a leer");

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe la lectura del path");

	//no se si aca deberia tener el tamanio del paquete
	lectura = deserializar_Read_Resp(payload);

	//memcpy(buf, lectura, tamanio de lectura);
  
  	//return tamanio de la lectura;
}

int sac_cliente_mknod(const char* path, mode_t mode, dev_t rdev){
	tPaquete paquete;

	path_encode(FF_MKNOD, path, &paquete);

	send_package(serverSocket, &paquete, logger, "Se envia el path donde crear el archivo");

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe si ha funcionado correctamente o no la operacion");

	int respuesta = deserializar_Resultado_Resp(payload);

	return respuesta;
}

int sac_cliente_write(const char* path, char *buf, size_t size, off_t offset, struct fuse_file_info* fi){
	tPaquete paquete;
	t_Write parametros;

	parametros.buf = buf;
	parametros.size = size;
	parametros.path = path;
	parametros.offset = offset;

	write_encode(FF_WRITE, parametros, &paquete);

	send_package(serverSocket, &paquete, logger, "Se envian los datos para escribir en el path");

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe la cantidad de bytes escritos");

	int escritos = deserializar_Write_Resp(payload);

	return escritos;
}

int sac_cliente_unlink(const char* path){
	tPaquete paquete;

	path_encode(FF_UNLINK, path, &paquete);

	send_package(serverSocket, &paquete, logger, "Se envia el path donde debe borrar");

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe si ha funcionado correctamente o no la operacion");

	int respuesta = deserializar_Resultado_Resp(payload);

	return respuesta;
}

int sac_cliente_mkdir(const char* path, mode_t mode){
	tPaquete paquete;

	path_encode(FF_MKDIR, path, &paquete);

	send_package(serverSocket, &paquete, logger, "Se envia el path del directorio a crear");

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe si ha funcionado correctamente o no la operacion");

	int respuesta = deserializar_Resultado_Resp(payload);

	return respuesta;
}

int sac_cliente_rmdir(const char* path){
	tPaquete paquete;

	path_encode(FF_RMDIR, path, &paquete);

	send_package(serverSocket, &paquete, logger, "Se envia el path del directorio a borrar");

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe si ha funcionado correctamente o no la operacion");

	int respuesta = deserializar_Resultado_Resp(payload);

	return respuesta;
}

// Dentro de los argumentos que recibe nuestro programa obligatoriamente
// debe estar el path al directorio donde vamos a montar nuestro FS
int main(int argc, char *argv[]) {
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	// Limpio la estructura que va a contener los parametros
	memset(&runtime_options, 0, sizeof(struct t_runtime_options));

	// Esta funcion de FUSE lee los parametros recibidos y los intepreta
	if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1){
		* error parsing options
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}

	// Si se paso el parametro --welcome-msg
	// el campo welcome_msg deberia tener el
	// valor pasado
	if( runtime_options.welcome_msg != NULL ){
		printf("%s\n", runtime_options.welcome_msg);
	}

	// Esta es la funcion principal de FUSE, es la que se encarga
	// de realizar el montaje, comuniscarse con el kernel, delegar todo
	// en varios threads
	return fuse_main(args.argc, args.argv, &sac_cliente_oper, NULL);
}
