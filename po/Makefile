# Makefile for PO directory in any package using GNU gettext.
# Copyright (C) 1995-1997, 2000-2007, 2009-2010 by Ulrich Drepper <drepper@gnu.ai.mit.edu>
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU General Public
# License but which still want to provide support for the GNU gettext
# functionality.
# Please note that the actual code of GNU gettext is covered by the GNU
# General Public License and is *not* in the public domain.
#
# Origin: gettext-0.18
GETTEXT_MACRO_VERSION = 0.18

PACKAGE = enlightenment
VERSION = 0.17.6
PACKAGE_BUGREPORT = enlightenment-devel@lists.sourceforge.net

SHELL = /bin/sh


srcdir = .
top_srcdir = ..


prefix = /usr
exec_prefix = ${prefix}
datarootdir = ${prefix}/share
datadir = ${datarootdir}
localedir = ${datarootdir}/locale
gettextsrcdir = $(datadir)/gettext/po

INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644

# We use $(mkdir_p).
# In automake <= 1.9.x, $(mkdir_p) is defined either as "mkdir -p --" or as
# "$(mkinstalldirs)" or as "$(install_sh) -d". For these automake versions,
# ${SHELL} /home/jeff/bodhi_debs/20150407/e17-20150407/install-sh does not start with $(SHELL), so we add it.
# In automake >= 1.10, /bin/mkdir -p is derived from ${MKDIR_P}, which is defined
# either as "/path/to/mkdir -p" or ".../install-sh -c -d". For these automake
# versions, $(mkinstalldirs) and $(install_sh) are unused.
mkinstalldirs = $(SHELL) ${SHELL} /home/jeff/bodhi_debs/20150407/e17-20150407/install-sh -d
install_sh = $(SHELL) ${SHELL} /home/jeff/bodhi_debs/20150407/e17-20150407/install-sh
MKDIR_P = /bin/mkdir -p
mkdir_p = /bin/mkdir -p

GMSGFMT_ = /usr/bin/msgfmt
GMSGFMT_no = /usr/bin/msgfmt
GMSGFMT_yes = /usr/bin/msgfmt
GMSGFMT = $(GMSGFMT_$(USE_MSGCTXT))
MSGFMT_ = /usr/bin/msgfmt
MSGFMT_no = /usr/bin/msgfmt
MSGFMT_yes = /usr/bin/msgfmt
MSGFMT = $(MSGFMT_$(USE_MSGCTXT))
XGETTEXT_ = /usr/bin/xgettext
XGETTEXT_no = /usr/bin/xgettext
XGETTEXT_yes = /usr/bin/xgettext
XGETTEXT = $(XGETTEXT_$(USE_MSGCTXT))
MSGMERGE = msgmerge
MSGMERGE_UPDATE = /usr/bin/msgmerge --update
MSGINIT = msginit
MSGCONV = msgconv
MSGFILTER = msgfilter

POFILES =  ar.po bg.po ca.po cs.po da.po de.po el.po eo.po es.po et.po fi.po fo.po fr.po fr_CH.po gl.po he.po hr.po hu.po it.po ja.po km.po ko.po ku.po lt.po ms.po nb.po nl.po pl.po pt.po pt_BR.po ro.po ru.po sk.po sl.po sr.po sv.po tr.po uk.po zh_CN.po zh_TW.po
GMOFILES =  ar.gmo bg.gmo ca.gmo cs.gmo da.gmo de.gmo el.gmo eo.gmo es.gmo et.gmo fi.gmo fo.gmo fr.gmo fr_CH.gmo gl.gmo he.gmo hr.gmo hu.gmo it.gmo ja.gmo km.gmo ko.gmo ku.gmo lt.gmo ms.gmo nb.gmo nl.gmo pl.gmo pt.gmo pt_BR.gmo ro.gmo ru.gmo sk.gmo sl.gmo sr.gmo sv.gmo tr.gmo uk.gmo zh_CN.gmo zh_TW.gmo
UPDATEPOFILES =  ar.po-update bg.po-update ca.po-update cs.po-update da.po-update de.po-update el.po-update eo.po-update es.po-update et.po-update fi.po-update fo.po-update fr.po-update fr_CH.po-update gl.po-update he.po-update hr.po-update hu.po-update it.po-update ja.po-update km.po-update ko.po-update ku.po-update lt.po-update ms.po-update nb.po-update nl.po-update pl.po-update pt.po-update pt_BR.po-update ro.po-update ru.po-update sk.po-update sl.po-update sr.po-update sv.po-update tr.po-update uk.po-update zh_CN.po-update zh_TW.po-update
DUMMYPOFILES =  ar.nop bg.nop ca.nop cs.nop da.nop de.nop el.nop eo.nop es.nop et.nop fi.nop fo.nop fr.nop fr_CH.nop gl.nop he.nop hr.nop hu.nop it.nop ja.nop km.nop ko.nop ku.nop lt.nop ms.nop nb.nop nl.nop pl.nop pt.nop pt_BR.nop ro.nop ru.nop sk.nop sl.nop sr.nop sv.nop tr.nop uk.nop zh_CN.nop zh_TW.nop
DISTFILES.common = Makefile.in.in remove-potcdate.sin \
$(DISTFILES.common.extra1) $(DISTFILES.common.extra2) $(DISTFILES.common.extra3)
DISTFILES = $(DISTFILES.common) Makevars POTFILES.in \
$(POFILES) $(GMOFILES) \
$(DISTFILES.extra1) $(DISTFILES.extra2) $(DISTFILES.extra3)

