#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "fuse.h"

#define MAX_BUFFER 1024

typedef struct {
	int8_t  type;
	int16_t length;
	char    payload[MAX_BUFFER];
} __attribute__ ((__packed__)) tPackage;

typedef enum {


} tMessage;

// Definition of payloads to be send in the package

// ------------ el contenido de las struct esta mal, deberian ser los parametros de las funciones
// que va a utilizar el sac_server ej de memcpy-----------------------------------------

typedef struct {
	const char *path;
	struct stat *stbuf;
} t_Getatrr;

typedef struct{
	const char *path;
	void *buf;
	fuse_fill_dir_t filler;
	off_t offset;
	struct fuse_file_info *fi;
} t_Readdir;

typedef struct{
	const char *path;
	char *buf;
	size_t size;
	off_t offset;
	struct fuse_file_info *fi;
} t_Read;

typedef struct{
	const char *pathname;
	mode_t mode;
} t_Mkdir;

typedef struct{
	const char *pathname;
	mode_t mode;
	dev_t dev;
} t_Mknod;

typedef struct{
	const char *pathname;
} t_Rmdir;

typedef struct{
	const char *pathname;
} t_Unlink;

typedef struct{
	const char *path;
	const char *buf;
	size_t size;
	off_t offset;
	struct fuse_file_info *fi;
} t_Write;

// encode / decode

int getatrr_encode(t_Message message_type, t_Getatrr function, tPaquete* pPaquete);
t_Getatrr* getatrr_decode(char* payload);

int readdir_encode(t_Message message_type, t_Readdir function, tPaquete* pPaquete);
t_Readdir* readdir_decode(char* payload);

int read_encode(t_Message message_type, t_Read function, tPaquete* pPaquete);
t_Read* read_decode(char* payload);

int mkdir_encode(t_Message message_type, t_Mkdir function, tPaquete* pPaquete);
t_Mkdir* mkdir_decode(char* payload);

int mknod_encode(t_Message message_type, t_Mknod function, tPaquete* pPaquete);
t_Mknod* mknod_decode(char* payload);

int rmdir_encode(t_Message message_type, t_Rmdir function, tPaquete* pPaquete);
t_Rmdir* rmdir_decode(char* payload);

int unlink_encode(t_Message message_type, t_Unlink function, tPaquete* pPaquete);
t_Unlink* unlink_decode(char* payload);

int write_encode(t_Message message_type, t_Write function, tPaquete* pPaquete);
t_Write* write_decode(char* payload);
