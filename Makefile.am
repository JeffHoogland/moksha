## Process this file with automake to produce Makefile.in

SUBDIRS = intl po src lib client

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.guess \
                       config.h.in config.sub configure install-sh \
		       ltconfig ltmain.sh missing mkinstalldirs \
		       stamp-h.in

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/data; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/data; \
	  for d in $(srcdir)/data/*; do \
	      cp -afr $$d $(DESTDIR)$(pkgdatadir)/data; \
	  done \
	fi

dist-hook:
	if test -d data; then \
	  mkdir $(distdir)/data; \
	  for d in data/*; do \
	      cp -ar $$d $(distdir)/data; \
	  done \
	fi

EXTRA_DIST = README AUTHORS COPYING e.spec
