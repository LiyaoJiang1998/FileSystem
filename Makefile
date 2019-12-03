# /**
#  * Name: Liyao Jiang
#  * ID: 1512446
#  */
CC = g++
CFLAGS = -Wall -O2 -Werror

HEADERS = $(wildcard *.h)
C_SOURCES = $(wildcard *.c)
CC_SOURCES = $(wildcard *.cc)

C_OBJECTS = $(C_SOURCES:%.c=%.o)
CC_OBJECTS = $(CC_SOURCES:%.cc=%.o)
OBJECTS = $(C_OBJECTS) $(CC_OBJECTS)

SUBMISSION_FILES = create_fs FileSystem.h FileSystem.cc Makefile readme.md
.PHONY: all clean

fs: $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) -o fs $(OBJECTS)

compile: $(OBJECTS) $(HEADERS)

$(C_OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

$(CC_OBJECTS): %.o: %.cc
	$(CC) $(CFLAGS) -c $^ -o $@

all: fs

clean:
	rm *.o fs
	
compress: $(SUBMISSION_FILES)
	zip -q -r fs-sim.zip $(SUBMISSION_FILES)