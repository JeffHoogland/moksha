MAINTAINERCLEANFILES = Makefile.in
SUBDIRS = images

EDJE_CC = @edje_cc@
EDJE_FLAGS = \
-v \
-id $(top_srcdir)/data/themes/images \
-fd $(top_srcdir)/data/fonts \
@EDJE_DEF@

filesdir = $(datadir)/enlightenment/data/themes
files_DATA = \
default.edj

EXTRA_DIST = \
default.edc

default.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/default.edc \
	$(top_builddir)/data/themes/default.edj

clean-local:
	rm -f *.edj
