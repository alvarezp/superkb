
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
cflags-$(DRAWKBLIBS_CAIRO) += $(shell pkg-config x11 renderproto xrender cairo cairo-xlib pangocairo --cflags)

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
OBJS=superkb.o main.o superkbrc.o imagelib.o drawkblib.o debug.o $(obj-y)
LDPARAMS=-lX11 -lm -L/usr/X11R6/lib -L/usr/X11/lib $(ldlibs-y)

#My variables
APP=superkb
SHARED=$(obj-m:.o=.so)

.PHONY : all
all:
	$(MAKE) configuration
	$(MAKE) check_dep
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

.PHONY : check_dep
check_dep:
	@pkg-config xft renderproto xrender xinerama --exists > /dev/null || { \
		echo ; \
		echo "ERROR: Superkb needs the development version of the following packages:"; \
		echo "       * xft"; \
		echo "       * renderproto"; \
		echo "       * xrender"; \
		echo "       * xinerama"; \
		echo ; \
		echo "       In addition, it needs their development headers"; \
		echo "       in order to compile."; \
		echo ; \
		echo "       Please install them according to your distribution"; \
		echo "       instructions. You will need to do it as root."; \
		echo ; \
		echo "       For example:"; \
		echo ; \
		echo "       Fedora: yum install libXft libXft-devel librender \\"; \
		echo "                   xorg-x11-proto-devel libXinerama-devel"; \
		echo "       Debian: apt-get install libxft2 libxft-dev \\"; \
		echo "                   x11proto-render-dev \\"; \
		echo "                   libxinerama1 libxinerama-dev \\"; \
		echo "                   libxrender1 libxrender-dev"; \
		echo "       Ubuntu: apt-get install libxft2 libxft-dev \\"; \
		echo "                   x11proto-render-dev \\"; \
		echo "                   libxinerama1 libxinerama-dev \\"; \
		echo "                   libxrender1 libxrender-dev"; \
		echo ; \
		rm configuration; \
		false ; \
	}

configuration:
	-pkg-config xinerama --exists > /dev/null \
		&& (echo "XINERAMA_SUPPORT=y" >> configuration) \
		|| (echo "XINERAMA_SUPPORT=n" >> configuration)
	-pkg-config gdk-pixbuf-xlib-2.0 --exists > /dev/null \
		&& (echo "PUTICON_GDKPIXBUF=m" >> configuration) \
		|| (echo "PUTICON_GDKPIXBUF=n" >> configuration)
	-imlib2-config --version > /dev/null \
		&& (echo "PUTICON_IMLIB2=m" >> configuration) \
		|| (echo "PUTICON_IMLIB2=n" >> configuration)
	-echo "DRAWKBLIBS_XLIB=m" >> configuration
	-pkg-config x11 renderproto xrender cairo cairo-xlib pangocairo --exists > /dev/null \
		&& (echo "DRAWKBLIBS_CAIRO=m" >> configuration) \
		|| (echo "DRAWKBLIBS_CAIRO=n" >> configuration)
	@. ./configuration; \
		if [ "$$PUTICON_IMLIB2 $$PUTICON_GDKPIXBUF" == "n n" ]; then \
			echo ; \
			echo "ERROR: Superkb needs either gdk-pixbuf-xlib or imlib2."; \
			echo "       In addition, it needs their development headers"; \
			echo "       in order to compile."; \
			echo ; \
			echo "       Please install them according to your distribution"; \
			echo "       instructions. You will need to do it as root."; \
			echo ; \
			echo "       For example:"; \
			echo ; \
			echo "       Fedora: yum install imlib2 imlib2-devel"; \
			echo "       Debian: apt-get install libimlib2 libimlib2-dev"; \
			echo "       Ubuntu: apt-get install libimlib2 libimlib2-dev"; \
			echo ; \
			rm configuration; \
			false ; \
		fi

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
	install man/superkb.1 $(DESTDIR)/usr/share/man/man1/superkb.1


.PHONY : install-shared
install-shared:
	mkdir -p $(DESTDIR)/usr/lib/superkb
	[ -n "$(SHARED)" ] && cp $(SHARED) $(DESTDIR)/usr/lib/superkb/ || true

.PHONY : uninstall
uninstall:
	[ -f /usr/bin/superkb ] && rm $(DESTDIR)/usr/bin/superkb
	[ -d /usr/lib/superkb ] && rm -fr $(DESTDIR)/usr/lib/superkb

.PHONY : clean
clean:
	-/bin/rm -f $(OBJS) $(APP) */*.o */*.so configuration
	-/bin/rm -f `find -name "*~"`
	-/bin/rm -f `find -name core`
	-/bin/rm -f `find -name "core.*"`
