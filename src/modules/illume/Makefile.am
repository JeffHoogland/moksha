MAINTAINERCLEANFILES = Makefile.in
MODULE = illume

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
                         -I$(top_srcdir)/src/lib \
                         -I$(top_srcdir)/src/modules \
                         @e_cflags@
pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h
                         e_mod_layout.h \
		         e_mod_layout.c \
			 e_mod_win.h \
                         e_mod_win.c \
                         e_kbd.h \
		         e_kbd.c \
			 e_slipshelf.h \
                         e_slipshelf.c \
                         e_slipwin.h \
		         e_slipwin.c \
			 e_mod_gad_wifi.c \
                         e_mod_gad_wifi.h \
                         e_mod_gad_gsm.c \
		         e_mod_gad_gsm.h \
			 e_mod_gad_usb.c \
                         e_mod_gad_usb.h \
                         e_mod_gad_bluetooth.c \
		         e_mod_gad_bluetooth.h \
			 e_pwr.c \
                         e_pwr.h \
                         e_busywin.h \
		         e_busywin.c \
			 e_cfg.h \
                         e_cfg.c \
                         e_mod_gad_cfg.c \
		         e_mod_gad_cfg.h \
			 e_winilist.c \
                         e_winilist.h \
                         e_simplelock.c \
		         e_simplelock.h \
			 e_kbd_send.c \
                         e_kbd_send.h \
                         e_kbd_int.c \
		         e_kbd_int.h \
			 e_kbd_buf.c \
                         e_kbd_buf.h \
                         e_kbd_dict.c \
		         e_kbd_dict.h \
			 e_flaunch.c \
                         e_flaunch.h \
                         e_busycover.c \
		         e_busycover.h

module_la_LIBADD       = @e_libs@ @dlopen_libs@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

wifigetdir = $(pkgdir)

wifiget_PROGRAMS        = wifiget
wifiget_SOURCES         = wifiget.c
wifiget_LDFLAGS         =

kbdsdir = $(datadir)/keyboards
kbds_DATA = \
            keyboards/ignore_built_in_keyboards \
            keyboards/Default.kbd \
            keyboards/alpha.png \
	    keyboards/Numbers.kbd \
            keyboards/numeric.png \
            keyboards/Terminal.kbd \
	    keyboards/qwerty.png \
	    keyboards/up.png \
	    keyboards/down.png \
	    keyboards/left.png \
            keyboards/right.png \
            keyboards/shift.png \
	    keyboards/tab.png \
	    keyboards/enter.png \
	    keyboards/backspace.png

dictsdir = $(datadir)/dicts
dicts_DATA = \
            dicts/English_(US).dic \
            dicts/English_(US)_Small.dic
			
uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
