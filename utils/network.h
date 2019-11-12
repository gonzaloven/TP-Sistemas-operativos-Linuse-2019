#ifndef NETWORK_H
#define NETWORK_H

#include <unistd.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "message.h"

//NO DEJAR ASI, ES PARA DEBUGGEAR ESTO NADA MAS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define NETWORK_LOGGER_PATH2 "../logs/server.log"
#define NETWORK_LOGGER_PATH "/home/utnso/tp-2019-2c-Los-Trapitos/logs/server.log"

/* Structure for storing client connection data. */
typedef struct ConnectionArgs_s
{
	int client_fd;
	struct sockaddr_in client_addr;
}ConnectionArgs;

/* Thread function created when client connects to server. */
typedef void* (ConnectionHandler)(void*);

/* Initializes server and waits for connections.
 * Once a connection has been established creates
 * a thread for servicing requests.
 * @param port: port to listen to.
 * @param handler: function to listen requests */
int server_start(int port,ConnectionHandler handler);

/* Stops the server from listening */
void server_stop();

/* Lets a client connect to a server
 * @param port: port to connect to.
 * @param ip: string containig the direction of the server. */
int connect_to(char *ip,int port);

/* Wrapper of the function recv
 * @param socket_fd: file descriptor of the socket from whom we're going
 * to receive.
 * @param buffer: pointer to the buffer receiving data.
 * @param buffer_size: size of buffer */
ssize_t receive_packet(int socket_fd,void *buffer,size_t buffer_size);
ssize_t receive_packet_no_wait(int socket_fd,void *buffer,size_t buffer_size);

/* Same as above but for sending */
ssize_t send_packet(int socket_fd,void *buffer,size_t buffer_size);

/* Sends a specified message to the socket 
 * @param socket_fd: socket to send the message
 * @param msg: message to be sended
 * */
ssize_t send_message(int socket_fd,Message *msg);

/* Receives a message from the specified socket
 * @param socket_fd: the socket to receive from
 * @param msg: the received message
 * */
ssize_t receive_message(int socket_fd,Message *msg);



#endif 
