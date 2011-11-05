AC_INIT([sigrok-gtk], [0.1], [gareth@blacksphere.co.nz])
AC_CONFIG_SRCDIR([main.c])
AC_PREREQ(2.57)
AM_INIT_AUTOMAKE([foreign])

AC_PROG_CC
AM_PROG_AS
AC_STDC_HEADERS
AC_PROG_INSTALL

AC_CHECK_TOOL([WINDRES], windres, [])
AM_CONDITIONAL([WINDOWS], test x$WINDRES != x)

PKG_CHECK_MODULES([gtk], [gtk+-2.0 gmodule-2.0],
	[CFLAGS="$CFLAGS $gtk_CFLAGS";
	LIBS="$LIBS $gtk_LIBS"])

PKG_CHECK_MODULES([libsigrok], [libsigrok],
	[CFLAGS="$CFLAGS $libsigrok_CFLAGS";
	LIBS="$LIBS $libsigrok_LIBS"])

CPPFLAGS_PYTHON="$(python-config --includes)"
LDFLAGS_PYTHON="$(python-config --ldflags)"
AC_SUBST(CPPFLAGS_PYTHON)
AC_SUBST(LDFLAGS_PYTHON)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

