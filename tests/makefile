TARGET:=libm
CC:=gcc
UTILS:= ../utils/
INCDIR:= -I$(UTILS) -I../libs/libMuse -I../libs/Hilolay
CFLAGS:= -g
LIB-SRCS:= memoria_test_libmuse.c $(UTILS)network.c $(UTILS)message.c 
LIBS:= -L../libs/bin/ 
LFLAGS:= -lcommons -lpthread -lmuse
LIBPATH:=../libs/bin

test-libmuse:
	$(CC) $(INCDIR) $(CFLAGS) $(LIB-SRCS) -o $(TARGET) $(LIBS) $(LFLAGS) -Wl,-rpath=$(LIBPATH)

test-libhilolay:
	$(CC) $(INCDIR) $(CFLAGS) libhilolay-test.c ../utils/network.c ../utils/message.c -o libh $(LIBS) -lpthread -lcommons -lhilolay -Wl,-rpath=$(LIBPATH)

test-muse-alloc:
	$(CC) $(INCDIR) $(CFLAGS) memoria_test_alloc.c $(UTILS)network.c $(UTILS)message.c -o muse_test_alloc $(LIBS) -lpthread -lcommons -lmuse -Wl,-rpath=$(LIBPATH)

test-hilolay-create:
	$(CC) $(INCDIR) $(CFLAGS) test_hilolay_create.c $(UTILS)network.c $(UTILS)message.c -o hilolay_test_create $(LIBS) -lpthread -lcommons -lhilolay -Wl,-rpath=$(LIBPATH)

test-filesystem:
	gcc -g filesystem_test.c -o fs_test
all:
	$(CC) $(INCDIR) $(INCDIR) -g main.c message-test.c $(UTILS)message.c -o main -lcunit

clean:
	rm libm
