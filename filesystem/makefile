TARGETCLI:= sac_cliente
TARGETSV:= sac_servidor
CC := gcc
CFLAGSCLI := -g -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=27
CFLAGSSV := -g
UTILS := ../utils_suse/
INCDIR := -I$(UTILS)
LFLAGSCLI := -lfuse -lcommons -pthread -lm
LFLAGSSV := -lcommons -pthread -lm
SRCSCLI := sac_handlers.c sac_cliente.c $(UTILS)network.c $(UTILS)message.c
SRCSSV := sac_handlers.c sac_servidor.c $(UTILS)network.c $(UTILS)message.c

all: sac_cliente sac_servidor

sac_cliente:
	$(CC) $(CFLAGSCLI) $(INCDIR) $(SRCSCLI) -o $(TARGETCLI) $(LFLAGSCLI)

sac_servidor:
	$(CC) $(CFLAGSSV) $(INCDIR) $(SRCSSV) -o $(TARGETSV) $(LFLAGSSV)
	
run_cliente:
	./sac_cliente ./home -f -d
	

clean:
	rm -f sac_cliente sac_servidor *.o

