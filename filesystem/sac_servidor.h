#ifndef SAC_SERVER_H
#define SAC_SERVER_H

#include "network.h"

#define BLOQUE_SIZE 4096
#define CANTIDAD_MAXIMA_ARCHIVOS 1024
#define HEADER_BLOCK_SIZE 1
#define MAXIMO_SIZE_NOMBRE_ARCHIVO 71

#define TIPOGETATTR 1
#define TIPOREADDIR 2
#define TIPOOPEN 3
#define TIPOREAD 4
#define TIPOWRITE 5
#define TIPOMKNOD 6
#define TIPOUNLINK 7
#define TIPOTRUNCATE 8
#define TIPORMDIR 9
#define TIPOMKDIR 10

#define SAC_CONFIG_PATH "../configs/filesystem.config"

typedef struct sac_configuration_s
{
	int listen_port;
}sac_configuration;

typedef struct sac_server_header
{
	unsigned char magic_number[3] //SAC
	uint32_t version;
	//uint32_t deberia ir el bloque de inicio del bitmap
	uint32_t bloques_bitmap;
	unsigned char padding[4081];
}Header;

typedef struct sac_server_gbloque
{
	uint32_t *ptrGbloque;
}GBloque;

typedef struct sac_server_gfile
{
	//0: BORRADO, 1: ARCHIVO, 2: DIRECTORIO
	uint8_t estado;
	unsigned char nombre_archivo[MAXIMO_SIZE_NOMBRE_ARCHIVO];
	//uint32_t bloque_padre
	uint32_t archivo_size;
	//fecha de creacion
	//fecha de modificacion
	//unsigned char *bloques_indirectos[1000 * sizeof(uint32_t)];
}GFile;

/* Starts server,creates a logger and loads configuration */
int sac_start_service(ConnectionHandler ch);

/* Stops server,frees resources */
void sac_stop_service();

/* Decides wich message has arrived
 * and depending on the message 
 * does an action */
void message_handler(Message *m,int sock);

#endif