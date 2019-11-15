#ifndef MUSE_H
#define MUSE_H

#include "network.h"
#include "main_memory.h"
#include <commons/log.h>
#include <commons/config.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#define MUSE_CONFIG_PATH "../configs/memoria.config"
#define MUSE_LOG_PATH "../logs/muse.log"

typedef struct muse_configuration_s
{
	int listen_port;
	int memory_size;
	int page_size;
	int swap_size;
}muse_configuration;

uint32_t muse_invoke_function(Function *f,uint32_t pid);

/* Starts server,logger and loads configuration. */
int muse_start_service(ConnectionHandler ch);

/* Stops the server and frees resources */
void muse_stop_service();

/* helpers */
muse_configuration *load_configuration(char *config_path);

/* Connection handler. */
void* handler(void *args);

/* Decides wich message has arrived
 * and depending on the message 
 * does an action */
void message_handler(Message *m,int sock);

#endif 
