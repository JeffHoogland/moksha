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
init.edj

EXTRA_DIST = init.edc

init.edj: Makefile init.edc
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/init/init.edc \
	$(top_builddir)/data/init/init.edj

clean-local:
	rm -f *.edj
	
