MAKE    := make
CC      := gcc
CFLAGS  := -fPIC -Wall -Wextra -O2 -pthread -Iinclude
LDFLAGS := -shared -lX11 -lXtst -pthread
TARGET  := libhotsbot.so

SRCS    := $(wildcard *.c)
OBJECTS := $(SRCS:.c=.o)

SUBDIRS := $(wildcard **/*/.)

.PHONY: all clean $(SUBDIRS)

all: clean $(TARGET) $(SUBDIRS)

clean:
	rm -f *.o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $^

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(SUBDIRS):
	$(MAKE) -C $@
