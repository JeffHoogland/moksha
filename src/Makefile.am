## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/intl \
	@evas_cflags@ @edb_cflags@ @ebits_cflags@ @ecore_cflags@ @efsd_cflags@

bin_PROGRAMS = enlightenment

enlightenment_SOURCES = \
	actions.h actions.c \
	background.h background.c \
	border.h border.c \
	config.h config.c \
	desktops.h desktops.c \
	entry.h entry.c \
	exec.h exec.c \
	fs.h fs.c \
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

enlightenment_LDADD = @evas_libs@ @edb_libs@ @ebits_libs@ @ecore_libs@ @efsd_libs@ -lm -lc $(INTLLIBS)

