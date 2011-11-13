MAINTAINERCLEANFILES = Makefile.in everything.pc module.desktop
MODULE = everything

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)
files_DATA = \
e-module-$(MODULE).edj \
e-module-$(MODULE)-start.edj \
module.desktop

EXTRA_DIST = $(files_DATA) everything.pc.in module.desktop.in

# the module .so file
INCLUDES               = -I. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)/src/modules/$(MODULE) \
                         -I$(top_srcdir)/src/bin \
                         -I$(top_builddir)/src/bin \
                         -I$(top_srcdir)/src/modules \
                         @e_cflags@

pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la

EVRYHEADERS            = evry_api.h evry_types.h

module_la_SOURCES      = $(EVRYHEADERS) \
			 e_mod_main.c \
                         e_mod_main.h \
			 evry.c \
                         evry_config.c \
	                 evry_util.c \
	                 evry_history.c \
	                 evry_plugin.c \
			 evry_plug_aggregator.c \
			 evry_plug_actions.c \
			 evry_view.c \
			 evry_view_tabs.c \
			 evry_view_help.c \
			 evry_plug_clipboard.c \
			 evry_plug_text.c \
			 evry_plug_collection.c \
			 evry_gadget.c \
			 md5.c \
                         md5.h \
			 evry_plug_apps.c \
			 evry_plug_files.c \
			 evry_plug_windows.c \
			 evry_plug_settings.c \
			 evry_plug_calc.c

module_la_LIBADD       = @e_libs@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

installed_headersdir   = $(includedir)/enlightenment
dist_installed_headers_DATA = $(EVRYHEADERS)

pkgconfigdir = $(libdir)/pkgconfig
dist_pkgconfig_DATA = everything.pc

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
	rm -f $(DESTDIR)$(includedir)/enlightenment/evry_api.h
	rm -f $(DESTDIR)$(includedir)/enlightenment/evry_types.h
	rm -f $(DESTDIR)$(libdir)/pkgconfig/everything.pc
        
