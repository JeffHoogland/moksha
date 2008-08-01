MAINTAINERCLEANFILES = Makefile.in
MODULE = mixer

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)
files_DATA = e-module-$(MODULE).edj module.desktop

EXTRA_DIST = $(files_DATA)

# the module .so file
INCLUDES          = -I. \
                    -I$(top_srcdir) \
                    -I$(top_srcdir)/src/modules/$(MODULE) \
                    -I$(top_srcdir)/src/bin \
                    -I$(top_srcdir)/src/lib \
                    -I$(top_srcdir)/src/modules \
                    @e_cflags@ @SOUND_CFLAGS@
pkgdir            = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES   = module.la

module_la_SOURCES = e_mod_main.h \
		    e_mod_main.c \
                    e_mod_system.h \
                    conf_gadget.c \
                    conf_module.c \
		    app_mixer.c

if HAVE_ALSA
module_la_SOURCES += sys_alsa.c
else
module_la_SOURCES += sys_dummy.c
endif

module_la_LIBADD       = @SOUND_LDFLAGS@ @e_libs@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
