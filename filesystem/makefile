TARGETCLI:= sac_cliente
TARGETSV:= sac_servidor
CC := gcc
CFLAGSCLI := -g -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=27
CFLAGSSV := -g
UTILS := ../utils/
INCDIR := -I$(UTILS)
LFLAGSCLI := -lfuse -lcommons -pthread
LFLAGSSV := -lcommons -pthread
SRCSCLI := sac_handlers.c sac_cliente.c $(UTILS)network.c $(UTILS)message.c
SRCSSV := sac_servidor.c $(UTILS)network.c $(UTILS)message.c 

sac_cliente: 
	$(CC) $(CFLAGS) $(INCDIR) $(SRCSCLI) -o $(TARGETCLI) $(CFLAGSCLI)

sac_servidor:
	$(CC) $(CFLAGS) $(INCDIR) $(SRCSSV) -o $(TARGETSV) $(CFLAGSSV)

clean:
	rm -f sac_cliente sac_servidor *.o

