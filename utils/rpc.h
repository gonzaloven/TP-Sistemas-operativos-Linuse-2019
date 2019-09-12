#ifndef RPC_H
#define RPC_H

/*
typedef enum 
{
	UINT32,
	INT,
	CHAR_PTR,
	VOID_PTR,
	SIZE_T
} VAR_TYPE;
*/

typedef struct rpc_function_params_s
{
	char type[20]; 
	char name[20];
	void *value;
}rpc_function_params;

typedef struct rpc_function_s
{
	char fname[20]; //function number
	int nparams;	//number of params
	rpc_function_params params[20];
}rpc_function;

/* This function let's a rpc server
 * ivoke a function called by a client
 * @param json_msg:the message sent by the client 
 * in json format
 */
int rpc_server_invoke(char *json_msg);

/* This function let's a client process call a remote 
 * procedure.
 * @param socket:the socket of the server to send
 * @param name of the functio to call example(alloc,cpy etc..).
 * @param nparams: the number of parameters of the function
 * @param param: the parameters od the function 
 */
int rpc_client_call(int socket,char* name,int nparams,rpc_function_params params[]);

/* This function creates a json message to be sended to a server,the
 * message contains the function name ,it's parameters and the paremters value
 * and type.
 * @param name: name of the function to be called in the server
 * @param nparams: the quantity of parameters the function needs
 * @param: params: list containig the variable type,variable name and value.
 * @return: a json string
 */
char* rpc_form_function_msg(char *name,int nparams,rpc_function_params params[]);
#endif
