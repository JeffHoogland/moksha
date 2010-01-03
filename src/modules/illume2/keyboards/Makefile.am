MAINTAINERCLEANFILES = Makefile.in
MODULE = illume2

filesdir = $(libdir)/enlightenment/modules/$(MODULE)/keyboards
files_DATA = ignore_built_in_keyboards

EXTRA_DIST = $(files_DATA)

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)/keyboards

