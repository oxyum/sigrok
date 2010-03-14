#!/bin/sh

OS=`uname`
if [ "x$OS" = "xDarwin" ]; then
    LIBTOOLIZE=glibtoolize
else
    LIBTOOLIZE=libtoolize
fi

echo "generating build system..."
touch NEWS AUTHORS ChangeLog
${LIBTOOLIZE} --install --copy --quiet
aclocal
autoheader
automake --add-missing --copy --gnu
autoconf

./configure "$@" && echo "Type 'make' to compile."

