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
if [ "x$OS" = "xDarwin" ]; then
    LIBTOOLIZE=glibtoolize
else
    LIBTOOLIZE=libtoolize
fi

echo "Generating build system..."
touch NEWS AUTHORS ChangeLog
${LIBTOOLIZE} --install --copy --quiet || exit 1
aclocal || exit 1
autoheader || exit 1
automake --add-missing --copy --gnu || exit 1
autoconf || exit 1

./configure "$@" && echo "Type 'make' to compile."

