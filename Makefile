CC?=gcc

SRC_FILES = $(wildcard ./src/lib/*.c) $(wildcard ./src/app/main.c)
INCLUDES = -I ./src/lib/

BUILD_COMMAND = $(CFLAGS) $(SRC_FILES) $(INCLUDES) -lSDL2
DEBUG_BUILD_COMMAND = $(BUILD_COMMAND) -g -DENABLE_DEBUG
PROFILE_BUILD_COMMAND = $(BUILD_COMMAND) -p

all:
	$(CC) $(DEBUG_BUILD_COMMAND) -o chester

profile:
	$(CC) $(PROFILE_BUILD_COMMAND) -p -o chester

release:
	$(CC) $(BUILD_COMMAND) -O3 -o chester

clean:
	rm -rf *.o chester
