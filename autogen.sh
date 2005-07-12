#!/bin/sh

rm -rf autom4te.cache
rm -f aclocal.m4

echo "Running aclocal..."; aclocal $ACLOCAL_FLAGS -I m4 \
&& echo "Running autoheader..."; autoheader \
&& echo "Running autoconf..."; autoconf \
&& echo "Running libtoolize..."; ( libtoolize --automake || glibtoolize --automake ) \
&& echo "Running automake..."; automake --add-missing --copy --gnu \
&& echo "Generating gettext enlightenment.pot template"; \
xgettext \
-n \
-C \
-d enlightenment \
-p po \
--copyright-holder="Enlightenment development team" \
--foreign-user \
--msgid-bugs-address="enlightenment-devel@lists.sourceforge.net" \
-k -k_ -kd_ \
--from-code=UTF-8 \
-o enlightenment.pot \
`find . -name "*.[ch]" -print` 

if [ -z "$NOCONFIGURE" ]; then
	./configure "$@"
fi
