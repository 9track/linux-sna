AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = $(man_MANS)
INCLUDES   = -I../include
AM_CFLAGS  = -Wall 

lib_LTLIBRARIES = libappc.la libattach.la libcpic.la \
	liblua.la libnof.la

libappc_la_LDFLAGS 	= -Wc,-nostartfiles -release $(VERSION)
libappc_la_SOURCES 	= libappc.c

libattach_la_LDFLAGS 	= -Wc,-nostartfiles -release $(VERSION)
libattach_la_SOURCES 	= libattach.c

libcpic_la_LDFLAGS 	= -Wc,-nostartfiles -release $(VERSION)
libcpic_la_SOURCES 	= libcpic.c

liblua_la_LDFLAGS 	= -Wc,-nostartfiles -release $(VERSION)
liblua_la_SOURCES 	= liblua.c

libnof_la_LDFLAGS 	= -Wc,-nostartfiles -release $(VERSION)
libnof_la_SOURCES 	= libnof.c

include_HEADERS = appc.h attach.h lua.h nof.h

man_MANS =
