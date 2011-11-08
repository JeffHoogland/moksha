MAINTAINERCLEANFILES = Makefile.in module.desktop
MODULE = conf_randr

EDJE_CC = @edje_cc@
EDJE_FLAGS = -v \
           -id $(top_srcdir)/src/modules/$(MODULE)/images \
           @EDJE_DEF@

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)
files_DATA = \
           e-module-$(MODULE).edj \
           module.desktop

EXTRA_DIST = \
           e-module-$(MODULE).edc \
           module.desktop.in

# the module .so file
INCLUDES               = -I. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)/src/modules/$(MODULE) \
                         -I$(top_srcdir)/src/bin \
                         -I$(top_builddir)/src/bin \
                         -I$(top_srcdir)/src/lib \
                         -I$(top_srcdir)/src/modules \
                         @e_cflags@
pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h \
                         e_int_config_randr_orientation.c \
                         e_int_config_randr_resolutions.c \
                         e_int_config_randr_arrangement.c \
                         e_int_config_randr_policies.c \
                         e_int_config_randr.c \
                         e_int_config_randr.h

module_la_LIBADD       = @e_libs@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

e-module-$(MODULE).edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/src/modules/$(MODULE)/e-module-$(MODULE).edc \
	$(top_builddir)/src/modules/$(MODULE)/e-module-$(MODULE).edj

clean-local:
	rm -f *.edj

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
