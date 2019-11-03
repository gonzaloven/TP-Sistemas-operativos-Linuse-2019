#include "../utils/message.h"

void func_encode()
{
	Message msg,msg2;
	MessageHeader header;
	void *buffer[2048];
	create_message_header(&header,MESSAGE_CALL,1,sizeof(Function));
	

	Function f;
	Arg arg[3];
	

	arg[0].type = VAR_UINT32;
	arg[0].size = sizeof(uint32_t);
	arg[0].value.val_u32 = 1;

	arg[1].type = VAR_VOID_PTR;
	arg[1].size = sizeof(uint32_t);
	arg[1].value.val_u32 = 2;

	arg[2].type = VAR_UINT32;
	arg[2].size = sizeof(uint32_t);
	arg[2].value.val_u32 =3 ;

	f.type = FUNCTION_COPY;
	f.num_args = 3;
	f.args[0] = arg[0];
	f.args[1] = arg[1];
	f.args[2] = arg[2];

	create_function_message(&msg,&header,&f);
	message_encode(&msg,buffer,2048);
	
	message_decode(buffer,2048,&msg2);

}

int main(void)
{
	func_encode();
	return 0;
}
