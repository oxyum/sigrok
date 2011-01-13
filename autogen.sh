#!/bin/sh
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

OS=`uname`

LIBTOOLIZE=libtoolize
ACLOCAL_DIR=

if [ "x$OS" = "xDarwin" ]; then
	LIBTOOLIZE=glibtoolize
elif [ "x$OS" = "xMINGW32_NT-5.1" ]; then
	ACLOCAL_DIR="-I /usr/local/share/aclocal"
fi

echo "Generating build system..."
${LIBTOOLIZE} --install --copy --quiet || exit 1
aclocal ${ACLOCAL_DIR} || exit 1
autoheader || exit 1
automake --add-missing --copy --foreign || exit 1
autoconf || exit 1

