#!/bin/sh

rm -rf autom4te.cache
rm -f aclocal.m4

echo "Running aclocal..."; aclocal $ACLOCAL_FLAGS -I m4 \
&& echo "Running autoheader..."; autoheader \
&& echo "Running autoconf..."; autoconf \
&& echo "Running libtoolize..."; libtoolize --automake \
&& echo "Running automake..."; automake --add-missing --copy --gnu

###  If you want this, uncomment it.
./configure "$@"
