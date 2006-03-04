MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(top_srcdir) \
	   -I$(top_srcdir)/src/bin \
	   -I$(top_srcdir)/src/lib \
	   @e_cflags@ \
	   @x_cflags@ \
	   @cf_cflags@ \
	   @VALGRIND_CFLAGS@ \
	   @EDJE_DEF@

bin_PROGRAMS = \
enlightenment \
enlightenment_remote \
enlightenment_eapp \
enlightenment_eapp_cache_gen \
enlightenment_imc

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
e_int_menus.h \
e_module.h \
e_apps.h \
e_atoms.h \
e_utils.h \
e_canvas.h \
e_focus.h \
e_place.h \
e_resist.h \
e_startup.h \
e_hints.h \
e_gadman.h \
e_gadget.h \
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
e_apps_cache.h \
e_entry.h \
e_fileman.h \
e_fileman_smart.h \
e_fileman_file.h \
e_fileman_icon.h \
e_fileman_mime.h \
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
e_icon_grid.h \
e_icon_canvas.h \
e_int_config_desks.h \
e_configure.h \
e_int_border_locks.h \
e_thumb.h \
e_int_border_remember.h \
e_eap_editor.h \
e_widget_iconsel.h \
e_widget_fileman.h \
e_scrollframe.h \
e_file_selector.h \
e_file_dialog.h \
e_int_border_menu.h \
e_ilist.h \
e_tlist.h \
e_livethumb.h \
e_int_border_border.h \
e_widget_ilist.h \
e_widget_tlist.h \
e_slider.h \
e_widget_slider.h \
e_int_config_window_manipulation.h \
e_int_config_window_display.h \
e_int_config_background.h \
e_int_config_background_import.h \
e_int_config_theme.h \
e_int_config_menus.h \
e_int_config_keybindings.h \
e_int_config_cursor.h \
e_int_config_startup.h \
e_int_config_performance.h \
e_int_config_winlist.h \
e_int_config_display.h \
e_int_config_desklock.h \
e_int_config_exebuf.h \
e_int_config_cfgdialogs.h \
e_int_config_hinting.h \
e_deskpreview.h \
e_exebuf.h \
e_desklock.h \
e_int_config_modules.h \
e_exehist.h \
e_color_class.h \
e_widget_textblock.h \
e_apps_error.h \
e_stolen.h \
e_gadcon.h \
e_shelf.h \
e_widget_preview.h

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
e_int_menus.c \
e_module.c \
e_apps.c \
e_atoms.c \
e_utils.c \
e_canvas.c \
e_focus.c \
e_place.c \
e_resist.c \
e_startup.c \
e_hints.c \
e_gadman.c \
e_gadget.c \
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
e_apps_cache.c \
e_entry.c \
e_fileman.c \
e_fileman_smart.c \
e_fileman_file.c \
e_fileman_icon.c \
e_fileman_mime.c \
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
e_icon_grid.c \
e_icon_canvas.c \
e_int_config_desks.c \
e_configure.c \
e_int_border_locks.c \
e_thumb.c \
e_int_border_remember.c \
e_eap_editor.c \
e_widget_iconsel.c \
e_widget_fileman.c \
e_scrollframe.c \
e_file_selector.c \
e_file_dialog.c \
e_int_border_menu.c \
e_ilist.c \
e_tlist.c \
e_livethumb.c \
e_int_border_border.c \
e_widget_ilist.c \
e_widget_tlist.c \
e_slider.c \
e_widget_slider.c \
e_int_config_window_manipulation.c \
e_int_config_window_display.c \
e_int_config_background.c \
e_int_config_background_import.c \
e_int_config_theme.c \
e_int_config_menus.c \
e_int_config_keybindings.c \
e_int_config_cursor.c \
e_int_config_startup.c \
e_int_config_performance.c \
e_int_config_winlist.c \
e_int_config_display.c \
e_int_config_desklock.c \
e_int_config_exebuf.c \
e_int_config_cfgdialogs.c \
e_int_config_hinting.c \
e_deskpreview.c \
e_exebuf.c \
e_desklock.c \
e_int_config_modules.c \
e_exehist.c \
e_color_class.c \
e_widget_textblock.c \
e_apps_error.c \
e_stolen.c \
e_gadcon.c \
e_shelf.c \
e_widget_preview.c \
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

enlightenment_eapp_SOURCES = \
e.h \
e_eapp_main.c

enlightenment_eapp_LDFLAGS = @e_libs@ @dlopen_libs@

enlightenment_eapp_cache_gen_SOURCES = \
e_eapp_cache_gen_main.c \
$(enlightenment_src)

enlightenment_eapp_cache_gen_LDFLAGS = @e_libs@ @dlopen_libs@

enlightenment_imc_SOURCES = \
e.h \
e_imc_main.c \
$(enlightenment_src)

enlightenment_imc_LDFLAGS = @e_libs@ @dlopen_libs@

installed_headersdir = $(prefix)/include/enlightenment
installed_headers_DATA = $(ENLIGHTENMENTHEADERS)

