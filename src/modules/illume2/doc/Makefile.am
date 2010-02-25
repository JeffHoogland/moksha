MAINTAINERCLEANFILES = Makefile.in illume.dox
MODULE_TARNAME = illume2
MODULE_VERSION = 2.0

.PHONY: doc

PACKAGE_DOCNAME = $(MODULE_TARNAME)-$(MODULE_VERSION)-doc

if EFL_BUILD_DOC

doc-clean:
	rm -rf html/ latex/ man/ xml/ $(PACKAGE_DOCNAME).tar*

doc: all doc-clean
	$(efl_doxygen)
	cp img/* html/
	rm -rf $(PACKAGE_DOCNAME).tar*
	mkdir -p $(PACKAGE_DOCNAME)/doc
	cp -R html/ latex/ man/ $(PACKAGE_DOCNAME)/doc
	tar cf $(PACKAGE_DOCNAME).tar $(PACKAGE_DOCNAME)/
	bzip2 -9 $(PACKAGE_DOCNAME).tar
	rm -rf $(PACKAGE_DOCNAME)/
	mv $(PACKAGE_DOCNAME).tar.bz2 $(top_srcdir)

clean-local: doc-clean

else

doc:
	@echo "Documentation not built. Run ./configure --help"

endif

EXTRA_DIST = Doxyfile $(wildcard img/*.*) e.css head.html foot.html illume.dox.in
