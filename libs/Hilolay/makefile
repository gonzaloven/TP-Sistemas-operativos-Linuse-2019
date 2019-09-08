CC:=gcc
TARGET:=libhilolay.so
UTILS:= ../../utils/
INCDIR:= -I$(UTILS)
CFLAGS:= -c -fpic
SRCS:= libhilolay.c $(UTILS)network.c $(UTILS)message.c
LIBFLAGS:= -shared -o 

libhilolay.o:
		$(CC) $(CFLAGS) $(INCDIR) $(SRCS)
			
libhilolay: libhilolay.o
		$(CC) -g $(LIBFLAGS) $(TARGET) libhilolay.o

clean:
		rm *.o

