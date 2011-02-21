#!/bin/sh

echo "Running autopoint..." ; autopoint -f || :
echo "Running aclocal..." ; aclocal -I m4 $ACLOCAL_FLAGS || exit 1
echo "Running autoconf..." ; autoconf || exit 1
echo "Running autoheader..." ; autoheader || exit 1
echo "Running libtoolize..." ; (libtoolize --copy --automake || glibtoolize --automake) || exit 1
echo "Running automake..." ; automake --add-missing --copy --gnu || exit 1

if [ -z "$NOCONFIGURE" ]; then
	./configure -C "$@"
fi
