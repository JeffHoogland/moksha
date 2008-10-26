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
                         page_010.la \
                         page_020.la \
                         page_030.la \
                         page_070.la

### disabled because profile selector really does the job. code here just for
### informational purposes on how to make wizard pages
# page_040.la
# page_050.la
# page_060.la

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

page_010_la_SOURCES      = page_010.c
page_010_la_LIBADD       = @e_libs@ @dlopen_libs@
page_010_la_LDFLAGS      = -module -avoid-version
page_010_la_DEPENDENCIES = $(top_builddir)/config.h

page_020_la_SOURCES      = page_020.c
page_020_la_LIBADD       = @e_libs@ @dlopen_libs@
page_020_la_LDFLAGS      = -module -avoid-version
page_020_la_DEPENDENCIES = $(top_builddir)/config.h

page_030_la_SOURCES      = page_030.c
page_030_la_LIBADD       = @e_libs@ @dlopen_libs@
page_030_la_LDFLAGS      = -module -avoid-version
page_030_la_DEPENDENCIES = $(top_builddir)/config.h

#page_040_la_SOURCES      = page_040.c
#page_040_la_LIBADD       = @e_libs@ @dlopen_libs@
#page_040_la_LDFLAGS      = -module -avoid-version
#page_040_la_DEPENDENCIES = $(top_builddir)/config.h

#page_050_la_SOURCES      = page_050.c
#page_050_la_LIBADD       = @e_libs@ @dlopen_libs@
#page_050_la_LDFLAGS      = -module -avoid-version
#page_050_la_DEPENDENCIES = $(top_builddir)/config.h

#page_060_la_SOURCES      = page_060.c
#page_060_la_LIBADD       = @e_libs@ @dlopen_libs@
#page_060_la_LDFLAGS      = -module -avoid-version
#page_060_la_DEPENDENCIES = $(top_builddir)/config.h

page_070_la_SOURCES      = page_070.c
page_070_la_LIBADD       = @e_libs@ @dlopen_libs@
page_070_la_LDFLAGS      = -module -avoid-version
page_070_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
