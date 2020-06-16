CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -pthread
LDFLAGS := -lX11 -lXtst -pthread
PROGS   := bot

.PHONY: all clean

all: clean $(PROGS)

clean:
	rm -f *.o $(PROGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^

bot: mem-stats.o message-list.o x-additions.o string-additions.o talent-data.o bot.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
