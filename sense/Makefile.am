AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = $(man_MANS)
INCLUDES=-I../include
LIBS= -lgdbm

bin_PROGRAMS = sense mksense

sense_SOURCES = sense.c

mksense_SOURCES = mksense.c

noinst_HEADERS = sense.h SenseError-1.0.txt SenseError-1.0.db

man_MANS = sense.1 mksense.1

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
