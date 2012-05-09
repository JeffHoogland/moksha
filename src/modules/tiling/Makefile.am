MAINTAINERCLEANFILES = Makefile.in module.desktop
MODULE = tiling

filesdir = $(libdir)/enlightenment/modules/$(MODULE)
files_DATA = e-module-$(MODULE).edj module.desktop

EXTRA_DIST = $(files_DATA) \
	     e-module-$(MODULE).edc \
	     module.desktop.in \
	     images/module_icon.png

EDJE_CC = @edje_cc@
EDJE_FLAGS = -id $(top_srcdir)/src/modules/$(MODULE)/images \
	     @EDJE_DEF@

e-module-$(MODULE).edj:  e-module-$(MODULE).edc
	$(EDJE_CC) $(EDJE_FLAGS) 	\
	$(top_srcdir)/src/modules/$(MODULE)/e-module-$(MODULE).edc \
	$(top_builddir)/src/modules/$(MODULE)/e-module-$(MODULE).edj

# the module .so file
INCLUDES               = -I. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)/src/modules/$(MODULE) \
                         -I$(top_srcdir)/src/bin \
                         -I$(top_builddir)/src/bin \
                         -I$(top_srcdir)/src/modules \
                         @e_cflags@
pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_tiling.c \
                         e_mod_tiling.h \
                         e_mod_config.c

module_la_LIBADD       = @e_libs@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
