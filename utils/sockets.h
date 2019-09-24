#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

#include <commons/log.h>
#include "protocolo.h"

int create_socket(t_log *logger);

void bindearSocket(int unSocket, struct sockaddr_in socketInfo, t_log *logger);

void listen_on(int unSocket);

int create_listen_socket(int port, t_log *logger);

void bindearSocket(int unSocket, struct sockaddr_in socketInfo, t_log *logger)

int send_package(int socketServer, tPackage *pPackageToSend, t_log *logger,char *info);

int recieve_package(int socketReceptor, tMessage *tipoMensaje, char **psPayload, t_log *pLogger, char *sMensajeLogger);

signed int get_connection(fd_set *master, int *maxSock, int sockListener, tMessage* messageType, char** payload, t_log* logger);

signed int get_connection_time_out(fd_set *setSockets, int *maxSock, int sockListener, tMessage* messageType, char** payload, struct timeval *timeout, t_log* logger);

signed int multiplexar(fd_set *master, fd_set *temp, int *maxSock, tMessage* messageType, char** buffer, t_log* logger);

signed int connect_to_Server(char *ip_server, int port, t_log *logger);

int desconnect_from(int iSocket);

#endif /* SOCKETS_H_ */
