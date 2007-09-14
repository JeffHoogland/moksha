MAINTAINERCLEANFILES = Makefile.in
MODULE = wizard

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)
files_DATA = \
e-module-$(MODULE).edj module.desktop

EXTRA_DIST = $(files_DATA)

# the module .so file
INCLUDES               = -I. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)/src/modules/$(MODULE) \
                         -I$(top_srcdir)/src/bin \
                         -I$(top_srcdir)/src/lib \
                         -I$(top_srcdir)/src/modules \
                         @e_cflags@
pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la \
                         page_000.la \
                         page_100.la \
                         page_200.la
			 
module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h \
			 e_wizard.c \
                         e_wizard.h

module_la_LIBADD       = @e_libs@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

page_000_la_SOURCES      = page_000.c
page_000_la_LIBADD       = @e_libs@ @dlopen_libs@
page_000_la_LDFLAGS      = -module -avoid-version
page_000_la_DEPENDENCIES = $(top_builddir)/config.h

page_100_la_SOURCES      = page_100.c
page_100_la_LIBADD       = @e_libs@ @dlopen_libs@
page_100_la_LDFLAGS      = -module -avoid-version
page_100_la_DEPENDENCIES = $(top_builddir)/config.h

page_200_la_SOURCES      = page_200.c
page_200_la_LIBADD       = @e_libs@ @dlopen_libs@
page_200_la_LDFLAGS      = -module -avoid-version
page_200_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
