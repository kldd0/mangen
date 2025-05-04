BUILD_DIR=build
TARGET_NAME=mangen
CFLAGS=-Wall -Wextra -std=c11 -pedantic -ggdb

.PHONY: all

all: build mangen

mangen: main.c
	$(CC) $(CFLAGS) main.c -o $(BUILD_DIR)/$(TARGET_NAME)

build:
	mkdir -p build