POTFILES = \
     ../src/bin/e_about.c \
     ../src/bin/e_actions.c \
     ../src/bin/e_alert.c \
     ../src/bin/e_atoms.c \
     ../src/bin/e_bg.c \
     ../src/bin/e_bindings.c \
     ../src/bin/e_border.c \
     ../src/bin/e_box.c \
     ../src/bin/e_canvas.c \
     ../src/bin/e_color.c \
     ../src/bin/e_color_class.c \
     ../src/bin/e_color_dialog.c \
     ../src/bin/e_config.c \
     ../src/bin/e_config_dialog.c \
     ../src/bin/e_configure.c \
     ../src/bin/e_confirm_dialog.c \
     ../src/bin/e_container.c \
     ../src/bin/e_datastore.c \
     ../src/bin/e_desk.c \
     ../src/bin/e_desklock.c \
     ../src/bin/e_dialog.c \
     ../src/bin/e_dnd.c \
     ../src/bin/e_dpms.c \
     ../src/bin/e_eap_editor.c \
     ../src/bin/e_entry.c \
     ../src/bin/e_entry_dialog.c \
     ../src/bin/e_error.c \
     ../src/bin/e_exec.c \
     ../src/bin/e_exehist.c \
     ../src/bin/e_filereg.c \
     ../src/bin/e_flowlayout.c \
     ../src/bin/e_fm.c \
     ../src/bin/e_fm_custom.c \
     ../src/bin/e_fm_device.c \
     ../src/bin/e_fm_mime.c \
     ../src/bin/e_fm_prop.c \
     ../src/bin/e_focus.c \
     ../src/bin/e_font.c \
     ../src/bin/e_gadcon.c \
     ../src/bin/e_grab_dialog.c \
     ../src/bin/e_grabinput.c \
     ../src/bin/e_hints.c \
     ../src/bin/e_icon.c \
     ../src/bin/e_ilist.c \
     ../src/bin/e_imc_main.c \
     ../src/bin/e_import_dialog.c \
     ../src/bin/e_import_config_dialog.c \
     ../src/bin/e_init.c \
     ../src/bin/e_int_border_locks.c \
     ../src/bin/e_int_border_menu.c \
     ../src/bin/e_int_border_prop.c \
     ../src/bin/e_int_border_remember.c \
     ../src/bin/e_int_config_modules.c \
     ../src/bin/e_int_gadcon_config.c \
     ../src/bin/e_int_toolbar_config.c \
     ../src/bin/e_intl.c \
     ../src/bin/e_int_menus.c \
     ../src/bin/e_int_shelf_config.c \
     ../src/bin/e_ipc.c \
     ../src/bin/e_ipc_codec.c \
     ../src/bin/e_layout.c \
     ../src/bin/e_livethumb.c \
     ../src/bin/e_main.c \
     ../src/bin/e_manager.c \
     ../src/bin/e_maximize.c \
     ../src/bin/e_menu.c \
     ../src/bin/e_module.c \
     ../src/bin/e_mouse.c \
     ../src/bin/e_moveresize.c \
     ../src/bin/e_msg.c \
     ../src/bin/e_obj_dialog.c \
     ../src/bin/e_object.c \
     ../src/bin/e_order.c \
     ../src/bin/e_pan.c \
     ../src/bin/e_path.c \
     ../src/bin/e_place.c \
     ../src/bin/e_pointer.c \
     ../src/bin/e_popup.c \
     ../src/bin/e_prefix.c \
     ../src/bin/e_remember.c \
     ../src/bin/e_resist.c \
     ../src/bin/e_screensaver.c \
     ../src/bin/e_scrollframe.c \
     ../src/bin/e_sha1.c \
     ../src/bin/e_shelf.c \
     ../src/bin/e_signals.c \
     ../src/bin/e_slider.c \
     ../src/bin/e_spectrum.c \
     ../src/bin/e_start_main.c \
     ../src/bin/e_startup.c \
     ../src/bin/e_stolen.c \
     ../src/bin/e_sys.c \
     ../src/bin/e_sys_main.c \
     ../src/bin/e_table.c \
     ../src/bin/e_test.c \
     ../src/bin/e_theme_about.c \
     ../src/bin/e_theme.c \
     ../src/bin/e_thumb.c \
     ../src/bin/e_thumb_main.c \
     ../src/bin/e_toolbar.c \
     ../src/bin/e_update.c \
     ../src/bin/e_user.c \
     ../src/bin/e_utils.c \
     ../src/bin/e_widget_aspect.c \
     ../src/bin/e_widget_button.c \
     ../src/bin/e_widget.c \
     ../src/bin/e_widget_check.c \
     ../src/bin/e_widget_color_well.c \
     ../src/bin/e_widget_config_list.c \
     ../src/bin/e_widget_csel.c \
     ../src/bin/e_widget_cslider.c \
     ../src/bin/e_widget_deskpreview.c \
     ../src/bin/e_widget_entry.c \
     ../src/bin/e_widget_filepreview.c \
     ../src/bin/e_widget_font_preview.c \
     ../src/bin/e_widget_framelist.c \
     ../src/bin/e_widget_frametable.c \
     ../src/bin/e_widget_fsel.c \
     ../src/bin/e_widget_ilist.c \
     ../src/bin/e_widget_image.c \
     ../src/bin/e_widget_label.c \
     ../src/bin/e_widget_list.c \
     ../src/bin/e_widget_preview.c \
     ../src/bin/e_widget_radio.c \
     ../src/bin/e_widget_scrollframe.c \
     ../src/bin/e_widget_slider.c \
     ../src/bin/e_widget_spectrum.c \
     ../src/bin/e_widget_table.c \
     ../src/bin/e_widget_textblock.c \
     ../src/bin/e_win.c \
     ../src/bin/e_xinerama.c \
     ../src/bin/e_xkb.c \
     ../src/bin/e_zone.c \
     ../src/modules/access/e_mod_config.c \
     ../src/modules/backlight/e_mod_main.c \
     ../src/modules/battery/e_mod_config.c \
     ../src/modules/battery/e_mod_main.c \
     ../src/modules/clock/e_mod_config.c \
     ../src/modules/clock/e_mod_main.c \
     ../src/modules/comp/e_mod_comp.c \
     ../src/modules/comp/e_mod_config.c \
     ../src/modules/comp/e_mod_main.c \
     ../src/modules/conf/e_conf.c \
     ../src/modules/conf/e_mod_config.c \
     ../src/modules/conf/e_mod_main.c \
     ../src/modules/conf_applications/e_int_config_apps.c \
     ../src/modules/conf_applications/e_int_config_apps_personal.c \
     ../src/modules/conf_applications/e_int_config_defapps.c \
     ../src/modules/conf_applications/e_int_config_deskenv.c \
     ../src/modules/conf_applications/e_mod_main.c \
     ../src/modules/conf_dialogs/e_int_config_profiles.c \
     ../src/modules/conf_dialogs/e_int_config_dialogs.c \
     ../src/modules/conf_dialogs/e_mod_main.c \
     ../src/modules/conf_display/e_int_config_display.c \
     ../src/modules/conf_display/e_int_config_desks.c \
     ../src/modules/conf_display/e_int_config_desk.c \
     ../src/modules/conf_display/e_int_config_desklock_fsel.c \
     ../src/modules/conf_display/e_int_config_desklock.c \
     ../src/modules/conf_display/e_int_config_screensaver.c \
     ../src/modules/conf_display/e_int_config_dpms.c \
     ../src/modules/conf_display/e_mod_main.c \
     ../src/modules/conf_edgebindings/e_int_config_edgebindings.c \
     ../src/modules/conf_edgebindings/e_int_config_signalbindings.c \
     ../src/modules/conf_edgebindings/e_mod_main.c \
     ../src/modules/conf_interaction/e_int_config_interaction.c \
     ../src/modules/conf_interaction/e_int_config_mouse.c \
     ../src/modules/conf_interaction/e_mod_main.c \
     ../src/modules/conf_intl/e_int_config_imc.c \
     ../src/modules/conf_intl/e_int_config_imc_import.c \
     ../src/modules/conf_intl/e_int_config_intl.c \
     ../src/modules/conf_intl/e_mod_main.c \
     ../src/modules/conf_keybindings/e_int_config_acpibindings.c \
     ../src/modules/conf_keybindings/e_int_config_keybindings.c \
     ../src/modules/conf_keybindings/e_int_config_mousebindings.c \
     ../src/modules/conf_keybindings/e_mod_main.c \
     ../src/modules/conf_menus/e_int_config_menus.c \
     ../src/modules/conf_menus/e_mod_main.c \
     ../src/modules/conf_paths/e_int_config_paths.c \
     ../src/modules/conf_paths/e_int_config_env.c \
     ../src/modules/conf_paths/e_mod_main.c \
     ../src/modules/conf_performance/e_int_config_engine.c \
     ../src/modules/conf_performance/e_int_config_performance.c \
     ../src/modules/conf_performance/e_int_config_powermanagement.c \
     ../src/modules/conf_performance/e_mod_main.c \
     ../src/modules/conf_randr/e_int_config_randr.c \
     ../src/modules/conf_randr/e_mod_main.c \
     ../src/modules/conf_shelves/e_int_config_shelf.c \
     ../src/modules/conf_shelves/e_mod_main.c \
     ../src/modules/conf_theme/e_int_config_wallpaper.c \
     ../src/modules/conf_theme/e_int_config_borders.c \
     ../src/modules/conf_theme/e_int_config_color_classes.c \
     ../src/modules/conf_theme/e_int_config_fonts.c \
     ../src/modules/conf_theme/e_int_config_scale.c \
     ../src/modules/conf_theme/e_int_config_startup.c \
     ../src/modules/conf_theme/e_int_config_transitions.c \
     ../src/modules/conf_theme/e_int_config_theme.c \
     ../src/modules/conf_theme/e_int_config_theme_import.c \
     ../src/modules/conf_theme/e_int_config_xsettings.c \
     ../src/modules/conf_theme/e_mod_main.c \
     ../src/modules/conf_window_manipulation/e_int_config_clientlist.c \
     ../src/modules/conf_window_manipulation/e_int_config_window_display.c \
     ../src/modules/conf_window_manipulation/e_int_config_focus.c \
     ../src/modules/conf_window_manipulation/e_int_config_window_geometry.c \
     ../src/modules/conf_window_manipulation/e_int_config_window_process.c \
     ../src/modules/conf_window_manipulation/e_mod_main.c \
     ../src/modules/conf_window_remembers/e_int_config_remembers.c \
     ../src/modules/conf_window_remembers/e_mod_main.c \
     ../src/modules/cpufreq/e_mod_main.c \
     ../src/modules/cpufreq/freqset.c \
     ../src/modules/dropshadow/e_mod_config.c \
     ../src/modules/dropshadow/e_mod_main.c \
     ../src/modules/everything/e_mod_main.c \
     ../src/modules/everything/evry.c \
     ../src/modules/everything/evry_config.c \
     ../src/modules/everything/evry_gadget.c \
     ../src/modules/everything/evry_history.c \
     ../src/modules/everything/evry_plug_actions.c \
     ../src/modules/everything/evry_plug_aggregator.c \
     ../src/modules/everything/evry_plug_apps.c \
     ../src/modules/everything/evry_plug_calc.c \
     ../src/modules/everything/evry_plug_clipboard.c \
     ../src/modules/everything/evry_plug_collection.c \
     ../src/modules/everything/evry_plug_files.c \
     ../src/modules/everything/evry_plug_settings.c \
     ../src/modules/everything/evry_plug_text.c \
     ../src/modules/everything/evry_plug_windows.c \
     ../src/modules/everything/evry_plugin.c \
     ../src/modules/everything/evry_util.c \
     ../src/modules/everything/evry_view.c \
     ../src/modules/everything/evry_view_help.c \
     ../src/modules/everything/evry_view_tabs.c \
     ../src/modules/fileman/e_fwin.c \
     ../src/modules/fileman/e_fwin_nav.c \
     ../src/modules/fileman/e_int_config_mime.c \
     ../src/modules/fileman/e_int_config_mime_edit.c \
     ../src/modules/fileman/e_mod_config.c \
     ../src/modules/fileman/e_mod_dbus.c \
     ../src/modules/fileman/e_mod_main.c \
     ../src/modules/fileman/e_mod_menu.c \
     ../src/modules/fileman_opinfo/e_mod_main.c \
     ../src/modules/gadman/e_mod_config.c \
     ../src/modules/gadman/e_mod_gadman.c \
     ../src/modules/gadman/e_mod_main.c \
     ../src/modules/winlist/e_int_config_winlist.c \
     ../src/modules/winlist/e_mod_main.c \
     ../src/modules/winlist/e_winlist.c \
     ../src/modules/ibar/e_mod_config.c \
     ../src/modules/ibar/e_mod_main.c \
     ../src/modules/ibox/e_mod_config.c \
     ../src/modules/ibox/e_mod_main.c \
     ../src/modules/pager/e_mod_config.c \
     ../src/modules/pager/e_mod_main.c \
     ../src/modules/physics/e_mod_config.c \
     ../src/modules/physics/e_mod_main.c \
     ../src/modules/start/e_mod_main.c \
     ../src/modules/syscon/e_int_config_syscon.c \
     ../src/modules/syscon/e_mod_main.c \
     ../src/modules/syscon/e_syscon.c \
     ../src/modules/syscon/e_syscon_gadget.c \
     ../src/modules/temperature/e_mod_config.c \
     ../src/modules/temperature/e_mod_main.c \
     ../src/modules/wizard/e_mod_main.c \
     ../src/modules/wizard/e_wizard.c \
     ../src/modules/wizard/page_000.c \
     ../src/modules/wizard/page_010.c \
     ../src/modules/wizard/page_011.c \
     ../src/modules/wizard/page_020.c \
     ../src/modules/wizard/page_030.c \
     ../src/modules/wizard/page_040.c \
     ../src/modules/wizard/page_050.c \
     ../src/modules/wizard/page_060.c \
     ../src/modules/wizard/page_070.c \
     ../src/modules/wizard/page_080.c \
     ../src/modules/wizard/page_090.c \
     ../src/modules/wizard/page_100.c \
     ../src/modules/wizard/page_110.c \
     ../src/modules/wizard/page_120.c \
     ../src/modules/wizard/page_130.c \
     ../src/modules/wizard/page_140.c \
     ../src/modules/wizard/page_150.c \
     ../src/modules/wizard/page_160.c \
     ../src/modules/wizard/page_170.c \
     ../src/modules/wizard/page_180.c \
     ../src/modules/wizard/page_200.c \
     ../src/modules/mixer/app_mixer.c \
     ../src/modules/mixer/conf_gadget.c \
     ../src/modules/mixer/conf_module.c \
     ../src/modules/mixer/e_mod_main.c \
     ../src/modules/connman/e_mod_main.c \
     ../src/modules/quickaccess/e_mod_config.c \
     ../src/modules/quickaccess/e_mod_main.c \
     ../src/modules/quickaccess/e_mod_quickaccess.c \
     ../src/modules/systray/e_mod_main.c \
     ../src/modules/shot/e_mod_main.c \
     ../src/modules/tasks/e_mod_main.c \
     ../src/modules/tasks/e_mod_config.c \
     ../src/modules/tiling/e_mod_config.c \
     ../src/modules/tiling/e_mod_tiling.c \
     ../src/modules/tiling/e_mod_tiling.h \
     ../src/modules/xkbswitch/e_mod_main.c \
     ../src/modules/xkbswitch/e_mod_config.c \
     ../src/modules/xkbswitch/e_mod_parse.c \
     ../src/modules/notification/e_mod_main.c \
     ../src/modules/notification/e_mod_config.c

