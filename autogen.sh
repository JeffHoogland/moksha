#!/bin/sh

autoreconf --symlink --install

if [ -z "$NOCONFIGURE" ]; then
	exec ./configure -C "$@"
fi
