BUILD_DIR=build
TARGET_NAME=mangen
CFLAGS=-Wall -Wextra -std=c11 -pedantic -ggdb
CFLAGS+=-I/usr/local/opt/openssl@1.1/include
LDFLAGS=-L/usr/local/opt/openssl@1.1/lib
LDFLAGS+=-lssl -lcrypto

.PHONY: all

all: build mangen

mangen: main.c
	$(CC) $(CFLAGS) main.c -o $(BUILD_DIR)/$(TARGET_NAME) $(LDFLAGS)

build:
	mkdir -p build
