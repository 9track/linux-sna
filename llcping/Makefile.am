AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = $(man_MANS)
INCLUDES=-I../include

bin_PROGRAMS = llcping

llcping_SOURCES = llcping.c

include_HEADERS =

man_MANS = llcping.8

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
