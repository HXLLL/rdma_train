CFLAGS	:= -g3 -Wall
LD		:= gcc
LD_ARM	:= aarch64-linux-gnu-gcc
LDFLAGS	:= ${LDFLAGS} -libverbs -lmemcached

APPS	:= client server

all: ${APPS}

hrd.o: hrd.c hrd.h

client: client.c hrd.o
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)

server: server.c hrd.o
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f *.o ${APPS}
