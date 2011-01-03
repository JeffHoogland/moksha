SUBDIRS = src data doc config

if HAVE_PO

SUBDIRS += po

endif

MAINTAINERCLEANFILES = \
ABOUT-NLS* \
Makefile.in \
aclocal.m4 \
compile \
config.guess \
config.h.in \
config.h.in~ \
config.rpath \
config.sub \
configure \
depcomp \
install-sh \
ltmain.sh \
missing \
$(PACKAGE_TARNAME)-$(PACKAGE_VERSION).tar.gz \
$(PACKAGE_TARNAME)-$(PACKAGE_VERSION).tar.bz2 \
$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)-doc.tar.bz2 \
m4/libtool.m4 \
m4/lt~obsolete.m4 \
m4/ltoptions.m4 \
m4/ltsugar.m4 \
m4/ltversion.m4 \
m4/codeset.m4 \
m4/gettext.m4 \
m4/glibc21.m4 \
m4/iconv.m4 \
m4/intdiv0.m4 \
m4/inttypes_h.m4 \
m4/inttypes.m4 \
m4/inttypes-pri.m4 \
m4/isc-posix.m4 \
m4/lcmessage.m4 \
m4/lib-ld.m4 \
m4/lib-link.m4 \
m4/lib-prefix.m4 \
m4/nls.m4 \
m4/po.m4 \
m4/progtest.m4 \
m4/stdint_h.m4 \
m4/uintmax_t.m4 \
m4/ulonglong.m4

if HAVE_PO

MAINTAINERCLEANFILES += \
po/boldquot.sed \
po/en@boldquot.header \
po/en@quot.header \
po/insert-header.sin \
po/Makefile.in.in \
po/Makevars.template \
po/quot.sed \
po/remove-potcdate.sin \
po/Rules-quot

endif

EXTRA_DIST = config.rpath  README AUTHORS COPYING \
             enlightenment.spec.in enlightenment.spec enlightenment.pc

filesdir = $(datadir)/enlightenment/
files_DATA = AUTHORS COPYING 

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = enlightenment.pc

.PHONY: doc

doc:
	@echo "entering doc/"
	make -C doc doc
