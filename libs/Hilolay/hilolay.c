#include "hilolay.h"
#include "hilolay_internal.h"
#include "hilolay_alumnos.h"
#include "network.h"
#include <commons/log.h>
#include <commons/config.h>

t_log *hilolay_logger = NULL;
hilolay_configuration *hilolay_config = NULL;

int suse_socket;

void hilolay_init() {
	hilolay_config = load_configuration(HILOLAY_CONFIG_PATH);
	hilolay_logger = log_create("../../logs/hilolay.log", "hilolay", true, LOG_LEVEL_TRACE);

	Message m;
	MessageHeader header;

	create_message_header(&header, MESSAGE_NEW_ULT, /* tamaño de TCB */);

	suse_socket = connect_to(hilolay_config->CONNECTION_ADDR, hilolay_config->CONNECTION_PORT);
}

void suse_create(struct TCB* tcb)
{
	MessageHeader header;
	Message msg;
	// se habia deleteado rpc porque usaba json, hay que hacerlo con la nueva serealizacion
	// idem los demas mensajes, estan deprecados
	//char *process_data = rpc_form_new_ult_msg(tcb);
	//create_message_header(&header,MESSAGE_NEW_ULT);
	//create_string_message(&msg,&header,process_data);
	//send_message(master_socket,&msg);
	//message_free_data(&msg);
}
