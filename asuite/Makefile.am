AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = $(man_MANS)
INCLUDES   = -I../include -I../libasuite
AM_CFLAGS  = -Wall -L../libasuite -L../libsna

bin_PROGRAMS  = acopy aftp aping arexec atell atest
sbin_PROGRAMS = aftpd anamed apingd arexecd atelld

atest_LDADD 	= -lcpic
atest_SOURCES	= atest.c

acopy_LDADD 	= -lcpic -lacomm -laftp
acopy_SOURCES 	= acopy.c

aftp_LDADD 	= -lcpic -lacomm -laftp
aftp_SOURCES 	= aftp.c

aftpd_LDADD 	= -lcpic -lacomm -laftp
aftpd_SOURCES 	= aftpd.c

anamed_LDADD 	= -lcpic -lacomm -laname
anamed_SOURCES 	= anamed.c

aping_LDADD 	= -lcpic -lacomm
aping_SOURCES 	= aping.c

apingd_LDADD 	= -lcpic -lacomm
apingd_SOURCES 	= apingd.c

arexec_LDADD 	= -lcpic -lacomm
arexec_SOURCES 	= arexec.c

arexecd_LDADD 	= -lcpic -lacomm
arexecd_SOURCES = arexecd.c

atell_LDADD	= -lcpic -lacomm
atell_SOURCES 	= atell.c

atelld_LDADD 	= -lcpic -lacomm
atelld_SOURCES 	= atelld.c

include_HEADERS = acopy.h aftpd.h aftp.h anamed.h aping.h arexec.h atell.h

man_MANS = acopy.8 aftp.8 aftpd.8 aping.8 apingd.8 aname.8 anamed.8 \
	arexec.8 arexecd.8 atell.8 atelld.8
