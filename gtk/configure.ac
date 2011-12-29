AC_INIT([sigrok-gtk], [0.1], [gareth@blacksphere.co.nz])
AC_CONFIG_SRCDIR([main.c])
AC_PREREQ(2.57)
AM_INIT_AUTOMAKE([-Wall -Werror foreign std-options])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

CFLAGS="-g -Wall -Wextra"

AC_PROG_CC
AM_PROG_AS
AC_STDC_HEADERS
AC_PROG_INSTALL

# Initialize libtool.
LT_INIT

# Initialize pkg-config.
# We require at least 0.22, as "Requires.private" behaviour changed there.
PKG_PROG_PKG_CONFIG([0.22])

AC_CHECK_TOOL([WINDRES], windres, [])
AM_CONDITIONAL([WINDOWS], test x$WINDRES != x)

PKG_CHECK_MODULES([gtk], [gtk+-2.0 gmodule-2.0],
	[CFLAGS="$CFLAGS $gtk_CFLAGS";
	LIBS="$LIBS $gtk_LIBS"])

PKG_CHECK_MODULES([libsigrok], [libsigrok],
	[CFLAGS="$CFLAGS $libsigrok_CFLAGS";
	LIBS="$LIBS $libsigrok_LIBS"])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

