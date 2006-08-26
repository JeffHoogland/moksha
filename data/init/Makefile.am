MAINTAINERCLEANFILES = Makefile.in
SUBDIRS = images

EDJE_CC = @edje_cc@
EDJE_FLAGS = \
-v \
-id $(top_srcdir)/data/init/images \
-fd $(top_srcdir)/data/fonts \
@EDJE_DEF@

filesdir = $(datadir)/enlightenment/data/init
files_DATA = \
default.edj

EXTRA_DIST = default.edc

default.edj: Makefile default.edc
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/init/default.edc \
	$(top_builddir)/data/init/default.edj

clean-local:
	rm -f *.edj
	
