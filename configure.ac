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
AC_INIT([sigrok], [0.1pre], [sigrok-devel@lists.sourceforge.net],
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

# Checks for libraries.
AM_PATH_GLIB_2_0(2.0.0,
	CFLAGS="$CFLAGS $GLIB_CFLAGS"; LIBS="$LIBS $GLIB_LIBS")

PKG_CHECK_MODULES([gmodule], [gmodule-2.0 >= 2.0],
	[CFLAGS="$CFLAGS $gmodule_CFLAGS";
	LIBS="$LIBS $gmodule_LIBS"])

PKG_CHECK_MODULES([libusb], [libusb-1.0 >= 1.0.5],
	[CFLAGS="$CFLAGS $libusb_CFLAGS";
	LIBS="$LIBS $libusb_LIBS"])

PKG_CHECK_MODULES([libzip], [libzip >= 0.8],
	[CFLAGS="$CFLAGS $libzip_CFLAGS";
	LIBS="$LIBS $libzip_LIBS"])

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
		 doc/Makefile
		 libsigrok/Makefile
		 libsigrokdecode/Makefile
		 libsigrokdecode/scripts/Makefile
		 cli/Makefile
		])

AC_SUBST(FIRMWARE_DIR, '$(datadir)/sigrok')
AC_SUBST(MAKEFLAGS, '--no-print-directory --silent')
AC_SUBST(AM_LIBTOOLFLAGS, '--silent')

AC_OUTPUT

