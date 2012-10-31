MAINTAINERCLEANFILES = Makefile.in
MODULE = comp

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)
files_DATA = \
e-module-$(MODULE).edj module.desktop

EXTRA_DIST = $(files_DATA)

# the module .so file
AM_CPPFLAGS               = -I. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)/src/modules/$(MODULE) \
                         -I$(top_srcdir)/src/bin \
                         -I$(top_builddir)/src/bin \
                         -I$(top_srcdir)/src/modules \
                         @e_cflags@ @WAYLAND_CFLAGS@

pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la

module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h \
			 e_mod_config.c \
			 e_mod_config.h \
                         e_mod_comp.c \
                         e_mod_comp.h \
                         e_mod_comp_update.c \
                         e_mod_comp_update.h \
                         e_mod_comp_cfdata.c \
                         e_mod_comp_cfdata.h
if HAVE_WAYLAND_CLIENTS
module_la_SOURCES += e_mod_comp_wl.h \
                     e_mod_comp_wl.c \
                     e_mod_comp_wl_comp.h \
                     e_mod_comp_wl_comp.c \
                     e_mod_comp_wl_output.h \
                     e_mod_comp_wl_output.c \
                     e_mod_comp_wl_input.h \
                     e_mod_comp_wl_input.c \
                     e_mod_comp_wl_shell.h \
                     e_mod_comp_wl_shell.c \
                     e_mod_comp_wl_surface.h \
                     e_mod_comp_wl_surface.c \
                     e_mod_comp_wl_buffer.h \
                     e_mod_comp_wl_buffer.c \
                     e_mod_comp_wl_region.h \
                     e_mod_comp_wl_region.c
endif

module_la_LIBADD       = @e_libs@ @dlopen_libs@ @WAYLAND_LIBS@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
