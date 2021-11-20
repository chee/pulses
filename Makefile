NAME = pulses

VERSION = 1

PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin
TARGET  = $(NAME)

ifneq ($(NAME),)
CCFLAGS += -D_NAME="$(NAME)"
endif

ifneq ($(VERSION),)
CCFLAGS += -D_VERSION="$(VERSION)"
endif

CCFLAGS += -g -O2 -std=c++11
CCFLAGS += -Wno-multichar

LDFLAGS += -ljack -lpthread

SOURCES  = pulses.c

all:	$(TARGET)

$(TARGET):	$(SOURCES)
	clang $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

install:	$(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m755 $(TARGET) $(DESTDIR)$(BINDIR)

uninstall:	$(DESTDIR)$(BINDIR)/$(TARGET)
	rm -vf $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -vf *.o $(TARGET)
