TARGET:=libm
CC:=gcc
UTILS:= ../utils/
INCDIR:= -I$(UTILS) -I../libs/libMuse -I../libs/Hilolay
CFLAGS:=-g
LIB-SRCS:= libmuse-test.c $(UTILS)network.c $(UTILS)message.c 
LIBS:= -L../libs/bin/ 
LFLAGS:= -lcommons -lpthread -lmuse
LIBPATH:=../libs/bin

test-libmuse:
	$(CC) $(INCDIR) $(CFLAGS) $(LIB-SRCS) -o $(TARGET) $(LIBS) $(LFLAGS) -Wl,-rpath=$(LIBPATH)

test-libhilolay:
	$(CC) $(INCDIR) $(CFLAGS) libhilolay-test.c ../utils/network.c ../utils/message.c -o libh $(LIBS) -lpthread -lcommons -lhilolay -Wl,-rpath=$(LIBPATH)
all:
	$(CC) $(INCDIR) $(INCDIR) -g main.c message-test.c $(UTILS)message.c -o main -lcunit

clean:
	rm libm
