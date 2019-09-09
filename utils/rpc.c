#include "rpc.h"
#include "network.h"
#include "cJSON.h"

//TODO: Change it so the number of parameters is variable
int rpc_client_call(int socket,char *name,int nparams,int param)
{
	Message m;
	MessageHeader header;

	create_message_header(&header,MESSAGE_CALL);

	cJSON *root = cJSON_CreateObject();

	cJSON_AddStringToObject(root,"fname",name);
	cJSON_AddNumberToObject(root,"nparams",nparams);
	cJSON_AddNumberToObject(root,"param",param);
	
	char *json_data = cJSON_Print(root);
	message_alloc_data(&m,strlen(json_data));

	create_string_message(&m,&header,json_data);
	send_message(socket,&m);

	message_free_data(&m);
	cJSON_Delete(root);
	free(json_data);
}

int rpc_server_invoke(char *json_msg)
{
	char func_name[20];
	cJSON *rpc_call = cJSON_Parse(json_msg);
	cJSON *fname = cJSON_GetObjectItem(rpc_call,"fname");
	char *json_data = cJSON_Print(fname);

	strcpy(func_name,json_data);

	//TODO: Use a hashtable for the function names
	//This is a test
	if((strcmp(func_name,"\"alloc\"")) == 0)
	{
		printf("memory allocated\n");
	}
	
	cJSON_Delete(rpc_call);
	free(json_data);
	//cJSON_Delete(fname);
}



