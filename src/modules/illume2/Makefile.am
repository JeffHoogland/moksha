MAINTAINERCLEANFILES = Makefile.in E_Illume.pc module.desktop
MODULE = illume2
SUBDIRS = keyboards policies

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)
files_DATA = e-module-$(MODULE).edj module.desktop

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

EILLUMEHEADERS = E_Illume.h

module_la_SOURCES      = $(EILLUMEHEADERS) \
			 e_mod_main.h \
                         e_mod_main.c \
                         e_mod_config.h \
                         e_mod_config.c \
                         e_mod_layout.h \
                         e_mod_layout.c \
                         e_mod_policy.h \
                         e_mod_policy.c \
                         e_mod_animation.h \
                         e_mod_animation.c \
                         e_mod_windows.h \
                         e_mod_windows.c \
                         e_mod_select_window.h \
                         e_mod_select_window.c \
                         e_kbd.h \
                         e_kbd.c \
                         e_kbd_dbus.h \
                         e_kbd_dbus.c \
                         e_quickpanel.h \
                         e_quickpanel.c \
                         e_illume_border.c

module_la_LIBADD       = @e_libs@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

installed_headersdir = $(prefix)/include/@PACKAGE@
installed_headers_DATA = $(EILLUMEHEADERS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = E_Illume.pc

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)/policies
        