CATALOGS =  ar.gmo bg.gmo ca.gmo cs.gmo da.gmo de.gmo el.gmo eo.gmo es.gmo et.gmo fi.gmo fo.gmo fr.gmo fr_CH.gmo gl.gmo he.gmo hr.gmo hu.gmo it.gmo ja.gmo km.gmo ko.gmo ku.gmo lt.gmo ms.gmo nb.gmo nl.gmo pl.gmo pt.gmo pt_BR.gmo ro.gmo ru.gmo sk.gmo sl.gmo sr.gmo sv.gmo tr.gmo uk.gmo zh_CN.gmo zh_TW.gmo

# Makevars gets inserted here. (Don't remove this line!)
# Makefile variables for PO directory in any package using GNU gettext.

# Usually the message domain is the same as the package name.
DOMAIN = $(PACKAGE)

# These two variables depend on the location of this directory.
subdir = po
top_builddir = ..

# These options get passed to xgettext.
XGETTEXT_OPTIONS = --keyword=_ --keyword=d_:1 --keyword=P_:1,2 \
	--keyword=dP_:1,2 --keyword=N_ --keyword=NP_:1,2 \
	--keyword=EVRY_PLUGIN_BASE:1 --keyword=EVRY_ACTION_NEW:1 \
	--keyword=ACTION_NEW:1 \
	--from-code=UTF-8 --foreign-user

