MAINTAINERCLEANFILES = Makefile.in everything.pc
MODULE = everything

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
                         evry_config.c


module_la_LIBADD       = @e_libs@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

installed_headersdir = $(prefix)/include/@PACKAGE@
installed_headers_DATA = $(EVRYHEADERS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = everything.pc

plugindir              = $(libdir)/enlightenment/everything_plugins/core/$(MODULE_ARCH)

evry_plug_appsdir = $(plugindir)
evry_plug_apps_LTLIBRARIES = evry_plug_apps.la
evry_plug_apps_la_SOURCES =  evry_plug_apps.c
evry_plug_apps_la_LIBADD       = @e_libs@
evry_plug_apps_la_LDFLAGS      = -no-undefined -module -avoid-version
evry_plug_apps_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_borderdir = $(plugindir)
evry_plug_border_LTLIBRARIES = evry_plug_border.la
evry_plug_border_la_SOURCES =  evry_plug_border.c
evry_plug_border_la_LIBADD       = @e_libs@
evry_plug_border_la_LDFLAGS      = -no-undefined -module -avoid-version
evry_plug_border_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_border_actdir = $(plugindir)
evry_plug_border_act_LTLIBRARIES = evry_plug_border_act.la
evry_plug_border_act_la_SOURCES =  evry_plug_border_act.c
evry_plug_border_act_la_LIBADD       = @e_libs@
evry_plug_border_act_la_LDFLAGS      = -no-undefined -module -avoid-version
evry_plug_border_act_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_dir_browsedir = $(plugindir)
evry_plug_dir_browse_LTLIBRARIES = evry_plug_dir_browse.la
evry_plug_dir_browse_la_SOURCES =  evry_plug_dir_browse.c
evry_plug_dir_browse_la_LIBADD       = @e_libs@
evry_plug_dir_browse_la_LDFLAGS      = -no-undefined -module -avoid-version
evry_plug_dir_browse_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_trackerdir = $(plugindir)
evry_plug_tracker_LTLIBRARIES = evry_plug_tracker.la
evry_plug_tracker_la_SOURCES =  evry_plug_tracker.c
evry_plug_tracker_la_LIBADD       = @e_libs@
evry_plug_tracker_la_LDFLAGS      = -no-undefined -module -avoid-version
evry_plug_tracker_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_calcdir = $(plugindir)
evry_plug_calc_LTLIBRARIES = evry_plug_calc.la
evry_plug_calc_la_SOURCES =  evry_plug_calc.c
evry_plug_calc_la_LIBADD       = @e_libs@
evry_plug_calc_la_LDFLAGS      = -no-undefined -module -avoid-version
evry_plug_calc_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_configdir = $(plugindir)
evry_plug_config_LTLIBRARIES = evry_plug_config.la
evry_plug_config_la_SOURCES =  evry_plug_config.c
evry_plug_config_la_LIBADD       = @e_libs@
evry_plug_config_la_LDFLAGS      = -no-undefined -module -avoid-version
evry_plug_config_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_aspelldir = $(plugindir)
evry_plug_aspell_LTLIBRARIES = evry_plug_aspell.la
evry_plug_aspell_la_SOURCES =  evry_plug_aspell.c
evry_plug_aspell_la_LIBADD       = @e_libs@
evry_plug_aspell_la_LDFLAGS      = -no-undefined -module -avoid-version
evry_plug_aspell_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_clipboarddir = $(plugindir)
evry_plug_clipboard_LTLIBRARIES = evry_plug_clipboard.la
evry_plug_clipboard_la_SOURCES =  evry_plug_clipboard.c
evry_plug_clipboard_la_LIBADD       = @e_libs@
evry_plug_clipboard_la_LDFLAGS      = -no-undefined -module -avoid-version
evry_plug_clipboard_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_previewdir = $(plugindir)
evry_plug_preview_LTLIBRARIES = evry_plug_preview.la
evry_plug_preview_la_SOURCES =  evry_plug_preview.c
evry_plug_preview_la_LIBADD       = @e_libs@
evry_plug_preview_la_LDFLAGS      = -no-undefined -module -avoid-version
evry_plug_preview_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_view_listdir = $(plugindir)
evry_plug_view_list_LTLIBRARIES = evry_plug_view_list.la
evry_plug_view_list_la_SOURCES =  evry_plug_view_list.c
evry_plug_view_list_la_LIBADD       = @e_libs@
evry_plug_view_list_la_LDFLAGS      = -no-undefined -module -avoid-version
evry_plug_view_list_la_LIBTOOLFLAGS = --tag=disable-static

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)

