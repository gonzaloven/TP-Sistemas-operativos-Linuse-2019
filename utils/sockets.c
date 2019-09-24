#include "sockets.h"

#define BACKLOG 100

int create_socket(t_log *logger) {
	int unSocket;
	int si = 1;
	// Creates the socket
	if ((unSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		log_error(logger, "Creacion socket: %s", strerror(errno));
		return EXIT_FAILURE;

	} else {
		// Set the options to listen to more than one at the same time
		if (setsockopt(unSocket, SOL_SOCKET, SO_REUSEADDR, &si, sizeof(int))
				== -1) {
			log_error(logger, "Setsockopt: %s", strerror(errno));
			return EXIT_FAILURE;
		}

		return unSocket;
	}
}

void bindear_socket(int unSocket, struct sockaddr_in socketInfo, t_log *logger) {
	//--Bindear socket al proceso server
	if (bind(unSocket, (struct sockaddr*) &socketInfo, sizeof(socketInfo))
			== -1) {
		log_error(logger, "Error al bindear socket escucha: %s",
				strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void listen_on(int unSocket) {
	if (listen(unSocket, BACKLOG) == -1) {
		perror("Error al poner a escuchar socket");
		exit(EXIT_FAILURE);
	}
}

int create_listen_socket(int port, t_log *logger) {
	struct sockaddr_in myAddress;
	int socketEscucha;

	socketEscucha = crearSocket(logger);

	//--Arma la información que necesita para mandar cosas
	myAddress.sin_family = AF_INET;
	myAddress.sin_addr.s_addr = INADDR_ANY;
	myAddress.sin_port = htons(port);
	memset(&(myAddress.sin_zero), '\0', 8); // Poner a cero el resto de la estructura

	bindear_socket(socketEscucha, myAddress, logger);

	//--Escuchar
	listen_on(socketEscucha);

	return socketEscucha;
}

int send_package(int socketServer, tPackage *pPackageToSend, t_log *logger,
		char *info) {
	int byteEnviados;
	log_debug(logger, ">>> %s", info);

	byteEnviados = send(socketServidor, (char*) pPackageToSend,
			sizeof(tHeader) + pPackageToSend->length, 0);

	if (byteEnviados == -1) {
		log_error(logger, "%s: %s", info, strerror(errno));
		return -1;

	} else {
		return byteEnviados;
	}
}

int recieve_package(int socketReceptor, tMessage *tipoMensaje, char **psPayload,
		t_log *pLogger, char *sMensajeLogger) {
	tHeader header;
	int bytesRecibidosHeader = 0;
	int bytesRecibidos = 0;

	log_debug(pLogger, "<<< %s", sMensajeLogger);
	bytesRecibidosHeader = recv(socketReceptor, &header, sizeof(tHeader),
			MSG_WAITALL);

	if (bytesRecibidosHeader == 0) {
		return 0;	// CERRO CONEXION

	} else if (bytesRecibidosHeader < 0) {
		log_error(pLogger, "%s: %s", sMensajeLogger, strerror(errno));
		return -1;	// ERROR
	}

	*tipoMensaje = (tMessage) header.type;

	if (header.length > 0) {
		*psPayload = malloc(header.length);

		bytesRecibidos = recv(socketReceptor, *psPayload, header.length,
				MSG_WAITALL);

		if (bytesRecibidos < 0) {
			log_error(pLogger, "%s: %s", sMensajeLogger, strerror(errno));
			free(*psPayload);	// ERROR, se libera el espacio reservado
			return -1;
		}
	}
	return bytesRecibidos + bytesRecibidosHeader;
}

/*
 * @NAME: getConnection
 * @DESC: Multiplexa con Select
 *
 * 	Valores de salida:
 * 	=0 = se agrego un nuevo socket
 * 	<0 = Se cerro el socket que devuelve
 * 	>0 = Cambio el socket que devuelve
 */
signed int getConnection(fd_set *setSockets, int *maxSock, int sockListener,
		tMessage *tipoMensaje, char **payload, t_log *logger) {
	int iSocket;
	int iNewSocket;
	int iBytesRecibidos;
	fd_set setTemporal;
	FD_ZERO(&setTemporal);
	setTemporal = *setSockets;

	struct sockaddr_in clientAddress;
	socklen_t sinClientSize;
	sinClientSize = sizeof(clientAddress);

	//--Multiplexa conexiones
	if (select(*maxSock + 1, &setTemporal, NULL, NULL, NULL) == -1) {
		log_error(logger, "select: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	//--Cicla las conexiones para ver cual cambio
	for (iSocket = 0; iSocket <= *maxSock; iSocket++) {

		//--Si el i° socket cambió
		if (FD_ISSET(iSocket, &setTemporal)) {
			//--Si el que cambió, es el listener

			if (iSocket == sockListener) {

				//--Gestiona nueva conexión
				iNewSocket = accept(sockListener,
						(struct sockaddr*) &clientAddress, &sinClientSize);

				if (iNewSocket == -1) {
					log_error(logger, "getConnection :: accept: %s",
							strerror(errno));

				} else {
					log_trace(logger, "Nueva conexion socket: %d", iNewSocket);
					//--Agrega el nuevo listener
					FD_SET(iNewSocket, setSockets);

					if (iNewSocket > *maxSock) {
						*maxSock = iNewSocket;
					}
				}

			} else {
				//--Gestiona un cliente ya conectado
				if ((iBytesRecibidos = recieve_package(iSocket, tipoMensaje,
						payload, logger, "Se recibe informacion")) <= 0) {
					//--Si cerró la conexión o hubo error
					if (iBytesRecibidos == 0) {
						log_trace(logger, "Fin de conexion de socket %d.",
								iSocket);

					} else {
						log_error(logger, "recv: %s", strerror(errno));
					}
					//--Cierra la conexión y lo saca de la lista
					close(iSocket);
					FD_CLR(iSocket, setSockets);
					*tipoMensaje = DESCONEXION;
				}

				return iSocket;
			}
		}
	}

	return -1;
}

signed int get_connection_time_out(fd_set *setSockets, int *maxSock,
		int sockListener, tMessage *tipoMensaje, char **payload,
		struct timeval *timeout, t_log *logger) {
	int iSocket;
	int iNewSocket;
	int iBytesRecibidos;
	fd_set setTemporal;
	FD_ZERO(&setTemporal);
	setTemporal = *setSockets;

	struct sockaddr_in clientAddress;
	socklen_t sinClientSize;
	sinClientSize = sizeof(clientAddress);

	//--Multiplexa conexiones
	if (select(*maxSock + 1, &setTemporal, NULL, NULL, timeout) == -1) {
		log_error(logger, "select: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	//--Cicla las conexiones para ver cual cambio
	for (iSocket = 0; iSocket <= *maxSock; iSocket++) {

		//--Si el i° socket cambió
		if (FD_ISSET(iSocket, &setTemporal)) {
			//--Si el que cambió, es el listener

			if (iSocket == sockListener) {

				//--Gestiona nueva conexión
				iNewSocket = accept(sockListener,
						(struct sockaddr*) &clientAddress, &sinClientSize);

				if (iNewSocket == -1) {
					log_error(logger, "getConnection :: accept: %s",
							strerror(errno));

				} else {
					log_trace(logger, "Nueva conexion socket: %d", iNewSocket);
					//--Agrega el nuevo listener
					FD_SET(iNewSocket, setSockets);

					if (iNewSocket > *maxSock) {
						*maxSock = iNewSocket;
					}
				}

			} else {
				//--Gestiona un cliente ya conectado
				if ((iBytesRecibidos = recieve_package(iSocket, tipoMensaje,
						payload, logger, "Se recibe informacion")) <= 0) {
					//--Si cerró la conexión o hubo error
					if (iBytesRecibidos == 0) {
						log_debug(logger, "Fin de conexion de socket %d.",
								iSocket);

					} else {
						log_error(logger, "recv: %s", strerror(errno));
					}

					//--Cierra la conexión y lo saca de la lista
					close(iSocket);
					FD_CLR(iSocket, setSockets);
					*tipoMensaje = DESCONEXION;
				}

				return iSocket;
			}
		}
	}

	return -1;
}

signed int multiplexar(fd_set *master, fd_set *temp, int *maxSock,
		tMessage *tipoMensaje, char **buffer, t_log *logger) {
	int iSocket;
	int nBytes;
	memcpy(temp, master, sizeof(fd_set));

//	struct timeval timeout;
//	timeout.tv_sec = 3; //TODO version con el time out
//	timeout.tv_usec = 0;
//	int ret;
//	//--Multiplexa conexiones
//	if (( ret = select(*maxSock + 1, temp, NULL, NULL, &timeout)) == -1) {
//		log_error(logger, "select: %s", strerror(errno));
//		exit(EXIT_FAILURE);
//	} else {
//		if(ret==0){
//
//			log_warning(logger, "TRAMPA CON EL TIMEOUT ---> %s", enumToString(*tipoMensaje));
//			*tipoMensaje = P_FIN_TURNO;
//			return 0;
//		}
//
//	}
	if (select(*maxSock + 1, temp, NULL, NULL, NULL) == -1) {
		log_error(logger, "select: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	//--Cicla las conexiones para ver cual cambió
	for (iSocket = 0; iSocket <= *maxSock; iSocket++) {

		if (FD_ISSET(iSocket, temp)) {
			//--Gestiona un cliente ya conectado

			if ((nBytes = recieve_package(iSocket, tipoMensaje, buffer, logger,
					"Se recibe Mensaje")) <= 0) {
				//--Si cerró la conexión o hubo error
				if (nBytes == 0) {
					log_trace(logger, "Fin de conexion de %d.", iSocket);

				} else {
					log_error(logger, "multiplexar :: recv in %d: %s", iSocket,
							strerror(errno));
				}
				//--Cierra la conexión y lo saca de la lista
				close(iSocket);
				FD_CLR(iSocket, master);
				*tipoMensaje = DESCONEXION;

			}

			return iSocket;
		}
	}

	return -1;
}

signed int connect_to_server(char *ip_server, int port, t_log *logger)
{
	int iSocket; 					// Escuchar sobre sock_fd, nuevas conexiones sobre new_fd
	struct sockaddr_in their_addr; 	// Información sobre mi dirección

	// Seteo IP y Puerto
	their_addr.sin_family = AF_INET;  					// Ordenación de bytes de la máquina
	their_addr.sin_port = htons(port); 				// short, Ordenación de bytes de la red
	their_addr.sin_addr.s_addr = inet_addr(ip_server);
	memset(&(their_addr.sin_zero), '\0', 8); 			// Poner a cero el resto de la estructura

	// Pido socket
	if ((iSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		log_error(logger, "socket: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	// Intento conectar
	if (connect(iSocket, (struct sockaddr *) &their_addr, sizeof their_addr) == -1) {
		log_error(logger, "connect: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	log_trace(logger, "Se realiza conexion con socket %d", iSocket);

	return iSocket;
}

int desconnect_from(int iSocket)
{
	if (close(iSocket)) {
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}
