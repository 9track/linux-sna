dnl Process this file with autoconf to produce a configure script.
AC_INIT(linux-sna, 0.1.28)
AC_CONFIG_SRCDIR(ltmain.sh)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(include/config.h)
LT_INIT
AC_CONFIG_MACRO_DIRS([m4])

CFLAGS="-Wall"

dnl Checks for programs.
AC_PROG_CC
AM_PROG_LD
AC_SUBST(LD)
AC_EXEEXT
AC_PROG_LIBTOOL
AM_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(strings.h unistd.h fcntl.h sys/file.h sys/ioctl.h sys/time.h \
		syslog.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

dnl Check for XML libraries.
PKG_CHECK_MODULES(XML, libxml-2.0)
CFLAGS="$CFLAGS $XML_CFLAGS"
LIBS="$LIBS $XML_LIBS"

dnl *
dnl * Not using gnome, so look for glib and GTK+ instead.
dnl *
PKG_CHECK_MODULES(GLIB, glib-2.0)
CFLAGS="$CFLAGS $GLIB_CFLAGS"
LIBS="$LIBS $GLIB_LIBS"

dnl Checks for library functions.

AC_TYPE_GETGROUPS
AC_TYPE_SIGNAL
AC_FUNC_WAIT3
AC_CHECK_FUNCS(gettimeofday socket strerror strtoul)

AC_CHECK_LIB(dl, main)
AC_OUTPUT(Makefile include/Makefile docs/Makefile libsna/Makefile \
	snaconfig/Makefile snatchd/Makefile sense/Makefile libasuite/Makefile \
	asuite/Makefile libtnX/Makefile tn3270d/Makefile)
