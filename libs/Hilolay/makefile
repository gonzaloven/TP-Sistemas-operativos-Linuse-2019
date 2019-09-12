CC:=gcc
TARGET:= libhilolay.so
UTILS:= ../../utils/
INCDIR:= -I$(UTILS)
CFLAGS:= -c -fpic
SRCS:= hilolay.c $(UTILS)network.c $(UTILS)message.c
LIBFLAGS:= -shared -o 

makedir:
	mkdir -p ../bin

hilolay.o: makedir
		$(CC) $(CFLAGS) $(INCDIR) $(SRCS) 
			
libhilolay: hilolay.o
		$(CC) -g $(LIBFLAGS) $(TARGET) hilolay.o
		mv -f libhilolay.so ../bin

clean:
		rm *.o

