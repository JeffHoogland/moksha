## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS     = 1.4 foreign

# A list of all the files in the current directory which can be regenerated
MAINTAINERCLEANFILES = Makefile.in

LDFLAGS		     = -L/usr/X11R6/lib -L/usr/local/lib
INCLUDES	     = -I/usr/X11R6/include -I/usr/local/include \
			$(X_CFLAGS) -I$(includedir)

lib_LTLIBRARIES      = libehack.la
include_HEADERS      =
libehack_la_SOURCES  = \
e_hack.c \
e_hack.h

libehack_la_LIBADD       = -lX11 -ldl $(LDFLAGS)
libehack_la_DEPENDENCIES = $(top_builddir)/config.h
libehack_la_LDFLAGS      = -version-info 0:1:0
