
#Build configuration file.
CONFIGURATION=configuration
-include $(CONFIGURATION)

PKG_CONFIG ?= pkg-config

#Module information:
#puticon/puticon-gdkpixbuf
obj-$(PUTICON_GDKPIXBUF) += puticon/puticon-gdkpixbuf.o
syms-$(PUTICON_GDKPIXBUF) += -DWITH_GDKPIXBUF
packages-$(PUTICON_GDKPIXBUF) += gdk-pixbuf-xlib-2.0

#puticon/puticon-imlib2 (which I prefer)
obj-$(PUTICON_IMLIB2) += puticon/puticon-imlib2.o
syms-$(PUTICON_IMLIB2) += -DWITH_IMLIB2
packages-$(PUTICON_IMLIB2) += imlib2

#drawkblibs/drawkblibs-xlib
obj-$(DRAWKBLIBS_XLIB) += drawkblibs/drawkblibs-xlib.o
syms-$(DRAWKBLIBS_XLIB) += -DWITH_DRAWKBLIBS_XLIB
packages-$(DRAWKBLIBS_XLIB) += x11

#drawkblibs/drawkblibs-cairo
obj-$(DRAWKBLIBS_CAIRO) += drawkblibs/drawkblibs-cairo.o
syms-$(DRAWKBLIBS_CAIRO) += -DWITH_DRAWKBLIBS_CAIRO
packages-$(DRAWKBLIBS_CAIRO) += x11 renderproto xrender cairo cairo-xlib pangocairo

packages-y += xft

CLEAN_MORE = */*.o */*.so superkb.1 configuration $(shell find . -iname "*~") core*

#Choose whether Xinerama will be emulated or real.
ifeq ($(XINERAMA_SUPPORT),n)
	obj-y += screeninfo-xlib.o
else
	obj-y += screeninfo-xinerama.o
	packages-y += xinerama
endif

version_extrainfo = $(shell ./extendedversioninfo.bash)

cflags-y := $(shell $(PKG_CONFIG) $(packages-y) --cflags 2> /dev/null) -DPANGO_ENABLE_BACKEND
cflags-m := $(shell $(PKG_CONFIG) $(packages-m) --cflags 2> /dev/null) -DPANGO_ENABLE_BACKEND
ldlibs-y := $(shell $(PKG_CONFIG) $(packages-y) --libs 2> /dev/null)
ldlibs-m := $(shell $(PKG_CONFIG) $(packages-m) --libs 2> /dev/null)
includes-y := $(shell $(PKG_CONFIG) $(packages-y) --cflags-only-I 2> /dev/null)
includes-m := $(shell $(PKG_CONFIG) $(packages-m) --cflags-only-I 2> /dev/null)

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

WEXTRA=-Wextra

#Special variables
SHELL=/bin/sh
CC ?= gcc
WNO=-Wno-unused-parameter
CFLAGS+=-Wall -std=c99 $(PEDANTIC_ERRORS) $(WEXTRA) $(WNO) $(syms-y) $(cflags-y) $(cflags-m) -ggdb -fPIC -DVEXTRA=\""$(version_extrainfo)"\" $(MACROS) -fcommon
OBJS=superkb.o main.o superkbrc.o imagelib.o drawkblib.o debug.o timeval.o $(obj-y)
LDPARAMS=-lX11 -lm -ldl -L/usr/X11R6/lib -L/usr/X11/lib $(ldlibs-y)
SKIP_SPLINT=y

#Convenience variables
HELP2MAN=help2man

#My variables
APP=superkb
SHARED=$(obj-m:.o=.so)

MODTYPE ?= m
LIBS=-lX11 -lm -ldl -L/usr/X11R6/lib -L/usr/X11/lib $(ldlibs-y)

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
	@echo -e '\n'===== $@, building app...
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

OBJS = debug.o timeval.o superkb.o superkbrc.o imagelib.o drawkblib.o main.o $(obj-y)

$(SHARED): %.so: %.o
	$(CC) -shared -o $@ $< $(LDFLAGS) $(ldlibs-m)

