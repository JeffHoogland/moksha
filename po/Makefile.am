MAINTAINERCLEANFILES = Makefile.in

MSGFMT = msgfmt

podir =
po_DATA = \
ja.mo

LANGS = \
ja

install-data-local:
	for L in $(LANGS); do \
	  $(INSTALL_DATA) -D \
	  $$L.mo $(datadir)/locale/$$L/LC_MESSAGES/$(PACKAGE).mo; \
	done
	
EXTRA_DIST = \
enlightenment.pot \
ja.po

.po.mo:
	$(MSGFMT) -f -o $@ $<

clean-local:
	rm -f *.mo
