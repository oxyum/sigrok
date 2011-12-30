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

