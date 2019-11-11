#include "sac_cliente.h"

int send_call(Function *f)
{
	Message msg;
	MessageHeader header;

	create_message_header(&header,MESSAGE_CALL,1,sizeof(Function));
	create_function_message(&msg,&header,f);

	int resultado = send_message(serverSocket,&msg);

	free(msg.data);

	return resultado;
}

int send_path(FuncType func_type, const char *path){
	Function f;
	Arg arg;

	arg.type = VAR_CHAR_PTR;
	arg.size = strlen(path) + 1;
	strcpy(arg.value.val_charptr,path);

	f.type = func_type;
	f.num_args = 1;
	f.args[0] = arg;

	int resultado = send_call(&f);

	return resultado;
}

static int sac_getattr(const char *path, struct stat *stbuf) {
	Message msg;

	memset(stbuf, 0, sizeof(struct stat));

	if(send_path(FUNCTION_GETATTR, path) == -1){
		return EXIT_FAILURE;
	}

	receive_message(serverSocket,&msg);

	Function* f = msg.data;

	if(f->type != FUNCTION_RTA_GETATTR){
		return -ENOENT;
	}
	
	stbuf->st_mode = f->args[0].value.val_u32; // fijarse que pegue int 32, sino agregarlo a menssage (junto con type etc) o usar char*
	stbuf->st_nlink = f->args[1].value.val_u32;
	stbuf->st_size = f->args[2].value.val_u32;

	free(f);

	return EXIT_SUCCESS;
}


static int sac_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	(void) fi;
	(void) offset;
	Message msg;
	uint8_t tam_dir = 0;
	uint16_t off = 0;

	if(send_path(FUNCTION_READDIR, path) == -1){
			return EXIT_FAILURE;
	}

	receive_message(serverSocket,&msg);

	Function* f = msg.data;

	if(f->type != FUNCTION_RTA_READDIR){
		return -ENOENT;
	}

	char* mandar;

	while(off < f->args[0].size){

		memcpy(&tam_dir, f->args[0].value.val_charptr + off, sizeof(uint8_t));
		off = off + sizeof(uint8_t);
		mandar = malloc(tam_dir);
		memcpy(mandar, f->args[0].value.val_charptr + off, tam_dir);
		off = off + tam_dir;
		filler(buf, mandar, NULL, 0);
		free(mandar);

	};
	// no veo que le haga malloc al char*, por las dudas verificar despues
	free(f);

	return EXIT_SUCCESS;
}

static int sac_open(const char *path, struct fuse_file_info *fi) {

	//chequeo si el archivo existe, ignoro permisos

	Message msg;

	if(send_path(FUNCTION_OPEN, path) == -1){
		return EXIT_FAILURE;
	}

	receive_message(serverSocket,&msg);

	int *response = msg.data;
	int respuesta = *response;

	free(msg.data);

	return respuesta; // retorna 0 si existe, o -EACCESS en caso contrario.
}

static int sac_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

	Message msg;
	char *lectura;
	
	Function fsend;
	Arg arg[3];

	arg[0].type = VAR_CHAR_PTR;
	arg[0].size = strlen(path) + 1;
	strcpy(arg[0].value.val_charptr,path);

	arg[1].type = VAR_SIZE_T;
	arg[1].size = sizeof(size_t);
	arg[1].value.val_sizet = size;

	arg[2].type = VAR_UINT32; // fijarse si pega con int32, sino agregarlo / usar char*
	arg[2].size = sizeof(uint32_t);
	arg[2].value.val_u32 = offset;

	fsend.type = FUNCTION_READ;
	fsend.num_args = 3;
	fsend.args[0] = arg[0];
	fsend.args[1] = arg[1];
	fsend.args[2] = arg[2];

	if(send_call(&fsend) == -1){
		return EXIT_FAILURE;
	}

	receive_message(serverSocket,&msg);

	Function* freceive = msg.data;

	if(freceive->type != FUNCTION_RTA_READ){
		return -ENOENT;
	}

	int tamanioLectura = freceive->args[0].size;

	memcpy(buf, freceive->args[0].value.val_charptr, tamanioLectura);

	// no veo que le haga malloc al char*, por las dudas verificar despues
	free(freceive);
  
  	return tamanioLectura;
}

