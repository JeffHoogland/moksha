## Process this file with automake to produce Makefile.in

CLEANFILES = e_ferite_gen_*

INCLUDES = \
	-I$(top_srcdir)/intl \
	@evas_cflags@ @edb_cflags@ @ebits_cflags@ @ecore_cflags@ @efsd_cflags@ @ferite_cflags@

bin_PROGRAMS = enlightenment

if USE_FERITE
ferite_c = e_ferite_gen_core.c \
	e_ferite_gen_header.h \
	e_ferite_gen_e.c \
	e_ferite_gen_Window.c \
	e_ferite.h e_ferite.c
endif

enlightenment_SOURCES = \
	actions.h actions.c \
	background.h background.c \
	border.h border.c \
	config.h config.c \
	desktops.h desktops.c \
	embed.c embed.h \
	entry.h entry.c \
	exec.h exec.c \
	fs.h fs.c \
	$(ferite_c) \
	guides.h guides.c \
	icccm.h icccm.c \
	icons.c \
	ipc.h ipc.c \
	keys.h keys.c \
	main.c \
	menu.h menu.c \
	menubuild.h menubuild.c \
	pack.c \
	resist.h resist.c \
	scrollbar.h scrollbar.c \
	shelf.c \
	text.h text.c \
	util.h util.c \
	view.h view.c \
	e.h

enlightenment_LDADD = @evas_libs@ @edb_libs@ @ebits_libs@ @ecore_libs@ @efsd_libs@ @ferite_libs@ -lm -lc $(INTLLIBS)

e_ferite_gen_core.c: e_ferite.fec
	rm -f e_ferite_gen_* ; \
	@FEBUILDER@ -m e_ferite_gen e_ferite.fec ; \
	mv e_ferite_gen_core.c e_ferite_gen_core.c.old ; \
	cat e_ferite_gen_core.c.old |  sed -e "s/module_init/e_ferite_register/g" > e_ferite_gen_core.c ; \
	rm e_ferite_gen_core.c.old