# This is the copyright holder that gets inserted into the header of the
# $(DOMAIN).pot file.  Set this to the copyright holder of the surrounding
# package.  (Note that the msgstr strings, extracted from the package's
# sources, belong to the copyright holder of the package.)  Translators are
# expected to transfer the copyright for their translations to this person
# or entity, or to disclaim their copyright.  The empty string stands for
# the public domain; in this case the translators are expected to disclaim
# their copyright.
COPYRIGHT_HOLDER = Enlightenment development team

# This is the email address or URL to which the translators shall report
# bugs in the untranslated strings:
# - Strings which are not entire sentences, see the maintainer guidelines
#   in the GNU gettext documentation, section 'Preparing Strings'.
# - Strings which use unclear terms or require additional context to be
#   understood.
# - Strings which make invalid assumptions about notation of date, time or
#   money.
# - Pluralisation problems.
# - Incorrect English spelling.
# - Incorrect formatting.
# It can be your email address, or a mailing list address where translators
# can write to without being subscribed, or the URL of a web page through
# which the translators can contact you.
MSGID_BUGS_ADDRESS = enlightenment-devel@lists.sourceforge.net

# This is the list of locale categories, beyond LC_MESSAGES, for which the
# message catalogs shall be used.  It is usually empty.
EXTRA_LOCALE_CATEGORIES =

