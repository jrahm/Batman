OBJS := batstr.o config.o batman.o

LIBS := $(shell pkg-config gtk+-2.0 --libs) -ludev -lpthread
CFLAGS := -Wall $(shell pkg-config gtk+-2.0 --cflags)
CC ?= gcc
BIN := batman

all: build

batstr.o: batstr.c batstr.h
	$(CC) $(CFLAGS) -c batstr.c -o batstr.o

batman.o: batman.c batstr.h
	$(CC) $(CFLAGS) -c batman.c -o batman.o

config.o: config.c
	$(CC) $(CFLAGS) -c config.c -o config.o

build: $(OBJS)
	$(CC) -o$(BIN) $(OBJS) $(LIBS)

clean:
	rm -rf $(BIN) $(OBJS)
