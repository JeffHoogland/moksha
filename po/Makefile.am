MAINTAINERCLEANFILES = Makefile.in

MSGFMT = msgfmt

podir =
po_DATA = $(addsuffix .mo,$(ALL_LINGUAS))

EXTRA_DIST = enlightenment.pot $(addsuffix .po,$(ALL_LINGUAS))

%.mo : %.po
	$(MSGFMT) -f -o $@ $<

install-poDATA:
uninstall-poDATA:
install-data-local:
	for L in $(ALL_LINGUAS); do \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/locale/$$L/LC_MESSAGES; \
	  $(INSTALL_DATA) \
	  $$L.mo $(DESTDIR)$(datadir)/locale/$$L/LC_MESSAGES/$(PACKAGE).mo; \
	done

uninstall:
	for L in $(ALL_LINGUAS); do \
	  rm -f  $(DESTDIR)$(datadir)/locale/$$L/LC_MESSAGES/$(PACKAGE).mo; \
	done

clean-local:
	rm -f *.mo
