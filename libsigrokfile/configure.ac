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

AC_PREREQ([2.61])

# libsigrokfile package version number (NOT the same as shared lib version!).
m4_define([libsigrokfile_major_version], [0])
m4_define([libsigrokfile_minor_version], [1])
m4_define([libsigrokfile_micro_version], [0])
m4_define([libsigrokfile_version], [libsigrokfile_major_version.libsigrokfile_minor_version.libsigrokfile_micro_version])

AC_INIT([libsigrokfile], [libsigrokfile_version],
	[sigrok-devel@lists.sourceforge.net],
	[libsigrokfile], [http://www.sigrok.org])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([autostuff])
AC_CONFIG_AUX_DIR([autostuff])

AM_INIT_AUTOMAKE([-Wall -Werror foreign std-options])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AH_TOP([#ifndef LIBSIGROKFILE_CONFIG_H
#define LIBSIGROKFILE_CONFIG_H    /* To stop multiple inclusions. */])
AH_BOTTOM([#endif /* LIBSIGROKFILE_CONFIG_H */])

CFLAGS="-g -Wall -Wextra"

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S

# Initialize libtool.
LT_INIT

# Initialize pkg-config.
# We require at least 0.22, as "Requires.private" behaviour changed there.
PKG_PROG_PKG_CONFIG([0.22])

# Library version for libsigrokfile (NOT the same as the package version).
# Carefully read the libtool docs before updating these numbers!
# The algorithm for determining which number to change (and how) is nontrivial!
# http://www.gnu.org/software/libtool/manual/libtool.html#Updating-version-info
LIBSIGROKFILE_LT_CURRENT=0
LIBSIGROKFILE_LT_REVISION=0
LIBSIGROKFILE_LT_AGE=0
LIBSIGROKFILE_LT_VERSION="$LIBSIGROKFILE_LT_CURRENT:$LIBSIGROKFILE_LT_REVISION:$LIBSIGROKFILE_LT_AGE"
LIBSIGROKFILE_LT_LDFLAGS="-version-info $LIBSIGROKFILE_LT_VERSION"
AC_SUBST(LIBSIGROKFILE_LT_LDFLAGS)

# Checks for libraries.

# This variable collects the pkg-config names of all detected libs.
# It is then used to construct the "Requires.private:" field in the
# libsigrokfile.pc file.
LIBSIGROKFILE_PKGLIBS=""

# libglib-2.0 is always needed.
AM_PATH_GLIB_2_0([2.22.0],
	[CFLAGS="$CFLAGS $GLIB_CFLAGS"; LIBS="$LIBS $GLIB_LIBS"])

# TODO: gthread-2.0 later?
# TODO: libzip later?

AC_SUBST(LIBSIGROKFILE_PKGLIBS)

# Checks for header files.
# These are already checked: inttypes.h stdint.h stdlib.h string.h unistd.h.
AC_CHECK_HEADERS([fcntl.h sys/time.h termios.h])

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
AC_CHECK_FUNCS([gettimeofday memset strchr strcspn strdup strerror strncasecmp strstr strtol strtoul strtoull])

AC_SUBST(MAKEFLAGS, '--no-print-directory')
AC_SUBST(AM_LIBTOOLFLAGS, '--silent')

LIBSIGROKFILE_MAJOR_VERSION=libsigrokfile_major_version
LIBSIGROKFILE_MINOR_VERSION=libsigrokfile_minor_version
LIBSIGROKFILE_MICRO_VERSION=libsigrokfile_micro_version
LIBSIGROKFILE_VERSION=libsigrokfile_version

AC_SUBST(LIBSIGROKFILE_MAJOR_VERSION)
AC_SUBST(LIBSIGROKFILE_MINOR_VERSION)
AC_SUBST(LIBSIGROKFILE_MICRO_VERSION)
AC_SUBST(LIBSIGROKFILE_VERSION)

AC_DEFINE(LIBSIGROKFILE_MAJOR_VERSION, [libsigrokfile_major_version],
	  [Define to the libsigrokfile major package version])
AC_DEFINE(LIBSIGROKFILE_MINOR_VERSION, [libsigrokfile_minor_version],
	  [Define to the libsigrokfile minor package version])
AC_DEFINE(LIBSIGROKFILE_MICRO_VERSION, [libsigrokfile_micro_version],
	  [Define to the libsigrokfile micro package version])
AC_DEFINE(LIBSIGROKFILE_VERSION, [libsigrokfile_version],
	  [Define to the libsigrokfile package version])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT

echo
echo "libsigrokfile configuration summary:"
echo
echo "  - Package version (major.minor.micro):    $LIBSIGROKFILE_VERSION"
echo "  - Library version (current:revision:age): $LIBSIGROKFILE_LT_VERSION"
echo "  - Prefix: $prefix"
echo
echo "Detected libraries:"
echo

# Note: This only works for libs with pkg-config integration.
for lib in "glib-2.0"; do
	if `$PKG_CONFIG --exists $lib`; then
		ver=`$PKG_CONFIG --modversion $lib`
		answer="yes ($ver)"
	else
		answer="no"
	fi
	echo "  - $lib: $answer"
done

echo
echo "Enabled input file formats:"
echo
echo "  - TODO"
echo
echo "Enabled output file formats:"
echo
echo "  - TODO"
echo

