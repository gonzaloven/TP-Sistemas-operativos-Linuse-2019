CC:=gcc
TARGET:=libmuse.so
UTILS:= ../../utils/
INCDIR:= -I$(UTILS)
CFLAGS:= -c -g -fpic
SRCS:= libmuse.c $(UTILS)network.c $(UTILS)message.c
LIBFLAGS:= -shared -o 

makedir:
	mkdir -p ../bin

libmuse.o:makedir
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS)
	
libmuse: libmuse.o
	$(CC) -g $(LIBFLAGS) $(TARGET) libmuse.o
	mv libmuse.so ../bin


clean:
	rm *.o
