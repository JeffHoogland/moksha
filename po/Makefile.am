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
bg.mo \
de.mo \
pl.mo \
zh_CN.mo \
hu.mo \
sl.mo \
it.mo \
cs.mo

LANGS = \
fr \
ja \
es \
pt \
fi \
ru \
bg \
de \
pl \
zh_CN \
hu \
sl \
it \
cs

EXTRA_DIST = \
enlightenment.pot \
fr.po \
ja.po \
es.po \
pt.po \
fi.po \
ru.po \
bg.po \
de.po \
pl.po \
zh_CN.po \
hu.po \
sl.po \
it.po \
cs.po

ja.mo:
	$(MSGFMT) -f -o $@ $(top_srcdir)/po/ja.po
fr.mo:
	$(MSGFMT) -f -o $@ $(top_srcdir)/po/fr.po
es.mo:
	$(MSGFMT) -f -o $@ $(top_srcdir)/po/es.po
pt.mo:
	$(MSGFMT) -f -o $@ $(top_srcdir)/po/pt.po
fi.mo:
	$(MSGFMT) -f -o $@ $(top_srcdir)/po/fi.po
ru.mo:
	$(MSGFMT) -f -o $@ $(top_srcdir)/po/ru.po
bg.mo:
	$(MSGFMT) -f -o $@ $(top_srcdir)/po/bg.po
de.mo:
	$(MSGFMT) -f -o $@ $(top_srcdir)/po/de.po
pl.mo:
	$(MSGFMT) -f -o $@ $(top_srcdir)/po/pl.po
zh_CN.mo:
	$(MSGFMT) -f -o $@ $(top_srcdir)/po/zh_CN.po
hu.mo:
	$(MSGFMT) -f -o $@ $(top_srcdir)/po/hu.po
sl.mo:
	$(MSGFMT) -f -o $@ $(top_srcdir)/po/sl.po
it.mo:
	$(MSGFMT) -f -o $@ $(top_srcdir)/po/it.po
cs.mo:
	$(MSGFMT) -f -o $@ $(top_srcdir)/po/cs.po

install-poDATA:
uninstall-poDATA:
install-data-local:
	for L in $(LANGS); do \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/locale/$$L/LC_MESSAGES; \
	  $(INSTALL_DATA) \
	  $$L.mo $(DESTDIR)$(datadir)/locale/$$L/LC_MESSAGES/$(PACKAGE).mo; \
	done
	
uninstall:
	for L in $(LANGS); do \
	  rm -f  $(DESTDIR)$(datadir)/locale/$$L/LC_MESSAGES/$(PACKAGE).mo; \
	done

clean-local:
	rm -f *.mo
