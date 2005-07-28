MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(includedir) \
	   -I$(top_srcdir) \
	   -I$(top_srcdir)/src/bin \
	   -I$(top_srcdir)/src/lib \
	   @e_cflags@ \
	   @x_cflags@ \
	   @EDJE_DEF@

bin_PROGRAMS = enlightenment enlightenment_remote enlightenment_eapp

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
e_dialog.h

enlightenment_SOURCES = \
e_main.c \
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
$(ENLIGHTENMENTHEADERS)

enlightenment_LDFLAGS = -export-dynamic @e_libs@ @x_libs@ @dlopen_libs@

enlightenment_remote_SOURCES = \
e.h \
e_ipc_codec.c \
e_remote_main.c

enlightenment_remote_LDFLAGS = @e_libs@ @dlopen_libs@

enlightenment_eapp_SOURCES = \
e.h \
e_eapp_main.c

enlightenment_eapp_LDFLAGS = @e_libs@ @dlopen_libs@

installed_headersdir = $(prefix)/include/enlightenment
installed_headers_DATA = $(ENLIGHTENMENTHEADERS)

