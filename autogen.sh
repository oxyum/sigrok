#!/bin/sh

echo "generating build system..."
touch NEWS AUTHORS ChangeLog
libtoolize --install --copy --quiet
aclocal
autoheader
automake --add-missing --copy --gnu
autoconf

./configure "$@" && echo "Type 'make' to compile."

