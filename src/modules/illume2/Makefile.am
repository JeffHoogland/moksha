MAINTAINERCLEANFILES = Makefile.in module.desktop
MODULE = illume2
SUBDIRS = images keyboards policies doc

EDJE_CC = @edje_cc@
EDJE_FLAGS_VERBOSE_ = 
EDJE_FLAGS_VERBOSE_0 = 
EDJE_FLAGS_VERBOSE_1 = -v
EDJE_FLAGS = $(EDJE_FLAGS_$(V)) \
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
                         -I$(top_srcdir)/src/modules \
                         @e_cflags@

pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la

module_la_SOURCES = \
                  e_illume.h \
                  e_illume.c \
                  e_illume_private.h \
                  e_mod_config_policy.h \
                  e_mod_config_policy.c \
                  e_mod_select_window.h \
                  e_mod_select_window.c \
                  e_mod_config_windows.h \
                  e_mod_config_windows.c \
                  e_mod_config_animation.h \
                  e_mod_config_animation.c \
                  e_mod_quickpanel.h \
                  e_mod_quickpanel.c \
                  e_mod_kbd_device.h \
                  e_mod_kbd_device.c \
                  e_mod_kbd.h \
                  e_mod_kbd.c \
                  e_mod_policy.h \
                  e_mod_policy.c \
                  e_mod_config.h \
                  e_mod_config.c \
                  e_mod_main.h \
                  e_mod_main.c

module_la_LIBADD       = @e_libs@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

e-module-$(MODULE).edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/src/modules/$(MODULE)/e-module-$(MODULE).edc \
	$(top_builddir)/src/modules/$(MODULE)/e-module-$(MODULE).edj

.PHONY: doc

# Documentation

doc:
	@echo "entering doc/"
	make -C doc doc

clean-local:
	rm -f *.edj

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)/policies
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)/keyboards
