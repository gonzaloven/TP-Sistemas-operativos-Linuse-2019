#include "libhilolay.h"
#include "network.h"

int master_socket = 0;

int say(char *smth)
{
	Message m;
	MessageHeader header;

	create_message_header(&header,MESSAGE_STRING);

	message_alloc_data(&m,strlen(smth)+1);
	create_string_message(&m,&header,smth);

	send_message(master_socket,&m);
	message_free_data(&m);
}

int libhilolay_init(char *ip,int port)
{
	master_socket = connect_to(ip,port);
	return master_socket;
}
