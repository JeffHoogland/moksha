MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(top_srcdir) \
	   -I$(top_srcdir)/src/bin \
	   -I$(top_srcdir)/src/lib \
	   @e_cflags@ \
	   @x_cflags@ \
	   @cf_cflags@ \
	   @VALGRIND_CFLAGS@ \
	   @EDJE_DEF@ \
	   -DPACKAGE_BIN_DIR=\"@PACKAGE_BIN_DIR@\" \
	   -DPACKAGE_LIB_DIR=\"@PACKAGE_LIB_DIR@\" \
	   -DPACKAGE_DATA_DIR=\"@PACKAGE_DATA_DIR@\" \
	   -DLOCALE_DIR=\"@LOCALE_DIR@\" \
	   -DPACKAGE_SYSCONF_DIR=\"@PACKAGE_SYSCONF_DIR@\"

bin_PROGRAMS = \
enlightenment \
enlightenment_remote \
enlightenment_imc \
enlightenment_start \
enlightenment_thumb \
enlightenment_sys \
enlightenment_fm

ENLIGHTENMENTHEADERS = \
e.h \
e_includes.h \
e_mmx.h \
e_ipc_handlers.h \
e_ipc_handlers_list.h \
e_user.h \
e_manager.h \
e_path.h \
e_init.h \
e_ipc.h \
e_error.h \
e_container.h \
e_zone.h \
e_desk.h \
e_border.h \
e_pointer.h \
e_config.h \
e_menu.h \
e_object.h \
e_icon.h \
e_box.h \
e_flowlayout.h \
e_int_menus.h \
e_module.h \
e_atoms.h \
e_utils.h \
e_canvas.h \
e_focus.h \
e_place.h \
e_resist.h \
e_startup.h \
e_hints.h \
e_signals.h \
e_xinerama.h \
e_table.h \
e_layout.h \
e_test.h \
e_font.h \
e_intl.h \
e_theme.h \
e_dnd.h \
e_bindings.h \
e_moveresize.h \
e_actions.h \
e_popup.h \
e_ipc_codec.h \
e_prefix.h \
e_datastore.h \
e_msg.h \
e_winlist.h \
e_alert.h \
e_maximize.h \
e_grabinput.h \
e_bg.h \
e_remember.h \
e_win.h \
e_pan.h \
e_dialog.h \
e_about.h \
e_theme_about.h \
e_editable.h \
e_entry.h \
e_widget.h \
e_widget_check.h \
e_widget_radio.h \
e_widget_framelist.h \
e_widget_list.h \
e_widget_button.h \
e_widget_label.h \
e_widget_frametable.h \
e_widget_table.h \
e_widget_entry.h \
e_widget_image.h \
e_config_dialog.h \
e_int_config_focus.h \
e_int_config_desks.h \
e_int_config_intl.h \
e_int_config_imc.h \
e_int_config_imc_import.h \
e_configure.h \
e_int_border_locks.h \
e_thumb.h \
e_int_border_remember.h \
e_eap_editor.h \
e_scrollframe.h \
e_int_border_menu.h \
e_ilist.h \
e_tlist.h \
e_livethumb.h \
e_widget_ilist.h \
e_widget_tlist.h \
e_widget_config_list.h \
e_slider.h \
e_widget_slider.h \
e_int_config_window_manipulation.h \
e_int_config_window_display.h \
e_int_config_theme.h \
e_int_config_theme_import.h \
e_int_config_icon_themes.h \
e_int_config_menus.h \
e_int_config_keybindings.h \
e_int_config_mousebindings.h \
e_int_config_cursor.h \
e_int_config_fonts.h \
e_int_config_startup.h \
e_int_config_performance.h \
e_int_config_winlist.h \
e_int_config_display.h \
e_int_config_desklock.h \
e_int_config_exebuf.h \
e_int_config_apps_menu.h \
e_int_config_apps_order.h \
e_int_config_cfgdialogs.h \
e_int_config_color_classes.h \
e_int_config_mime.h \
e_int_config_mime_edit.h \
e_int_config_screensaver.h \
e_int_config_dpms.h \
e_exebuf.h \
e_desklock.h \
e_screensaver.h \
e_dpms.h \
e_int_config_modules.h \
e_exehist.h \
e_color_class.h \
e_widget_textblock.h \
e_stolen.h \
e_gadcon.h \
e_shelf.h \
e_widget_preview.h \
e_int_config_paths.h \
e_int_shelf_config.h \
e_int_config_shelf.h \
e_int_gadcon_config.h \
e_confirm_dialog.h \
e_int_border_prop.h \
e_entry_dialog.h \
e_fm.h \
e_widget_scrollframe.h \
e_sha1.h \
e_widget_fsel.h \
e_fm_mime.h \
e_color.h \
e_spectrum.h \
e_widget_spectrum.h \
e_widget_cslider.h \
e_widget_csel.h \
e_widget_color_well.h \
e_int_config_wallpaper.h \
e_int_config_wallpaper_import.h \
e_int_config_wallpaper_gradient.h \
e_color_dialog.h  \
e_sys.h \
e_obj_dialog.h \
e_int_config_transitions.h \
e_fwin.h \
e_widget_aspect.h \
e_filereg.h \
e_widget_desk_preview.h \
e_int_config_borders.h \
e_int_config_desk.h \
e_int_config_clientlist.h \
e_fm_prop.h \
e_int_config_mouse.h \
e_mouse.h \
e_order.h \
e_exec.h \
e_widget_font_preview.h
 
