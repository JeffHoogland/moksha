MAINTAINERCLEANFILES = Makefile.in
SUBDIRS = images

EDJE_CC = @edje_cc@
EDJE_FLAGS_VERBOSE_ = 
EDJE_FLAGS_VERBOSE_0 = 
EDJE_FLAGS_VERBOSE_1 = -v
EDJE_FLAGS = $(EDJE_FLAGS_$(V)) \
-id $(top_srcdir)/data/themes/images \
-fd $(top_srcdir)/data/themes/fonts \
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
