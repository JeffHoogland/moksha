MAINTAINERCLEANFILES = Makefile.in
MODULE = wizard

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)
files_DATA = \
data/def-ibar.txt

desksdir = $(libdir)/enlightenment/modules/$(MODULE)/extra_desktops
desks_DATA = \
data/extra_desktops/mplayer.desktop \
data/extra_desktops/xterm.desktop

favsdir = $(libdir)/enlightenment/modules/$(MODULE)/favorites
favs_DATA = \
data/favorites/.order \
data/favorites/desktop.desktop \
data/favorites/home.desktop \
data/favorites/root.desktop \
data/favorites/tmp.desktop

desktopdir = $(libdir)/enlightenment/modules/$(MODULE)/desktop
desktop_DATA = \
data/desktop/home.desktop \
data/desktop/root.desktop \
data/desktop/tmp.desktop

### dont install these - this way e wont list the module for people to
# accidentally load
#files_DATA = \
# e-module-$(MODULE).edj 
# module.desktop

EXTRA_DIST = $(files_DATA) $(desks_DATA)

# the module .so file
INCLUDES               = -I. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)/src/modules/$(MODULE) \
                         -I$(top_srcdir)/src/bin \
                         -I$(top_builddir)/src/bin \
                         -I$(top_srcdir)/src/modules \
                         @ECONNMAN_CFLAGS@ \
                         @e_cflags@ \
                         -I$(top_srcdir)/src/modules/comp
pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la \
                         page_000.la \
                         page_010.la \
                         page_020.la \
                         page_030.la \
                         page_040.la \
                         page_050.la \
                         page_060.la \
                         page_070.la \
                         page_080.la \
                         page_090.la \
                         page_100.la \
                         page_110.la \
                         page_120.la \
                         page_130.la \
                         page_140.la \
                         page_150.la \
                         page_160.la \
                         page_200.la

module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h \
			 e_wizard.c \
                         e_wizard.h

module_la_LIBADD       = @e_libs@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

#Auto - Splash screen
page_000_la_SOURCES      = page_000.c
page_000_la_LIBADD       = @e_libs@ @dlopen_libs@
page_000_la_LDFLAGS      = -module -avoid-version
page_000_la_DEPENDENCIES = $(top_builddir)/config.h

#Choose language
page_010_la_SOURCES      = page_010.c
page_010_la_LIBADD       = @e_libs@ @dlopen_libs@
page_010_la_LDFLAGS      = -module -avoid-version
page_010_la_DEPENDENCIES = $(top_builddir)/config.h

#Choose Profile
page_020_la_SOURCES      = page_020.c
page_020_la_LIBADD       = @e_libs@ @dlopen_libs@
page_020_la_LDFLAGS      = -module -avoid-version
page_020_la_DEPENDENCIES = $(top_builddir)/config.h

#Auto - set up menu
page_030_la_SOURCES      = page_030.c
page_030_la_LIBADD       = @e_libs@ @dlopen_libs@
page_030_la_LDFLAGS      = -module -avoid-version
page_030_la_DEPENDENCIES = $(top_builddir)/config.h

#Auto - add new app icons
page_040_la_SOURCES      = page_040.c
page_040_la_LIBADD       = @e_libs@ @dlopen_libs@
page_040_la_LDFLAGS      = -module -avoid-version
page_040_la_DEPENDENCIES = $(top_builddir)/config.h

#Request what scaling to use
page_050_la_SOURCES      = page_050.c
page_050_la_LIBADD       = @e_libs@ @dlopen_libs@
page_050_la_LDFLAGS      = -module -avoid-version
page_050_la_DEPENDENCIES = $(top_builddir)/config.h

#Request which focus style to use
page_060_la_SOURCES      = page_060.c
page_060_la_LIBADD       = @e_libs@ @dlopen_libs@
page_060_la_LDFLAGS      = -module -avoid-version
page_060_la_DEPENDENCIES = $(top_builddir)/config.h

#Auto - Figure out icon theme to use
page_070_la_SOURCES      = page_070.c
page_070_la_LIBADD       = @e_libs@ @dlopen_libs@
page_070_la_LDFLAGS      = -module -avoid-version
page_070_la_DEPENDENCIES = $(top_builddir)/config.h

#Auto - fill ibar
page_080_la_SOURCES      = page_080.c
page_080_la_LIBADD       = @e_libs@ @dlopen_libs@
page_080_la_LDFLAGS      = -module -avoid-version
page_080_la_DEPENDENCIES = $(top_builddir)/config.h

#Auto - do we need battery?
page_090_la_SOURCES      = page_090.c
page_090_la_LIBADD       = @e_libs@ @dlopen_libs@
page_090_la_LDFLAGS      = -module -avoid-version
page_090_la_DEPENDENCIES = $(top_builddir)/config.h

#Auto - do we need cpufreq?
page_100_la_SOURCES      = page_100.c
page_100_la_LIBADD       = @e_libs@ @dlopen_libs@
page_100_la_LDFLAGS      = -module -avoid-version
page_100_la_DEPENDENCIES = $(top_builddir)/config.h

#Auto+feedback - do we need connman?
page_110_la_SOURCES      = page_110.c
page_110_la_LIBADD       = @e_libs@ @dlopen_libs@ @ECONNMAN_LIBS@
page_110_la_LDFLAGS      = -module -avoid-version
page_110_la_DEPENDENCIES = $(top_builddir)/config.h

#Auto - do we need temperature?
page_120_la_SOURCES      = page_120.c
page_120_la_LIBADD       = @e_libs@ @dlopen_libs@
page_120_la_LDFLAGS      = -module -avoid-version
page_120_la_DEPENDENCIES = $(top_builddir)/config.h

#Auto - do we need backlight?
page_130_la_SOURCES      = page_130.c
page_130_la_LIBADD       = @e_libs@ @dlopen_libs@
page_130_la_LDFLAGS      = -module -avoid-version
page_130_la_DEPENDENCIES = $(top_builddir)/config.h

#Auto - do we need temperature?
page_140_la_SOURCES      = page_140.c
page_140_la_LIBADD       = @e_libs@ @dlopen_libs@
page_140_la_LDFLAGS      = -module -avoid-version
page_140_la_DEPENDENCIES = $(top_builddir)/config.h

#Auto - do we need temperature?
page_150_la_SOURCES      = page_150.c $(top_srcdir)/src/modules/comp/e_mod_comp_cfdata.c
page_150_la_LIBADD       = @e_libs@ @dlopen_libs@
page_150_la_LDFLAGS      = -module -avoid-version
page_150_la_DEPENDENCIES = $(top_builddir)/config.h

#Auto - install favorites and ~/Desktop and entries
page_160_la_SOURCES      = page_160.c
page_160_la_LIBADD       = @e_libs@ @dlopen_libs@
page_160_la_LDFLAGS      = -module -avoid-version
page_160_la_DEPENDENCIES = $(top_builddir)/config.h

#Auto - finish up applying
page_200_la_SOURCES      = page_200.c
page_200_la_LIBADD       = @e_libs@ @dlopen_libs@
page_200_la_LDFLAGS      = -module -avoid-version
page_200_la_DEPENDENCIES = $(top_builddir)/config.h

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
