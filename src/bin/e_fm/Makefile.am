MAINTAINERCLEANFILES = Makefile.in

INCLUDES = -I$(top_builddir) \
           -I$(top_srcdir) \
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

internal_bindir = $(libdir)/enlightenment/utils
internal_bin_PROGRAMS = \
enlightenment_fm 

enlightenment_fm_SOURCES = \
e_fm_main.c \
e_fm_main_dbus.c \
e_fm_ipc.c \
../e_fm_shared_codec.c \
../e_fm_shared_dbus.c \
../e_user.c \
../e_sha1.c

enlightenment_fm_LDADD = @E_FM_LIBS@
enlightenment_fm_CFLAGS = $(INCLUDES)


