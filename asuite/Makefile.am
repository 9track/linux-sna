AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = $(man_MANS)
INCLUDES = -I../include -I../libasuite

bin_PROGRAMS = acopy aftp aping arexec atell
sbin_PROGRAMS = aftpd anamed apingd arexecd atelld

acopy_LDADD = ../libsna/libcpic.la ../libasuite/libacomm.la \
	../libasuite/libaftp.la
acopy_SOURCES = acopy.c

aftp_LDADD = ../libsna/libcpic.la ../libasuite/libacomm.la \
	../libasuite/libaftp.la
aftp_SOURCES = aftp.c

aftpd_LDADD = ../libsna/libcpic.la ../libasuite/libacomm.la \
	../libasuite/libaftp.la
aftpd_SOURCES = aftpd.c

anamed_LDADD = ../libsna/libcpic.la ../libasuite/libacomm.la \
	../libasuite/libaname.la
anamed_SOURCES = anamed.c

aping_LDADD = ../libsna/libcpic.la ../libasuite/libacomm.la
aping_SOURCES = aping.c

apingd_LDADD = ../libsna/libcpic.la ../libasuite/libacomm.la
apingd_SOURCES = apingd.c

arexec_LDADD = ../libsna/libcpic.la ../libasuite/libacomm.la
arexec_SOURCES = arexec.c

arexecd_LDADD = ../libsna/libcpic.la ../libasuite/libacomm.la
arexecd_SOURCES = arexecd.c

atell_LDADD = ../libsna/libcpic.la ../libasuite/libacomm.la
atell_SOURCES = atell.c

atelld_LDADD = ../libsna/libcpic.la ../libasuite/libacomm.la
atelld_SOURCES = atelld.c

include_HEADERS = acopy.h  aftpd.h  aftp.h  anamed.h  aping.h  arexec.h  atell.h

man_MANS = acopy.8 aftp.8 aftpd.8 aping.8 apingd.8 aname.8 anamed.8 \
	arexec.8 arexecd.8 atell.8 atelld.8

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
