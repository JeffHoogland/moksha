MAINTAINERCLEANFILES = Makefile.in
SUBDIRS = images

EDJE_CC = @edje_cc@
EDJE_FLAGS = \
-v \
-id $(top_srcdir)/data/icons/images \
-fd $(top_srcdir)/data/fonts \
@EDJE_DEF@

filesdir = $(datadir)/enlightenment/data/icons
files_DATA = \
default.edj

EXTRA_DIST = default.edc

default.edj: Makefile default.edc
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/icons/default.edc \
	$(top_builddir)/data/icons/default.edj

clean-local:
	rm -f *.edj
	
