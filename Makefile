BUILD_DIR = build
SRC_DIR = src
THIRDPARTY_DIR = thirdparty
TARGET_NAME = mangen
CFLAGS = -Wall -Wextra -std=c11 -pedantic -ggdb
CFLAGS += -I/usr/local/opt/openssl@1.1/include
LDFLAGS = -L/usr/local/opt/openssl@1.1/lib
LDFLAGS += -lssl -lcrypto

.PHONY: all

all: build mangen test

mangen: $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) $(SRC_DIR)/main.c -o $(BUILD_DIR)/$(TARGET_NAME) $(LDFLAGS)

test: $(SRC_DIR)/test.c
	$(CC) $(CFLAGS) $(SRC_DIR)/test.c -I$(THIRDPARTY_DIR) -o $(BUILD_DIR)/test $(LDFLAGS)

build:
	mkdir -p build

clean:
	rm -r build
