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
init.eet

EXTRA_DIST = init.edc

init.eet: Makefile init.edc
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/init/init.edc \
	$(top_builddir)/data/init/init.eet

clean-local:
	rm -f *.eet
	
