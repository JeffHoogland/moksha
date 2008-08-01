MAINTAINERCLEANFILES = Makefile.in
SUBDIRS = images

EDJE_CC = @edje_cc@
EDJE_FLAGS = \
-v \
-id $(top_srcdir)/data/themes/images \
-fd $(top_srcdir)/data/fonts \
@EDJE_DEF@

filesdir = $(datadir)/enlightenment/data/themes
files_DATA = \
default.edj

EXTRA_DIST = \
default.edc \
default_background.edc \
default_battery.edc \
default_border.edc \
default_clock.edc \
default_gadman.edc \
default_ibar.edc \
default_menu.edc \
default_pager.edc \
default_resize.edc \
default_move.edc \
default_temperature.edc \
default_error.edc \
default_cpufreq.edc \
default_ibox.edc \
default_start.edc \
default_winlist.edc \
default_transitions.edc \
default_dialog.edc \
default_icons.edc \
default_pointer.edc \
default_configure.edc \
default_about.edc \
default_theme_about.edc \
default_entry.edc \
default_fileman.edc \
default_filesel.edc \
default_check.edc \
default_radio.edc \
default_frame.edc \
default_label.edc \
default_button.edc \
default_scrollframe.edc \
default_ilist.edc \
default_tlist.edc \
default_slider.edc \
default_exebuf.edc \
default_desklock.edc \
default_textblock.edc \
default_shelf.edc \
default_preview.edc \
default_cslider.edc \
default_spectrum.edc \
default_color_well.edc \
default_sys.edc \
default_deskpreview.edc \
default_fontpreview.edc \
default_wizard.edc \
default_toolbar.edc \
default_slidesel.edc \
default_mixer.edc

default.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/default.edc \
	$(top_builddir)/data/themes/default.edj

clean-local:
	rm -f *.edj
