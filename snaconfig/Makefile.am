AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = $(man_MANS)
INCLUDES=-I../include -I../libsna
CFLAGS=-Wall 

sbin_PROGRAMS = snaconfig

snaconfig_LDADD = ../libsna/libappc.la ../libsna/libattach.la \
	../libsna/libcpic.la ../libsna/liblua.la \
	../libsna/libnof.la
snaconfig_SOURCES = snaconfig.c display.c utility.c parse.c load.c \
	parse_file.c parse_stdin.c

include_HEADERS = snaconfig.h paths.h

man_MANS = snaconfig.8 sna.conf.5

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
