#include "sac_servidor.h"
#include "network.h"
#include "rpc.h"
#include <commons/log.h>
#include <commons/config.h>
#include <signal.h>

t_log *sac_logger = NULL;
sac_configuration *sac_config = NULL;

void *handler(void *args);
sac_configuration *load_configuration(char *path);

int sac_start_service(ConnectionHandler ch)
{
	sac_config = load_configuration(SAC_CONFIG_PATH);
	sac_logger = log_create("../logs/sac_server.log","SUSE",true,LOG_LEVEL_TRACE);
	server_start(sac_config->listen_port,ch);
}

void sac_stop_service()
{
	log_info(sac_logger,"Received SIGINT signal shuting down!");

	log_destroy(sac_logger);
	server_stop();
}

void *handler(void *args)
{
	ConnectionArgs *conna = (ConnectionArgs *)args;
	Message msg;
	char buffer[1024];
	int n=0;
	int sock = conna->client_fd;
	struct sockaddr_in client_address = conna->client_addr;
	
	while((n=receive_packet(sock,buffer,1024)) > 0)
	{
		if((n = message_decode(buffer,n,&msg)) > 0)
		{
			message_handler(&msg,sock);
		}
	}	
	log_debug(sac_logger,"The client was disconnected!");
	close(sock);
	return (void*)NULL;
}

sac_configuration *load_configuration(char *path)
{
	t_config *config = config_create(path);
	sac_configuration *sc = (sac_configuration *)malloc(sizeof(sac_configuration));

	if(config == NULL)
	{	
		log_error(sac_logger,"Configuration couldn't be loaded.Quitting program!");
		free(sc);
		exit(-1);
	}
	
	sc->listen_port = config_get_int_value(config,"LISTEN_PORT");
	config_destroy(config);
	return sc;
}

void message_handler(Message *m,int sock)
{
	switch(m->header.message_type)
	{
		case MESSAGE_STRING:
			log_debug(sac_logger,"Received -> %s",m->data);	
			break;
		case MESSAGE_CALL:
			log_debug(sac_logger,"Remote call received!");
			//rpc_server_invoke(m->data,sock);
			//message_free_data(m);
			break;
		default:
			log_error(sac_logger,"Undefined message");
			break;
	}
	return;

}

int main(int argc,char *argv[])
{
	//When Ctrl-C is pressed stops SUSE and frees resources
	signal(SIGINT,sac_stop_service);

	sac_start_service(handler);
	return 0;
}