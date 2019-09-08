CC:=gcc
TARGET:=libmuse.so
UTILS:= ../../utils/
INCDIR:= -I$(UTILS)
CFLAGS:= -c -fpic
SRCS:= libmuse.c $(UTILS)network.c $(UTILS)message.c
LIBFLAGS:= -shared -o 

libmuse.o:
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS)
	
libmuse: libmuse.o
	$(CC) -g $(LIBFLAGS) $(TARGET) libmuse.o

#libmuse: libmuse.o libmuse.a

clean:
	rm *.o
