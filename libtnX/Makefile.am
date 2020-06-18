AUTOMAKE_OPTIONS = foreign

EXTRA_DIST  = $(man_MANS) transmaps
AM_CPPFLAGS = -I$(srcdir)/../include
AM_CFLAGS   = -fPIC -Wall 

lib_LTLIBRARIES = libtnX.la

libtnX_la_LDFLAGS 	= -Wc,-nostartfiles -release $(VERSION)
libtnX_la_SOURCES 	= libtnX.c tnX_buffer.c tnX_log.c tnX_stream.c \
			  tnX_record.c tnX_conf.c

include_HEADERS = libtnX.h tnX_codes.h
noinst_HEADERS	= transmaps.h

man_MANS =
