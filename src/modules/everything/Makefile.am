MAINTAINERCLEANFILES = Makefile.in everything.pc
MODULE = everything

SUBDIRS = sources actions views

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
pkg_LTLIBRARIES        = module.la


EVRYHEADERS = Evry.h

module_la_SOURCES      = $(EVRYHEADERS) \
			 e_mod_main.c \
                         e_mod_main.h \
			 evry.c \
                         evry_config.c \
	                 evry_util.c \
			 evry_plug_aggregator.c \
			 evry_plug_actions.c \
			 evry_view_plugin_tabs.c


module_la_LIBADD       = @e_libs@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

installed_headersdir = $(prefix)/include/@PACKAGE@
installed_headers_DATA = $(EVRYHEADERS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = everything.pc

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
	rm -rf $(DESTDIR)$(libdir)/enlightenment/everything_plugins/core/
