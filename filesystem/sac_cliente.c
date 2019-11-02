#include "sac_cliente.h"
#include "protocol.h"
#include <stddef.h>
#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

int serverSocket = 0;

int sac_cliente_getattr(const char *path, struct stat *stbuf) {
	t_GetAttrResp* attr = malloc(sizeof(t_GetAttrResp));
	tPaquete* paquete = malloc(sizeof(tPaquete));

	memset(stbuf, 0, sizeof(struct stat));

	path_encode(FF_GETATTR, path, paquete);

	send_package(serverSocket, paquete, logger, "Se envia el path del cual se necesita la metadata");

	free(paquete->payload);
	free(paquete);

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe la estructura con la metadata");

	if(tipoDeMensaje != RTA_GETATTR){
		return -ENOENT;
	}

	deserializar_Gettattr_Resp(payload, attr);

	free(payload);

	stbuf->st_mode = attr->modo;
	stbuf->st_nlink = attr->nlink;
	stbuf->st_size = attr->total_size;
	
	free(attr);

	return EXIT_SUCCESS;
}


int sac_cliente_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	(void) fi;
	(void) offset;

	tPaquete* paquete = malloc(sizeof(tPaquete));
	t_ReaddirResp* respuesta = malloc(sizeof(t_ReaddirResp));
	uint8_t tam_dir = 0;
	uint16_t off = 0;

	path_encode(FF_READDIR, path, paquete);

	send_package(serverSocket, paquete, logger, "Se envia el path del cual se necesita la lista de archivos que contiene");

	free(paquete->payload);
	free(paquete);

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe la estructura con la lista");


	if(tipoDeMensaje != RTA_READDIR){
		return -ENOENT;
	}

	deserializar_Readdir_Rta(payload, respuesta);

	free(payload);

	char* mandar;

	while(off < respuesta->tamano){

		memcpy(&tam_dir, respuesta->lista_nombres + off, sizeof(uint8_t));
		off = off + sizeof(uint8_t);
		mandar = malloc(tam_dir);
		memcpy(mandar, respuesta->lista_nombres + off, tam_dir);
		off = off + tam_dir;
		filler(buf, mandar, NULL, 0);
		free(mandar);

	};

	free(respuesta->lista_nombres);
	free(respuesta);

	return EXIT_SUCCESS;
}

int sac_cliente_open(const char *path, struct fuse_file_info *fi) {

	//chequeo si el archivo existe, ignoro permisos

	tPaquete* paquete = malloc(sizeof(tPaquete));

	path_encode(FF_OPEN, path, paquete);

	send_package(serverSocket, paquete, logger, "Se envia el path a abrir");

	free(paquete->payload);
	free(paquete);

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe si ha funcionado correctamente o no la operacion");

	if(tipoDeMensaje != RTA_OPEN){
			return -ENOENT;
		}

	int respuesta = deserializar_int(payload);
	
	free(payload);

	return respuesta; // retorna 0 si existe, o -EACCESS en caso contrario.
}

int sac_cliente_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

	tPaquete* paquete = malloc(sizeof(tPaquete));
	t_Read parametros;
	char *lectura;
	
	parametros.pathLength = strlen(path) + 1;
	parametros.path = path;
	parametros.offset = offset;
	parametros.size = size;

	read_encode(FF_READ, parametros, paquete);

	send_package(serverSocket, paquete, logger, "Se envia el path a leer");

	free(paquete->payload);
	free(paquete);

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe la lectura del path");

	if(tipoDeMensaje != RTA_READ){
		return -ENOENT;
	}

	// se podria checkear por si se recibe payload length 0

	int tamanioLectura = deserializar_Read_Rta(&lectura, payload);

	free(payload);

	memcpy(buf, lectura, tamanioLectura);

	free(lectura);
  
  	return tamanioLectura;
}

int sac_cliente_mknod(const char* path, mode_t mode, dev_t rdev){
	tPaquete* paquete = malloc(sizeof(tPaquete));

	path_encode(FF_MKNOD, path, paquete); // ?? no envia mas info?

	send_package(serverSocket, paquete, logger, "Se envia el path donde crear el archivo");

	free(paquete->payload);
	free(paquete);

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe si ha funcionado correctamente o no la operacion");

	if(tipoDeMensaje != RTA_MKNOD){
		return -ENOENT;
	}

	int respuesta = deserializar_int(payload); // 0 exito, -1 error

	free(payload);

	return respuesta;
}

int sac_cliente_write(const char* path, char *buf, size_t size, off_t offset, struct fuse_file_info* fi){
	tPaquete* paquete = malloc(sizeof(tPaquete));
	t_Write parametros;

	parametros.pathLength = strlen(path) + 1;
	parametros.bufLength = strlen(buf) + 1;
	parametros.buf = buf;
	parametros.size = size;
	parametros.path = path;
	parametros.offset = offset;

	write_encode(FF_WRITE, parametros, paquete);

	send_package(serverSocket, &paquete, logger, "Se envian los datos para escribir en el path");

	free(paquete->payload);
	free(paquete);

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe la cantidad de bytes escritos");

	if (tipoDeMensaje != RTA_WRITE) {
		return -ENOENT;
	}


	int escritos = deserializar_int(payload);

	free(payload);

	return escritos; // podria verificar que sean los mismos que los pedidos o sino error
}

// aca voy

int sac_cliente_unlink(const char* path){
	tPaquete* paquete = malloc(sizeof(tPaquete));

	path_encode(FF_UNLINK, path, paquete);

	send_package(serverSocket, paquete, logger, "Se envia el path donde debe borrar");

	free(paquete->payload);
	free(paquete);

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe si ha funcionado correctamente o no la operacion");

	if (tipoDeMensaje != RTA_UNLINK) {
		return -ENOENT;
	}

	int respuesta = deserializar_int(payload);

	return respuesta;  // 0 exito, -1 error
}

int sac_cliente_mkdir(const char* path, mode_t mode){
	tPaquete* paquete = malloc(sizeof(tPaquete));

	path_encode(FF_MKDIR, path, paquete);

	send_package(serverSocket, paquete, logger, "Se envia el path del directorio a crear");

	free(paquete->payload);
	free(paquete);

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe si ha funcionado correctamente o no la operacion");

	if (tipoDeMensaje != RTA_MKDIR) {
		return -ENOENT;
	}

	int respuesta = deserializar_int(payload);

	free(payload);

	return respuesta; // 0 exito, -1 error
}

int sac_cliente_rmdir(const char* path){
	tPaquete* paquete = malloc(sizeof(tPaquete));

	path_encode(FF_RMDIR, path, paquete);

	send_package(serverSocket, paquete, logger, "Se envia el path del directorio a borrar");

	free(paquete->payload);
	free(paquete);

	tMessage tipoDeMensaje;
	char* payload;

	recieve_package(serverSocket, &tipoDeMensaje, &payload, logger, "Se recibe si ha funcionado correctamente o no la operacion");

	int respuesta = deserializar_int(payload);

	free(payload);

	return respuesta; // 0 exito, -1 error
}

// Dentro de los argumentos que recibe nuestro programa obligatoriamente
// debe estar el path al directorio donde vamos a montar nuestro FS
int main(int argc, char *argv[]) {
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	////////////////////////////////no se donde hacer el close del socket////////////////////////////////////////
	//esta bastante hardcodeado esto, creo que deberia tomarlo del config
	serverSocket = connect_to("127.0.0.1",8003);

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
	return fuse_main(args.argc, args.argv, &sac_oper, NULL);
}
