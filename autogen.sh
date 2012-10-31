#!/bin/sh

autoreconf --symlink --install -Wno-portability

if [ -z "$NOCONFIGURE" ]; then
	./configure -C "$@"
fi
