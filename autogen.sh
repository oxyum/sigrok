#!/bin/sh

echo "generating build system..."
touch NEWS README AUTHORS ChangeLog
aclocal
autoheader
automake --add-missing --copy --gnu
autoconf

./configure "$@" && echo "Type 'make' to compile."

