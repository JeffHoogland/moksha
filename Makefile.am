SUBDIRS = src data doc po

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.guess \
                       config.h.in config.sub configure install-sh \
		       ltconfig ltmain.sh missing mkinstalldirs \
		       stamp-h.in acconfig.h \
		       enlightenment.spec

bin_SCRIPTS = enlightenment-config

EXTRA_DIST = config.rpath  README AUTHORS COPYING COPYING-PLAIN \
             enlightenment.spec.in enlightenment.spec

filesdir = $(datadir)/enlightenment/
files_DATA = AUTHORS COPYING 

ACLOCAL_AMFLAGS = -I m4
