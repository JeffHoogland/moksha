SUBDIRS = src data doc po config

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.guess \
                       config.h.in config.sub configure install-sh \
		       ltconfig ltmain.sh missing mkinstalldirs \
		       stamp-h.in acconfig.h \
		       enlightenment.spec enlightenment.pc

EXTRA_DIST = config.rpath  README AUTHORS COPYING COPYING-PLAIN \
             enlightenment.spec.in enlightenment.spec enlightenment.pc

filesdir = $(datadir)/enlightenment/
files_DATA = AUTHORS COPYING 

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = enlightenment.pc
