## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.3e foreign

BUILD_SUBDIRS = . include libsna libasuite snaconfig snatchd asuite llcping \
	sense tn3270d tn5250d docs 
SUBDIRS = $(BUILD_SUBDIRS)
DIST_SUBDIRS = $(BUILD_SUBDIRS) $(CONF_SUBDIRS)

# We need to export these variables
CFLAGS = @CFLAGS@
CPPLAGS = @CPPFLAGS@
LDFLAGS = @LDFLAGS@
LIBS= @LIBS@

EXTRA_DIST = mkstamp BUGS isapplied patch-o-matic

# Patch-o-matic happenings
KERNEL_DIR=/usr/src/linux
PENDING_PATCHES:=net-fun.patch

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else (snarfed from automake/Makefile.am).
##

# Tag before making distribution.  Also, don't make a distribution if
# checks fail.  Also, make sure the NEWS file is up-to-date.
cvs-dist: distcheck
	@if sed '1,2d;3q' $(srcdir)/NEWS | grep -e "$(VERSION)" > /dev/null; then :; else \
	  echo "NEWS not updated; not releasing" 1>&2; \
	  exit 1;				\
	fi
	cd $(srcdir) && \
	 $(CVS) -q tag `echo "release-$(VERSION)" | sed 's/\./-/g'`
	$(MAKE) dist

cvs-diff:
	thisver=`echo "release-$(VERSION)" | sed 's/\./-/g'`; \
	if test -z "$$OLDVERSION"; then \
	  prevno=`echo "$(VERSION)" - 0.01 | bc | sed 's/^\./0./'`; \
	else prevno="$$OLDVERSION"; fi; \
	prevver=release-`echo $$prevno | sed 's/\./-/g'`; \
	$(CVS) -f rdiff -c -r $$prevver -r $$thisver $(PACKAGE) \
	    > $(PACKAGE)-$$prevno-$(VERSION).diff

pending-patches:
	@cd patch-o-matic && KERNEL_DIR=$(KERNEL_DIR) ./runme $(PENDING_PATCHES)
