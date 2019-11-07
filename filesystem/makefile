TARGET:= sac
CC := gcc
CFLAGS := -g -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=27
UTILS := ../utils/
INCDIR := -I$(UTILS)
LFLAGS := -lfuse -lcommons -pthread
SRCS := sac_handlers.c sac_cliente.c sac_servidor.c $(UTILS)network.c $(UTILS)message.c 

sac: 
	$(CC) $(CFLAGS) $(INCDIR) $(SRCS) -o $(TARGET) $(LFLAGS)

clean:
	rm -f sac *.o

