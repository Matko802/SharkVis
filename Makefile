CC ?= cc
CFLAGS ?= -O2
CFLAGS += -std=c11 -Wall -Wextra -pedantic
PKG_CONFIG ?= pkg-config
CPPFLAGS += -D_POSIX_C_SOURCE=200809L
CPPFLAGS += $(shell $(PKG_CONFIG) --cflags libpulse-simple)
LDLIBS += $(shell $(PKG_CONFIG) --libs libpulse-simple) -lm -pthread
PREFIX ?= /usr/local

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
BIN := SharkVis

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

install: $(BIN)
	install -Dm755 $(BIN) $(DESTDIR)$(PREFIX)/bin/$(BIN)

clean:
	rm -f $(BIN) $(OBJ)

.PHONY: all install clean
