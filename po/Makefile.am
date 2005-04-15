MAINTAINERCLEANFILES = Makefile.in

MSGFMT = msgfmt

podir =
po_DATA = \
fr.mo \
ja.mo \
es.mo

LANGS = \
fr \
ja \
es

EXTRA_DIST = \
enlightenment.pot \
fr.po \
ja.po \
es.po

ja.mo:
	$(MSGFMT) -f -o $@ ja.po
fr.mo:
	$(MSGFMT) -f -o $@ fr.po
es.mo:
	$(MSGFMT) -f -o $@ es.po

install-poDATA:
uninstall-poDATA:
install-data-local:
	for L in $(LANGS); do \
	  $(INSTALL_DATA) -D \
	  $$L.mo $(DESTDIR)$(datadir)/locale/$$L/LC_MESSAGES/$(PACKAGE).mo; \
	done
	
uninstall:
	for L in $(LANGS); do \
	  rm -f  $(DESTDIR)$(datadir)/locale/$$L/LC_MESSAGES/$(PACKAGE).mo; \
	done

clean-local:
	rm -f *.mo
