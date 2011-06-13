MAINTAINERCLEANFILES = Makefile.in e.dox

.PHONY: doc

PACKAGE_DOCNAME = $(PACKAGE_TARNAME)-$(PACKAGE_VERSION)-doc

if EFL_BUILD_DOC

doc-clean:
	rm -rf html/ latex/ man/ xml/ $(top_builddir)/$(PACKAGE_DOCNAME).tar*

doc: all doc-clean
	$(efl_doxygen)
	cp $(srcdir)/img/* html/
	rm -rf $(PACKAGE_DOCNAME).tar*
	mkdir -p $(PACKAGE_DOCNAME)/doc
	cp -R html/ latex/ man/ $(PACKAGE_DOCNAME)/doc
	tar cf $(PACKAGE_DOCNAME).tar $(PACKAGE_DOCNAME)/
	bzip2 -9 $(PACKAGE_DOCNAME).tar
	rm -rf $(PACKAGE_DOCNAME)/
	mv $(PACKAGE_DOCNAME).tar.bz2 $(top_builddir)

clean-local: doc-clean

else

doc:
	@echo "Documentation not built. Run ./configure --help"

endif

filesdir = $(datadir)/enlightenment/doc
files_DATA = \
documentation.html \
illume2.html \
FDO.txt \
cache.txt \
enlightenment.png \
illume2.png

EXTRA_DIST = $(files_DATA) Doxyfile.in $(wildcard img/*.*) e.css head.html foot.html e.dox.in