enlightenment_src = \
e_user.c \
e_manager.c \
e_path.c \
e_init.c \
e_ipc.c \
e_error.c \
e_container.c \
e_zone.c \
e_desk.c \
e_border.c \
e_pointer.c \
e_config.c \
e_menu.c \
e_object.c \
e_icon.c \
e_box.c \
e_flowlayout.c \
e_int_menus.c \
e_module.c \
e_atoms.c \
e_utils.c \
e_canvas.c \
e_focus.c \
e_place.c \
e_resist.c \
e_startup.c \
e_hints.c \
e_signals.c \
e_xinerama.c \
e_table.c \
e_layout.c \
e_test.c \
e_font.c \
e_intl.c \
e_theme.c \
e_dnd.c \
e_bindings.c \
e_moveresize.c \
e_actions.c \
e_popup.c \
e_ipc_codec.c \
e_prefix.c \
e_datastore.c \
e_msg.c \
e_winlist.c \
e_alert.c \
e_maximize.c \
e_grabinput.c \
e_bg.c \
e_remember.c \
e_win.c \
e_pan.c \
e_dialog.c \
e_about.c \
e_theme_about.c \
e_editable.c \
e_entry.c \
e_widget.c \
e_widget_check.c \
e_widget_radio.c \
e_widget_framelist.c \
e_widget_list.c \
e_widget_button.c \
e_widget_label.c \
e_widget_frametable.c \
e_widget_table.c \
e_widget_entry.c \
e_widget_image.c \
e_config_dialog.c \
e_int_config_focus.c \
e_int_config_desks.c \
e_configure.c \
e_int_border_locks.c \
e_thumb.c \
e_int_border_remember.c \
e_eap_editor.c \
e_scrollframe.c \
e_int_border_menu.c \
e_ilist.c \
e_tlist.c \
e_livethumb.c \
e_widget_ilist.c \
e_widget_tlist.c \
e_widget_config_list.c \
e_slider.c \
e_widget_slider.c \
e_int_config_window_manipulation.c \
e_int_config_window_display.c \
e_int_config_theme.c \
e_int_config_theme_import.c \
e_int_config_icon_themes.c \
e_int_config_menus.c \
e_int_config_keybindings.c \
e_int_config_mousebindings.c \
e_int_config_cursor.c \
e_int_config_fonts.c \
e_int_config_startup.c \
e_int_config_performance.c \
e_int_config_winlist.c \
e_int_config_display.c \
e_int_config_desklock.c \
e_int_config_exebuf.c \
e_int_config_apps_menu.c \
e_int_config_apps_order.c \
e_int_config_cfgdialogs.c \
e_int_config_intl.c \
e_int_config_imc.c \
e_int_config_imc_import.c \
e_int_config_color_classes.c \
e_int_config_mime.c \
e_int_config_mime_edit.c \
e_int_config_screensaver.c \
e_int_config_dpms.c \
e_exebuf.c \
e_desklock.c \
e_screensaver.c \
e_dpms.c \
e_int_config_modules.c \
e_exehist.c \
e_color_class.c \
e_widget_textblock.c \
e_stolen.c \
e_gadcon.c \
e_shelf.c \
e_widget_preview.c \
e_int_config_paths.c \
e_int_shelf_config.c \
e_int_config_shelf.c \
e_int_gadcon_config.c \
e_confirm_dialog.c \
e_int_border_prop.c \
e_entry_dialog.c \
e_fm.c \
e_widget_scrollframe.c \
e_sha1.c \
e_widget_fsel.c \
e_fm_mime.c \
e_color.c \
e_spectrum.c \
e_widget_spectrum.c \
e_widget_cslider.c \
e_widget_csel.c \
e_widget_color_well.c \
e_int_config_wallpaper.c \
e_int_config_wallpaper_import.c \
e_int_config_wallpaper_gradient.c \
e_color_dialog.c \
e_sys.c \
e_int_config_transitions.c \
e_obj_dialog.c \
e_fwin.c \
e_widget_aspect.c \
e_widget_desk_preview.c \
e_filereg.c \
e_int_config_borders.c \
e_int_config_desk.c \
e_fm_prop.c \
e_int_config_clientlist.c \
e_int_config_mouse.c \
e_mouse.c \
e_order.c \
e_exec.c \
e_widget_font_preview.c \
$(ENLIGHTENMENTHEADERS)

enlightenment_SOURCES = \
e_main.c \
$(enlightenment_src)

enlightenment_LDFLAGS = -export-dynamic @e_libs@ @x_libs@ @dlopen_libs@ @cf_libs@ @VALGRIND_LIBS@

enlightenment_remote_SOURCES = \
e.h \
e_ipc_codec.c \
e_remote_main.c

enlightenment_remote_LDFLAGS = @e_libs@ @dlopen_libs@

enlightenment_imc_SOURCES = \
e.h \
e_imc_main.c \
$(enlightenment_src)

enlightenment_imc_LDFLAGS = @e_libs@ @dlopen_libs@

enlightenment_start_SOURCES = \
e_start_main.c

enlightenment_start_LDFLAGS = @dlopen_libs@

enlightenment_thumb_SOURCES = \
e_thumb_main.c \
e_user.c \
e_sha1.c

enlightenment_thumb_LDFLAGS = @e_libs@ @dlopen_libs@

enlightenment_fm_SOURCES = \
e_fm_main.c \
e_user.c \
e_sha1.c

enlightenment_fm_LDFLAGS = @e_libs@ @dlopen_libs@

enlightenment_sys_SOURCES = \
e_sys_main.c

enlightenment_sys_LDFLAGS = @e_libs@ @dlopen_libs@

setuid_root_mode = a=rx,u+xs
install-exec-hook:
	@chmod $(setuid_root_mode) $(DESTDIR)$(bindir)/enlightenment_sys$(EXEEXT) || true

installed_headersdir = $(prefix)/include/enlightenment
installed_headers_DATA = $(ENLIGHTENMENTHEADERS)

