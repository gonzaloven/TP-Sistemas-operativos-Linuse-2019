#include "muse.h"
#include "message.h"

void* handler(void *args);

int muse_start_service(int port,ConnectionHandler handler)
{
	server_start(port,handler);
}

void* handler(void *args)
{
	ConnectionArgs *conna = (ConnectionArgs *)args;
	Message msg;
	char buffer[100];
	int n=0;
	int sock = conna->client_fd;
	struct sockaddr_in client_address = conna->client_addr;

	printf("A client has connected!\n");
	while((n=receive_packet(sock,buffer,100)) > 0)
	{
		if((n = message_decode(buffer,n,&msg)) > 0)
		{
			switch(msg.header.message_type)
			{
				case MESSAGE_STRING:
					//use logs!
					printf("received -> %s\n",msg.data);
					fflush(stdout);
					break;
				default:
					//use logs!!
					printf("Error\n");
					fflush(stdout);
					break;
			}
		}
	}	
	//use logs!!
	printf("The client was disconnected!\n");
	close(sock);
	return (void*)NULL;
}


int main(int argc,char *argv[])
{
	muse_start_service(8000,handler);
	return 0;
}
