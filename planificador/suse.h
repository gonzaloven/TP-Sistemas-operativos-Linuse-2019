#ifndef SUSE_H
#define SUSE_H

#include <stdio.h>
#include <stdlib.h>
#include "network.h"
#include <commons/log.h>
#include <commons/config.h>
#include <signal.h>

#define SUSE_CONFIG_PATH "../configs/planificador.config"

typedef struct suse_configuration_s
{
	int listen_port;
	int metrics_timer;
	int max_multiprog;
	char **sem_id;
	char **sem_init;
	char **sem_max;
	double alpha_sjf;
}suse_configuration;

/* Starts server,creates a logger and loads configuration */
int _suse_init(ConnectionHandler ch);
/* Stops server,frees resources */
void _suse_stop_service();

/* Decides wich message has arrived
 * and depending on the message 
 * does an action */
void message_handler(Message *m,int sock);


#endif 

