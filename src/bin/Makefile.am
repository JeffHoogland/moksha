MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I$(includedir) \
	   -I$(top_srcdir) \
	   -I$(top_srcdir)/src/bin \
	   -I$(top_srcdir)/src/lib \
	   @e_cflags@ \
	   @EDJE_DEF@

bin_PROGRAMS = enlightenment enlightenment_remote enlightenment_eapp

enlightenment_SOURCES = \
e.h \
e_file.h \
e_file.c \
e_main.c \
e_user.h \
e_user.c \
e_manager.h \
e_manager.c \
e_path.h \
e_path.c \
e_init.h \
e_init.c \
e_ipc.h \
e_ipc.c \
e_error.h \
e_error.c \
e_container.h \
e_container.c \
e_desk.h \
e_desk.c \
e_border.h \
e_border.c \
e_pointer.h \
e_pointer.c \
e_config.h \
e_config.c \
e_menu.h \
e_menu.c \
e_object.h \
e_object.c \
e_icon.h \
e_icon.c \
e_box.h \
e_box.c \
e_int_menus.h \
e_int_menus.c \
e_module.h \
e_module.c \
e_apps.h \
e_apps.c \
e_atoms.h \
e_atoms.c \
e_utils.h \
e_utils.c \
e_canvas.h \
e_canvas.c \
e_focus.h \
e_focus.c \
e_place.h \
e_place.c \
e_resist.h \
e_resist.c \
e_startup.h \
e_startup.c

enlightenment_LDFLAGS = -export-dynamic @e_libs@ @dlopen_libs@

enlightenment_remote_SOURCES = \
e.h \
e_remote_main.c

enlightenment_remote_LDFLAGS = @e_libs@ @dlopen_libs@

enlightenment_eapp_SOURCES = \
e.h \
e_eapp_main.c

enlightenment_eapp_LDFLAGS = @e_libs@ @dlopen_libs@
