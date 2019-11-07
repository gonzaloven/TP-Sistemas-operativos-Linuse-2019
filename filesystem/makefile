CC := gcc
CFLAGS := -g -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=27
UTILS := ../utils/
INCDIR := -I$(UTILS)
SAC_CLI_SRC := sac_cliente.c $(UTILS)network.c $(UTILS)message.c 
SAC_SERV_SRC := sac_servidor.c $(UTILS)network.c $(UTILS)message.c
TARGET_SAC_CLI := sac_cliente
TARGET_SAC_SERVER := sac_servidor
LIBSCLI := -lfuse -lcommons -pthread
LIBSSERV := -lcommons -pthread
OBJS := sac_cliente.o sac_handlers.o sac_servidor.o

#ALL
all: sac 

#SAC

sac: $(OBJS)
	$(CC) $(CFLAGS) -o sac $(OBJS)

sac_cliente.o: sac_cliente.c
	$(CC) -g $(INCDIR) $(CFLAGS) $(SAC_CLI_SRC) -o $(TARGET_SAC_CLI) $(LIBSCLI)

sac_servidor.o: sac_servidor.c sac_handlers.c
	$(CC) -g $(INCDIR) $(SAC_SERV_SRC) -o $(TARGET_SAC_SERVER) $(LIBSCLI)

clean:
	rm -f sac *.o

