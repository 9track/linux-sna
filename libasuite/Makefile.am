AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = $(man_MANS)
INCLUDES   = -I../include
AM_CFLAGS  = -Wall

lib_LTLIBRARIES = libacomm.la libaftp.la libaname.la 

libacomm_la_LDFLAGS = -Wc,-nostartfiles -release $(VERSION)
libacomm_la_SOURCES = asuitecommon.c cpicsession.c wildcard.c filemode.c \
		      util.c tdb.c

libaftp_la_LDFLAGS  = -Wc,-nostartfiles -release $(VERSION)
libaftp_la_SOURCES  = libaftp.c filemode.c

libaname_la_LDFLAGS = -Wc,-nostartfiles -release $(VERSION)
libaname_la_SOURCES = libaname.c

include_HEADERS = anameapi.h appfftp.h asuitecommon.h cpicsession.h \
	filemode.h libaftp.h libaname.h port.h tdb.h util.h wildcard.h

man_MANS =
