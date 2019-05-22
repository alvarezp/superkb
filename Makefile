
#Build configuration file.
CONFIGURATION=configuration
-include $(CONFIGURATION)

PKG_CONFIG ?= pkg-config

#Module information:
#puticon/puticon-gdkpixbuf
obj-$(PUTICON_GDKPIXBUF) += puticon/puticon-gdkpixbuf.o
syms-$(PUTICON_GDKPIXBUF) += -DWITH_GDKPIXBUF
ldlibs-$(PUTICON_GDKPIXBUF) += $(shell $(PKG_CONFIG) gdk-pixbuf-xlib-2.0 --libs)
cflags-$(PUTICON_GDKPIXBUF) += $(shell $(PKG_CONFIG) gdk-pixbuf-xlib-2.0 --cflags)

#puticon/puticon-imlib2 (which I prefer)
obj-$(PUTICON_IMLIB2) += puticon/puticon-imlib2.o
syms-$(PUTICON_IMLIB2) += -DWITH_IMLIB2
ldlibs-$(PUTICON_IMLIB2) += $(shell $(PKG_CONFIG) imlib2 --libs)
cflags-$(PUTICON_IMLIB2) += $(shell $(PKG_CONFIG) imlib2 --cflags)

#drawkblibs/drawkblibs-xlib
obj-$(DRAWKBLIBS_XLIB) += drawkblibs/drawkblibs-xlib.o
syms-$(DRAWKBLIBS_XLIB) += -DWITH_DRAWKBLIBS_XLIB
ldlibs-$(DRAWKBLIBS_XLIB) += $(shell $(PKG_CONFIG) x11 --libs)
cflags-$(DRAWKBLIBS_XLIB) += $(shell $(PKG_CONFIG) x11 --cflags)

#drawkblibs/drawkblibs-cairo
obj-$(DRAWKBLIBS_CAIRO) += drawkblibs/drawkblibs-cairo.o
syms-$(DRAWKBLIBS_CAIRO) += -DWITH_DRAWKBLIBS_CAIRO
ldlibs-$(DRAWKBLIBS_CAIRO) += $(shell $(PKG_CONFIG) x11 renderproto xrender cairo cairo-xlib pangocairo --libs)
cflags-$(DRAWKBLIBS_CAIRO) += $(shell $(PKG_CONFIG) x11 renderproto xrender cairo cairo-xlib pangocairo --cflags) -DPANGO_ENABLE_BACKEND

cflags-y += $(shell $(PKG_CONFIG) xft --cflags)
ldlibs-y += $(shell $(PKG_CONFIG) xft --libs)

#Choose whether Xinerama will be emulated or real.
ifeq ($(XINERAMA_SUPPORT),n)
	obj-y += screeninfo-xlib.o
else
	obj-y += screeninfo-xinerama.o
	ldlibs-y += $(shell $(PKG_CONFIG) xinerama --libs)
	cflags-y += $(shell $(PKG_CONFIG) xinerama --cflags)
endif

version_extrainfo = $(shell ./extendedversioninfo.bash)

#Conditional -pedantic-errors because of pango 1.32.3, 1.32.4 and 1.32.5.
PEDANTIC_ERRORS := -pedantic-errors
ifeq ($(shell $(PKG_CONFIG) --modversion pango),1.32.3)
       PEDANTIC_ERRORS :=
endif
ifeq ($(shell $(PKG_CONFIG) --modversion pango),1.32.4)
       PEDANTIC_ERRORS :=
endif
ifeq ($(shell $(PKG_CONFIG) --modversion pango),1.32.5)
       PEDANTIC_ERRORS :=
endif

#Directory variables
ifndef PREFIX
	PREFIX=usr/local/
endif

ifndef LIBDIRNAME
	LIBDIRNAME=lib/
endif

MACROS=-DPREFIX=$(PREFIX) -DLIBDIRNAME=$(LIBDIRNAME)

#Replace -I with -isystem
cflags-y := $(subst -I/,-isystem/,$(cflags-y))
cflags-m := $(subst -I/,-isystem/,$(cflags-m))

#Special variables
SHELL=/bin/sh
CC=gcc
CFLAGS+=-Wall -std=c99 $(PEDANTIC_ERRORS) $(WEXTRA) $(syms-y) $(cflags-y) $(cflags-m) -ggdb -fPIC -DVEXTRA=\""$(version_extrainfo)"\" $(MACROS)
OBJS=superkb.o main.o superkbrc.o imagelib.o drawkblib.o debug.o timeval.o $(obj-y)
LDPARAMS=-lX11 -lm -ldl -L/usr/X11R6/lib -L/usr/X11/lib $(ldlibs-y)

#Convenience variables
HELP2MAN=help2man

#My variables
APP=superkb
SHARED=$(obj-m:.o=.so)

MODTYPE ?= m

.PHONY : all
all:
	$(MAKE) configuration
	$(MAKE) checkdep
	$(MAKE) $(SHARED) $(APP)
	$(MAKE) $(APP).1

