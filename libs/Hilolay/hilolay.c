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

	
