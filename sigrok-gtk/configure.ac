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

# We require at least autoconf 2.63 (AC_INIT format changed there).
AC_PREREQ([2.63])

AC_INIT([sigrok-gtk], [0.1.0], [gareth@blacksphere.co.nz],
        [sigrok-gtk], [http://www.sigrok.org])

AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_MACRO_DIR([autostuff])
AC_CONFIG_AUX_DIR([autostuff])

# We require at least automake 1.11 (needed for 'silent rules').
AM_INIT_AUTOMAKE([1.11 -Wall -Werror foreign std-options])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AH_TOP([#ifndef SIGROK_GTK_CONFIG_H
#define SIGROK_GTK_CONFIG_H    /* To stop multiple inclusions. */])
AH_BOTTOM([#endif /* SIGROK_GTK_CONFIG_H */])

CFLAGS="-g -Wall -Wextra"

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Initialize libtool.
LT_INIT

# Initialize pkg-config.
# We require at least 0.22, as "Requires.private" behaviour changed there.
PKG_PROG_PKG_CONFIG([0.22])

AC_CHECK_TOOL([WINDRES], windres, [])
AM_CONDITIONAL([WINDOWS], test x$WINDRES != x)

# Checks for libraries.

PKG_CHECK_MODULES([gtk], [gtk+-2.0 gmodule-2.0],
	[CFLAGS="$CFLAGS $gtk_CFLAGS";
	LIBS="$LIBS $gtk_LIBS"])

PKG_CHECK_MODULES([libsigrok], [libsigrok >= 0.2.0],
	[CFLAGS="$CFLAGS $libsigrok_CFLAGS";
	LIBS="$LIBS $libsigrok_LIBS"])

# Checks for header files.
# These are already checked: inttypes.h stdint.h stdlib.h string.h unistd.h.
# AC_CHECK_HEADERS([])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memset strtoull])

AC_CONFIG_FILES([Makefile
		 contrib/Makefile
		 contrib/sigrok-gtk.nsi
		 doc/Makefile
		])

AC_OUTPUT

echo
echo "sigrok-gtk configuration summary:"
echo
echo "  - Package version: $VERSION"
echo "  - Prefix: $prefix"
echo
echo "Detected libraries:"
echo

# Note: This only works for libs with pkg-config integration.
for lib in "glib-2.0" "gmodule-2.0" "gtk+-2.0" "libsigrok"; do
        if `$PKG_CONFIG --exists $lib`; then
                ver=`$PKG_CONFIG --modversion $lib`
                answer="yes ($ver)"
        else
                answer="no"
        fi
        echo "  - $lib: $answer"
done

echo