.SUFFIXES:
.SUFFIXES: .po .gmo .mo .sed .sin .nop .po-create .po-update

.po.mo:
	@echo "$(MSGFMT) -c -o $@ $<"; \
	$(MSGFMT) -c -o t-$@ $< && mv t-$@ $@

.po.gmo:
	@lang=`echo $* | sed -e 's,.*/,,'`; \
	test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	echo "$${cdcmd}rm -f $${lang}.gmo && $(GMSGFMT) -c --statistics --verbose -o $${lang}.gmo $${lang}.po"; \
	cd $(srcdir) && rm -f $${lang}.gmo && $(GMSGFMT) -c --statistics --verbose -o t-$${lang}.gmo $${lang}.po && mv t-$${lang}.gmo $${lang}.gmo

.sin.sed:
	sed -e '/^#/d' $< > t-$@
	mv t-$@ $@


all: check-macro-version all-yes

all-yes: stamp-po
all-no:

# Ensure that the gettext macros and this Makefile.in.in are in sync.
check-macro-version:
	@test "$(GETTEXT_MACRO_VERSION)" = "0.18" \
	  || { echo "*** error: gettext infrastructure mismatch: using a Makefile.in.in from gettext version $(GETTEXT_MACRO_VERSION) but the autoconf macros are from gettext version 0.18" 1>&2; \
	       exit 1; \
	     }

# $(srcdir)/$(DOMAIN).pot is only created when needed. When xgettext finds no
# internationalized messages, no $(srcdir)/$(DOMAIN).pot is created (because
# we don't want to bother translators with empty POT files). We assume that
# LINGUAS is empty in this case, i.e. $(POFILES) and $(GMOFILES) are empty.
# In this case, stamp-po is a nop (i.e. a phony target).

# stamp-po is a timestamp denoting the last time at which the CATALOGS have
# been loosely updated. Its purpose is that when a developer or translator
# checks out the package via CVS, and the $(DOMAIN).pot file is not in CVS,
# "make" will update the $(DOMAIN).pot and the $(CATALOGS), but subsequent
# invocations of "make" will do nothing. This timestamp would not be necessary
# if updating the $(CATALOGS) would always touch them; however, the rule for
# $(POFILES) has been designed to not touch files that don't need to be
# changed.
stamp-po: $(srcdir)/$(DOMAIN).pot
	test ! -f $(srcdir)/$(DOMAIN).pot || \
	  test -z "$(GMOFILES)" || $(MAKE) $(GMOFILES)
	@test ! -f $(srcdir)/$(DOMAIN).pot || { \
	  echo "touch stamp-po" && \
	  echo timestamp > stamp-poT && \
	  mv stamp-poT stamp-po; \
	}

# Note: Target 'all' must not depend on target '$(DOMAIN).pot-update',
# otherwise packages like GCC can not be built if only parts of the source
# have been downloaded.

