## Process this file with automake to produce Makefile.in

SUBDIRS = intl po src

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.guess \
                       config.h.in config.sub configure install-sh \
		       ltconfig ltmain.sh missing mkinstalldirs \
		       stamp-h.in

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/data; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/data; \
	  for d in $(srcdir)/data/*; do \
	    if test -f $$d; then \
	      $(INSTALL_DATA) $$d $(DESTDIR)$(pkgdatadir)/data; \
	    fi \
	  done \
	fi

dist-hook:
	if test -d data; then \
	  mkdir $(distdir)/data; \
	  for d in data/*; do \
	    if test -f $$d; then \
	      cp -p $$d $(distdir)/d; \
	    fi \
	  done \
	fi

EXTRA_DIST = README AUTHORS COPYING e.spec
