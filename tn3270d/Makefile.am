AUTOMAKE_OPTIONS = no-dependencies foreign

INCLUDES=-I../include -I../tn5250d

EXTRA_DIST = $(man_MANS)

sbin_PROGRAMS = tn3270d

tn3270d_SOURCES = host3270.c tn3270d.c

include_HEADERS = host3270.h codes3270.h tn3270d.h

man_MANS = 

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
