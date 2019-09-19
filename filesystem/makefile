CC := gcc
CFLAGS := -g -D_FILE_OFFSET_BITS=64 
UTILS := ../utils/
INCDIR := -I$(UTILS)
SAC_CLI_SRC := sac_cli.c
SAC_SERV_SRC := sac_server.c $(UTILS)network.c $(UTILS)message.c $(UTILS)rpc.c $(UTILS)cJSON.c
TARGET_SAC_CLI := sac_cli
TARGET_SAC_SERVER := sac_srv
LIBS := -lfuse

sac_cli:
	$(CC) $(CFLAGS) $(SAC_CLI_SRC) -o $(TARGET_SAC_CLI) $(LIBS)

sac_server:
	$(CC) -g $(INCDIR) $(SAC_SERV_SRC) -o $(TARGET_SAC_SERVER) -lpthread -lcommons

