AUTOMAKE_OPTIONS = foreign

INCLUDES=-I../include -I../libsna
CFLAGS=-Wall

EXTRA_DIST 	= $(man_MANS)

sbin_PROGRAMS 	= snatchd

snatchd_LDADD 	= ../libsna/libattach.la
snatchd_SOURCES = snatchd.c signal.c util.c parsecfg.c

include_HEADERS =
noinst_HEADERS 	= snatchd.h util.h parsecfg.h pathnames.h

man_MANS 	= snatchd.8

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
