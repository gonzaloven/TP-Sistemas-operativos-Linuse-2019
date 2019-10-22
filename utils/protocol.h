#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "fuse.h"

#define MAX_BUFFER 1024

typedef struct {
	int8_t type;
	int16_t length;
} __attribute__ ((__packed__)) tHeader;

typedef struct {
	int8_t type;
	int16_t length;
	char payload[MAX_BUFFER];
} __attribute__ ((__packed__)) tPaquete;

typedef enum {
	/* Filesystem functions  */
	FF_GETATTR,
	RTA_GETATTR,
	FF_READDIR,
	FF_OPEN,
	FF_READ,
	FF_MKDIR,
	FF_RMDIR,
	FF_WRITE,
	FF_MKNOD,
	FF_UNLINK,
	FF_ERROR,
	DESCONEXION
} tMessage;

typedef struct {
	mode_t modo;
	nlink_t nlink;
	off_t total_size;
}DesAttr_resp;

typedef struct {
	uint16_t tamano;
	char* lista_nombres;
}DesReaddir_resp;

// Definition of payloads to be send in the package

// ------------ el contenido de las struct esta mal, deberian ser los parametros de las funciones
// que va a utilizar el sac_server ej de memcpy-----------------------------------------

typedef struct {
	const char *path;
} t_Getattr;

typedef struct {
	mode_t modo, 
	nlink_t links, 
	off_t total_size
} t_Rta_Getattr;

typedef struct{
	const char *path;
	void *buf;
	fuse_fill_dir_t filler;
	off_t offset;
	struct fuse_file_info *fi;
} t_Readdir;

typedef struct{
	const char *path;
	size_t size;
	off_t offset;
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
} t_Write;

// encode / decode

int int_encode(t_Message message_type, int num, tPaquete* pPaquete);
int int_decode(char* payload);

int path_encode(t_message message_type, const char* path, tPaquete* pPaquete);

int getattr_encode(t_Message message_type, t_Getattr parameters, tPaquete* pPaquete);
t_Getatrr* getattr_decode(char* payload);

int readdir_encode(t_Message message_type, t_Readdir parameters, tPaquete* pPaquete);
t_Readdir* readdir_decode(char* payload);

int read_encode(t_Message message_type, t_Read parameters, tPaquete* pPaquete);
t_Read* read_decode(char* payload);

int mkdir_encode(t_Message message_type, t_Mkdir parameters, tPaquete* pPaquete);
t_Mkdir* mkdir_decode(char* payload);

int mknod_encode(t_Message message_type, t_Mknod parameters, tPaquete* pPaquete);
t_Mknod* mknod_decode(char* payload);

int rmdir_encode(t_Message message_type, t_Rmdir parameters, tPaquete* pPaquete);
t_Rmdir* rmdir_decode(char* payload);

int unlink_encode(t_Message message_type, t_Unlink parameters, tPaquete* pPaquete);
t_Unlink* unlink_decode(char* payload);

int write_encode(t_Message message_type, t_Write parameters, tPaquete* pPaquete);
t_Write* write_decode(char* payload);
