AUTOMAKE_OPTIONS = foreign

EXTRA_DIST  = $(man_MANS) sense.data
AM_CPPFLAGS = -I$(srcdir)/../include
LIBS	    =

bin_PROGRAMS = getsense

getsense_SOURCES = getsense.c

install-data-local:
	$(INSTALL_DATA) -D $(srcdir)/sense.data $(prefix)/lib/getsense/sense.data

noinst_HEADERS =

man_MANS = getsense.1
