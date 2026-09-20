CC := cc
CFLAGS := -std=c17 -Wall -Wextra -pedantic

APP := build/cobra
PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin
INSTALL := /usr/bin/install

.PHONY: all run install uninstall clean

all: $(APP)

$(APP): shell.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) shell.c -o $(APP)

run: $(APP)
	./$(APP)

install: $(APP)
	mkdir -p "$(DESTDIR)$(BINDIR)"
	$(INSTALL) -m 755 "$(APP)" "$(DESTDIR)$(BINDIR)/cobra"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/cobra"

clean:
	rm -rf build
