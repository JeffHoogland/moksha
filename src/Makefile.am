## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/intl \
	@evas_cflags@ @edb_cflags@ @ebits_cflags@ @ecore_cflags@

bin_PROGRAMS = e

e_SOURCES = \
actions.c \
actions.h \
border.c \
border.h \
desktops.c \
icccm.c \
icccm.h \
main.c \
resist.c \
util.c \
e.h

e_LDADD = @evas_libs@ @edb_libs@ @ebits_libs@ @ecore_libs@ -lecore -lm $(INTLLIBS)

