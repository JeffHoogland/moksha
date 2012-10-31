#!/bin/sh

autoreconf --symlink --install

if [ -z "$NOCONFIGURE" ]; then
	./configure -C "$@"
fi
