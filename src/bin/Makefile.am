SUBDIRS = e_fm

MAINTAINERCLEANFILES = Makefile.in
DISTCLEANFILES = e_fm_shared_types.h

AM_CPPFLAGS = \
-I$(top_builddir) \
-I$(top_builddir)/src/bin \
-I$(top_srcdir) \
-I$(top_srcdir)/src/bin \
@e_cflags@ \
@cf_cflags@ \
@VALGRIND_CFLAGS@ \
@EDJE_DEF@ \
@ELM_CFLAGS@ \
@EMOTION_CFLAGS@ \
-DE_BINDIR=\"$(bindir)\" \
-DPACKAGE_BIN_DIR=\"@PACKAGE_BIN_DIR@\" \
-DPACKAGE_LIB_DIR=\"@PACKAGE_LIB_DIR@\" \
-DPACKAGE_DATA_DIR=\"@PACKAGE_DATA_DIR@\" \
-DLOCALE_DIR=\"@LOCALE_DIR@\" \
-DPACKAGE_SYSCONF_DIR=\"@PACKAGE_SYSCONF_DIR@\"

bin_PROGRAMS = \
enlightenment \
enlightenment_imc \
enlightenment_start \
enlightenment_open

internal_bindir = $(libdir)/enlightenment/utils
internal_bin_PROGRAMS = \
enlightenment_fm_op \
enlightenment_init \
enlightenment_sys \
enlightenment_thumb \
enlightenment_alert \
enlightenment_static_grabber

if HAVE_EEZE
internal_bin_PROGRAMS += enlightenment_backlight
endif

ENLIGHTENMENTHEADERS = \
e_about.h \
e_acpi.h \
e_actions.h \
e_alert.h \
e_atoms.h \
e_backlight.h \
e_bg.h \
e_bindings.h \
e_bodhi_about.h \
e_border.h \
e_box.h \
e_canvas.h \
e_color_class.h \
e_color_dialog.h  \
e_color.h \
e_config_data.h \
e_config_dialog.h \
e_config.h \
e_configure.h \
e_confirm_dialog.h \
e_container.h \
e_datastore.h \
e_desk.h \
e_deskenv.h \
e_desklock.h \
e_dialog.h \
e_dnd.h \
e_dpms.h \
e_eap_editor.h \
e_entry.h \
e_entry_dialog.h \
e_env.h \
e_error.h \
e_exec.h \
e_exehist.h \
e_filereg.h \
e_flowlayout.h \
e_fm_custom.h \
e_fm_device.h \
e_fm.h \
e_fm_mime.h \
e_fm_op.h \
e_fm_op_registry.h \
e_fm_prop.h \
e_fm_shared_codec.h \
e_fm_shared_device.h \
e_fm_shared_types.h \
e_focus.h \
e_font.h \
e_gadcon.h \
e_gadcon_popup.h \
e_grabinput.h \
e_grab_dialog.h \
e.h \
e_hints.h \
e_icon.h \
e_ilist.h \
e_import_config_dialog.h \
e_import_dialog.h \
e_includes.h \
e_init.h \
e_int_border_locks.h \
e_int_border_menu.h \
e_int_border_prop.h \
e_int_border_remember.h \
e_int_config_modules.h \
e_int_gadcon_config.h \
e_intl_data.h \
e_intl.h \
e_int_menus.h \
e_int_shelf_config.h \
e_int_toolbar_config.h \
e_ipc_codec.h \
e_ipc.h \
e_macros.h \
e_layout.h \
e_livethumb.h \
e_log.h \
e_manager.h \
e_maximize.h \
e_menu.h \
e_mmx.h \
e_module.h \
e_mouse.h \
e_moveresize.h \
e_msgbus.h \
e_msg.h \
e_notification.h \
e_obj_dialog.h \
e_object.h \
e_order.h \
e_pan.h \
e_path.h \
e_place.h \
e_pointer.h \
e_popup.h \
e_powersave.h \
e_prefix.h \
e_randr.h \
e_remember.h \
e_resist.h \
e_scale.h \
e_screensaver.h \
e_scrollframe.h \
e_sha1.h \
e_shelf.h \
e_signals.h \
e_slidecore.h \
e_slider.h \
e_slidesel.h \
e_spectrum.h \
e_startup.h \
e_stolen.h \
e_sys.h \
e_table.h \
e_test.h \
e_theme_about.h \
e_theme.h \
e_thumb.h \
e_toolbar.h \
e_update.h \
e_user.h \
e_utils.h \
e_widget_aspect.h \
e_widget_button.h \
e_widget_check.h \
e_widget_color_well.h \
e_widget_config_list.h \
e_widget_csel.h \
e_widget_cslider.h \
e_widget_deskpreview.h \
e_widget_entry.h \
e_widget_filepreview.h \
e_widget_flist.h \
e_widget_font_preview.h \
e_widget_framelist.h \
e_widget_frametable.h \
e_widget_fsel.h \
e_widget.h \
e_widget_ilist.h \
e_widget_image.h \
e_widget_label.h \
e_widget_list.h \
e_widget_preview.h \
e_widget_radio.h \
e_widget_scrollframe.h \
e_widget_slider.h \
e_widget_spectrum.h \
e_widget_table.h \
e_widget_textblock.h \
e_widget_toolbar.h \
e_widget_toolbook.h \
e_win.h \
e_xinerama.h \
e_xkb.h \
e_xsettings.h \
e_zone.h


