CC:= gcc
TARGET:= libhilolay.so
UTILS:= ../../utils/
INCDIR:= -I$(UTILS)
CFLAGS:= -g -c -fpic
SRCS:= hilolay_alumnos.c $(UTILS)network.c $(UTILS)message.c $(UTILS)semaphore.c
LIBFLAGS:= -shared -o
OBJS:= hilolay.o

#all
all:libhilolay

#libhilolay
libhilolay: hilolay.o hilolay_internal.c hilolay_internal.h hilolay_alumnos.h
	$(CC) -g $(LIBFLAGS) $(TARGET) hilolay.o
	mv -f libhilolay.so ../bin

hilolay_alumnos.o: makedir hilolay_alumnos.c
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS)

makedir:
	mkdir -p ../bin

clean:
	rm -f *.o
	rm ../bin/libhilolay.so

