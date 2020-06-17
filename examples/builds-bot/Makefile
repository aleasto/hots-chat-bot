LIB_PATH = ../..

CC      := gcc
CFLAGS  := -fPIC -Wall -Wextra -O2 -I$(LIB_PATH)/include
LDFLAGS := -L$(LIB_PATH) -lhotsbot
TARGET  := buildsbot

SRCS    := $(shell echo *.c)
OBJECTS := $(SRCS:.c=.o)

.PHONY: all clean

all: clean $(TARGET)

clean:
	rm -f *.o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $^

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
