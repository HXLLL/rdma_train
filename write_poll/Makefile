CFLAGS	:= -g3 -Wall
LD		:= gcc
LDFLAGS	:= ${LDFLAGS} -libverbs

all: client server

client: client.c
	$(LD) $(CFLAGS) client.c -o client $(LDFLAGS)

server: server.c
	$(CC) $(CFLAGS) server.c -o server $(LDFLAGS)
