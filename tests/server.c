#include "server.h"
#include "network.h"
#include "CUnit/Basic.h"

int main(int argc ,char *argv[])
{
	server_start_test();
	return 0;	
}

void server_start_test()
{
	server_start(8000,handler);
}

void* handler(void *args)
{
	ConnectionArgs *conna = (ConnectionArgs*)args;	
	int running = 1;

	char buffer[999];
	int n = 0;
	int sock = conna->client_fd;
	struct sockaddr_in client_address = conna->client_addr;

	printf("Cliente conectado con la direccion ip->%s\n",inet_ntoa(client_address.sin_addr));

	while((n = receive_packet(sock,buffer,100)) > 0)
	{
			buffer[n] = '\0';
			printf("%s",buffer);
	}
		running = 0;
		close(sock);
		printf("Se desconecto el cliente con ip: %s",inet_ntoa(client_address.sin_addr));
		fflush(stdout);
	

}

