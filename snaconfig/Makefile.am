AUTOMAKE_OPTIONS = foreign

EXTRA_DIST  = $(man_MANS)
AM_CPPFLAGS = -I$(srcdir)/../include -I$(srcdir)/../libsna
AM_CFLAGS   = -Wall -L../libsna 

sbin_PROGRAMS = snaconfig

snaconfig_LDADD   = -lappc -lattach -lcpic -llua -lnof
snaconfig_SOURCES = snaconfig.c sna_list.c sna_xwords.c sna_find.c \
	sna_load.c sna_ctrl.c sna_show.c sna_utility.c

include_HEADERS =
noinst_HEADERS	= snaconfig.h sna_list.h sna_xwords.h

man_MANS 	= snaconfig.8 sna.conf.5