# This target rebuilds $(DOMAIN).pot; it is an expensive operation.
# Note that $(DOMAIN).pot is not touched if it doesn't need to be changed.
$(DOMAIN).pot-update: $(POTFILES) $(srcdir)/POTFILES.in remove-potcdate.sed
	if LC_ALL=C grep 'GNU enlightenment' $(top_srcdir)/* 2>/dev/null | grep -v 'libtool:' >/dev/null; then \
	  package_gnu='GNU '; \
	else \
	  package_gnu=''; \
	fi; \
	if test -n '$(MSGID_BUGS_ADDRESS)' || test '$(PACKAGE_BUGREPORT)' = '@'PACKAGE_BUGREPORT'@'; then \
	  msgid_bugs_address='$(MSGID_BUGS_ADDRESS)'; \
	else \
	  msgid_bugs_address='$(PACKAGE_BUGREPORT)'; \
	fi; \
	case `$(XGETTEXT) --version | sed 1q | sed -e 's,^[^0-9]*,,'` in \
	  '' | 0.[0-9] | 0.[0-9].* | 0.1[0-5] | 0.1[0-5].* | 0.16 | 0.16.[0-1]*) \
	    $(XGETTEXT) --default-domain=$(DOMAIN) --directory=$(top_srcdir) \
	      --add-comments=TRANSLATORS: $(XGETTEXT_OPTIONS)  \
	      --files-from=$(srcdir)/POTFILES.in \
	      --copyright-holder='$(COPYRIGHT_HOLDER)' \
	      --msgid-bugs-address="$$msgid_bugs_address" \
	    ;; \
	  *) \
	    $(XGETTEXT) --default-domain=$(DOMAIN) --directory=$(top_srcdir) \
	      --add-comments=TRANSLATORS: $(XGETTEXT_OPTIONS)  \
	      --files-from=$(srcdir)/POTFILES.in \
	      --copyright-holder='$(COPYRIGHT_HOLDER)' \
	      --package-name="$${package_gnu}enlightenment" \
	      --package-version='0.17.6' \
	      --msgid-bugs-address="$$msgid_bugs_address" \
	    ;; \
	esac
	test ! -f $(DOMAIN).po || { \
	  if test -f $(srcdir)/$(DOMAIN).pot; then \
	    sed -f remove-potcdate.sed < $(srcdir)/$(DOMAIN).pot > $(DOMAIN).1po && \
	    sed -f remove-potcdate.sed < $(DOMAIN).po > $(DOMAIN).2po && \
	    if cmp $(DOMAIN).1po $(DOMAIN).2po >/dev/null 2>&1; then \
	      rm -f $(DOMAIN).1po $(DOMAIN).2po $(DOMAIN).po; \
	    else \
	      rm -f $(DOMAIN).1po $(DOMAIN).2po $(srcdir)/$(DOMAIN).pot && \
	      mv $(DOMAIN).po $(srcdir)/$(DOMAIN).pot; \
	    fi; \
	  else \
	    mv $(DOMAIN).po $(srcdir)/$(DOMAIN).pot; \
	  fi; \
	}

# This rule has no dependencies: we don't need to update $(DOMAIN).pot at
# every "make" invocation, only create it when it is missing.
# Only "make $(DOMAIN).pot-update" or "make dist" will force an update.
$(srcdir)/$(DOMAIN).pot:
	$(MAKE) $(DOMAIN).pot-update

# This target rebuilds a PO file if $(DOMAIN).pot has changed.
# Note that a PO file is not touched if it doesn't need to be changed.
$(POFILES): $(srcdir)/$(DOMAIN).pot
	@lang=`echo $@ | sed -e 's,.*/,,' -e 's/\.po$$//'`; \
	if test -f "$(srcdir)/$${lang}.po"; then \
	  test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	  echo "$${cdcmd}$(MSGMERGE_UPDATE) $(MSGMERGE_OPTIONS) --lang=$${lang} $${lang}.po $(DOMAIN).pot"; \
	  cd $(srcdir) \
	    && { case `$(MSGMERGE_UPDATE) --version | sed 1q | sed -e 's,^[^0-9]*,,'` in \
	           '' | 0.[0-9] | 0.[0-9].* | 0.1[0-7] | 0.1[0-7].*) \
	             $(MSGMERGE_UPDATE) $(MSGMERGE_OPTIONS) $${lang}.po $(DOMAIN).pot;; \
	           *) \
	             $(MSGMERGE_UPDATE) $(MSGMERGE_OPTIONS) --lang=$${lang} $${lang}.po $(DOMAIN).pot;; \
	         esac; \
	       }; \
	else \
	  $(MAKE) $${lang}.po-create; \
	fi


install: install-exec install-data
install-exec:
install-data: install-data-yes
	if test "$(PACKAGE)" = "gettext-tools"; then \
	  $(mkdir_p) $(DESTDIR)$(gettextsrcdir); \
	  for file in $(DISTFILES.common) Makevars.template; do \
	    $(INSTALL_DATA) $(srcdir)/$$file \
			    $(DESTDIR)$(gettextsrcdir)/$$file; \
	  done; \
	  for file in Makevars; do \
	    rm -f $(DESTDIR)$(gettextsrcdir)/$$file; \
	  done; \
	else \
	  : ; \
	fi
