#ifndef MESSAGE_H
#define MESSAGE_H

#include <inttypes.h>
#include <stdio.h>

typedef enum{
	FUNCTION_MALLOC,
	FUNCTION_FREE,
	FUNCTION_COPY,
	FUNCTION_GET
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


/* Creates a header for a message. 
 * @param header: pointer to the message header
 * @param message_type: the type of message will contain
 * @return: 0 if successful, -1 on failure */
int create_message_header(MessageHeader *header,uint8_t message_type,uint16_t data_size);
int create_function_message(Message *msg,MessageHeader *header,Function *f);
int create_response_message(Message *msg,MessageHeader *header,uint16_t response);

/* Deserializes a message a stores it in result
 * @param buffer: pointer to encoded message
 * @param buf_size: size of buffer in bytes
 * @param result: pointer where the decoded message is stored
 * @return: number of bytes of the result, -1 on failure */
int message_decode(void *buffer,size_t buf_size,Message *result);

/* Decodes a message header
 * @param buffer: pointer to encoded header
 * @param buf_size: size of buffer in bytes
 * @param return: pointer where the decoded header is stored
 * @return: size of buffer in bytes, -1 if buffer size is smaller than sizeof(uint8_t) */
int header_decode(void *buffer,size_t buf_size,MessageHeader *result);
int message_function_decode(void *buffer,size_t buf_size,Message *result); 
int function_arg_decode(void *buffer,size_t buff_size,Arg *arg);

/* Serializes a message and stores it in a buffer stream 
 * @param msg: pointer to message to encode
 * @param buffer: pointer to buffer where to store the encoded message
 * @param buffer_size: size of buffer in bytes 
 * @return: size of the buffer in bytes, -1 if buffer size is smaller than sizeof(uint8_t) */
int message_encode(Message *msg,void *buffer,size_t buffer_size);

/* Serializes a header and stores it in a buffer stream 
 * @param msg: pointer to header to encode
 * @param buffer: pointer to buffer where to store the encoded header
 * @param buffer_size: size of buffer in bytes 
 * @return: size of the buffer in bytes, -1 on failure */
int header_encode(MessageHeader *header,void *buffer,size_t buffer_size);
int message_function_encode(Message *msg,void *buffer,size_t buffer_size);
int function_arg_encode(Arg arg,void *buffer,size_t buffer_size);

/* Allocate memory in a message for data
 * @param msg: message to allocate memory for
 * @param size: size in bytes to allocate 
 * @return: 0 if successful, -1 on failure */
int message_alloc_data(Message *msg,unsigned int size);


/* Frees data allocated for a message
 * @param msg: message to free memory from
 * @return: 0 if successful, -1 if data pointer pointed to NULL already */
int message_free_data(Message *msg);

#endif
