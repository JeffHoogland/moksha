## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/intl \
	@evas_cflags@ @edb_cflags@ @ebits_cflags@ @ecore_cflags@

bin_PROGRAMS = enlightenment

enlightenment_SOURCES = \
actions.c \
border.c \
config.c \
desktops.c \
exec.c \
icccm.c \
main.c \
menu.c \
resist.c \
util.c \
view.c \
e.h

enlightenment_LDADD = @evas_libs@ @edb_libs@ @ebits_libs@ @ecore_libs@ -lm $(INTLLIBS)

