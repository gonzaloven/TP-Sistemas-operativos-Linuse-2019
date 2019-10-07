int serializar_Gettattr_Resp(t_Rta_Getattr parametros, tPaquete *paquete){

	uint16_t offset = 0;

	paquete->type = TIPOGETATTR;
	paquete->length = sizeof(parametros->modo) + sizeof(parametros->nlink) + sizeof(parametros->total_size);
	memcpy(paquete->payload, &parametros->modo, sizeof(parametros->modo));
	offset = sizeof(parametros->modo);
	memcpy(paquete->payload+offset, &parametros->nlink, sizeof(parametros->nlink));
	offset = offset + sizeof(parametros->nlink);
	memcpy(paquete->payload+offset, &parametros->total_size, sizeof(parametros->total_size));

	return EXIT_SUCCESS;
};

DesAttr_resp deserializar_Gettattr_Resp(char* payload){

	DesAttr_resp attr;
	uint16_t off = 0;
	mode_t modo;
	nlink_t nlink;
	uint32_t total_size;

	memcpy(&modo,payload,sizeof(mode_t));
	off = sizeof(mode_t);
	memcpy(&nlink,payload+off,sizeof(nlink_t));
	off = off + sizeof(nlink_t);
	memcpy(&total_size, payload+off,sizeof(uint32_t));

	attr.modo = modo;
	attr.nlink = nlink;
	attr.total_size = (off_t)total_size;

	return attr;
};