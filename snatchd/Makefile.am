AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = $(man_MANS)
INCLUDES   = -I../include -I../libsna
AM_CFLAGS  = -Wall -L../libsna

sbin_PROGRAMS 	= snatchd

snatchd_LDADD 	= -lattach
snatchd_SOURCES = snatchd.c snatch_list.c snatch_xml.c snatch_xwords.c

include_HEADERS =
noinst_HEADERS 	= snatchd.h snatch_list.h snatch_xwords.h

man_MANS 	= snatchd.8
