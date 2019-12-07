#include "network.h"
#include <pthread.h>
#include <commons/log.h>
#include <stdlib.h>
#include <string.h>

int running = 1;
int connected_clients = 0;
int listen_socket = 0; //master socket
t_log *server_logger = NULL;

int server_listen_connections(int listen_socket,ConnectionHandler f,ConnectionArgs *args);

int server_start(int port,ConnectionHandler handler)
{
	//server_logger = log_create("/home/utnso/git/tp-2019-2c-Los-Trapitos/logs/server.log","SERVER",true,LOG_LEVEL_TRACE);
	server_logger = log_create("/home/utnso/workspace/tp-2019-2c-Los-Trapitos/logs/server.log","SERVER",true,LOG_LEVEL_TRACE);
    struct sockaddr_in server_address;
	memset(&server_address,0,sizeof(server_address));
	server_address.sin_family = AF_INET;

	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if((listen_socket = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		log_error(server_logger,"No se pudo crear el socket\n");
		return -1;
	}

	if((bind(listen_socket,(struct sockaddr*)&server_address,
											sizeof(server_address))) < 0 )
	{
		log_error(server_logger,"Bind fallo");
		return -1;
	}

	int wait_size = 16;
	ConnectionArgs *conn = (ConnectionArgs*)malloc(sizeof(ConnectionArgs));

	if(listen(listen_socket,wait_size) < 0)
	{
		log_error(server_logger,"No se pudo abrir el socket para escuchar");
		return -1;
	}

	log_debug(server_logger,"Servidor inicializado correctamente. Escuchando conexiones");
	return server_listen_connections(listen_socket,handler,conn);
}

int server_listen_connections(int listen_socket,ConnectionHandler f,ConnectionArgs *args)
{
	int sock;
	int client_address_len = 0;
	pthread_t client_thread;

	while(running)
	{
		if((sock = accept(listen_socket,(struct sockaddr *)&args->client_addr,&client_address_len)) < 0)
		{
			if(running)
			{
				log_error(server_logger,"No se pudo abrir un socket para aceptar datos!");
			}

			free(args);
			return -1;
		}
		connected_clients++;
		args->client_fd = sock;
		if(pthread_create(&client_thread,NULL,f,(void*)args) != 0)
		{
			log_error(server_logger,"Error al crear hilo de escucha!");
			return -1;
		}

		if(pthread_detach(client_thread) != 0)
		{
			log_error(server_logger,"El thread no se pudo detachar");
			return -1;
		}
	}
	free(args);
	close(listen_socket);
	return 0;
}

void server_stop()
{
	running = 0;
	close(listen_socket);
	log_debug(server_logger,"Cerrando Servidor");
	log_destroy(server_logger);
}

int connect_to(char *ip,int port)
{
	const int server_port = port;
	struct sockaddr_in server_addr;
	memset(&server_addr,0,sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(server_port);

	int sock;
	if((sock = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		//Failed
		return -1;
	}
	if(connect(sock,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0)
	{
		//Failed
		return -1;
	}
	
	return sock;
}

ssize_t receive_packet(int socket,void *buffer,size_t buffer_size)
{
	memset(buffer,0,buffer_size);
	MessageHeader header;
	void *cursor = buffer;
	unsigned int header_size = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t);
	unsigned int recv_bytes = recv(socket,cursor,header_size,MSG_WAITALL);

	header_decode(cursor,buffer_size,&header);
	cursor += recv_bytes;
	recv_bytes += recv(socket,cursor,header.data_size,MSG_WAITALL);

	//log_debug(server_logger,"Se recibio un paquete de %d bytes",recv_bytes);
	
	return recv_bytes;
}

ssize_t receive_packet_var(int socket,void** bufferReal){
	MessageHeader header;
	unsigned int header_size = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t);
	void *buffer = malloc(header_size);
	unsigned int recv_bytes = recv(socket,buffer,header_size,MSG_WAITALL);

	header_decode(buffer,header_size,&header);
	free(buffer);

	*bufferReal = malloc(header.data_size + header_size);
	header_encode(&header, *bufferReal, header.data_size + header_size);

	void* cursor = *bufferReal;

	cursor+= recv_bytes;
	recv_bytes += recv(socket,cursor,header.data_size,MSG_WAITALL);

	return recv_bytes;
}

ssize_t send_packet(int socket_fd,void *buffer,size_t buffer_size)
{
	return send(socket_fd,buffer,buffer_size,0);
}

ssize_t send_message(int socket_fd,Message *msg)
{
	size_t buffer_size = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t) + msg->header.data_size;
	char buffer[buffer_size];
	memset(buffer, 0, buffer_size);
	message_encode(msg,buffer,buffer_size);
	return send_packet(socket_fd,buffer,buffer_size);
	
}

ssize_t receive_message(int socket_fd,Message *msg)
{
	size_t buffer_size = 999;
	char buffer[buffer_size];

	receive_packet(socket_fd,buffer,buffer_size);
	return message_decode(buffer,buffer_size,msg);
}

ssize_t receive_message_var(int socket_fd, Message* msg){
	MessageHeader header;
	unsigned int header_size = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t);
	void *buffer = malloc(header_size);
	unsigned int recv_bytes = recv(socket_fd,buffer,header_size,MSG_WAITALL);

	header_decode(buffer,header_size,&header);
	free(buffer);

	char bufferReal[header.data_size + header_size];
	header_encode(&header, bufferReal, header.data_size + header_size);

	void* cursor = bufferReal;

	cursor+= recv_bytes;
	recv_bytes += recv(socket_fd,cursor,header.data_size,MSG_WAITALL);

	return message_decode(bufferReal, header.data_size + header_size, msg);
}
