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
default.eet

EXTRA_DIST = \
default.edc \
default_background.edc \
default_battery.edc \
default_border.edc \
default_clock.edc \
default_gadman.edc \
default_ibar.edc \
default_menu.edc \
default_pager.edc \
default_temperature.edc

default.eet: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/default.edc \
	$(top_builddir)/data/themes/default.eet

clean-local:
	rm -f *.eet
