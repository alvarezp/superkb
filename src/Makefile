
SHELL=/bin/sh
CC=gcc
CXX=g++
CPPFLAGS=-Wall
CFLAGS=
CXXFLAGS=
OBJS=superkb.o main.o
LDLIBS=-lX11
LDFLAGS=-L/usr/X11/lib
APP=superkb

$(APP):	$(OBJS)
	$(CC) $(LDFLAGS) $(LDLIBS) -o $(APP) $(OBJS)

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
