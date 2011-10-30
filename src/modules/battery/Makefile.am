MAINTAINERCLEANFILES = Makefile.in
MODULE = battery

if HAVE_EEZE
DEVICE_FILE = e_mod_udev.c
else
if HAVE_OPENBSD
DEVICE_FILE = e_mod_openbsd.c
else
DEVICE_FILE = e_mod_dbus.c
endif
endif

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
                         -I$(top_builddir)/src/bin \
                         -I$(top_srcdir)/src/modules \
                         @e_cflags@ \
                         @cf_cflags@
pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h \
			 e_mod_config.c \
			 $(DEVICE_FILE)

module_la_LIBADD       = @e_libs@ @cf_libs@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

batgetdir = $(pkgdir)

batget_PROGRAMS        = batget
batget_SOURCES         = batget.c
batget_LDADD           = @BATTERY_LIBS@
batget_LDFLAGS         = @BATTERY_LDFLAGS@


uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
