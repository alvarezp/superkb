
SHELL=/bin/sh
CC=gcc
CXX=g++
CPPFLAGS=-Wall
CFLAGS=$(shell pkg-config --cflags gdk-pixbuf-2.0 gdk-pixbuf-xlib-2.0) -ggdb
CXXFLAGS=
OBJS=superkb.o main.o drawkb.o puticon_gdk_pixbuf_xlib.o
LDLIBS=-lX11 -lm
LDFLAGS=-L/usr/X11R6/lib -L/usr/X11/lib $(shell pkg-config gdk-pixbuf-xlib-2.0 --libs)
APP=superkb

$(APP):	$(OBJS)
	$(CC) $(LDFLAGS) $(LDLIBS) -o $(APP) $(OBJS)

superkb.o main.o: superkb.h

.PHONY : relink
relink:
	-/bin/rm -f $(APP)
	$(MAKE)

.PHONY : force
force:
	$(MAKE) clean
	$(MAKE)

.PHONY : install
install:
	$(MAKE)
	cp superkb /usr/bin

.PHONY : uninstall
uninstall:
	[ -f /usr/bin/superkb ] && rm /usr/bin/superkb

.PHONY : clean
clean:
	-/bin/rm -f $(OBJS) $(APP)
	-/bin/rm -f *.*~
	-/bin/rm -f core
	-/bin/rm -f core.*
