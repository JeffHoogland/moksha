## Process this file with automake to produce Makefile.in

DISTCLEANFILES = e_ferite_gen_*

## Enable -DDEBUG for debugging messages, -DDEBUG_NEST
## for call tracing. Caution -- this will produce a *lot*
## of output!
DEBUGFLAGS = -g -DDEBUG #-DDEBUG_NEST

INCLUDES = \
	-I$(top_srcdir)/intl \
	@evas_cflags@ @edb_cflags@ @ebits_cflags@ @ecore_cflags@ @efsd_cflags@ @ferite_cflags@ $(DEBUGFLAGS)

bin_PROGRAMS = enlightenment

EXTRA_DIST = e_ferite.fec

if USE_FERITE
ferite_c = e_ferite_gen_core.c \
	e_ferite_gen_header.h \
	e_ferite_gen_e.c \
	e_ferite_gen_window.c \
	e_ferite.h e_ferite.c
endif

enlightenment_SOURCES = \
	actions.h actions.c \
	background.h background.c \
	border.h border.c \
	config.h config.c \
	cursors.c cursors.h \
	debug.c debug.h \
	delayed.h delayed.c \
	desktops.h desktops.c \
	embed.c embed.h \
	entry.h entry.c \
	exec.h exec.c \
	fs.h fs.c \
	$(ferite_c) \
	guides.h guides.c \
	icccm.h icccm.c \
	iconbar.h iconbar.c\
	icons.c \
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
