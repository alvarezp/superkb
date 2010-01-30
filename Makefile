
#Build configuration file.
CONFIGURATION=configuration
include $(CONFIGURATION)

#Module information:
#puticon/puticon-gdkpixbuf
obj-$(PUTICON_GDKPIXBUF) += puticon/puticon-gdkpixbuf.o
syms-$(PUTICON_GDKPIXBUF) += -DWITH_GDKPIXBUF
ldlibs-$(PUTICON_GDKPIXBUF) += $(shell pkg-config gdk-pixbuf-xlib-2.0 --libs)
cflags-$(PUTICON_GDKPIXBUF) += $(shell pkg-config gdk-pixbuf-xlib-2.0 --cflags)

#puticon/puticon-imlib2 (which I prefer)
obj-$(PUTICON_IMLIB2) += puticon/puticon-imlib2.o
syms-$(PUTICON_IMLIB2) += -DWITH_IMLIB2
ldlibs-$(PUTICON_IMLIB2) += $(shell pkg-config imlib2 --libs)
cflags-$(PUTICON_IMLIB2) += $(shell pkg-config imlib2 --cflags)

#drawkblibs/drawkblibs-xlib
obj-$(DRAWKBLIBS_XLIB) += drawkblibs/drawkblibs-xlib.o
syms-$(DRAWKBLIBS_XLIB) += -DWITH_DRAWKBLIBS_XLIB
ldlibs-$(DRAWKBLIBS_XLIB) += $(shell pkg-config x11 --libs)
cflags-$(DRAWKBLIBS_XLIB) += $(shell pkg-config x11 --cflags)

#drawkblibs/drawkblibs-cairo
obj-$(DRAWKBLIBS_CAIRO) += drawkblibs/drawkblibs-cairo.o
syms-$(DRAWKBLIBS_CAIRO) += -DWITH_DRAWKBLIBS_CAIRO
ldlibs-$(DRAWKBLIBS_CAIRO) += $(shell pkg-config x11 renderproto xrender cairo cairo-xlib pangocairo --libs)
cflags-$(DRAWKBLIBS_CAIRO) += $(shell pkg-config x11 renderproto xrender cairo cairo-xlib pangocairo --cflags) -DPANGO_ENABLE_BACKEND

cflags-y += $(shell pkg-config xft --cflags)
ldlibs-y += $(shell pkg-config xft --libs)

#Choose whether Xinerama will be emulated or real.
ifeq ($(XINERAMA_SUPPORT),n)
	obj-y += screeninfo-xlib.o
else
	obj-y += screeninfo-xinerama.o
	ldlibs-y += $(shell pkg-config xinerama --libs)
	cflags-y += $(shell pkg-config xinerama --cflags)
endif

#Special variables
SHELL=/bin/sh
CC=gcc
CFLAGS=-Wall -std=c99 -pedantic-errors $(WEXTRA) $(syms-y) $(cflags-y) $(cflags-m) -ggdb -fPIC
OBJS=superkb.o main.o superkbrc.o imagelib.o drawkblib.o debug.o timeval.o $(obj-y)
LDPARAMS=-lX11 -lm -L/usr/X11R6/lib -L/usr/X11/lib $(ldlibs-y)

#My variables
APP=superkb
SHARED=$(obj-m:.o=.so)

.PHONY : all
all:
	$(MAKE) configuration
	$(MAKE) checkdep
	$(MAKE) $(SHARED) $(APP)

$(APP): $(OBJS)
	$(CC) $(LDPARAMS) -o $(APP) $(OBJS)
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
	-pkg-config xinerama --exists > /dev/null \
		&& (echo "XINERAMA_SUPPORT=y" >> configuration) \
		|| (echo "XINERAMA_SUPPORT=n" >> configuration)
	-pkg-config gdk-pixbuf-xlib-2.0 --exists > /dev/null \
		&& (echo "PUTICON_GDKPIXBUF=m" >> configuration) \
		|| (echo "PUTICON_GDKPIXBUF=n" >> configuration)
	-pkg-config imlib2 --exists > /dev/null \
		&& (echo "PUTICON_IMLIB2=m" >> configuration) \
		|| (echo "PUTICON_IMLIB2=n" >> configuration)
	-echo "DRAWKBLIBS_XLIB=m" >> configuration
	-pkg-config x11 renderproto xrender cairo cairo-xlib pangocairo --exists > /dev/null \
		&& (echo "DRAWKBLIBS_CAIRO=m" >> configuration) \
		|| (echo "DRAWKBLIBS_CAIRO=n" >> configuration)

checkdep:
	@./approve-config

superkb.o: superkb.h
superkbrc.o: superkbrc.h globals.h
imagelib.o: imagelib.h configuration puticon/puticon.h $(obj-y)
drawkblib.o: drawkblib.h configuration drawkblibs/drawkblibs.h $(obj-y)
main.o: superkb.h imagelib.h drawkblib.h superkbrc.h screeninfo.h
drawkblibs/drawkblibs-xlib.o: drawkblibs/drawkblibs-xlib.h configuration
drawkblibs/drawkblibs-cairo.o: drawkblibs/drawkblibs-cairo.h drawkblib.o configuration
puticon/puticon-imlib2.o: puticon/puticon-imlib2.h configuration
puticon/puticon-gdkpixbuf.o: puticon/puticon-gdkpixbuf.h configuration


$(SHARED): %.so: %.o
	gcc $(ldlibs-m) -shared -o $@ $<

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
	cp $(APP) $(DESTDIR)/usr/bin
	@[ -f ${DESTDIR}/usr/bin/superkb ] && { \
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
	install man/superkb.1 $(DESTDIR)/usr/share/man/man1/superkb.1


.PHONY : install-shared
install-shared:
	mkdir -p $(DESTDIR)/usr/lib/superkb
	[ -n "$(SHARED)" ] && cp $(SHARED) $(DESTDIR)/usr/lib/superkb/ || true

.PHONY : uninstall
uninstall:
	$(MAKE) uninstall-app
	$(MAKE) uninstall-man
	$(MAKE) uninstall-shared
	
.PHONY : uninstall-shared
uninstall-shared:
	[ -d /usr/lib/superkb ] && rm -fr $(DESTDIR)/usr/lib/superkb

.PHONY : uninstall-man
uninstall-man:
	rm -fr $(DESTDIR)/usr/share/man/man1/superkb.1

.PHONY : uninstall-app
uninstall-app:
	rm -fr $(DESTDIR)/usr/bin/superkb

.PHONY : clean
clean:
	-/bin/rm -f $(OBJS) $(APP) */*.o */*.so configuration
	-/bin/rm -f `find -name "*~"`
	-/bin/rm -f `find -name core`
	-/bin/rm -f `find -name "core.*"`
