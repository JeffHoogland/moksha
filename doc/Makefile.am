## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES = Makefile.in 

EXTRA_DIST = \
manual.raw		\
kernel-doc.in 		\
stylesheet.css		\
figures/*.gif           # Add any images you create here


## Fill in all source files that you documented
## with extractable comments here:
##
SOURCEDOC = $(top_srcdir)/src/e.h

## For details on what can be specified in the
## comments, see the beginning of kernel-doc
## in this directory!               -- cK.


SGMLFILE  = $(PACKAGE)-manual.sgml

docs: manual.raw $(SOURCEDOC)
	./kernel-doc -docbook <manual.raw >$(SGMLFILE)

if HAVE_JADE

FULLNAME = $(PACKAGE)-manual-$(VERSION)

html-docs: docs html-customizations.dsl
	$(mkinstalldirs) ./$(FULLNAME)/figures
	cd $(FULLNAME) && @JADE@ -t sgml -d ../html-customizations.dsl ../$(SGMLFILE)
	- cd figures && cp -f *.gif ../$(FULLNAME)/figures
	- cd figures && cp -f *.png ../$(FULLNAME)/figures
	- cd figures && cp -f *.jpg ../$(FULLNAME)/figures
	cp -f stylesheet.css $(FULLNAME)/stylesheet.css
	tar cfvz $(FULLNAME).tar.gz $(FULLNAME)

html-filetypes: filetypes.xsl
	$(mkinstalldirs) ./$(FULLNAME)/filetypes
	cp -fr filetypes-templates/* ./$(FULLNAME)/filetypes
	./generate-trees.sh ./$(FULLNAME)/filetypes

endif

