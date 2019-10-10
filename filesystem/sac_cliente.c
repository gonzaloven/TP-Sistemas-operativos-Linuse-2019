#include "sac_cliente.h"
#include <stddef.h>
#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

/* * @DESC
 *  Esta función va a ser llamada cuando a la biblioteca de FUSE le llege un pedido
 * para obtener la metadata de un archivo/directorio. Esto puede ser tamaño, tipo,
 * permisos, dueño, etc ...
 *
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		stbuf - Esta esta estructura es la que debemos completar
 *
 * 	@RETURN
 * 		O archivo/directorio fue encontrado. -ENOENT archivo/directorio no encontrado*/

static int sac_cliente_getattr(const char *path, struct stat *stbuf) {
	DesAttr_resp attr;
	tPaquete paquete;
	char* path_;

	memset(stbuf, 0, sizeof(struct stat));

	int pathSize = strlen(path) + 1;
	path_ = malloc(pathSize);
	memcpy(path_, path, pathSize);

	paquete.type = FF_GETATTR;
	paquete.length = pathSize;
	memcpy(paquete.payload, path_, pathSize);

	send_package(master_socket, &paquete, logger, "Se envia el path del cual se necesita la metadata");

	tMensaje tipoDeMensaje;
	char* payload;

	recieve_package(master_socket, &tipoDeMensaje, &payload, logger, "Se recibe la estructura con la metadata");

	if(tipoDeMensaje != RTA_GETATTR){
		return -ENOENT;
		free(path_);
	}
	
	free(path_);

	attr = deserializar_readdir_Rta(paquete.length, payload);

	stbuf->st_mode = attr.modo;
	stbuf->st_nlink = attr.nlink;
	stbuf->st_size = attr.total_size;
	
	return 0;
}


/* * @DESC
 *  Esta función va a ser llamada cuando a la biblioteca de FUSE le llege un pedido
 * para obtener la lista de archivos o directorios que se encuentra dentro de un directorio
 *
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		buf - Este es un buffer donde se colocaran los nombres de los archivos y directorios
 * 		      que esten dentro del directorio indicado por el path
 * 		filler - Este es un puntero a una función, la cual sabe como guardar una cadena dentro
 * 		         del campo buf
 *
 * 	@RETURN
 * 		O directorio fue encontrado. -ENOENT directorio no encontrado*/

static int sac_cliente_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
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

	tMensaje tipoDeMensaje;
	char* payload;

	recieve_package(master_socket, &tipoDeMensaje, &payload, logger, "Se recibe la estructura con la lista");


	if(tipoDeMensaje != RTA_READDIR){
		return -ENOENT;
		free(path_);
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


/* * @DESC
 *  Esta función va a ser llamada cuando a la biblioteca de FUSE le llege un pedido
 * para tratar de abrir un archivo
 *
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		fi - es una estructura que contiene la metadata del archivo indicado en el path
 *
 * 	@RETURN
 * 		O archivo fue encontrado. -EACCES archivo no es accesible*/

static int sac_cliente_open(const char *path, struct fuse_file_info *fi) {
	if (strcmp(path, DEFAULT_FILE_PATH) != 0)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}


/* * @DESC
 *  Esta función va a ser llamada cuando a la biblioteca de FUSE le llege un pedido
 * para obtener el contenido de un archivo
 *
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		buf - Este es el buffer donde se va a guardar el contenido solicitado
 * 		size - Nos indica cuanto tenemos que leer
 * 		offset - A partir de que posicion del archivo tenemos que leer
 *
 * 	@RETURN
 * 		Si se usa el parametro direct_io los valores de retorno son 0 si  elarchivo fue encontrado
 * 		o -ENOENT si ocurrio un error. Si el parametro direct_io no esta presente se retorna
 * 		la cantidad de bytes leidos o -ENOENT si ocurrio un error. ( Este comportamiento es igual
 * 		para la funcion write )*/

static int sac_cliente_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	size_t len;
	(void) fi;
	if (strcmp(path, DEFAULT_FILE_PATH) != 0)
		return -ENOENT;

	len = strlen(DEFAULT_FILE_CONTENT);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, DEFAULT_FILE_CONTENT + offset, size);
	} else
		size = 0;

  return size;
}

static int sac_cliente_mknod(const char* path, mode_t mode, dev_t rdev){
	return 0;
}

static int sac_cliente_write(const char* path, char *buf, size_t size, off_t offset, struct fuse_file_info* fi){
	return 0;
}

static int sac_cliente_unlink(const char* path){
	return 0;
}

static int sac_cliente_mkdir(const char* path, mode_t mode){
	return 0;
}

static int sac_cliente_rmdir(const char* path){
	return 0;
}


/* * Esta es la estructura principal de FUSE con la cual nosotros le decimos a
 * biblioteca que funciones tiene que invocar segun que se le pida a FUSE.
 * Como se observa la estructura contiene punteros a funciones.*/


static struct fuse_operations sac_cliente_oper = {
		.getattr = sac_cliente_getattr,
		.readdir = sac_cliente_readdir,
		.open = sac_cliente_open,
		.read = sac_cliente_read,
		.mknod = sac_cliente_mknod,
		.write = sac_cliente_write,
		.unlink = sac_cliente_unlink,
		.mkdir = sac_cliente_mkdir,
		.rmdir = sac_cliente_rmdir,
};


* keys for FUSE_OPT_ options
enum {
	KEY_VERSION,
	KEY_HELP,
};



/* * Esta estructura es utilizada para decirle a la biblioteca de FUSE que
 * parametro puede recibir y donde tiene que guardar el valor de estos*/

static struct fuse_opt fuse_options[] = {
		// Este es un parametro definido por nosotros
		CUSTOM_FUSE_OPT_KEY("--welcome-msg %s", welcome_msg, 0),

		// Estos son parametros por defecto que ya tiene FUSE
		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END,
};

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
