#include "protocol.h"

int path_encode(tMessage message_type, const char *path, tPaquete *pPaquete) {

	int off = 0;
	int pathSize = strlen(path) + 1;

	pPaquete->type = message_type;
	pPaquete->length = sizeof(char) * pathSize + sizeof(uint16_t);

	pPaquete->payload = malloc(sizeof(char) * pPaquete->length);
	memcpy(pPaquete->payload, &pathSize, sizeof(uint16_t));
	off = sizeof(uint16_t);
	memcpy(pPaquete->payload + off, path, pathSize);

	return EXIT_SUCCESS;
}

int path_decode(char* payload, t_Path* tipoPath){

	int off = 0;

	memcpy(&(tipoPath->length), payload, sizeof(uint16_t));
	off = sizeof(uint16_t);

	tipoPath->path = malloc(sizeof(char) * tipoPath->length);
	memcpy(tipoPath->path, payload + off, tipoPath->length);

	return EXIT_SUCCESS;
}

int serializar_Gettattr_Resp(t_GetAttrResp parametros, tPaquete *paquete) {

	uint16_t offset = 0;

	paquete->type = FF_GETATTR;
	paquete->length = sizeof(parametros.modo) + sizeof(parametros.nlink)
			+ sizeof(parametros.total_size);

	paquete->payload = malloc(sizeof(char) * paquete->length);
	memcpy(paquete->payload, &parametros.modo, sizeof(parametros.modo));

	offset = sizeof(parametros.modo);
	memcpy(paquete->payload + offset, &(parametros.nlink),
			sizeof(parametros.nlink));
	offset = offset + sizeof(parametros.nlink);
	memcpy(paquete->payload + offset, &(parametros.total_size),
			sizeof(parametros.total_size));

	return EXIT_SUCCESS;
}

int deserializar_Gettattr_Resp(char *payload, t_GetAttrResp* attr) {

	uint16_t off = 0;

	memcpy(&(attr->modo), payload, sizeof(mode_t));
	off = sizeof(mode_t);
	memcpy(&(attr->nlink), payload + off, sizeof(nlink_t));
	off = off + sizeof(nlink_t);
	memcpy(&(attr->total_size), payload + off, sizeof(uint32_t));

	return EXIT_SUCCESS;
}

int serializar_readdir_Rta(t_list* lista, tPaquete* paquete){

	uint16_t offset = 0;
	uint8_t size = 0;
	int32_t tope = lista->elements_count;
	int32_t nodo;
	char* dir_nombre;
	t_ReaddirResp respuesta;
	paquete->length = 0;
	paquete->type = RTA_READDIR;
	for(nodo = 0; nodo < tope; nodo++) {

		dir_nombre = (char*)list_get(lista,nodo);
		paquete->length = paquete->length + strlen(dir_nombre) + 1 + sizeof(uint8_t);
	}

	respuesta.tamano = paquete->length;

	paquete->payload = malloc(sizeof(char) * paquete->length);

	for(nodo = 0; nodo < tope; nodo++) {

		dir_nombre = (char*)list_get(lista,nodo);
		size = strlen(dir_nombre) + 1;
		memcpy(paquete->payload+offset,&size,sizeof(uint8_t));
		offset = offset + sizeof(uint8_t);
		memcpy(paquete->payload+offset,dir_nombre,size);
		offset = offset + size;
	}

}

int deserializar_Readdir_Rta(char* payload, t_ReaddirResp* respuesta) {

	uint16_t off = 0;
	memcpy(&(respuesta->tamano), payload, sizeof(uint16_t));
	off = sizeof(uint16_t);

	respuesta->lista_nombres = malloc(sizeof(char) * respuesta->tamano);
	memcpy(respuesta->lista_nombres, payload + off, respuesta->tamano);

	return EXIT_SUCCESS;
}

int deserializar_int(char* payload){

	int respuesta;

	memcpy(&respuesta,payload,sizeof(int));

	return respuesta;
}

int read_encode(tMessage message_type, t_Read parameters, tPaquete* paquete){

	uint16_t offset = 0;

	paquete->type = message_type;
	paquete->length = sizeof(char) * parameters.pathLength + sizeof(size_t) + sizeof(off_t);

	paquete->payload = malloc(sizeof(char) * paquete->length);

	memcpy(paquete->payload, &(parameters.pathLength), sizeof(uint16_t));
	offset = sizeof(uint16_t);
	memcpy(paquete->payload + offset, parameters.path, parameters.pathLength);
	offset = offset + parameters.pathLength;
	memcpy(paquete->payload + offset, &(parameters.size), sizeof(size_t));
	offset = offset + sizeof(size_t);
	memcpy(paquete->payload + offset, &(parameters.offset), sizeof(off_t));

	return EXIT_SUCCESS;

}

int deserializar_Read_Rta(char** lectura, char* payload){
	int off = 0;
	int length;

	memcpy(&length, payload, sizeof(uint16_t));
	off = sizeof(uint16_t);

	*lectura = malloc(sizeof(char) * length);
	memcpy(*lectura, payload + off, length);

	return length;
}

int write_encode(tMessage message_type, t_Write parameters, tPaquete* paquete){

	uint16_t offset = 0;
	int pathSize = strlen(parameters.path) + 1;
	int bufSize = strlen(parameters.buf) + 1;

	paquete->type = message_type;
	paquete->length = sizeof(uint16_t) + sizeof(char) * pathSize + sizeof(uint16_t) + sizeof(char) * bufSize + sizeof(size_t) + sizeof(off_t);

	paquete->payload = malloc(sizeof(char) * paquete->length);

	memcpy(paquete->payload, &(parameters.pathLength), sizeof(uint16_t));
	offset = sizeof(uint16_t);
	memcpy(paquete->payload + offset, parameters.path, pathSize);
	offset = offset + pathSize;
	memcpy(paquete->payload, &(parameters.bufLength), sizeof(uint16_t));
	offset = offset + sizeof(uint16_t);
	memcpy(paquete->payload + offset, parameters.buf, pathSize);
	offset = offset + pathSize;
	memcpy(paquete->payload + offset, &(parameters.size), sizeof(size_t));
	offset = offset + sizeof(size_t);
	memcpy(paquete->payload + offset, &(parameters.offset), sizeof(off_t));

	return EXIT_SUCCESS;

}
