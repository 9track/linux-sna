INCLUDES=-I../include

bin_PROGRAMS = snaconfig

snaconfig_SOURCES = snaconfig.c parse.c utility.c display.c load.c \
	parse_stdin.c parse_file.c

noinst_HEADERS = paths.h snaconfig.h

man_MANS = sna.conf.5 snaconfig.8

EXTRA_DIST = $(man_MANS)