.PHONY : relink
relink:
	-/bin/rm -f $(APP)
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

#These defaults are really aggressive. You may want to tweak them.
VALGRIND_EXTRA = --suppressions=/dev/null
CPPCHECK_EXTRA = --suppress=memleakOnRealloc
SPLINT_EXTRA = -unrecog -fullinitblock -initallelements

#Strictly speaking you should rebuild your entire project if you change the
#GNUmakefile, but it can be quite cumbersome if your project is really big
#and you are debugging or hacking the GNUmakefile.
REBUILD_ON=GNUmakefile configuration

# ===== MODIFICATIONS SHOULD NOT BE NEEDED BELOW THIS LINE =====

VALGRIND_LINE = valgrind --error-exitcode=255 --leak-check=full -q --track-origins=yes

CPPCHECK_LINE = cppcheck --error-exitcode=1 --std=c99 --quiet

CLANG_LINE = clang --analyze -pedantic

SPLINT_LINE = splint +quiet -weak

# Disable builtin rules. This lets us avoid all kinds of surprises.
.SUFFIXES:

# ===== MODIFICATIONS SHOULD REALLY NOT BE NEEDED BELOW THIS LINE =====

# Copyright (c) 2013, Octavio Alvarez <alvarezp@alvarezp.com>
# Released under the Simplified BSD License. See the LICENSE file.

# Automatic dependency generation adapted from
# http://www.scottmcpeak.com/autodepend/autodepend.html

ifeq ($(.DEFAULT_GOAL),)
    .DEFAULT_GOAL := all
endif

