## Process this file with automake to produce Makefile.in

DISTCLEANFILES = e_ferite_gen_*

## Enable -DDEBUG for debugging messages, -DDEBUG_NEST
## for call tracing. Caution -- this will produce a *lot*
## of output!
DEBUGFLAGS = -W -Wall -g -DDEBUG #-DDEBUG_NEST

INCLUDES = \
	-I$(top_srcdir)/intl \
	@evas_cflags@ @edb_cflags@ @ebits_cflags@ @ecore_cflags@ @efsd_cflags@ @ferite_cflags@ $(DEBUGFLAGS)

EXTRA_DIST = e_ferite.fec

if USE_FERITE
ferite_c = e_ferite_gen_core.c \
	e_ferite_gen_header.h \
	e_ferite_gen_e.c \
	e_ferite_gen_Border.c \
	e_ferite_gen_Epplet.c \
	e_ferite_gen_Ebits.c \
	e_ferite_gen_EvasObject.c \
	e_ferite.h e_ferite.c

BUILT_SOURCES = e_ferite_gen_core.c \
	e_ferite_gen_header.h \
	e_ferite_gen_e.c \
	e_ferite_gen_Border.c \
	e_ferite_gen_Epplet.c \
	e_ferite_gen_Ebits.c \
	e_ferite_gen_EvasObject.c

# Yes, blank because we don't want it to supply
# it's own build rule.
e_ferite_gen_e_c_SOURCES =

e_ferite_gen_e.c: e_ferite.fec
	rm -f e_ferite_gen_*
	$(FEBUILDER) -m e_ferite_gen -i e_ferite_register -d -c e_ferite.fec

E_FERITE_GEN_E_C = e_ferite_gen_e.c

endif

bin_PROGRAMS = enlightenment $(E_FERITE_GEN_E_C)

enlightenment_SOURCES = \
	actions.h actions.c \
	background.h background.c \
	block.c block.h \
	border.h border.c \
	bordermenu.h bordermenu.c \
	config.h config.c \
	cursors.c cursors.h \
	debug.c debug.h \
	delayed.h delayed.c \
	desktops.h desktops.c \
	embed.c embed.h \
	entry.h entry.c \
	epplet.h epplet.c \
	exec.h exec.c \
	focus.h focus.c \
	file.h file.c \
	fs.h fs.c \
	$(ferite_c) \
	guides.h guides.c \
	globals.h globals.c \
	icccm.h icccm.c \
	iconbar.h iconbar.c\
	icons.h icons.c\
	ipc.h ipc.c \
	keys.h keys.c \
	main.c \
	match.c match.h \
	menu.h menu.c \
	menubuild.h menubuild.c \
	object.h object.c \
	observer.h observer.c\
	pack.c \
	place.c place.h \
	resist.h resist.c \
	scrollbar.h scrollbar.c \
	shelf.c \
	text.h text.c \
	util.h util.c \
	view.h view.c \
	e.h

enlightenment_LDADD = @evas_libs@ @edb_libs@ @ebits_libs@ @ecore_libs@ @efsd_libs@ @ferite_libs@ -lm -lc $(INTLLIBS)
