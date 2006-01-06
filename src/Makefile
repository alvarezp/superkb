
SHELL=/bin/sh
CC=gcc
CXX=g++
CPPFLAGS=-Wall
CFLAGS=`pkg-config --cflags gdk-pixbuf-2.0 gdk-pixbuf-xlib-2.0` -ggdb
CXXFLAGS=
OBJS=superkb.o main.o
LDLIBS=-lX11
LDFLAGS=-L/usr/X11/lib `pkg-config gdk-pixbuf-xlib-2.0 --libs`
APP=superkb

$(APP):	$(OBJS)
	$(CC) $(LDFLAGS) $(LDLIBS) -o $(APP) $(OBJS)

superkb.o main.o: superkb.h

relink: 
	-/bin/rm -f $(APP)
	$(MAKE)

.PHONY : force
force:
	$(MAKE) clean
	$(MAKE)

.PHONY : clean
clean:
	-/bin/rm -f $(OBJS) $(APP)
	-/bin/rm -f *.*~
	-/bin/rm -f core.*
