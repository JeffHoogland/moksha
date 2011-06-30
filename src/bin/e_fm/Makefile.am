MAINTAINERCLEANFILES = Makefile.in

AM_CFLAGS = \
-I$(top_builddir) \
-I$(top_builddir)/src/bin \
-I$(top_srcdir) \
-I$(top_srcdir)/src/bin \
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

if HAVE_HAL_MOUNT
hal_s = \
e_fm_main_hal.h \
e_fm_main_hal.c
else
hal_s =
endif

if HAVE_UDISKS_MOUNT
udisks_s = \
e_fm_main_udisks.h \
e_fm_main_udisks.c
else
udisks_s =
endif

if HAVE_EEZE_MOUNT
AM_CFLAGS += @EET_CFLAGS@
LIBS += @EET_LIBS@
eeze_s = \
../e_prefix.c \
e_fm_main_eeze.h \
e_fm_main_eeze.c
else
eeze_s =
endif

enlightenment_fm_SOURCES = \
e_fm_main.h \
e_fm_ipc.h \
e_fm_main.c \
e_fm_ipc.c \
$(hal_s) \
$(udisks_s) \
$(eeze_s) \
../e_fm_shared_codec.c \
../e_fm_shared_device.c \
../e_user.c \
../e_sha1.c

enlightenment_fm_LDADD = @E_FM_LIBS@

