#include "muse.h"
#include "network.h"
#include "rpc.h"
#include <commons/log.h>
#include <commons/config.h>

t_log *muse_logger = NULL;
muse_configuration *muse_config = NULL;

int muse_start_service(ConnectionHandler ch)
{
	muse_config = load_configuration(MUSE_CONFIG_PATH);
	muse_logger = log_create("../logs/muse.log","MUSE",true,LOG_LEVEL_TRACE);
	server_start(muse_config->listen_port,ch);
}

void muse_stop_service()
{
	free(muse_config);
	free(muse_logger);
	server_stop();
}

void* handler(void *args)
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
	log_debug(muse_logger,"The client was disconnected!");
	close(sock);
	return (void*)NULL;
}

muse_configuration *load_configuration(char *path)
{
	t_config *config = config_create(path);
	muse_configuration *mc = (muse_configuration *)malloc(sizeof(muse_configuration)); 

	if(config == NULL)
	{
		log_error(muse_logger,"Configuration couldn't be loaded.Quitting program!");
		muse_stop_service();
		exit(-1);
	}
	
	mc->listen_port = config_get_int_value(config,"LISTEN_PORT");
	mc->memory_size = config_get_int_value(config,"MEMORY_SIZE");
	mc->page_size = config_get_int_value(config,"PAGE_SIZE");
	mc->swap_size = config_get_int_value(config,"SWAP_SIZE");
	return mc;
}

void message_handler(Message *m,int sock)
{
	switch(m->header.message_type)
	{
		case MESSAGE_STRING:
			log_debug(muse_logger,"Received -> %s",m->data);	
			break;
		case MESSAGE_CALL:
			uint32_t *res = rpc_server_invoke(m->data);
			message_free_data(m);

			Message msg;
			MessageHeader header;
			create_message_header(&header,MESSAGE_FUNCTION_RET); //MESSAGE_FUNCTION_RET
			msg.data_size=sizeof(uint32_t);
			msg.data = res;
			send_message(sock,&msg);

			break;
		default:
			log_error(muse_logger,"Undefined message");
			break;
	}
	return;

}

int main(int argc,char *argv[])
{
	muse_start_service(handler); 
	return 0;
}

