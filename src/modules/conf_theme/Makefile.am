MAINTAINERCLEANFILES = Makefile.in
MODULE = conf_theme

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)
files_DATA = \
module.desktop

EXTRA_DIST = $(files_DATA)

# the module .so file
INCLUDES               = -I. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)/src/modules/$(MODULE) \
                         -I$(top_srcdir)/src/bin \
                         -I$(top_builddir)/src/bin \
                         -I$(top_srcdir)/src/modules \
                         @e_cflags@ \
			 @EXCHANGE_CFLAGS@

pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h \
                         e_int_config_borders.c \
                         e_int_config_borders.h \
                         e_int_config_color_classes.c \
                         e_int_config_color_classes.h \
                         e_int_config_cursor.c \
                         e_int_config_cursor.h \
                         e_int_config_fonts.c \
                         e_int_config_fonts.h \
                         e_int_config_icon_themes.c \
                         e_int_config_icon_themes.h \
                         e_int_config_scale.c \
                         e_int_config_scale.h \
                         e_int_config_startup.c \
                         e_int_config_startup.h \
                         e_int_config_theme.c \
                         e_int_config_theme.h \
                         e_int_config_theme_import.c \
                         e_int_config_theme_import.h \
                         e_int_config_theme_web.c \
                         e_int_config_theme_web.h \
                         e_int_config_transitions.c \
                         e_int_config_transitions.h \
                         e_int_config_wallpaper.c \
                         e_int_config_wallpaper.h \
                         e_int_config_wallpaper_import.c \
                         e_int_config_wallpaper_import.h \
                         e_int_config_wallpaper_web.c \
                         e_int_config_wallpaper_web.h \
                         e_int_config_xsettings.c

module_la_LIBADD       = @e_libs@ @dlopen_libs@ @EXCHANGE_LIBS@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