enlightenment_src = \
e_about.c \
e_acpi.c \
e_alert.c \
e_actions.c \
e_atoms.c \
e_backlight.c \
e_bg.c \
e_bindings.c \
e_bodhi_about.c \
e_border.c \
e_box.c \
e_canvas.c \
e_color.c \
e_color_class.c \
e_color_dialog.c \
e_config.c \
e_config_data.c \
e_config_dialog.c \
e_configure.c \
e_confirm_dialog.c \
e_container.c \
e_datastore.c \
e_desk.c \
e_deskenv.c \
e_desklock.c \
e_dialog.c \
e_dnd.c \
e_dpms.c \
e_eap_editor.c \
e_entry.c \
e_entry_dialog.c \
e_env.c \
e_error.c \
e_exec.c \
e_exehist.c \
e_filereg.c \
e_flowlayout.c \
e_fm.c \
e_fm_custom.c \
e_fm_device.c \
e_fm_mime.c \
e_fm_op_registry.c \
e_fm_prop.c \
e_fm_shared_codec.c \
e_fm_shared_device.c \
e_focus.c \
e_font.c \
e_gadcon.c \
e_gadcon_popup.c \
e_grabinput.c \
e_grab_dialog.c \
e_hints.c \
e_icon.c \
e_ilist.c \
e_import_config_dialog.c \
e_import_dialog.c \
e_init.c \
e_int_border_locks.c \
e_int_border_menu.c \
e_int_border_prop.c \
e_int_border_remember.c \
e_int_config_modules.c \
e_int_gadcon_config.c \
e_intl.c \
e_intl_data.c \
e_int_menus.c \
e_int_shelf_config.c \
e_int_toolbar_config.c \
e_ipc.c \
e_ipc_codec.c \
e_layout.c \
e_livethumb.c \
e_log.c \
e_manager.c \
e_maximize.c \
e_menu.c \
e_module.c \
e_mouse.c \
e_moveresize.c \
e_msgbus.c \
e_msg.c \
e_notification.c \
e_obj_dialog.c \
e_object.c \
e_order.c \
e_pan.c \
e_path.c \
e_place.c \
e_pointer.c \
e_popup.c \
e_powersave.c \
e_prefix.c \
e_randr.c \
e_remember.c \
e_resist.c \
e_scale.c \
e_screensaver.c \
e_scrollframe.c \
e_sha1.c \
e_shelf.c \
e_signals.c \
e_slidecore.c \
e_slider.c \
e_slidesel.c \
e_spectrum.c \
e_startup.c \
e_stolen.c \
e_sys.c \
e_table.c \
e_test.c \
e_theme_about.c \
e_theme.c \
e_thumb.c \
e_toolbar.c \
e_update.c \
e_user.c \
e_utils.c \
e_widget_aspect.c \
e_widget_button.c \
e_widget.c \
e_widget_check.c \
e_widget_color_well.c \
e_widget_config_list.c \
e_widget_csel.c \
e_widget_cslider.c \
e_widget_deskpreview.c \
e_widget_entry.c \
e_widget_filepreview.c \
e_widget_flist.c \
e_widget_font_preview.c \
e_widget_framelist.c \
e_widget_frametable.c \
e_widget_fsel.c \
e_widget_ilist.c \
e_widget_image.c \
e_widget_label.c \
e_widget_list.c \
e_widget_preview.c \
e_widget_radio.c \
e_widget_scrollframe.c \
e_widget_slider.c \
e_widget_spectrum.c \
e_widget_table.c \
e_widget_textblock.c \
e_widget_toolbar.c \
e_widget_toolbook.c \
e_win.c \
e_xinerama.c \
e_xkb.c \
e_xsettings.c \
e_zone.c \
$(ENLIGHTENMENTHEADERS)

