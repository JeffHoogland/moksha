MAINTAINERCLEANFILES = Makefile.in everything.pc
MODULE = everything

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)
files_DATA = \
e-module-$(MODULE).edj module.desktop

EXTRA_DIST = $(files_DATA) $(EVRYHEADERS) everything.pc.in

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
# noinst_LTLIBRARIES      = libfoo.la

EVRYHEADERS = Evry.h

module_la_SOURCES      = $(EVRYHEADERS) \
			 e_mod_main.c \
                         e_mod_main.h \
			 evry.c \
                         evry_config.c \
	                 evry_util.c \
	                 evry_history.c \
			 evry_plug_aggregator.c \
			 evry_plug_actions.c \
			 evry_view_plugin_tabs.c \
			 evry_plug_view_thumb.c \
			 evry_plug_view_help.c \
			 evry_plug_clipboard.c

module_la_LIBADD       = @e_libs@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

installed_headersdir = $(prefix)/include/@PACKAGE@
installed_headers_DATA = $(EVRYHEADERS)

# libfoo_la_SOURCES = evry.c e_mod_main.h evry_util.c evry_config.c evry_history.c evry_view_plugin_tabs.c
# libfoo_la_LIBADD       = @e_libs@ @dlopen_libs@
# libfoo_la_LDFLAGS      = -avoid-version
# libfoo_la_DEPENDENCIES = $(top_builddir)/config.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = everything.pc

uninstall-local:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
