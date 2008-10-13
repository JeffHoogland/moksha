MAINTAINERCLEANFILES = Makefile.in
MODULE = msgbus_lang

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)

# the module .so file
INCLUDES               = -I. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)/src/modules/$(MODULE) \
                         -I$(top_srcdir)/src/bin \
                         -I$(top_srcdir)/src/lib \
                         -I$(top_srcdir)/src/modules \
                         @e_cflags@ \
                         @EINA_CFLAGS@

pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)

if HAVE_EDBUS
files_DATA = \
e-module-$(MODULE).edj \
module.desktop

EXTRA_DIST = $(files_DATA)

pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h

module_la_LIBADD       = @e_libs@ @dlopen_libs@ @E_DBUS_LIBS@ @EINA_LIBS@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h
endif

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
