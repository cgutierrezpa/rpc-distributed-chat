
# Parameters
RPC_PATH = ./rpc_store_service
LISTS_PATH = ./lists

SERVER  = server
CLIENT = client
MD5_WS = ./md5/server/endpoint/MD5Publisher
MONITOR = monitor
RPC_SERVER = storeServer

SOURCES_CLNT.c = 
SOURCES_CLNT.h = 
SOURCES_SVC.c = 
SOURCES_SVC.h = 
SOURCES.x = $(RPC_PATH)/store_service.x

TARGETS_SERVER.c = $(LISTS_PATH)/user_list.c $(LISTS_PATH)/msg_list.c $(LISTS_PATH)/read_line.c
TARGETS_SVC.c = $(RPC_PATH)/store_service_svc.c $(RPC_PATH)/store_service_xdr.c 
TARGETS_CLNT.c = $(RPC_PATH)/store_service_clnt.c $(RPC_PATH)/store_service_xdr.c 

OBJECTS_SERVER = $(TARGETS_SERVER.c:%.c=%.o) 
OBJECTS_CLNT = $(SOURCES_CLNT.c:%.c=%.o) $(TARGETS_CLNT.c:%.c=%.o)
OBJECTS_SVC = $(SOURCES_SVC.c:%.c=%.o) $(TARGETS_SVC.c:%.c=%.o)
# Compiler flags 

CPPFLAGS += -D_REENTRANT
CFLAGS += -g -Wall
LDLIBS += -lnsl -lpthread 
LDFLAGS = -L$(INSTALL_PATH)/lib/
 RPCGENFLAGS = 

# Targets 

all : $(MONITOR) $(RPC_SERVER) $(SERVER) $(CLIENT) $(MD5_WS)

$(MONITOR) : $(MONITOR).o $(OBJECTS_CLNT) 
	$(LINK.c) -o $(MONITOR) $(MONITOR).o $(OBJECTS_CLNT) $(LDLIBS)  

$(RPC_SERVER) : $(RPC_SERVER).o $(OBJECTS_SVC) 
	$(LINK.c) -o $(RPC_SERVER) $(RPC_SERVER).o $(OBJECTS_SVC) $(LDLIBS)

$(SERVER) : $(SERVER).o $(OBJECTS_SERVER) $(OBJECTS_CLNT) 
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(CLIENT) :
	javac $(CLIENT).java

$(MD5_WS) :
	javac -cp . $(MD5_WS).java

 clean:
	 $(RM) core $(SERVER) $(MONITOR) $(RPC_SERVER) *.o ./*/*.o *.class ./*/*.class ./*/*/*.class

