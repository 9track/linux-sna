AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = $(man_MANS)
INCLUDES = -I../include
CFLAGS=-fPIC -Wall 

lib_LTLIBRARIES = libappc.la libattach.la libcpic.la \
	liblua.la libnof.la

libappc_la_LDFLAGS = -Wc,-nostartfiles
libappc_la_SOURCES = libappc.c

libattach_la_LDFLAGS = -Wc,-nostartfiles
libattach_la_SOURCES = libattach.c

libcpic_la_LDFLAGS = -Wc,-nostartfiles
libcpic_la_SOURCES = libcpic.c

liblua_la_LDFLAGS = -Wc,-nostartfiles
liblua_la_SOURCES = liblua.c

libnof_la_LDFLAGS = -Wc,-nostartfiles
libnof_la_SOURCES = libnof.c

include_HEADERS = appc.h attach.h cpic.h lua.h nof.h

man_MANS =

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
