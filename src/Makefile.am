## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/intl \
	@evas_cflags@ @edb_cflags@ @ebits_cflags@ @ecore_cflags@ @efsd_cflags@

bin_PROGRAMS = enlightenment

enlightenment_SOURCES = \
actions.c \
background.c \
border.c \
config.c \
desktops.c \
entry.c \
exec.c \
fs.c \
icccm.c \
icons.c \
ipc.c \
keys.c \
main.c \
menu.c \
menubuild.c \
pack.c \
resist.c \
shelf.c \
util.c \
view.c \
e.h

enlightenment_LDADD = @evas_libs@ @edb_libs@ @ebits_libs@ @ecore_libs@ @efsd_libs@ -lm $(INTLLIBS)

