MAINTAINERCLEANFILES = Makefile.in

INCLUDES               = -I. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)src/preload \
                         @x_cflags@
pkgdir                 = $(libdir)/enlightenment/preload
pkg_LTLIBRARIES        = e_hack.la
e_hack_la_SOURCES      = \
e_hack.c \
e_hack.h
e_hack_la_LIBADD       = @x_libs@ @dlopen_libs@
e_hack_la_LDFLAGS      = -module -avoid-version
e_hack_la_DEPENDENCIES = $(top_builddir)/config.h
