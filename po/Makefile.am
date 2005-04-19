MAINTAINERCLEANFILES = Makefile.in

MSGFMT = msgfmt

podir =
po_DATA = \
fr.mo \
ja.mo \
es.mo \
pt.mo \
fi.mo \
ru.mo \
bg.mo

LANGS = \
fr \
ja \
es \
pt \
fi \
ru \
bg

EXTRA_DIST = \
enlightenment.pot \
fr.po \
ja.po \
es.po \
pt.po \
fi.po \
ru.po \
bg.po

ja.mo:
	$(MSGFMT) -f -o $@ ja.po
fr.mo:
	$(MSGFMT) -f -o $@ fr.po
es.mo:
	$(MSGFMT) -f -o $@ es.po
pt.mo:
	$(MSGFMT) -f -o $@ pt.po
fi.mo:
	$(MSGFMT) -f -o $@ fi.po
ru.mo:
	$(MSGFMT) -f -o $@ ru.po
bg.mo:
	$(MSGFMT) -f -o $@ bg.po

install-poDATA:
uninstall-poDATA:
install-data-local:
	for L in $(LANGS); do \
	  $(MKINSTALLDIRS) $(DESTDIR)$(datadir)/locale/$$L/LC_MESSAGES; \
	  $(INSTALL_DATA) \
	  $$L.mo $(DESTDIR)$(datadir)/locale/$$L/LC_MESSAGES/$(PACKAGE).mo; \
	done
	
uninstall:
	for L in $(LANGS); do \
	  rm -f  $(DESTDIR)$(datadir)/locale/$$L/LC_MESSAGES/$(PACKAGE).mo; \
	done

clean-local:
	rm -f *.mo
