CC:=gcc
TARGET:=libhilolay.so
UTILS:= ../../utils/
INCDIR:= -I$(UTILS)
CFLAGS:= -c -fpic
SRCS:= libhilolay.c $(UTILS)network.c $(UTILS)message.c
LIBFLAGS:= -shared -o 

makedir:
	mkdir -p ../bin

libhilolay.o: makedir
		$(CC) $(CFLAGS) $(INCDIR) $(SRCS)
			
libhilolay: libhilolay.o
		$(CC) -g $(LIBFLAGS) $(TARGET) libhilolay.o
		mv -f libhilolay.so ../bin

clean:
		rm *.o

