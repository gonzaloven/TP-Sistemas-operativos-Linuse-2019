#ifndef MESSAGE_H
#define MESSAGE_H

#include <inttypes.h>
#include <stdio.h>

typedef struct MessageHeader_s
{
	uint8_t message_type;
}MessageHeader;

/* Main message structure for message passing. */
typedef struct Message_s
{
	MessageHeader header;
	uint16_t data_size;
	void *data;
}Message;

typedef enum{
	MESSAGE_STRING = 1,
	MESSAGE_CALL
}MESSAGE_TYPE;

/* Creates a header for a message.Returns -1 if fails to 
 * do so. 
 * @param header: pointer to the message header
 * @param message_type: the type of message will contain*/
int create_message_header(MessageHeader *header,uint8_t message_type);

/* Creates message with string as data.Careful with the null terminating 
 * character!.
 * @param msg: pointer to the message
 * @param: pointer to a header to include in the message
 * @oaram: string to put in the message 
 * */
int create_string_message(Message *msg,MessageHeader *header,char *data);

/* Deserializes a message a stores it in result*/
int message_decode(void *buffer,size_t buf_size,Message *result);
int header_decode(void *buffer,size_t buf_size,MessageHeader *result);
int message_string_decode(void *buffer,size_t buf_size,Message *result);

/* Serializes a message and stores it in a buffer stream */
int message_encode(Message *msg,void *buffer,size_t buffer_size);
int header_encode(MessageHeader *header,void *buffer,size_t buffer_size);
int message_string_encode(Message *msg,void *buffer,size_t buffer_size);

/* Allocate memory in a message for data*/
int message_alloc_data(Message *msg,unsigned int size);
/* Frees data in a message*/
int message_free_data(Message *msg);

#endif
