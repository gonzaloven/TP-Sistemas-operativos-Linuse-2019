#ifndef SAC_SERVER_H
#define SAC_SERVER_H

#include "network.h"

#define BLOQUE_SIZE 4096
#define MAX_FILE_SIZE 1024
#define HEADER_BLOCK_SIZE 1
#define MAX_NAME_SIZE 71
#define BLKINDIRECT 1000

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

typedef struct sac_configuration_s{
	int listen_port;
}sac_configuration;

typedef uint32_t ptrGBloque;

typedef struct sac_server_header{
	unsigned char identificador[] = "SAC";
	uint32_t version = 1;
	uint32_t bitmap_start;
	uint32_t bitmap_size;
	unsigned char padding[4081];
}Header;

typedef struct sac_server_gfile{
	uint8_t state;
	unsigned char fname[MAX_NAME_SIZE];
	uint32_t parent_dir_block;
	uint32_t file_size;
	uint64_t create_date;
	uint64_t modify_date;
	ptrGBloque blk_indirect[BLKINDIRECT];
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
