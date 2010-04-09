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

evry_plug_previewdir = $(plugindir)
evry_plug_preview_LTLIBRARIES = evry_plug_preview.la
evry_plug_preview_la_SOURCES =  evry_plug_preview.c
evry_plug_preview_la_LIBADD       = @e_libs@ ../libfoo.la
evry_plug_preview_la_LDFLAGS      = -module -avoid-version
evry_plug_preview_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_view_helpdir = $(plugindir)
evry_plug_view_help_LTLIBRARIES = evry_plug_view_help.la
evry_plug_view_help_la_SOURCES =  evry_plug_view_help.c
evry_plug_view_help_la_LIBADD       = @e_libs@ ../libfoo.la
evry_plug_view_help_la_LDFLAGS      = -module -avoid-version
evry_plug_view_help_la_LIBTOOLFLAGS = --tag=disable-static

evry_plug_view_thumbdir = $(plugindir)
evry_plug_view_thumb_LTLIBRARIES = evry_plug_view_thumb.la
evry_plug_view_thumb_la_SOURCES =  evry_plug_view_thumb.c
evry_plug_view_thumb_la_LIBADD       = @e_libs@ ../libfoo.la
evry_plug_view_thumb_la_LDFLAGS      = -module -avoid-version
evry_plug_view_thumb_la_LIBTOOLFLAGS = --tag=disable-static