static int sac_mknod(const char* path, mode_t mode, dev_t rdev){
	Message msg;

	if(send_path(FUNCTION_MKNOD, path) == -1){
		return EXIT_FAILURE;
	}

	receive_message(serverSocket,&msg);

	int *response = msg.data;
	int respuesta = *response;

	free(msg.data);

	return respuesta; // retorna 0 si existe, o -EACCESS en caso contrario.
}

static int sac_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){

	Message msg;

	Function f;
	Arg arg[4];

	arg[0].type = VAR_CHAR_PTR;
	arg[0].size = strlen(path) + 1;
	strcpy(arg[0].value.val_charptr,path);

	arg[1].type = VAR_CHAR_PTR;
	arg[1].size = strlen(buf) + 1;
	strcpy(arg[1].value.val_charptr,buf);

	arg[2].type = VAR_SIZE_T;
	arg[2].size = sizeof(size_t);
	arg[2].value.val_sizet = size;

	arg[3].type = VAR_UINT32; // fijarse si pega con int32, sino agregarlo / usar char*
	arg[3].size = sizeof(uint32_t);
	arg[3].value.val_u32 = offset;

	f.type = FUNCTION_WRITE;
	f.num_args = 4;
	f.args[0] = arg[0];
	f.args[1] = arg[1];
	f.args[2] = arg[2];
	f.args[3] = arg[3];

	if(send_call(&f) == -1){
		return EXIT_FAILURE;
	}

	receive_message(serverSocket,&msg);

	int *response = msg.data;
	int bytesEscritos = *response;

	free(msg.data);

	return bytesEscritos; // podria verificar que sean los mismos que los pedidos o sino error
}


static int sac_unlink(const char* path){
	Message msg;

	if(send_path(FUNCTION_UNLINK, path) == -1){
		return EXIT_FAILURE;
	}

	receive_message(serverSocket,&msg);

	int *response = msg.data;
	int respuesta = *response;

	free(msg.data);

	return respuesta; // retorna 0 si existe, o -EACCESS en caso contrario.
}

static int sac_mkdir(const char* path, mode_t mode){
	Message msg;

	if(send_path(FUNCTION_MKDIR, path) == -1){
		return EXIT_FAILURE;
	}

	receive_message(serverSocket,&msg);

	int *response = msg.data;
	int respuesta = *response;

	free(msg.data);

	return respuesta; // retorna 0 si existe, o -EACCESS en caso contrario.
}

static int sac_rmdir(const char* path){
	Message msg;

	if(send_path(FUNCTION_RMDIR, path) == -1){
		return EXIT_FAILURE;
	}

	receive_message(serverSocket,&msg);

	int *response = msg.data;
	int respuesta = *response;

	free(msg.data);

	return respuesta; // retorna 0 si existe, o -EACCESS en caso contrario.
}

static struct fuse_operations sac_oper = {
		.getattr = sac_getattr,
		.read = sac_read,
		.readdir = sac_readdir,
		.mkdir = sac_mkdir,
		.rmdir = sac_rmdir,
		.write = sac_write,
		.mknod = sac_mknod,
		.unlink = sac_unlink,
};

// Dentro de los argumentos que recibe nuestro programa obligatoriamente
// debe estar el path al directorio donde vamos a montar nuestro FS
int main(int argc, char *argv[]){
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	////////////////////////////////no se donde hacer el close del socket////////////////////////////////////////
	//esta bastante hardcodeado esto, creo que deberia tomarlo del config
	serverSocket = connect_to("127.0.0.1",8003);

	// Esta es la funcion principal de FUSE, es la que se encarga
	// de realizar el montaje, comuniscarse con el kernel, delegar todo
	// en varios threads
	return fuse_main(args.argc, args.argv, &sac_oper, NULL);
}
