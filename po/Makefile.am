MAINTAINERCLEANFILES = Makefile.in

MSGFMT = msgfmt

podir =
po_DATA = \
fr.mo \
ja.mo

LANGS = \
fr \
ja

install-data-am: install-data-local
install-data-local:
	for L in $(LANGS); do \
	  $(INSTALL_DATA) -D \
	  $$L.mo $(datadir)/locale/$$L/LC_MESSAGES/$(PACKAGE).mo; \
	done

uninstall-am: uninstall-info-am

EXTRA_DIST = \
enlightenment.pot \
fr.po \
ja.po

.po.mo:
	$(MSGFMT) -f -o $@ $<

clean-local:
	rm -f *.mo
