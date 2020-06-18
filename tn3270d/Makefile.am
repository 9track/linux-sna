AUTOMAKE_OPTIONS = foreign

EXTRA_DIST  = $(man_MANS)
AM_CPPFLAGS = -I$(srcdir)/../include -I$(srcdir)/../libtnX -I$(srcdir)/../libsna
AM_CFLAGS   = -Wall -L../libtnX -L../libsna

sbin_PROGRAMS = tn3270d

tn3270d_LDADD	= -ltnX -llua
tn3270d_SOURCES = tn3270_host.c tn3270_list.c tn3270_xwords.c tn3270_xml.c \
		  tn3270_daemon.c

noinst_HEADERS  = tn3270_host.h tn3270_codes.h tn3270_list.h tn3270_xwords.h \
		  tn3270.h
include_HEADERS =

man_MANS = 
