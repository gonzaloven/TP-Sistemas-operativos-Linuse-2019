#ifndef SUSE_H
#define SUSE_H

#include "network.h"

#define SUSE_CONFIG_PATH "../configs/planificador.config"

typedef struct suse_configuration_s
{
	int listen_port;
	int metrics_timer;
	int max_multiprog;
	char **sem_id;
	char **sem_init;
	char **sem_max;
	int alpha_sjf;
}suse_configuration;

/* Starts server,creates a logger and loads configuration */
int suse_start_service(ConnectionHandler ch);
/* Stops server,frees resources */
void suse_stop_service();


#endif 

