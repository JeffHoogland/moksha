## Process this file with automake to produce Makefile.in

## Enable -DDEBUG for debugging messages, -DDEBUG_NEST
## for call tracing. Caution -- this will produce a *lot*
## of output!
DEBUGFLAGS = -W -Wall -g -DDEBUG #-DDEBUG_NEST

INCLUDES = \
	-I$(top_srcdir)/intl \
	@evas_cflags@ @edb_cflags@ @ebits_cflags@ @ecore_cflags@ @efsd_cflags@ @ebg_cflags@ $(DEBUGFLAGS)

bin_PROGRAMS = enlightenment 

enlightenment_SOURCES = \
	actions.h actions.c \
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
	exec.h exec.c \
	focus.h focus.c \
	file.h file.c \
	fs.h fs.c \
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
	e_file.h e_file.c \
	e_dir.h e_dir.c \
	e_view_machine.h e_view_machine.c \
	view_layout.h view_layout.c \
	e_view_look.h e_view_look.c \
	e.h

enlightenment_LDADD = @evas_libs@ @edb_libs@ @ebits_libs@ @ecore_libs@ @efsd_libs@ @ebg_libs@ -lm -lc $(INTLLIBS)
