CC = gcc
CFLAGS = -Wall -Os -march=native -mtune=native
LDFLAGS = -lm -lrt
SRCS = main.c launch.c
OBJS = $(SRCS:.c=.o)
BIN = redfst
.PHONY: all clean install
all: $(SRCS) $(OBJS) $(BIN)

install: $(BIN)
	install -m755 -D "$(BIN)" "$(PREFIX)/bin/$(BIN)"

$(BIN) : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -fr $(OBJS) $(BIN)
