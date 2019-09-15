#ifndef MUSE_H
#define MUSE_H
#include "network.h"

#define MUSE_CONFIG_PATH "../configs/memoria.config"

typedef struct muse_configuration_s
{
	int listen_port;
	int memory_size;
	int page_size;
	int swap_size;
}muse_configuration;

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
