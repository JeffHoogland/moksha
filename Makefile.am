## Process this file with automake to produce Makefile.in

SUBDIRS = intl po src lib client

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.guess \
                       config.h.in config.sub configure install-sh \
		       ltconfig ltmain.sh missing mkinstalldirs \
		       stamp-h.in

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/data; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir); \
	  (cd $(srcdir); tar -cf - data) | (cd $(DESTDIR)$(pkgdatadir); tar -xf -) \
	fi

dist-hook:
	if test -d data; then \
	  (cd $(srcdir); tar -cf - data) | (cd $(distdir); tar -xf -) \
	fi

EXTRA_DIST = README AUTHORS COPYING e.spec