install-data-no: all
install-data-yes: all
	@catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed -e 's/\.gmo$$//'`; \
	  dir=$(localedir)/$$lang/LC_MESSAGES; \
	  $(mkdir_p) $(DESTDIR)$$dir; \
	  if test -r $$cat; then realcat=$$cat; else realcat=$(srcdir)/$$cat; fi; \
	  $(INSTALL_DATA) $$realcat $(DESTDIR)$$dir/$(DOMAIN).mo; \
	  echo "installing $$realcat as $(DESTDIR)$$dir/$(DOMAIN).mo"; \
	  for lc in '' $(EXTRA_LOCALE_CATEGORIES); do \
	    if test -n "$$lc"; then \
	      if (cd $(DESTDIR)$(localedir)/$$lang && LC_ALL=C ls -l -d $$lc 2>/dev/null) | grep ' -> ' >/dev/null; then \
	        link=`cd $(DESTDIR)$(localedir)/$$lang && LC_ALL=C ls -l -d $$lc | sed -e 's/^.* -> //'`; \
	        mv $(DESTDIR)$(localedir)/$$lang/$$lc $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	        mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	        (cd $(DESTDIR)$(localedir)/$$lang/$$lc.old && \
	         for file in *; do \
	           if test -f $$file; then \
	             ln -s ../$$link/$$file $(DESTDIR)$(localedir)/$$lang/$$lc/$$file; \
	           fi; \
	         done); \
	        rm -f $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	      else \
	        if test -d $(DESTDIR)$(localedir)/$$lang/$$lc; then \
	          :; \
	        else \
	          rm -f $(DESTDIR)$(localedir)/$$lang/$$lc; \
	          mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	        fi; \
	      fi; \
	      rm -f $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo; \
	      ln -s ../LC_MESSAGES/$(DOMAIN).mo $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo 2>/dev/null || \
	      ln $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(DOMAIN).mo $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo 2>/dev/null || \
	      cp -p $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(DOMAIN).mo $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo; \
	      echo "installing $$realcat link as $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo"; \
	    fi; \
	  done; \
	done

install-strip: install

installdirs: installdirs-exec installdirs-data
installdirs-exec:
installdirs-data: installdirs-data-yes
	if test "$(PACKAGE)" = "gettext-tools"; then \
	  $(mkdir_p) $(DESTDIR)$(gettextsrcdir); \
	else \
	  : ; \
	fi
installdirs-data-no:
installdirs-data-yes:
	@catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed -e 's/\.gmo$$//'`; \
	  dir=$(localedir)/$$lang/LC_MESSAGES; \
	  $(mkdir_p) $(DESTDIR)$$dir; \
	  for lc in '' $(EXTRA_LOCALE_CATEGORIES); do \
	    if test -n "$$lc"; then \
	      if (cd $(DESTDIR)$(localedir)/$$lang && LC_ALL=C ls -l -d $$lc 2>/dev/null) | grep ' -> ' >/dev/null; then \
	        link=`cd $(DESTDIR)$(localedir)/$$lang && LC_ALL=C ls -l -d $$lc | sed -e 's/^.* -> //'`; \
	        mv $(DESTDIR)$(localedir)/$$lang/$$lc $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	        mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	        (cd $(DESTDIR)$(localedir)/$$lang/$$lc.old && \
	         for file in *; do \
	           if test -f $$file; then \
	             ln -s ../$$link/$$file $(DESTDIR)$(localedir)/$$lang/$$lc/$$file; \
	           fi; \
	         done); \
	        rm -f $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	      else \
	        if test -d $(DESTDIR)$(localedir)/$$lang/$$lc; then \
	          :; \
	        else \
	          rm -f $(DESTDIR)$(localedir)/$$lang/$$lc; \
	          mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	        fi; \
	      fi; \
	    fi; \
	  done; \
	done

# Define this as empty until I found a useful application.
installcheck:

uninstall: uninstall-exec uninstall-data
uninstall-exec:
uninstall-data: uninstall-data-yes
	if test "$(PACKAGE)" = "gettext-tools"; then \
	  for file in $(DISTFILES.common) Makevars.template; do \
	    rm -f $(DESTDIR)$(gettextsrcdir)/$$file; \
	  done; \
	else \
	  : ; \
	fi
