CC      := gcc
CFLAGS  := -Wall -Wextra -O2
TARGET  := proct 
SRC     := proct.c
PREFIX  ?= /usr/local
BINDIR  := $(PREFIX)/bin

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

install: $(TARGET)
	install -d $(BINDIR)
	install -m 0755 $(TARGET) $(BINDIR)/$(TARGET)

uninstall:
	rm -f $(BINDIR)/$(TARGET)

clean:
	$(RM) $(TARGET)