APP_TESTS = $(wildcard tests/*.t.c)
APP_TESTS_OBJ = $(patsubst tests/%.t.c,.caddeus/testobj/%.to,$(APP_TESTS))
APP_TESTS_BIN = $(patsubst tests/%.t.c,.caddeus/testbin/%.t,$(APP_TESTS))
APP_TESTS_TS = $(patsubst tests/%.t.c,.caddeus/timestamps/%.ts,$(APP_TESTS))

APP_TESTS_TT = $(wildcard tests/*.tt)
APP_TESTS_TTS = $(patsubst tests/%.tt,.caddeus/timestamps/%.tts,$(APP_TESTS_TT))

OBJ_TESTS = $(foreach d,$(patsubst %.o,tests/%/*.t.c,$(OBJS)),$(wildcard $(d)))
OBJ_TESTS_OBJ = $(patsubst tests/%.t.c,.caddeus/testobj/%.to,$(OBJ_TESTS))
OBJ_TESTS_BIN = $(patsubst tests/%.t.c,.caddeus/testbin/%.t,$(OBJ_TESTS))
OBJ_TESTS_TS = $(patsubst tests/%.t.c,.caddeus/timestamps/%.ts,$(OBJ_TESTS))

DONT_HAVE_VALGRIND = $(if $(shell which valgrind),,y)

VALGRIND = $(if $(or $(DONT_HAVE_VALGRIND),$(SKIP_VALGRIND)),,$(VALGRIND_LINE) $(VALGRIND_EXTRA))

DONT_HAVE_CPPCHECK = $(if $(shell which cppcheck),,y)

CPPCHECK = $(if $(or $(DONT_HAVE_CPPCHECK),$(SKIP_CPPCHECK)),true '-- skipping Cppcheck --',$(CPPCHECK_LINE) $(CPPCHECK_EXTRA))

DONT_HAVE_CLANG = $(if $(shell which clang),,y)

CLANG = $(if $(or $(DONT_HAVE_CLANG),$(SKIP_CLANG)),true '-- skipping Clang --',$(CLANG_LINE) $(CLANG_EXTRA))

DONT_HAVE_SPLINT = $(if $(shell which splint),,y)

SPLINT = $(if $(or $(DONT_HAVE_SPLINT),$(SKIP_SPLINT)),true '-- skipping Splint --',$(SPLINT_LINE) $(SPLINT_EXTRA))

DEFAULT_TIMEOUT=0
ifdef TIMEOUT
	DEFAULT_TIMEOUT=$(TIMEOUT)
endif

.PHONY : strict
strict: $(OBJ_TESTS_TS) $(APP_TESTS_TS) $(APP_TESTS_TTS) $(SHARED) $(APP)
	@echo
	@echo "Strict build completed successfully."

.PHONY : check
check: $(SHARED) $(APP) $(OBJ_TESTS_TS) $(APP_TESTS_TS) $(APP_TESTS_TTS)
	@echo
	@echo "Test suite completed successfully."

.PHONY : cheat
cheat:
	mkdir -p $(foreach ts,$(OBJ_TESTS_TS) $(APP_TESTS_TS) $(APP_TESTS_TTS),$(dir $(ts)))
	touch $(foreach ts,$(OBJ_TESTS_TS) $(APP_TESTS_TS) $(APP_TESTS_TTS),$(ts))
	@echo
	@echo "Test suite timestamps generated successfully."

# Pull in dependency info for existing .o and .t files.
-include $(patsubst %.o,.caddeus/dependencies/%.d,$(OBJS))
-include $(patsubst .caddeus/testobj/%.to,.caddeus/dependencies/tests/%.t.d,$(OBJ_TESTS_OBJ) $(APP_TESTS_OBJ))

# All lower targets depend on $(REBUILD_ON) so everything rebuilds if $(REBUILD_ON)
# changes.
$(REBUILD_ON):

# Compile plus generate dependency information.
%.o: %.c $(REBUILD_ON)
	@echo -e '\n'===== $@, building module...
	$(CPPCHECK) $<
	$(SPLINT) $<
	@mkdir -p .caddeus/clang/$(*D)
	$(CLANG) $(CFLAGS) -o .caddeus/clang/$*.plist $<
	$(CC) $(CFLAGS) -o $@ -c $<
	@echo -e '\n'===== $@, generating dependency information...
	@mkdir -p .caddeus/dependencies/$(*D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -M $< | sed '1s,^\(.*\).o:,$*.o:,' \
	  > .caddeus/dependencies/$*.d

.caddeus/timestamps/%.ts: .caddeus/testbin/%.t
	@echo -e '\n'===== running test \"$*\" with timeout=$(DEFAULT_TIMEOUT)...
	@mkdir -p $(@D)
	timeout $(DEFAULT_TIMEOUT) $(VALGRIND) $< && touch $@

.caddeus/timestamps/%.tts: tests/%.tt $(APP)
	@echo -e '\n'===== running test \"$*\" test with timeout=$(DEFAULT_TIMEOUT)...
	@mkdir -p $(@D)
	timeout $(DEFAULT_TIMEOUT) $< && touch $@

.caddeus/testobj/%.to: tests/%.t.c $(REBUILD_ON)
	@echo -e '\n'===== $@, building test module...
	$(CPPCHECK) -I. $<
	$(SPLINT) -I. $<
	@mkdir -p .caddeus/clang/tests/$(*D)
	$(CLANG) $(CFLAGS) -I. -o .caddeus/clang/tests/$*.t.plist $<
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -I. -o $@ -c $<
	@echo -e '\n'===== $@, generating dependency information...
	@mkdir -p .caddeus/dependencies/tests/$(*D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. -M $< | sed '1s,^\(.*\).t.o:,$@:,' \
	  > .caddeus/dependencies/tests/$*.t.d

.caddeus/testbin/%.t: .caddeus/testobj/%.to
	@echo -e '\n'===== $@, building test...
	@mkdir -p $(@D)
	$(CC) $($*_LDFLAGS) $^ -o $@ $($*_LDLIBS)

.PHONY : clean
clean:
	@echo -e '\n'===== Cleaning...
	rm -fr .caddeus
	rm -f $(APP)
	rm -f $(OBJS)
	rm -f $(CLEAN_MORE)

.PHONY : force
force:
	$(MAKE) clean
	$(MAKE)
