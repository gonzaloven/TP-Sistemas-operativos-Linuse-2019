#include "muse.h"
#include "network.h"
#include "main_memory.h"
#include <commons/log.h>
#include <commons/config.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

t_log *muse_logger = NULL;
muse_configuration *muse_config = NULL;

int muse_invoke_function(Function *f);

int muse_start_service(ConnectionHandler ch)
{
	muse_config = load_configuration(MUSE_CONFIG_PATH);
	muse_logger = log_create("../logs/muse.log","MUSE",true,LOG_LEVEL_TRACE);
	muse_main_memory_init(muse_config->memory_size,muse_config->page_size);
	server_start(muse_config->listen_port,ch);
}

void muse_stop_service()
{
	log_info(muse_logger,"SIGINT received.Shuting down!");
	free(muse_config);
	log_destroy(muse_logger);
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

	//cambiar esto
	while((n=receive_packet(sock,buffer,1024)) > 0)
	{
		if((n = message_decode(buffer,n,&msg)) > 0)
		{
			message_handler(&msg,sock);
			memset(buffer,'\0',1024);
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
	config_destroy(config);
	return mc;
}

void message_handler(Message *m,int sock)
{
	int res= 0;;
	Message msg;
	MessageHeader head;
	switch(m->header.message_type)
	{
		case MESSAGE_CALL:
			res = muse_invoke_function((Function *)m->data);
			log_info(muse_logger,"Call received!");
			message_free_data(m);
			
			create_message_header(&head,MESSAGE_FUNCTION_RET,sizeof(uint8_t));
			create_response_message(&msg,&head,res);
			send_message(sock,&msg);
			message_free_data(&msg);
			break;
		default:
			log_error(muse_logger,"Undefined message");
			break;
	}
	return;

}

int muse_invoke_function(Function *f)
{
	log_debug(muse_logger,"function type %d function args %d arg[0] %d ",f->type,f->num_args,f->args[0].value);
	return 0;

}

int main(int argc,char *argv[])
{
	signal(SIGINT,muse_stop_service);
	muse_start_service(handler); 
	return 0;
}


