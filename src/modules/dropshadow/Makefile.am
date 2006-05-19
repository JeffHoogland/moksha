MAINTAINERCLEANFILES = Makefile.in
MODULE = dropshadow

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)
files_DATA = \
module.eap \
module_icon.png \
menu_icon_close.png \
menu_icon_dark.png \
menu_icon_far.png \
menu_icon_fuzzy.png \
menu_icon_light.png \
menu_icon_medium.png \
menu_icon_sharp.png \
menu_icon_underneath.png \
menu_icon_very_dark.png \
menu_icon_very_far.png \
menu_icon_very_fuzzy.png \
menu_icon_very_light.png \
menu_icon_very_sharp.png
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
module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h \
			 e_mod_config.c \
			 e_mod_config.h
module_la_LIBADD       = @e_libs@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h
