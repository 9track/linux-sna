AUTOMAKE_OPTIONS = no-dependencies foreign

INCLUDES=-I../include
CFLAGS=-Wall
LIBS=

EXTRA_DIST = $(man_MANS)

sbin_PROGRAMS = tn5250d

tn5250d_SOURCES = host5250.c tn5250d.c

include_HEADERS = host5250.h tn5250d.h

man_MANS = 

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
