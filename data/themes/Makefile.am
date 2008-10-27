MAINTAINERCLEANFILES = Makefile.in
SUBDIRS = images

EDJE_CC = @edje_cc@
EDJE_FLAGS = \
-v \
-id $(top_srcdir)/data/themes/images \
-fd $(top_srcdir)/data/themes/fonts \
@EDJE_DEF@

filesdir = $(datadir)/enlightenment/data/themes
files_DATA = \
default.edj \
illume.edj

EXTRA_DIST = \
default.edc \
illume.edc

default.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/default.edc \
	$(top_builddir)/data/themes/default.edj

illume.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/illume.edc \
	$(top_builddir)/data/themes/illume.edj

clean-local:
	rm -f *.edj
