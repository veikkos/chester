CC?=gcc

SRC_FILES = $(wildcard ./src/*.c)
INCLUDES = -I.

BUILD_COMMAND = $(CFLAGS) $(SRC_FILES) $(INCLUDES) -lSDL2
DEBUG_BUILD_COMMAND = $(BUILD_COMMAND) -g -DENABLE_DEBUG
PROFILE_BUILD_COMMAND = $(BUILD_COMMAND) -p

all:
	$(CC) $(DEBUG_BUILD_COMMAND) -o gbe

profile:
	$(CC) $(PROFILE_BUILD_COMMAND) -p -o gbe

release:
	$(CC) $(BUILD_COMMAND) -O3 -o gbe

clean:
	rm -rf *.o gbe
