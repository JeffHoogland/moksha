## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/intl \
	 @evas_cflags@ @edb_cflags@ @ebits_cflags@ @ecore_cflags@ @efsd_cflags@ @ebg_cflags@ @ferite_cflags@

bin_PROGRAMS = e_img_import e_img_export e_setup

e_img_import_SOURCES = \
e_img_import.c

e_img_import_LDADD = @evas_libs@ @edb_libs@ @ebits_libs@ @ecore_libs@ @ferite_libs@ -lm -lc $(INTLLIBS)

e_img_export_SOURCES = \
e_img_export.c

e_img_export_LDADD = @evas_libs@ @edb_libs@ @ebits_libs@ @ecore_libs@ @ferite_libs@ -lm -lc $(INTLLIBS)

e_setup_SOURCES = \
e_setup.c

e_setup_LDADD = @evas_libs@ @edb_libs@ @ebits_libs@ @ecore_libs@ @ebg_libs@ @ferite_libs@ -lm -lc $(INTLLIBS)
