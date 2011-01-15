##
## This file is part of the sigrok project.
##
## Copyright (C) 2010 Bert Vermeulen <bert@biot.com>
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

AC_PREREQ(2.61)
AC_INIT([sigrok], [0.1pre2], [sigrok-devel@lists.sourceforge.net],
	[sigrok], [http://www.sigrok.org])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall -Werror foreign dist-bzip2 std-options])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AH_TOP([#ifndef SGIROK_CONFIG_H
#define SIGROK_CONFIG_H    /* To stop multiple inclusions. */])
AH_BOTTOM([#endif /* SIGROK_CONFIG_H */])

# Checks for programs.
CFLAGS="-g -Wall -Wextra"
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
LT_INIT

# Logic analyzer hardware support '--enable' options.

AC_ARG_ENABLE(demo, AC_HELP_STRING([--enable-demo],
	      [enable demo driver support [default=yes]]),
	      [LA_DEMO="$enableval"],
	      [LA_DEMO=yes])
AM_CONDITIONAL(LA_DEMO, test x$LA_DEMO = xyes)
if test "x$LA_DEMO" = "xyes"; then
	AC_DEFINE(HAVE_LA_DEMO, 1, [Demo driver support])
fi

AC_ARG_ENABLE(asix-sigma, AC_HELP_STRING([--enable-asix-sigma],
	      [enable ASIX Sigma support [default=yes]]),
	      [LA_ASIX_SIGMA="$enableval"],
	      [LA_ASIX_SIGMA=yes])
AM_CONDITIONAL(LA_ASIX_SIGMA, test x$LA_ASIX_SIGMA = xyes)
if test "x$LA_ASIX_SIGMA" = "xyes"; then
	AC_DEFINE(HAVE_LA_ASIX_SIGMA, 1, [ASIX Sigma support])
fi

AC_ARG_ENABLE(link-mso19, AC_HELP_STRING([--enable-link-mso19],
	      [enable Link Instruments MSO-19 support [default=yes]]),
	      [LA_LINK_MSO19="$enableval"],
	      [LA_LINK_MSO19=yes])
AM_CONDITIONAL(LA_LINK_MSO19, test x$LA_LINK_MSO19 = xyes)
if test "x$LA_LINK_MSO19" = "xyes"; then
	AC_DEFINE(HAVE_LA_LINK_MSO19, 1, [Link Instruments MSO-19 support])
fi

AC_ARG_ENABLE(ols, AC_HELP_STRING([--enable-ols],
	      [enable OpenBench Logic Sniffer (OLS) support [default=yes]]),
	      [LA_OLS="$enableval"],
	      [LA_OLS=yes])
AM_CONDITIONAL(LA_OLS, test x$LA_OLS = xyes)
if test "x$LA_OLS" = "xyes"; then
	AC_DEFINE(HAVE_LA_OLS, 1, [OpenBench Logic Sniffer (OLS) support])
fi

AC_ARG_ENABLE(saleae-logic, AC_HELP_STRING([--enable-saleae-logic],
	      [enable Saleae Logic support [default=yes]]),
	      [LA_SALEAE_LOGIC="$enableval"],
	      [LA_SALEAE_LOGIC=yes])
AM_CONDITIONAL(LA_SALEAE_LOGIC, test x$LA_SALEAE_LOGIC = xyes)
if test "x$LA_SALEAE_LOGIC" = "xyes"; then
	AC_DEFINE(HAVE_LA_SALEAE_LOGIC, 1, [Saleae Logic support])
fi

AC_ARG_ENABLE(zeroplus-logic-cube,
	      AC_HELP_STRING([--enable-zeroplus-logic-cube],
	      [enable Zeroplus Logic Cube support [default=yes]]),
	      [LA_ZEROPLUS_LOGIC_CUBE="$enableval"],
	      [LA_ZEROPLUS_LOGIC_CUBE=yes])
AM_CONDITIONAL(LA_ZEROPLUS_LOGIC_CUBE, test x$LA_ZEROPLUS_LOGIC_CUBE = xyes)
if test "x$LA_ZEROPLUS_LOGIC_CUBE" = "xyes"; then
	AC_DEFINE(HAVE_LA_ZEROPLUS_LOGIC_CUBE, 1, [Zeroplus Logic Cube support])
fi

# Checks for libraries.

# libglib-2.0 is always needed.
AM_PATH_GLIB_2_0(2.0.0,
	CFLAGS="$CFLAGS $GLIB_CFLAGS"; LIBS="$LIBS $GLIB_LIBS")

# libgthread-2.0 is always needed (e.g. for the demo hardware driver).
AC_CHECK_LIB(gthread-2.0, g_thread_init)

# libusb is only needed for some hardware drivers.
if test "x$LA_SALEAE_LOGIC" != xno \
   -o "x$LA_ASIX_SIGMA" != xno \
   -o "x$LA_ZEROPLUS_LOGIC_CUBE" != xno; then
PKG_CHECK_MODULES([libusb], [libusb-1.0 >= 1.0.5],
	[CFLAGS="$CFLAGS $libusb_CFLAGS";
	LIBS="$LIBS $libusb_LIBS"])
fi

# libzip is always needed.
PKG_CHECK_MODULES([libzip], [libzip >= 0.8],
	[CFLAGS="$CFLAGS $libzip_CFLAGS";
	LIBS="$LIBS $libzip_LIBS"])

# libftdi is only needed for some hardware drivers.
if test "x$LA_ASIX_SIGMA" != xno; then
	PKG_CHECK_MODULES([libftdi], [libftdi >= 0.16],
		[CFLAGS="$CFLAGS $libftdi_CFLAGS";
		LIBS="$LIBS $libftdi_LIBS"])
fi

# libudev is only needed for some hardware drivers.
if test "x$LA_LINK_MSO19" != xno; then
	PKG_CHECK_MODULES([libudev], [libudev >= 151],
		[CFLAGS="$CFLAGS $libudev_CFLAGS";
		LIBS="$LIBS $libudev_LIBS"])
fi

# Python support.
CPPFLAGS_PYTHON=""
LDFLAGS_PYTHON=""
case "$build" in
*linux* | *darwin*)
	# We know that Linux and Darwin / Mac OS X have 'python-config'.
	CPPFLAGS_PYTHON="$(python-config --includes)"
	LDFLAGS_PYTHON="$(python-config --ldflags)"
	;;
*mingw*)
	# We currently hardcode the paths to the Python 2.6 default install
	# location as there's no 'python-config' script on Windows, it seems.
	CPPFLAGS_PYTHON="-I/c/Python26/include"
	LDFLAGS_PYTHON="-L/c/Python26/libs -lpython26"
	;;
*)
	# Everything else is untested, we just hope 'python-config' works.
	CPPFLAGS_PYTHON="$(python-config --includes)"
	LDFLAGS_PYTHON="$(python-config --ldflags)"
	;;
esac
AC_SUBST(CPPFLAGS_PYTHON)
AC_SUBST(LDFLAGS_PYTHON)

# Checks for header files.
# These are already checked: inttypes.h stdint.h stdlib.h string.h unistd.h.
AC_CHECK_HEADERS([fcntl.h sys/time.h termios.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday memset strchr strerror strncasecmp strtol strtoul strtoull])

AC_CONFIG_FILES([Makefile
		 contrib/Makefile
		 contrib/nsis/Makefile
		 contrib/nsis/sigrok.nsi
		 doc/Makefile
		 libsigrok/Makefile
		 libsigrok/hardware/Makefile
		 libsigrok/hardware/asix-sigma/Makefile
		 libsigrok/hardware/common/Makefile
		 libsigrok/hardware/demo/Makefile
		 libsigrok/hardware/link-mso19/Makefile
		 libsigrok/hardware/openbench-logic-sniffer/Makefile
		 libsigrok/hardware/saleae-logic/Makefile
		 libsigrok/hardware/zeroplus-logic-cube/Makefile
		 libsigrok/input/Makefile
		 libsigrok/output/Makefile
		 libsigrok/libsigrok.pc
		 libsigrokdecode/Makefile
		 libsigrokdecode/libsigrokdecode.pc
		 libsigrokdecode/decoders/Makefile
		 cli/Makefile
		])

AC_SUBST(FIRMWARE_DIR, '$(datadir)/sigrok/firmware')
AC_SUBST(DECODERS_DIR, '$(datadir)/sigrok/decoders')
AC_SUBST(MAKEFLAGS, '--no-print-directory --silent')
AC_SUBST(AM_LIBTOOLFLAGS, '--silent')

AC_DEFINE_UNQUOTED(DECODERS_DIR, "$DECODERS_DIR",
                   [The protocol decoders directory.])

AC_OUTPUT

