MAINTAINERCLEANFILES = Makefile.in
MODULE = everything

# the module .so file
INCLUDES               = -I.. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)/src/modules/$(MODULE) \
                         -I$(top_srcdir)/src/bin \
                         -I$(top_srcdir)/src/lib \
                         -I$(top_srcdir)/src/modules \
                         @e_cflags@

plugindir              = $(libdir)/enlightenment/modules/$(MODULE)/plugins/core/$(MODULE_ARCH)

evry_plug_appsdir = $(plugindir)
evry_plug_apps_LTLIBRARIES = evry_plug_apps.la
evry_plug_apps_la_SOURCES =  evry_plug_apps.c
evry_plug_apps_la_LIBADD       = @e_libs@  ../libfoo.la
evry_plug_apps_la_LDFLAGS      = -module  -avoid-version
evry_plug_apps_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_borderdir = $(plugindir)
evry_plug_border_LTLIBRARIES = evry_plug_border.la
evry_plug_border_la_SOURCES =  evry_plug_border.c
evry_plug_border_la_LIBADD       = @e_libs@  ../libfoo.la
evry_plug_border_la_LDFLAGS      = -module -avoid-version
evry_plug_border_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_dir_browsedir = $(plugindir)
evry_plug_dir_browse_LTLIBRARIES = evry_plug_dir_browse.la
evry_plug_dir_browse_la_SOURCES =  evry_plug_dir_browse.c
evry_plug_dir_browse_la_LIBADD       = @e_libs@  ../libfoo.la
evry_plug_dir_browse_la_LDFLAGS      = -module -avoid-version
evry_plug_dir_browse_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_calcdir = $(plugindir)
evry_plug_calc_LTLIBRARIES = evry_plug_calc.la
evry_plug_calc_la_SOURCES =  evry_plug_calc.c
evry_plug_calc_la_LIBADD       = @e_libs@  ../libfoo.la
evry_plug_calc_la_LDFLAGS      = -module -avoid-version
evry_plug_calc_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_configdir = $(plugindir)
evry_plug_config_LTLIBRARIES = evry_plug_config.la
evry_plug_config_la_SOURCES =  evry_plug_config.c
evry_plug_config_la_LIBADD       = @e_libs@  ../libfoo.la
evry_plug_config_la_LDFLAGS      = -module -avoid-version
evry_plug_config_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_aspelldir = $(plugindir)
evry_plug_aspell_LTLIBRARIES = evry_plug_aspell.la
evry_plug_aspell_la_SOURCES =  evry_plug_aspell.c
evry_plug_aspell_la_LIBADD       = @e_libs@  ../libfoo.la
evry_plug_aspell_la_LDFLAGS      = -module -avoid-version
evry_plug_aspell_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_textdir = $(plugindir)
evry_plug_text_LTLIBRARIES = evry_plug_text.la
evry_plug_text_la_SOURCES =  evry_plug_text.c
evry_plug_text_la_LIBADD       = @e_libs@  ../libfoo.la
evry_plug_text_la_LDFLAGS      = -module -avoid-version
evry_plug_text_la_LIBTOOLFLAGS = --tag=disable-static


