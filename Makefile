OBJS := batstr.o config.o batman.o draw.o

LIBS := $(shell pkg-config gtk+-3.0 --libs) -ludev -lpthread -lm
CFLAGS := -Wall $(shell pkg-config gtk+-3.0 --cflags)
CC ?= gcc
BIN := batman

all: build

batstr.o: batstr.c batstr.h
	$(CC) $(CFLAGS) -c batstr.c -o batstr.o

batman.o: batman.c batstr.h
	$(CC) $(CFLAGS) -c batman.c -o batman.o

config.o: config.c
	$(CC) $(CFLAGS) -c config.c -o config.o

draw.o: draw.c
	$(CC) $(CFLAGS) -c draw.c -o draw.o

build: $(OBJS)
	$(CC) -o$(BIN) $(OBJS) $(LIBS)

clean:
	rm -rf $(BIN) $(OBJS)

install: all
	mv batman /usr/bin/
	tar -xzvf batman.tgz
	mkdir -p /usr/share/batman
	mv batman/* /usr/share/batman
	rmdir batman