uninstall-data-no:
uninstall-data-yes:
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed -e 's/\.gmo$$//'`; \
	  for lc in LC_MESSAGES $(EXTRA_LOCALE_CATEGORIES); do \
	    rm -f $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo; \
	  done; \
	done

check: all

info dvi ps pdf html tags TAGS ctags CTAGS ID:

mostlyclean:
	rm -f remove-potcdate.sed
	rm -f stamp-poT
	rm -f core core.* $(DOMAIN).po $(DOMAIN).1po $(DOMAIN).2po *.new.po
	rm -fr *.o

clean: mostlyclean

distclean: clean
	rm -f Makefile Makefile.in POTFILES *.mo

maintainer-clean: distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."
	rm -f stamp-po $(GMOFILES)

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)
dist distdir:
	$(MAKE) update-po
	@$(MAKE) dist2
# This is a separate target because 'update-po' must be executed before.
dist2: stamp-po $(DISTFILES)
	dists="$(DISTFILES)"; \
	if test "$(PACKAGE)" = "gettext-tools"; then \
	  dists="$$dists Makevars.template"; \
	fi; \
	if test -f $(srcdir)/$(DOMAIN).pot; then \
	  dists="$$dists $(DOMAIN).pot stamp-po"; \
	fi; \
	if test -f $(srcdir)/ChangeLog; then \
	  dists="$$dists ChangeLog"; \
	fi; \
	for i in 0 1 2 3 4 5 6 7 8 9; do \
	  if test -f $(srcdir)/ChangeLog.$$i; then \
	    dists="$$dists ChangeLog.$$i"; \
	  fi; \
	done; \
	if test -f $(srcdir)/LINGUAS; then dists="$$dists LINGUAS"; fi; \
	for file in $$dists; do \
	  if test -f $$file; then \
	    cp -p $$file $(distdir) || exit 1; \
	  else \
	    cp -p $(srcdir)/$$file $(distdir) || exit 1; \
	  fi; \
	done

update-po: Makefile
	$(MAKE) $(DOMAIN).pot-update
	test -z "$(UPDATEPOFILES)" || $(MAKE) $(UPDATEPOFILES)
	$(MAKE) update-gmo

# General rule for creating PO files.

.nop.po-create:
	@lang=`echo $@ | sed -e 's/\.po-create$$//'`; \
	echo "File $$lang.po does not exist. If you are a translator, you can create it through 'msginit'." 1>&2; \
	exit 1

# General rule for updating PO files.

.nop.po-update:
	@lang=`echo $@ | sed -e 's/\.po-update$$//'`; \
	if test "$(PACKAGE)" = "gettext-tools"; then PATH=`pwd`/../src:$$PATH; fi; \
	tmpdir=`pwd`; \
	echo "$$lang:"; \
	test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	echo "$${cdcmd}$(MSGMERGE) $(MSGMERGE_OPTIONS) --lang=$$lang $$lang.po $(DOMAIN).pot -o $$lang.new.po"; \
	cd $(srcdir); \
	if { case `$(MSGMERGE) --version | sed 1q | sed -e 's,^[^0-9]*,,'` in \
	       '' | 0.[0-9] | 0.[0-9].* | 0.1[0-7] | 0.1[0-7].*) \
	         $(MSGMERGE) $(MSGMERGE_OPTIONS) -o $$tmpdir/$$lang.new.po $$lang.po $(DOMAIN).pot;; \
	       *) \
	         $(MSGMERGE) $(MSGMERGE_OPTIONS) --lang=$$lang -o $$tmpdir/$$lang.new.po $$lang.po $(DOMAIN).pot;; \
	     esac; \
	   }; then \
	  if cmp $$lang.po $$tmpdir/$$lang.new.po >/dev/null 2>&1; then \
	    rm -f $$tmpdir/$$lang.new.po; \
	  else \
	    if mv -f $$tmpdir/$$lang.new.po $$lang.po; then \
	      :; \
	    else \
	      echo "msgmerge for $$lang.po failed: cannot move $$tmpdir/$$lang.new.po to $$lang.po" 1>&2; \
	      exit 1; \
	    fi; \
	  fi; \
	else \
	  echo "msgmerge for $$lang.po failed!" 1>&2; \
	  rm -f $$tmpdir/$$lang.new.po; \
	fi

$(DUMMYPOFILES):

update-gmo: Makefile $(GMOFILES)
	@:

# Recreate Makefile by invoking config.status. Explicitly invoke the shell,
# because execution permission bits may not work on the current file system.
# Use /bin/bash, which is the shell determined by autoconf for the use by its
# scripts, not $(SHELL) which is hardwired to /bin/sh and may be deficient.
Makefile: Makefile.in.in Makevars $(top_builddir)/config.status POTFILES.in LINGUAS
	cd $(top_builddir) \
	  && /bin/bash ./config.status $(subdir)/$@.in po-directories

force:

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
# Special Makefile rules for English message catalogs with quotation marks.

DISTFILES.common.extra1 = quot.sed boldquot.sed en@quot.header en@boldquot.header insert-header.sin Rules-quot

.SUFFIXES: .insert-header .po-update-en

en@quot.po-create:
	$(MAKE) en@quot.po-update
en@boldquot.po-create:
	$(MAKE) en@boldquot.po-update

en@quot.po-update: en@quot.po-update-en
en@boldquot.po-update: en@boldquot.po-update-en

.insert-header.po-update-en:
	@lang=`echo $@ | sed -e 's/\.po-update-en$$//'`; \
	if test "$(PACKAGE)" = "gettext"; then PATH=`pwd`/../src:$$PATH; GETTEXTLIBDIR=`cd $(top_srcdir)/src && pwd`; export GETTEXTLIBDIR; fi; \
	tmpdir=`pwd`; \
	echo "$$lang:"; \
	ll=`echo $$lang | sed -e 's/@.*//'`; \
	LC_ALL=C; export LC_ALL; \
	cd $(srcdir); \
	if $(MSGINIT) -i $(DOMAIN).pot --no-translator -l $$lang -o - 2>/dev/null | sed -f $$tmpdir/$$lang.insert-header | $(MSGCONV) -t UTF-8 | $(MSGFILTER) sed -f `echo $$lang | sed -e 's/.*@//'`.sed 2>/dev/null > $$tmpdir/$$lang.new.po; then \
	  if cmp $$lang.po $$tmpdir/$$lang.new.po >/dev/null 2>&1; then \
	    rm -f $$tmpdir/$$lang.new.po; \
	  else \
	    if mv -f $$tmpdir/$$lang.new.po $$lang.po; then \
	      :; \
	    else \
	      echo "creation of $$lang.po failed: cannot move $$tmpdir/$$lang.new.po to $$lang.po" 1>&2; \
	      exit 1; \
	    fi; \
	  fi; \
	else \
	  echo "creation of $$lang.po failed!" 1>&2; \
	  rm -f $$tmpdir/$$lang.new.po; \
	fi

en@quot.insert-header: insert-header.sin
	sed -e '/^#/d' -e 's/HEADER/en@quot.header/g' $(srcdir)/insert-header.sin > en@quot.insert-header

en@boldquot.insert-header: insert-header.sin
	sed -e '/^#/d' -e 's/HEADER/en@boldquot.header/g' $(srcdir)/insert-header.sin > en@boldquot.insert-header

mostlyclean: mostlyclean-quot
mostlyclean-quot:
	rm -f *.insert-header
