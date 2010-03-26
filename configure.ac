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
		 backend/Makefile
		 hardware/Makefile
		 hardware/common/Makefile
		 hardware/saleae-logic/Makefile
		 hardware/openbench-logic-sniffer/Makefile
		 frontend/Makefile
		 frontend/cli/Makefile
		])

AC_SUBST(HWPLUGIN_DIR,'$(libdir)/sigrok')
AC_SUBST(FIRMWARE_DIR,'$(datadir)/sigrok')
AC_SUBST(MAKEFLAGS, '--no-print-directory --silent')
AC_SUBST(AM_LIBTOOLFLAGS, '--silent')

AC_OUTPUT

