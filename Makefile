
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
ldlibs-$(PUTICON_IMLIB2) += $(shell imlib2-config --libs)
cflags-$(PUTICON_IMLIB2) += $(shell imlib2-config --cflags)

#Special variables
SHELL=/bin/sh
CC=gcc
CFLAGS=-Wall $(syms-y) $(cflags-y) $(cflags-m) -ggdb
OBJS=superkb.o main.o drawkb.o superkbrc.o imagelib.o $(obj-y)
LDPARAMS=-lX11 -lm -L/usr/X11R6/lib -L/usr/X11/lib $(ldlibs-y)

#My variables
APP=superkb
SHARED=$(obj-m:.o=.so)

.PHONY : all
all:
	$(MAKE) configuration
	$(MAKE) $(APP) $(SHARED)

$(APP): $(OBJS)
	$(CC) $(LDPARAMS) -o $(APP) $(OBJS)

configuration:
	-pkg-config gdk-pixbuf-xlib-2.0 --exists > /dev/null \
		&& (echo "PUTICON_GDKPIXBUF=m" >> configuration) \
		|| (echo "PUTICON_GDKPIXBUF=n" >> configuration)
	-imlib2-config --version > /dev/null \
		&& (echo "PUTICON_IMLIB2=m" >> configuration) \
		|| (echo "PUTICON_IMLIB2=n" >> configuration)
	@. ./configuration; \
		if [ "$$PUTICON_IMLIB2 $$PUTICON_GDKPIXBUF" == "n n" ]; then \
		echo ; \
		echo "Error: I didn't find a suitable image library. Please install"; \
		echo "       either Gdk-pixbuf-xlib or Imlib2. You might want to"; \
		echo "       install them from source."; \
		fi

superkb.o: superkb.h
superkbrc.o: superkbrc.h globals.h
drawkb.o: drawkb.h imagelib.h
imagelib.o: imagelib.h configuration puticon/puticon.h $(obj-y)
main.o: superkb.h drawkb.h superkbrc.h
puticon/puticon-imlib2.o: puticon/puticon-imlib2.h configuration
puticon/puticon-gdk-pixbuf.o: puticon/puticon-gdk-pixbuf.h configuration

$(SHARED): %.so: %.o
	ld $(ldlibs-m) -shared -o $@ $<

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
	$(MAKE) install-app
	$(MAKE) install-shared

.PHONY : install-app
install-app:
	cp $(APP) $(DESTDIR)/usr/bin

.PHONY : install-shared
install-shared:
	mkdir -p /usr/lib/superkb
	[ -n "$(SHARED)" ] && cp $(SHARED) $(DESTDIR)/usr/lib/superkb/ || true

.PHONY : uninstall
uninstall:
	[ -f /usr/bin/superkb ] && rm $(DESTDIR)/usr/bin/superkb
	[ -d /usr/lib/superkb ] && rm -fr $(DESTDIR)/usr/lib/superkb

.PHONY : clean
clean:
	-/bin/rm -f $(OBJS) $(APP) $(SHARED) puticon/*.o puticon/*.so configuration
	-/bin/rm -f `find -name "*~"`
	-/bin/rm -f `find -name core`
	-/bin/rm -f `find -name "core.*"`
