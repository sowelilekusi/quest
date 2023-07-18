TARGET = quest
LIBS   = -lm -luiohook -lcjson -lxcb -lXinerama -lX11
CC     = gcc
CFLAGS = -g -Wall
INSTALL_PATH = /usr/local
VERSION = 0.6.0

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst src/%.c, %.o, $(wildcard src/*.c))
HEADERS = $(wildcard *.h)

%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)

install: all
	mkdir -p $(DESTDIR)$(INSTALL_PATH)/bin
	cp -f $(TARGET) $(DESTDIR)$(INSTALL_PATH)/bin
	chmod 755 $(DESTDIR)$(INSTALL_PATH)/bin/$(TARGET)
	mkdir -p $(DESTDIR)$(INSTALL_PATH)/share/man/man1
	sed "s/VERSION/$(VERSION)/g" < docs/$(TARGET).1 > $(DESTDIR)$(INSTALL_PATH)/share/man/man1/$(TARGET).1
	chmod 644 $(DESTDIR)$(INSTALL_PATH)/share/man/man1/$(TARGET).1

uninstall:
	rm -f $(DESTDIR)$(INSTALL_PATH)/bin/$(TARGET)\
		$(DESTDIR)$(INSTALL_PATH)/share/man/man1/$(TARGET).1
