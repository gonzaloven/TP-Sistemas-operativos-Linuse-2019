TARGET:=main_test
CC:=gcc
UTILS:= ../utils/
INCDIR:= -I$(UTILS) -I../libs/libMuse -I../libs/Hilolay -I $(UTILS_SUSE)
CFLAGS:= -g
LIB-SRCS:= libmuse-test.c $(UTILS)network.c $(UTILS)message.c $(UTILS_SUSE)libraries.c
LIBS:= -L../libs/bin/ 
LIBPATH:=../libs/bin

test-libmuse:
	$(CC) $(INCDIR) $(CFLAGS) memoria_test_libmuse.c 	$(UTILS)network.c $(UTILS)message.c  -o test_libmuse 	$(LIBS) $(LFLAGS) 
#-lmuse -Wl,

test-libhilolay:
	$(CC) $(INCDIR) $(CFLAGS) libhilolay-test.c 		$(UTILS)network.c $(UTILS)message.c -o libh 			$(LIBS) $(LFLAGS) -lhilolay -Wl,-rpath=$(LIBPATH)

test-muse-alloc:
	$(CC) $(INCDIR) $(CFLAGS) memoria_test_alloc.c 		$(UTILS)network.c $(UTILS)message.c -o muse_test_alloc 	$(LIBS) $(LFLAGS) 
#-lmuse -Wl,

test-hilolay-create:
	$(CC) $(INCDIR) $(CFLAGS) test_hilolay_create.c 	$(UTILS)network.c $(UTILS)message.c -o hilolay_test_create $(LIBS) $(LFLAGS) -lhilolay -Wl,-rpath=$(LIBPATH)

test-filesystem:
	$(CC) $(CFLAGS) filesystem_test.c -o fs_test

all:
	$(CC) $(INCDIR) $(CFLAGS) main.c message-test.c $(UTILS)message.c -o $(TARGET) -lcunit

clean:
	rm $(TARGET)
