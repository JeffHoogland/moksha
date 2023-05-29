#!/bin/sh

rm -rf autom4te.cache
rm -f aclocal.m4 config.cache

autoreconf --force --install || exit 1

if [ -z "$NOCONFIGURE" ]; then
  exec ./configure -C "$@"
fi
exit 0
