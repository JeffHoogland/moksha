MAINTAINERCLEANFILES = Makefile.in

INCLUDES               = -I. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)src/preload \
                         @x_cflags@ \
                         @e_cflags@ \
                         @EINA_CFLAGS@

pkgdir                 = $(libdir)/enlightenment/preload

#pkg_LTLIBRARIES        = e_hack.la e_precache.la
pkg_LTLIBRARIES        = e_precache.la

#e_hack_la_SOURCES      = \
#e_hack.c \
#e_hack.h
#e_hack_la_LIBADD       = @x_libs@ @dlopen_libs@ @EINA_LIBS@
#e_hack_la_LDFLAGS      = -module -avoid-version
#e_hack_la_DEPENDENCIES = $(top_builddir)/config.h

e_precache_la_SOURCES      = \
e_precache.c \
e_precache.h
e_precache_la_LIBADD       = @e_libs@ @dlopen_libs@ @EINA_LIBS@
e_precache_la_LDFLAGS      = -module -avoid-version
e_precache_la_DEPENDENCIES = $(top_builddir)/config.h
