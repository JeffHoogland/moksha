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

evry_plug_border_actdir = $(plugindir)
evry_plug_border_act_LTLIBRARIES = evry_plug_border_act.la
evry_plug_border_act_la_SOURCES =  evry_plug_border_act.c
evry_plug_border_act_la_LIBADD       = @e_libs@ ../libfoo.la
evry_plug_border_act_la_LDFLAGS      = -module -avoid-version
evry_plug_border_act_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_clipboarddir = $(plugindir)
evry_plug_clipboard_LTLIBRARIES = evry_plug_clipboard.la
evry_plug_clipboard_la_SOURCES =  evry_plug_clipboard.c
evry_plug_clipboard_la_LIBADD       = @e_libs@ ../libfoo.la
evry_plug_clipboard_la_LDFLAGS      = -module -avoid-version
evry_plug_clipboard_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_wallpaperdir = $(plugindir)
evry_plug_wallpaper_LTLIBRARIES = evry_plug_wallpaper.la
evry_plug_wallpaper_la_SOURCES =  evry_plug_wallpaper.c
evry_plug_wallpaper_la_LIBADD       = @e_libs@ ../libfoo.la
evry_plug_wallpaper_la_LDFLAGS      = -module -avoid-version
evry_plug_wallpaper_la_LIBTOOLFLAGS = --tag=disable-static