enlightenment_CPPFLAGS = $(AM_CPPFLAGS) -DE_LOGGING=1
enlightenment_SOURCES = \
e_main.c \
$(enlightenment_src)

enlightenment_LDFLAGS = -export-dynamic
enlightenment_LDADD = @e_libs@ @ELM_LIBS@ @EMOTION_LIBS@ @dlopen_libs@ @cf_libs@ @VALGRIND_LIBS@ -lm

enlightenment_imc_SOURCES = \
e.h \
e_config_data.c \
e_imc_main.c \
e_intl_data.c

enlightenment_imc_LDADD = @E_IMC_LIBS@

enlightenment_start_SOURCES = \
e_start_main.c
enlightenment_start_CFLAGS = $(AM_CPPFLAGS) @E_START_CFLAGS@
enlightenment_start_LDADD = @dlopen_libs@ @E_START_LIBS@

enlightenment_thumb_SOURCES = \
e_sha1.c \
e_thumb_main.c \
e_user.c

enlightenment_thumb_LDADD = @E_THUMB_LIBS@

enlightenment_fm_op_SOURCES = \
e_fm_op.c

enlightenment_fm_op_LDADD = @E_FM_OP_LIBS@ -lm

enlightenment_sys_SOURCES = \
e_sys_main.c

enlightenment_sys_LDADD = @SUID_LDFLAGS@ @E_SYS_LIBS@
enlightenment_sys_CFLAGS = @SUID_CFLAGS@ @E_SYS_CFLAGS@

if HAVE_EEZE
enlightenment_backlight_SOURCES = \
e_backlight_main.c

enlightenment_backlight_CFLAGS = @SUID_CFLAGS@ @EEZE_CFLAGS@
enlightenment_backlight_LDADD = @SUID_LDFLAGS@ @EEZE_LIBS@
endif

enlightenment_init_SOURCES = \
e_init_main.c \
e_xinerama.c

enlightenment_init_LDADD = @E_INIT_LIBS@

enlightenment_alert_SOURCES = \
e_alert_main.c

enlightenment_alert_LDADD = @E_ALERT_LIBS@
enlightenment_alert_CFLAGS = @E_ALERT_CFLAGS@

enlightenment_open_SOURCES = e_open.c
enlightenment_open_LDADD = @E_OPEN_LIBS@
enlightenment_open_CFLAGS = @E_OPEN_CFLAGS@

enlightenment_static_grabber_SOURCES = e_static_grab.c
enlightenment_static_grabber_LDADD = @E_GRABBER_LIBS@
enlightenment_static_grabber_CFLAGS = @E_GRABBER_CFLAGS@

# HACK! why install-data-hook? install-exec-hook is run after bin_PROGRAMS
# and before internal_bin_PROGRAMS are installed. install-data-hook is
# run after both
setuid_root_mode = a=rx,u+xs
if HAVE_EEZE
install-data-hook:
	@chmod $(setuid_root_mode) $(DESTDIR)$(libdir)/enlightenment/utils/enlightenment_sys$(EXEEXT) || true
	@chmod $(setuid_root_mode) $(DESTDIR)$(libdir)/enlightenment/utils/enlightenment_backlight$(EXEEXT) || true
else
install-data-hook:
	@chmod $(setuid_root_mode) $(DESTDIR)$(libdir)/enlightenment/utils/enlightenment_sys$(EXEEXT) || true
endif
installed_headersdir = $(prefix)/include/enlightenment
installed_headers_DATA = $(ENLIGHTENMENTHEADERS)
