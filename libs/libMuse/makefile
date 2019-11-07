CC:=gcc
TARGET:=libmuse.so
UTILS:= ../../utils/
INCDIR:= -I$(UTILS)
CFLAGS:= -c -g -fpic
SRCS:= libmuse.c $(UTILS)network.c $(UTILS)message.c
LIBFLAGS:= -shared -o 

#all
all: libmuse

#libmuse
libmuse: libmuse.o
	$(CC) -g $(LIBFLAGS) $(TARGET) libmuse.o
	mv libmuse.so ../bin

libmuse.o:makedir libmuse.c
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS)

makedir:
	mkdir -p ../bin


clean:
	rm *.o
