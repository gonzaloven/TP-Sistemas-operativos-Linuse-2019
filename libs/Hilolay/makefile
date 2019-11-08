CC:= gcc
TARGET:= libhilolay.so
UTILS:= ../../utils/
INCDIR:= -I$(UTILS)
CFLAGS:= -g -c -fpic
SRCS:= hilolay.c $(UTILS)network.c $(UTILS)message.c
LIBFLAGS:= -shared -o
OBJS:= hilolay.o

#all
all:libhilolay

#libhilolay
libhilolay: hilolay.o
	$(CC) -g $(LIBFLAGS) $(TARGET) hilolay.o
	mv -f libhilolay.so ../bin

hilolay.o: makedir hilolay.c
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS)

makedir:
	mkdir -p ../bin

clean:
	rm -f libhilolay *.o

