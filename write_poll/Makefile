CFLAGS	:= -g3 -Wall
LD		:= gcc
LDFLAGS	:= ${LDFLAGS} -libverbs -lmemcached

APPS	:= client server

all: ${APPS}

client: client.c hrd.o
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)

server: server.c hrd.o
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f *.o ${APPS}