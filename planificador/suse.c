#include "suse.h"
#include "network.h"
#include "semaphore.h"
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/node.h>
#include <commons/collections/queue.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <commons/collections/queue.h>

#define SEM_ID_MAX_LENGTH 32

t_log *suse_logger = NULL;
suse_configuration *suse_config = NULL;

void *client_handler(void *args);
void *report_metrics(void *args);
suse_configuration *load_configuration(char *path);

semaphore_t * suse_semaphores;
unsigned int suse_nsemaphores;

int suse_init(ConnectionHandler ch)
{
	suse_config = load_configuration(SUSE_CONFIG_PATH);
	suse_logger = log_create("../logs/suse.log","SUSE",true,LOG_LEVEL_TRACE);

	t_queue *ready_queue = queue_create();
	t_queue *running_queue = queue_create();

	suse_initialize_semaphores();

	signal(SIGALRM, report_metrics);
	alarm(suse_config->metrics_timer);

	server_start(suse_config->listen_port, ch);
}

void suse_stop_service()
{
	int i;
	log_info(suse_logger,"Received SIGINT signal shuting down!");

	log_destroy(suse_logger);
	
	free(suse_config->sem_id[0]);
	free(suse_config->sem_id[1]);
	free(suse_config->sem_id);

	free(suse_config->sem_init[0]);
	free(suse_config->sem_init[1]);
	free(suse_config->sem_init);

	free(suse_config->sem_max[0]);
	free(suse_config->sem_max[1]);
	free(suse_config->sem_max);
	free(suse_config);

	suse_config = NULL;
	suse_logger = NULL;
	
	server_stop();
}

int suse_create(void (*f)(void *))
{

}

/* TCB??? */ suse_schedule_next(/* ??? */)
{

}

int suse_wait(char * sem_id)
{
	int i = 0;

	do {
		if (i == suse_nsemaphores)
		{
			log_error(suse_logger, "Semaphore identifier not found");
			return -1;
		}

		i++;
	} while (strncmp(sem_id, suse_semaphores[i], SEM_ID_MAX_LENGTH));


}

int suse_signal(/* ??? */)
{

}

/* ??? */ suse_join(/* ??? */)
{

}

void *client_handler(void *args)
{
	ConnectionArgs *conna = (ConnectionArgs *)args;
	Message msg;
	char buffer[1024];
	int n=0;
	int sock = conna->client_fd;
	struct sockaddr_in client_address = conna->client_addr;
	
	printf("A client has connected!\n");
	while((n=receive_packet(sock,buffer,1024)) > 0)
	{
		if((n = message_decode(buffer,n,&msg)) > 0)
		{
			message_handler(&msg,sock);
		}
	}	

	log_debug(suse_logger,"The client was disconnected!");
	close(sock);

	return (void*) NULL;
}

void *report_metrics(void *args) {
	char *log_string;

	/* Thread metrics */

	/* Process metrics */

	/* Semaphore metrics */
	for (int i = 0; i < suse_nsemaphores; i++) {
		sprintf(log_string, "Semaphore \"%s\" is currently at value %d", suse_semaphores[i]->id, suse_semaphores[i]->val);
		log_info(suse_logger, log_string);
	}
		
	alarm(suse_config->metrics_timer);
}

suse_configuration *load_configuration(char *path)
{
	t_config *config = config_create(path);
	suse_configuration *sc = (suse_configuration *)malloc(sizeof(suse_configuration));

	if(config == NULL)
	{	
		log_error(suse_logger,"Configuration couldn't be loaded.Quitting program!");
		free(sc);
		exit(-1);
	}
	
	sc->listen_port = config_get_int_value(config,"LISTEN_PORT");
	sc->metrics_timer = config_get_int_value(config,"METRICS_TIMER");
	sc->max_multiprog = config_get_int_value(config,"MAX_MULTIPROG");
	sc->sem_id = config_get_array_value(config,"SEM_IDS");
	sc->sem_init = config_get_array_value(config,"SEM_INIT");
	sc->sem_max = config_get_array_value(config,"SEM_MAX");
	sc->alpha_sjf = config_get_int_value(config,"ALPHA_SJF"); 

	config_destroy(config);
	return sc;
}

void suse_initialize_semaphores()
{
	suse_nsemaphores = sizeof(suse_configuration->sem_id) / sizeof(char *);

	for (int i = 0; i < suse_nsemaphores, i++)
	{
		suse_semaphores[i]->id = suse_config->sem_id[i];
		suse_semaphores[i]->val = atoi(suse_config->sem_init[i]);
		suse_semaphores[i]->max_val = atoi(suse_config->sem_max[i]);
		suse_semaphores[i]->waiting_queue = NULL;
	}
}

void message_handler(Message *m,int sock)
{
	switch(m->header.message_type)
	{
		case MESSAGE_STRING:
			log_debug(suse_logger,"Received -> %s",m->data);	
			break;
		case MESSAGE_CALL:
			log_debug(suse_logger,"Remote call received!");
			rpc_server_invoke(m->data,sock);
			message_free_data(m);
			break;
		case MESSAGE_NEW_ULT:
			log_debug(suse_logger,"New ult arrived in the ready queue!\n");
			//do_somth
			break;
		default:
			log_error(suse_logger,"Undefined message");
			break;
	}
	return;

}

int main(int argc,char *argv[])
{
	//When Ctrl-C is pressed stops SUSE and frees resources
	signal(SIGINT,suse_stop_service);

	suse_init(client_handler);
	return 0;
}
