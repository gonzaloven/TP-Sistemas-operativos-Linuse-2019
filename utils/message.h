#ifndef MESSAGE_H
#define MESSAGE_H

#include <inttypes.h>
#include <stdio.h>

typedef enum{
	FUNCTION_MALLOC,
	FUNCTION_FREE,
	FUNCTION_COPY,
	FUNCTION_GET,
	FUNCTION_MAP,
	FUNCTION_UNMAP,
	FUNCTION_SYNC,
	FUNCTION_GETATTR,
	FUNCTION_RTA_GETATTR,
	FUNCTION_READDIR,
	FUNCTION_RTA_READDIR,
	FUNCTION_OPEN,
	FUNCTION_RTA_OPEN,
	FUNCTION_READ,
	FUNCTION_RTA_READ,
	FUNCTION_MKNOD,
	FUNCTION_WRITE,
	FUNCTION_UNLINK,
	FUNCTION_MKDIR,
	FUNCTION_RMDIR
}FuncType;

typedef enum{
	VAR_UINT32,
	VAR_CHAR_PTR,
	VAR_VOID_PTR,
	VAR_SIZE_T
}VarType;

typedef union{
	uint32_t val_u32;
	char* val_charptr;
	size_t val_sizet;
	void* val_voidptr;
}VarValue;

typedef struct Arg_s
{
	VarType type;
	uint16_t size;
	VarValue value;
}Arg;

typedef struct Function_s
{
	FuncType type;
	uint8_t num_args;
	Arg args[10]; //maximun 10 function arguments
}Function;

typedef struct MessageHeader_s
{
	uint8_t message_type;
	uint16_t caller_id;
	uint16_t data_size;
}MessageHeader;

/* Main message structure for message passing. */
typedef struct Message_s
{
	MessageHeader header;
	void *data;
} Message;

typedef enum{
	MESSAGE_STRING, //Deprecated
	MESSAGE_CALL,
	MESSAGE_FUNCTION_RET,
	MESSAGE_NEW_ULT
}MESSAGE_TYPE;

/* Creates a header for a message.Returns -1 if fails to 
 * do so. 
 * @param header: pointer to the message header
 * @param message_type: the type of message will contain*/
int create_message_header(MessageHeader *header,uint8_t message_type,uint16_t caller_id,uint16_t data_size);
int create_function_message(Message *msg,MessageHeader *header,Function *f);
int create_response_message(Message *msg,MessageHeader *header,uint32_t response);

/* Deserializes a message a stores it in result*/
int message_decode(void *buffer,size_t buf_size,Message *result);
int header_decode(void *buffer,size_t buf_size,MessageHeader *result);
int message_function_decode(void *buffer,size_t buf_size,Message *result); 
int function_arg_decode(void *buffer,size_t buff_size,Arg *arg);

/* Serializes a message and stores it in a buffer stream */
int message_encode(Message *msg,void *buffer,size_t buffer_size);
int header_encode(MessageHeader *header,void *buffer,size_t buffer_size);
int message_function_encode(Message *msg,void *buffer,size_t buffer_size);
int function_arg_encode(Arg arg,void *buffer,size_t buffer_size);

/* Allocate memory in a message for data*/
int message_alloc_data(Message *msg,unsigned int size);
/* Frees data in a message*/
int message_free_data(Message *msg);

#endif