HELP_STUB=help2man-stub/superkb
$(APP).1: $(APP) superkb.1.inc $(HELP_STUB)
	$(HELP2MAN) -n 'Graphical keyboard launcher with on-screen hints' --help-option=-h --version-option=-v -N -i superkb.1.inc $(HELP_STUB) > $(APP).1

$(APP): $(OBJS)
	$(CC) -o $(APP) $(OBJS) $(LDFLAGS) $(LDPARAMS)
	@[ -f superkb ] && { \
		echo ; \
		echo "Superkb has been successfully compiled!"; \
		echo ; \
		echo "Please proceed to installation by issuing 'make install' as"; \
		echo "root."; \
		echo ; \
		echo "(If you wish to use the program without installing, edit the"; \
		echo "'configuration' file and replace '=m' values with '=y' and"; \
		echo "recompile.)"; \
		echo ; \
	}

configuration:
	-$(PKG_CONFIG) xinerama --exists > /dev/null \
		&& (echo "XINERAMA_SUPPORT=y" >> configuration) \
		|| (echo "XINERAMA_SUPPORT=n" >> configuration)
	-$(PKG_CONFIG) gdk-pixbuf-xlib-2.0 --exists > /dev/null \
		&& (echo "PUTICON_GDKPIXBUF=$(MODTYPE)" >> configuration) \
		|| (echo "PUTICON_GDKPIXBUF=n" >> configuration)
	-$(PKG_CONFIG) imlib2 --exists > /dev/null \
		&& (echo "PUTICON_IMLIB2=$(MODTYPE)" >> configuration) \
		|| (echo "PUTICON_IMLIB2=n" >> configuration)
	-echo "DRAWKBLIBS_XLIB=$(MODTYPE)" >> configuration
	-$(PKG_CONFIG) x11 renderproto xrender cairo cairo-xlib pangocairo --exists > /dev/null \
		&& (echo "DRAWKBLIBS_CAIRO=$(MODTYPE)" >> configuration) \
		|| (echo "DRAWKBLIBS_CAIRO=n" >> configuration)

checkdep:
	@./approve-config

superkb.o: superkb.h
superkbrc.o: superkbrc.h globals.h
imagelib.o: imagelib.h configuration puticon/puticon.h $(obj-y)
drawkblib.o: drawkblib.h configuration drawkblibs/drawkblibs.h $(obj-y)
main.o: superkb.h imagelib.h drawkblib.h superkbrc.h screeninfo.h main-help-message.h
drawkblibs/drawkblibs-xlib.o: drawkblibs/drawkblibs-xlib.h configuration
drawkblibs/drawkblibs-cairo.o: drawkblibs/drawkblibs-cairo.h configuration
puticon/puticon-imlib2.o: puticon/puticon-imlib2.h configuration
puticon/puticon-gdkpixbuf.o: puticon/puticon-gdkpixbuf.h configuration


$(SHARED): %.so: %.o
	gcc -shared -o $@ $< $(LDFLAGS) $(ldlibs-m)

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
	$(MAKE) install-shared
	$(MAKE) install-man
	$(MAKE) install-app

.PHONY : install-app
install-app:
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	cp $(APP) $(DESTDIR)/$(PREFIX)/bin
	@[ -f ${DESTDIR}/$(PREFIX)/bin/superkb ] && { \
		echo ; \
		echo "Superkb has been successfully installed!"; \
		echo ; \
		echo "You will now need a .superkbrc configuration file in your home"; \
		echo "directory. (Not root's home directory, but yours.)"; \
		echo ; \
		echo "There are sample files under the sample-config directory."; \
		echo ; \
		echo "Once you have done that, issue the 'superkb' command."; \
		echo ; \
	}

.PHONY : install-man
install-man:
	mkdir -p $(DESTDIR)/$(PREFIX)/share/man/man1
	install $(APP).1 $(DESTDIR)/$(PREFIX)/share/man/man1/


.PHONY : install-shared
install-shared:
	mkdir -p $(DESTDIR)/$(PREFIX)/$(LIBDIRNAME)/superkb
	[ -n "$(SHARED)" ] && cp $(SHARED) $(DESTDIR)/$(PREFIX)/$(LIBDIRNAME)/superkb/ || true

.PHONY : uninstall
uninstall:
	$(MAKE) uninstall-app
	$(MAKE) uninstall-man
	$(MAKE) uninstall-shared
	
.PHONY : uninstall-shared
uninstall-shared:
	[ -d /$(PREFIX)/$(LIBDIRNAME)/superkb ] && rm -fr $(DESTDIR)/$(PREFIX)/$(LIBDIRNAME)/superkb

.PHONY : uninstall-man
uninstall-man:
	rm -fr $(DESTDIR)/$(PREFIX)/share/man/man1/$(APP).1

.PHONY : uninstall-app
uninstall-app:
	rm -fr $(DESTDIR)/$(PREFIX)/bin/superkb

.PHONY : clean
clean:
	-/bin/rm -f $(OBJS) $(APP) */*.o */*.so configuration
	-/bin/rm -f `find -name "*~"`
	-/bin/rm -f `find -name core`
	-/bin/rm -f `find -name "core.*"`
	-/bin/rm -fr $(APP).1
