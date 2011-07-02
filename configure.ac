##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_define([v_maj], [0])
m4_define([v_min], [16])
m4_define([v_mic], [999])
m4_define([v_rev], m4_esyscmd([(svnversion "${SVN_REPO_PATH:-.}" | grep -v export || echo 0) | awk -F : '{printf("%s\n", $1);}' | tr -d ' :MSP\n']))
m4_if(v_rev, [0], [m4_define([v_rev], m4_esyscmd([git log 2> /dev/null | (grep -m1 git-svn-id || echo 0) | sed -e 's/.*@\([0-9]*\).*/\1/' | tr -d '\n']))])
##--   When released, remove the dnl on the below line
dnl m4_undefine([v_rev])
##--   When doing snapshots - change soname. remove dnl on below line
m4_define([relname], [ver-pre-svn-08])
m4_define([v_rel], [-release relname])
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_ifdef([v_rev], [m4_define([v_ver], [v_maj.v_min.v_mic.v_rev])], [m4_define([v_ver], [v_maj.v_min.v_mic])])
m4_define([lt_rev], m4_eval(v_maj + v_min))
m4_define([lt_cur], v_mic)
m4_define([lt_age], v_min)
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##

AC_INIT([enlightenment], [v_ver], [enlightenment-devel@lists.sourceforge.net])
AC_PREREQ([2.52])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_ISC_POSIX

AC_CONFIG_HEADERS([config.h])
AH_TOP([
#ifndef EFL_CONFIG_H__
#define EFL_CONFIG_H__
])
AH_BOTTOM([
#endif /* EFL_CONFIG_H__ */
])

AM_INIT_AUTOMAKE([1.6 dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_GNU_SOURCE
AC_C_BIGENDIAN
AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_C_CONST
AC_FUNC_ALLOCA
AC_C___ATTRIBUTE__

define([AC_LIBTOOL_LANG_CXX_CONFIG], [:])
define([AC_LIBTOOL_LANG_GCJ_CONFIG], [:])
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_ifdef([v_rev], , [m4_define([v_rev], [0])])
m4_ifdef([v_rel], , [m4_define([v_rel], [])])
AC_DEFINE_UNQUOTED(VMAJ, [v_maj], [Major version])
AC_DEFINE_UNQUOTED(VMIN, [v_min], [Minor version])
AC_DEFINE_UNQUOTED(VMIC, [v_mic], [Micro version])
AC_DEFINE_UNQUOTED(VREV, [v_rev], [Revison])
version_info="lt_rev:lt_cur:lt_age"
release_info="v_rel"
AC_SUBST(version_info)
AC_SUBST(release_info)
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##

release=relname
AC_SUBST(release)

PKG_PROG_PKG_CONFIG

EFL_CHECK_PATH_MAX

AC_CHECK_FUNCS(setenv)
AC_CHECK_FUNCS(unsetenv)

AC_CHECK_HEADERS(X11/extensions/shape.h,, AC_MSG_ERROR([Cannot find X11/extensions/shape.h. Make sure your CFLAGS environment variable contains include lines for the location of this file]))

AC_CHECK_HEADERS(fnmatch.h,, AC_MSG_ERROR([Cannot find fnmatch.h. Make sure your CFLAGS environment variable contains include lines for the location of this file]))

AC_CHECK_FUNCS(fnmatch, res=yes, res=no)
if test "x$res" = "xno"; then
  AC_CHECK_LIB(fnmatch, fnmatch, res=yes, res=no)
  if test "x$res" = "xno"; then
    AC_MSG_ERROR([Cannot find fnmatch() in neither libc nor libfnmatch])
  else
    fnmatch_libs="-lfnmatch"
  fi
fi

AC_SUBST(fnmatch_libs)

have_pam=no
AC_ARG_ENABLE(pam,
  AC_HELP_STRING([--enable-pam], [enable PAM support @<:@default=detect@:>@]),
  [want_pam=$enableval],
  [want_pam=auto]
)
if test "x$want_pam" != "xno" ; then
  AC_CHECK_HEADERS(security/pam_appl.h, [
    LIBS="$LIBS -lpam"
    have_pam=yes
    AC_DEFINE(HAVE_PAM, 1, [PAM Authentication Support])])
    if test "x$want_pam$have_pam" = "xyesno" ; then
      AC_MSG_ERROR([PAM support was requested but not found])
    fi
fi

dnl AC_E_CHECK_PKG(VALGRIND, [valgrind >= 2.4.0], [], [:])
AC_SUBST(VALGRIND_CFLAGS)
AC_SUBST(VALGRIND_LIBS)

MODULE_ARCH="$host_os-$host_cpu-relname"
AC_SUBST(MODULE_ARCH)
AC_DEFINE_UNQUOTED(MODULE_ARCH, "$MODULE_ARCH", "Module architecture")

if test "x${bindir}" = 'xNONE'; then
  if test "x${prefix}" = "xNONE"; then
    PACKAGE_BIN_DIR="${ac_default_prefix}/bin"
  else
    PACKAGE_BIN_DIR="${prefix}/bin"
  fi
else
  PACKAGE_BIN_DIR="${bindir}"
fi
AC_SUBST(PACKAGE_BIN_DIR)

if test "x${libdir}" = 'xNONE'; then
  if test "x${prefix}" = "xNONE"; then
    PACKAGE_LIB_DIR="${ac_default_prefix}/lib"
  else
    PACKAGE_LIB_DIR="${prefix}/lib"
  fi
else
  PACKAGE_LIB_DIR="${libdir}"
fi
AC_SUBST(PACKAGE_LIB_DIR)

if test "x${prefix}" = "xNONE"; then
  PACKAGE_DATA_DIR="${ac_default_prefix}/share/${PACKAGE}"
  LOCALE_DIR="${ac_default_prefix}/share/locale"
  PACKAGE_SYSCONF_DIR="${ac_default_prefix}/etc"
else
  PACKAGE_DATA_DIR="${prefix}/share/${PACKAGE}"
  LOCALE_DIR="${prefix}/share/locale"
  PACKAGE_SYSCONF_DIR="${sysconfdir}"
fi
AC_SUBST(PACKAGE_DATA_DIR)
AC_SUBST(LOCALE_DIR)
AC_SUBST(PACKAGE_SYSCONF_DIR)

#######################################
## Simple X11 build/link

AC_ARG_ENABLE(simple-x11,
  AC_HELP_STRING([--enable-simple-x11], [enable simple x11 linking]),
  [ want_evas_simple_x11=$enableval ]
)
    
x_dir=""
x_cflags=""
x_libs=""
AC_PATH_XTRA
AC_CHECK_HEADER(X11/X.h,
  [
    if test "x$want_evas_simple_x11" = "xyes"; then
      x_libs="${x_libs} -lX11 -lXext"
    else
      x_dir=${x_dir:-/usr/X11R6}
      x_cflags=${x_cflags:--I${x_includes:-$x_dir/include}}
      x_libs="${x_libs:--L${x_libraries:-$x_dir/lib}} -lX11 -lXext"
    fi
  ],[
    AC_MSG_ERROR([Cannot find X headers and libraries.])
  ]
)
AC_SUBST(x_cflags)
AC_SUBST(x_libs)

cf_cflags=""
cf_libs=""
PCPPFLAGS="${CPPFLAGS}"
CPPFLAGS="${CPPFLAGS} -I /System/Library/Frameworks/CoreFoundation.framework/Headers"
AC_CHECK_HEADERS(CFBase.h,
  [
    cf_cflags="-I /System/Library/Frameworks/CoreFoundation.framework/Headers"
    cf_cflags="${cf_cflags} -I /System/Library/Frameworks/IOKit.framework/Headers"
    cf_libs="-framework 'CoreFoundation' -framework 'IOKit'"
  ]
)
CPPFLAGS="${PCPPFLAGS}"
AC_SUBST(cf_cflags)
AC_SUBST(cf_libs)


execinfo_libs=""
AC_CHECK_HEADERS([execinfo.h], [have_execinfo="yes"], [have_execinfo="no"])
if test "x${have_execinfo}" = "xyes" ; then
  AC_MSG_CHECKING([whether backtrace_symbols_fd() is in libc])

  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
      [[
#include <stdlib.h>
#include <execinfo.h>
      ]],
      [[
backtrace_symbols_fd(NULL, 0, 0);
      ]])],
    [link_execinfo="yes"],
    [link_execinfo="no"])

  AC_MSG_RESULT([${link_execinfo}])
fi

if test "x${link_execinfo}" = "xno" ; then
  AC_MSG_CHECKING([whether backtrace_symbols_fd() is in libexecinfo])

  LDFLAGS_save="${LDFLAGS}"
  LDFLAGS="${LDFLAGS} -lexecinfo"
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
      [[
#include <stdlib.h>
#include <execinfo.h>
      ]],
      [[
backtrace_symbols_fd(NULL, 0, 0);
      ]])],
    [
      link_execinfo="yes"
      execinfo_libs="-lexecinfo"
    ],
    [link_execinfo="no"])
  LDFLAGS="${LDFLAGS_save}"

  AC_MSG_RESULT([${link_execinfo}])
fi

dnl if test "x${link_execinfo}" = "xno" ; then
dnl   AC_MSG_ERROR([Cannot use backtrace_symbols_fd()])
dnl fi


#ALL_LINGUAS="bg de eo es fi fr ja pl pt ru zh_CN hu sl it cs da sk sv nb nl zh_TW ko"
#ALL_LINGUAS="bg de eo es fi fr ja pt ru zh_CN hu sl it cs da sk sv nb nl zh_TW"
# a lot of .po's are broken
ALL_LINGUAS="bg ca cs da de el eo es fi fr fr_CH he hu it ja ko nb nl pl pt_BR ru sk sl sv tr zh_CN zh_TW"
AC_SUBST(ALL_LINGUAS)

m4_ifdef([AM_GNU_GETTEXT_VERSION], [
AM_GNU_GETTEXT_VERSION([0.12.1])
])

m4_ifdef([AM_GNU_GETTEXT], [
AM_GNU_GETTEXT([external])
po_makefile_in=po/Makefile.in
AM_CONDITIONAL([HAVE_PO], [true])
],[
AM_CONDITIONAL([HAVE_PO], [false])
])
AC_SUBST(LTLIBINTL)
if test "x$LIBINTL" = "x"; then
  LIBINTL="$INTLLIBS"
fi

AC_CHECK_LIB(dl, dlopen, dlopen_libs=-ldl)
AC_SUBST(dlopen_libs)

AC_MSG_CHECKING([whether to install sysactions.conf])
AC_ARG_ENABLE([install-sysactions],
  AS_HELP_STRING([--disable-install-sysactions],[disable installing /etc/enlightenment/sysactions.conf @<:@default=enabled@:>@]),
    [e_cv_enable_install_sysactions=$enableval],
    AC_CACHE_VAL([e_cv_enable_install_sysactions], [e_cv_enable_install_sysactions=yes]))
AC_MSG_RESULT([$e_cv_enable_install_sysactions])
AM_CONDITIONAL(INSTALL_SYSACTIONS, test x${e_cv_enable_install_sysactions} = xyes)

AC_MSG_CHECKING([whether to install enlightenment.menu])
AC_ARG_ENABLE([install-enlightenment-menu],
  AS_HELP_STRING([--disable-enlightenment-menu],[disable installing /etc/xdg/menus/enlightenment.menu @<:@default=enabled@:>@]),
    [e_cv_enable_install_enlightenment_menu=$enableval],
    AC_CACHE_VAL([e_cv_enable_install_enlightenment_menu], [e_cv_enable_install_enlightenment_menu=yes]))
AC_MSG_RESULT([$e_cv_enable_install_enlightenment_menu])
AM_CONDITIONAL(INSTALL_ENLIGHTENMENT_MENU, test x${e_cv_enable_install_enlightenment_menu} = xyes)

AC_MSG_CHECKING([whether to enable Files menu item])
AC_ARG_ENABLE([files],
  AS_HELP_STRING([--disable-files],[disable Files menu item @<:@default=enabled@:>@]),
    [e_cv_enable_files=$enableval],
    AC_CACHE_VAL([e_cv_enable_files], [e_cv_enable_files=yes]))
if test "[${e_cv_enable_files}]" = yes; then
  AC_DEFINE_UNQUOTED([ENABLE_FILES],[1],[enable Files menu item])
fi
AC_MSG_RESULT([$e_cv_enable_files])

AC_MSG_CHECKING([which device backend to use])
AC_ARG_ENABLE([device-hal],
  AS_HELP_STRING([--enable-device-hal],[enable hal device backend @<:@default=disabled@:>@]),
  [e_cv_want_device_hal=$enableval]
  AC_CACHE_VAL([e_cv_want_device_hal], [e_cv_want_device_hal=no]))

AC_ARG_ENABLE([device-udev],
  AS_HELP_STRING([--enable-device-udev],[enable udev device backend @<:@default=enabled@:>@]),
  [e_cv_want_device_udev=$enableval],
  AC_CACHE_VAL([e_cv_want_device_udev], [e_cv_want_device_udev=yes]))

if test "x$e_cv_want_device_udev" != "xno";then
  PKG_CHECK_EXISTS([eeze >= 1.0.999],
    [
      device_backend=eeze
      AC_DEFINE_UNQUOTED([HAVE_EEZE],[1],[enable udev support])
    ],[device_backend=ehal])
else
  device_backend=ehal
fi
AC_MSG_RESULT($device_backend)
AM_CONDITIONAL([HAVE_EEZE], [test "x${device_backend}" = "xeeze"])
test "x${device_backend}" = "xeeze" || AC_DEFINE_UNQUOTED([HAVE_HAL],[1],[enable HAL support])


DBUS_MOUNT_CONFIG=1
dbus_mount="dbus-1"
hal_mount="ehal >= 1.0.999, edbus >= 1.0.999"
udisks_mount="eukit >= 1.0.999"
AC_ARG_ENABLE([mount-hal],
  AS_HELP_STRING([--enable-mount-hal],[enable hal mounting support @<:@default=enabled@:>@]),
  [e_cv_want_mount_hal=$enableval],
  AC_CACHE_VAL([e_cv_want_mount_hal], [e_cv_want_mount_hal=yes]))
if test "x$e_cv_want_mount_hal" != "xno";then
  PKG_CHECK_EXISTS([ehal >= 1.0.999],
    [],[
      hal_mount=
      AC_MSG_NOTICE([HAL mounting disabled])
    ])
else
  hal_mount=
  AC_MSG_NOTICE([HAL mounting disabled])
fi

AC_ARG_ENABLE([mount-udisks],
  AS_HELP_STRING([--enable-mount-udisks],[enable udisks mounting support @<:@default=enabled@:>@]),
  [e_cv_want_mount_udisks=$enableval],
  AC_CACHE_VAL([e_cv_want_mount_udisks], [e_cv_want_mount_udisks=yes]))
if test "x$e_cv_want_mount_udisks" != "xno";then
  PKG_CHECK_EXISTS([eukit >= 1.0.999],
    [],[
      udisks_mount=
      AC_MSG_NOTICE([udisks mounting disabled])
    ])
else
  udisks_mount=
  AC_MSG_NOTICE([udisks mounting disabled])
fi


if test "x${e_cv_want_mount_udisks}" = "xno" && test "x${e_cv_want_mount_hal}" = "xno" ; then
  DBUS_MOUNT_CONFIG=0
  dbus_mount=
  test "x${e_cv_want_mount_udisks}" = "xno" && udisks_mount=
  test "x${e_cv_want_mount_hal}" = "xno" && hal_mount=
fi

AC_ARG_ENABLE([mount-eeze],
  AS_HELP_STRING([--enable-mount-eeze],[enable eeze mounting support @<:@default=enabled@:>@]),
  [e_cv_want_mount_eeze=$enableval],
  AC_CACHE_VAL([e_cv_want_mount_eeze], [e_cv_want_mount_eeze=yes]))

eeze_mount=
EEZE_MOUNT_CONFIG=0
if test "x$e_cv_want_mount_eeze" != "xno" ; then
      AC_CHECK_LIB([eeze], [eeze_disk_function],
        [
           eeze_mount="eeze >= 1.0.99 ecore-con >= 1.0.999"
           EEZE_MOUNT_CONFIG=1
           AC_DEFINE_UNQUOTED([HAVE_EEZE_MOUNT], [1], [enable eeze mounting])
           AC_MSG_NOTICE([eeze mounting enabled])
        ],
      )
else
  AC_MSG_NOTICE([eeze mounting disabled])
fi

AC_SUBST([EEZE_MOUNT_CONFIG])
AC_SUBST([DBUS_MOUNT_CONFIG])
AM_CONDITIONAL([HAVE_HAL_MOUNT], [test -n "$hal_mount"])
AM_CONDITIONAL([HAVE_EEZE_MOUNT], [false])
test -n "$hal_mount" && AC_DEFINE_UNQUOTED([HAVE_HAL_MOUNT], [1], [enable HAL mounting])
test -n "$udisks_mount" && AC_DEFINE_UNQUOTED([HAVE_UDISKS_MOUNT], [1], [enable Udisks mounting])
AM_CONDITIONAL([HAVE_UDISKS_MOUNT], [test -n "$udisks_mount"])
AM_CONDITIONAL([HAVE_EEZE_MOUNT], [test -n "$eeze_mount"])

# doxygen program for documentation building

EFL_CHECK_DOXYGEN([build_doc="yes"], [build_doc="no"])

# explicit libs for each binary to reduce linkage requirements
PKG_CHECK_MODULES(E_REMOTE, [
  ecore >= 1.0.999
  ecore-ipc >= 1.0.999
  eet >= 1.4.0
  eina >= 1.0.999
])

PKG_CHECK_MODULES(E_IMC, [
  ecore-file >= 1.0.999
  eet >= 1.4.0
  eina >= 1.0.999
])

PKG_CHECK_MODULES(E_THUMB, [
  eina >= 1.0.999
  evas >= 1.0.999
  ecore >= 1.0.999
  ecore-evas >= 1.0.999
  edje >= 1.0.999
  ecore-file >= 1.0.999
  ecore-ipc >= 1.0.999
  eet >= 1.4.0
  efreet >= 1.0.999
])

efm_requires="\
ecore >= 1.0.999 \
ecore-con >= 1.0.999 \
ecore-file >= 1.0.999 \
ecore-ipc >= 1.0.999 \
eet >= 1.4.0 \
efreet >= 1.0.999 \
eina >= 1.0.999"
efm_requires+=" $eeze_mount $dbus_mount $hal_mount $udisks_mount"

PKG_CHECK_MODULES(E_FM, [$efm_requires])

PKG_CHECK_MODULES(E_FM_OP, [
  ecore >= 1.0.999
  ecore-file >= 1.0.999
  eina >= 1.0.999
])

PKG_CHECK_MODULES(E_FM_OPEN, [
  ecore >= 1.0.999
  ecore-ipc >= 1.0.999
  eet >= 1.4.0
  eina >= 1.0.999
])

PKG_CHECK_MODULES(E_SYS, [
  eina >= 1.0.999
])

PKG_CHECK_MODULES(E_INIT, [
  evas >= 1.0.999
  ecore >= 1.0.999
  ecore-evas >= 1.0.999
  ecore-x >= 1.0.999
  edje >= 1.0.999
  ecore-ipc >= 1.0.999
  eina >= 1.0.999
])

PKG_CHECK_MODULES(E, [
  evas >= 1.0.999
  ecore >= 1.0.999
  ecore-x >= 1.0.999
  ecore-evas >= 1.0.999
  ecore-input >= 1.0.999
  ecore-input-evas >= 1.0.999
  ecore-con >= 1.0.999
  ecore-ipc >= 1.0.999
  ecore-file >= 1.0.999
  eet >= 1.4.0
  edje >= 1.0.999
  efreet >= 1.0.999
  efreet-mime >= 1.0.999
  efreet-trash >= 1.0.999
  eina >= 1.0.999
  dbus-1
  edbus >= 1.0.999
  $hal_mount
  $eeze_mount
  $udisks_mount
  $device_backend
])
requirements_e="\
evas >= 1.0.999 \
ecore >= 1.0.999 \
ecore-x >= 1.0.999 \
ecore-evas >= 1.0.999 \
ecore-con >= 1.0.999 \
ecore-ipc >= 1.0.999 \
ecore-file >= 1.0.999 \
eet >= 1.4.0 \
edje >= 1.0.999 \
efreet >= 1.0.999 \
efreet-mime >= 1.0.999 \
eina >= 1.0.999 \
dbus-1 \
edbus >= 1.0.999 \
$hal_mount \
$udisks_mount \
$eeze_mount \
$device_backend"

AC_E_CHECK_PKG(ECORE_IMF, [ ecore-imf >= 1.0.999 ecore-imf-evas >= 1.0.999 ], [], [:])

e_libs=$E_LIBS" "$LIBINTL" "$fnmatch_libs" "$ECORE_IMF_LIBS" "$execinfo_libs
e_cflags="-DUSE_E_CONFIG_H "$E_CFLAGS" "$ECORE_IMF_CFLAGS
e_configflags="-DUSE_E_CONFIG_H"

AC_SUBST(e_libs)
AC_SUBST(e_cflags)
AC_SUBST(e_configflags)
AC_SUBST(requirements_e)

profile="SLOW_PC"
AC_ARG_WITH(profile,
  AS_HELP_STRING([--with-profile=TARGET_PROFILE], 
  [specify a target format profile of:
                                    LOWRES_PDA, MEDIUMRES_PDA, HIRES_PDA, SLOW_PC, MEDIUM_PC, FAST_PC]),
  [profile=$withval])
AC_MSG_NOTICE([Enlightenment build profile set to $profile])
EDJE_DEF="-DLOWRES_PDA=1 -DMEDIUMRES_PDA=2 -DHIRES_PDA=3 -DSLOW_PC=4 -DMEDIUM_PC=5 -DFAST_PC=6"
EDJE_DEF=$EDJE_DEF" -DE17_PROFILE"=$profile
AC_SUBST(EDJE_DEF)

AC_ARG_WITH(edje-cc,
[  --with-edje-cc=PATH              specify a specific path to edje_cc],
[
  v=$withval;
  edje_cc=$v
  echo "  Enlightenment edje_cc explicitly set to "$edje_cc;
],[
  edje_cc=$(pkg-config --variable=prefix edje)/bin/edje_cc
])
AC_SUBST(edje_cc)

PKG_CHECK_MODULES(EET, [eet >= 1.4.0])
AC_ARG_WITH(eet-eet,
[  --with-eet-eet=PATH              specify a specific path to eet utility],
[
  v=$withval;
  eet_eet=$v
  echo "  Enlightenment eet explicitly set to "$eet_eet;
],[
  eet_eet=$(pkg-config --variable=prefix eet)/bin/eet
])
AC_SUBST(eet_eet)

AC_DEFINE(E_INTERNAL, 1, "This define can be used to wrap internal E stuff, as config.h isn't exported")

AM_CONDITIONAL(HAVE_BATTERY, false)
define([CHECK_MODULE_BATTERY],
[
  AC_E_CHECK_PKG(BATTERY, [ ecore >= 1.0.999 ecore-file >= 1.0.999 ecore-con >= 1.0.999 eina >= 1.0.999 ], [], [BATTERY=false])
])

BATTERY_LDFLAGS=
case "$host_os" in
  darwin*)
    BATTERY_LDFLAGS="-framework Foundation -framework IOKit"
  ;;
esac

AC_SUBST(BATTERY_LDFLAGS)

AM_CONDITIONAL(HAVE_TEMPERATURE, false)
define([CHECK_MODULE_TEMPERATURE],
[
  AC_E_CHECK_PKG(TEMPERATURE, [ ecore >= 1.0.999 ecore-file >= 1.0.999 eina >= 1.0.999 ], [], [TEMPERATURE=false])
])

have_exchange=no
AC_ARG_ENABLE(exchange,
  AC_HELP_STRING([--disable-exchange], [disable Exchange support @<:@default=detect@:>@]),
  [want_exchange=$enableval],
  [want_exchange=auto]
)

AM_CONDITIONAL(HAVE_EXCHANGE, false)
define([CHECK_MODULE_CONF_THEME],
[
  if test "x$want_exchange" != "xno" ; then
    AC_E_CHECK_PKG(EXCHANGE, [exchange], [], [:])
  else
    AC_MSG_NOTICE([requested to not use exchange library])
  fi
])


have_enotify=no
AM_CONDITIONAL([HAVE_ENOTIFY], [false])
AC_ARG_ENABLE([enotify],
  AC_HELP_STRING([--disable-enotify], [disable Enotify support @<:@default=detect@:>@]),
  [e_cv_want_enotify=$enableval],
  AC_CACHE_VAL([e_cv_want_enotify], [e_cv_want_enotify=yes])
)
if test "x$e_cv_want_enotify" != "xno" ; then
  AC_E_CHECK_PKG(ENOTIFY, [ edbus >= 1.0.999 enotify >= 1.0.999 ], 
  [
    AC_DEFINE_UNQUOTED([HAVE_ENOTIFY], [1], [enable enotify])
  ], 
  [
    AC_MSG_NOTICE([enotify disabled])
    e_cv_want_enotify=no
  ])
else
  AC_MSG_NOTICE([enotify disabled])
    e_cv_want_enotify=no
fi
AC_SUBST([ENOTIFY_CFLAGS])
AC_SUBST([ENOTIFY_LIBS])


AM_CONDITIONAL(HAVE_ALSA, false)
define([CHECK_MODULE_MIXER],
[
  if test "x$enable_alsa" = "x" || test "x$enable_alsa" = "xdefault" || test "x$enable_alsa" = "xyes"; then
    AC_E_CHECK_PKG(ALSA, [alsa >= 1.0.8],
    [ SOUND_CFLAGS="$ALSA_CFLAGS -DHAVE_ALSA $SOUND_CFLAGS"
      SOUND_LIBS="$ALSA_LIBS $SOUND_LDFLAGS"
    ],
    [ if test "x$enable_alsa" = "xyes"; then
        AC_MSG_ERROR([alsa library >= 1.0.8 not found])
      else
        AC_MSG_WARN([alsa library development files not present. no alsa support.])
      fi
    ])
  else
    have_alsa=no
  fi

  if test "$have_alsa" = "yes"; then
    AC_DEFINE(HAVE_ALSA, 1, [Define if the ALSA output plugin should be built])
  else
    have_alsa=no
  fi
])

AC_SUBST(SOUND_CFLAGS)
AC_SUBST(SOUND_LIBS)

AM_CONDITIONAL(HAVE_ECONNMAN, false)
define([CHECK_MODULE_CONNMAN],
[
  AC_E_CHECK_PKG(ECONNMAN, [ edbus >= 1.0.999 econnman >= 1.0.999 ], [], [CONNMAN=false])
])
AC_SUBST(ECONNMAN_CFLAGS)
AC_SUBST(ECONNMAN_LIBS)

AM_CONDITIONAL(HAVE_EBLUEZ, false)
define([CHECK_MODULE_BLUEZ],
[
  AC_E_CHECK_PKG(EBLUEZ, [ edbus >= 1.0.999 ebluez >= 1.0.999 ], [], [BLUEZ=false])
])
AC_SUBST(EBLUEZ_CFLAGS)
AC_SUBST(EBLUEZ_LIBS)

AM_CONDITIONAL(HAVE_EOFONO, false)
define([CHECK_MODULE_OFONO],
[
  AC_E_CHECK_PKG(EOFONO, [ edbus >= 1.0.999 eofono >= 1.0.999 ], [], [OFONO=false])
])
AC_SUBST(EOFONO_CFLAGS)
AC_SUBST(EOFONO_LIBS)


AC_E_OPTIONAL_MODULE([ibar], true)
AC_E_OPTIONAL_MODULE([dropshadow], true)
AC_E_OPTIONAL_MODULE([clock], true)
AC_E_OPTIONAL_MODULE([pager], true)
AC_E_OPTIONAL_MODULE([battery], true, [CHECK_MODULE_BATTERY])
AC_E_OPTIONAL_MODULE([temperature], true, [CHECK_MODULE_TEMPERATURE])
AC_E_OPTIONAL_MODULE([cpufreq], true)
AC_E_OPTIONAL_MODULE([ibox], true)
AC_E_OPTIONAL_MODULE([start], true)
AC_E_OPTIONAL_MODULE([winlist], true)
AC_E_OPTIONAL_MODULE([fileman], true)
AC_E_OPTIONAL_MODULE([fileman_opinfo], true)
AC_E_OPTIONAL_MODULE([wizard], true)
AC_E_OPTIONAL_MODULE([conf], true)
AC_E_OPTIONAL_MODULE([conf_wallpaper2], true)
AC_E_OPTIONAL_MODULE([conf_theme], true, [CHECK_MODULE_CONF_THEME])
AC_E_OPTIONAL_MODULE([conf_intl], true)
AC_E_OPTIONAL_MODULE([msgbus], true)
AC_E_OPTIONAL_MODULE([conf_applications], true)
AC_E_OPTIONAL_MODULE([conf_display], true)
AC_E_OPTIONAL_MODULE([conf_shelves], true)
AC_E_OPTIONAL_MODULE([conf_keybindings], true)
AC_E_OPTIONAL_MODULE([conf_edgebindings], true)
AC_E_OPTIONAL_MODULE([conf_window_remembers], true)
AC_E_OPTIONAL_MODULE([conf_window_manipulation], true)
AC_E_OPTIONAL_MODULE([conf_menus], true)
AC_E_OPTIONAL_MODULE([conf_dialogs], true)
AC_E_OPTIONAL_MODULE([conf_performance], true)
AC_E_OPTIONAL_MODULE([conf_paths], true)
AC_E_OPTIONAL_MODULE([conf_interaction], true)
AC_E_OPTIONAL_MODULE([gadman], true)
AC_E_OPTIONAL_MODULE([mixer], true, [CHECK_MODULE_MIXER])
AC_E_OPTIONAL_MODULE([connman], true, [CHECK_MODULE_CONNMAN])
AC_E_OPTIONAL_MODULE([bluez], true, [CHECK_MODULE_BLUEZ])
AC_E_OPTIONAL_MODULE([ofono], true, [CHECK_MODULE_OFONO])
AC_E_OPTIONAL_MODULE([illume2], true)
AC_E_OPTIONAL_MODULE([syscon], true)
AC_E_OPTIONAL_MODULE([everything], true)
AC_E_OPTIONAL_MODULE([systray], true)
AC_E_OPTIONAL_MODULE([comp], true)
AC_E_OPTIONAL_MODULE([shot], true)
AC_E_OPTIONAL_MODULE([backlight], true)

SUSPEND=""
HIBERNATE=""
case "$host_os" in
   freebsd*|pcbsd*)
      SUSPEND="acpiconf -s3"
      AC_MSG_NOTICE([use suspend method: $SUSPEND])
      AC_SUBST(SUSPEND)
      HIBERNATE="acpiconf -s4"
      AC_MSG_NOTICE([use hibernate method: $HIBERNATE])
      AC_SUBST(HIBERNATE)
      ;;
   *)
      AC_PATH_PROGS([SUSPEND], [sleep.sh pm-suspend],[/etc/acpi/sleep.sh], [/etc/acpi:/sbin:/usr/sbin:$PATH])
      if test "$SUSPEND" = "/etc/acpi/sleep.sh" ; then
         SUSPEND="/etc/acpi/sleep.sh force"
         AC_MSG_NOTICE([use suspend method: $SUSPEND])
      fi

      AC_PATH_PROGS([HIBERNATE], [hibernate.sh pm-hibernate],[/etc/acpi/hibernate.sh], [/etc/acpi:/sbin:/usr/sbin:$PATH])
      if test "$HIBERNATE" = "/etc/acpi/hibernate.sh" ; then
         HIBERNATE="/etc/acpi/hibernate.sh force"
         AC_MSG_NOTICE([use hibernate method: $HIBERNATE])
      fi
      ;;
esac


AC_OUTPUT([
Makefile
enlightenment.spec
enlightenment.pc
src/Makefile
src/bin/Makefile
src/bin/e_fm_shared_types.h
src/bin/e_fm/Makefile
src/modules/Makefile
src/modules/ibar/Makefile
src/modules/ibar/module.desktop
src/modules/dropshadow/Makefile
src/modules/dropshadow/module.desktop
src/modules/clock/Makefile
src/modules/clock/module.desktop
src/modules/pager/Makefile
src/modules/pager/module.desktop
src/modules/battery/Makefile
src/modules/battery/module.desktop
src/modules/temperature/Makefile
src/modules/temperature/module.desktop
src/modules/cpufreq/Makefile
src/modules/cpufreq/module.desktop
src/modules/ibox/Makefile
src/modules/ibox/module.desktop
src/modules/start/Makefile
src/modules/start/module.desktop
src/modules/winlist/Makefile
src/modules/winlist/module.desktop
src/modules/fileman/Makefile
src/modules/fileman/module.desktop
src/modules/fileman_opinfo/Makefile
src/modules/fileman_opinfo/module.desktop
src/modules/wizard/Makefile
src/modules/conf/Makefile
src/modules/conf/module.desktop
src/modules/conf_wallpaper2/Makefile
src/modules/conf_wallpaper2/module.desktop
src/modules/conf_theme/Makefile
src/modules/conf_theme/module.desktop
src/modules/conf_intl/Makefile
src/modules/conf_intl/module.desktop
src/modules/conf_applications/Makefile
src/modules/conf_applications/module.desktop
src/modules/conf_display/Makefile
src/modules/conf_display/module.desktop
src/modules/conf_shelves/Makefile
src/modules/conf_shelves/module.desktop
src/modules/conf_keybindings/Makefile
src/modules/conf_keybindings/module.desktop
src/modules/conf_edgebindings/Makefile
src/modules/conf_edgebindings/module.desktop
src/modules/conf_window_remembers/Makefile
src/modules/conf_window_remembers/module.desktop
src/modules/conf_window_manipulation/Makefile
src/modules/conf_window_manipulation/module.desktop
src/modules/conf_menus/Makefile
src/modules/conf_menus/module.desktop
src/modules/conf_dialogs/Makefile
src/modules/conf_dialogs/module.desktop
src/modules/conf_performance/Makefile
src/modules/conf_performance/module.desktop
src/modules/conf_paths/Makefile
src/modules/conf_paths/module.desktop
src/modules/conf_interaction/Makefile
src/modules/conf_interaction/module.desktop
src/modules/msgbus/Makefile
src/modules/msgbus/module.desktop
src/modules/gadman/Makefile
src/modules/gadman/module.desktop
src/modules/mixer/Makefile
src/modules/mixer/module.desktop
src/modules/connman/Makefile
src/modules/connman/module.desktop
src/modules/bluez/Makefile
src/modules/bluez/module.desktop
src/modules/ofono/Makefile
src/modules/ofono/module.desktop
src/modules/illume2/Makefile
src/modules/illume2/doc/illume.dox
src/modules/illume2/doc/Makefile
src/modules/illume2/images/Makefile
src/modules/illume2/module.desktop
src/modules/illume2/keyboards/Makefile
src/modules/illume2/policies/Makefile
src/modules/illume2/policies/illume/Makefile
src/modules/illume-home/Makefile
src/modules/illume-home/module.desktop
src/modules/illume-home-toggle/Makefile
src/modules/illume-home-toggle/module.desktop
src/modules/illume-softkey/Makefile
src/modules/illume-softkey/module.desktop
src/modules/illume-softkey/images/Makefile
src/modules/illume-keyboard/Makefile
src/modules/illume-keyboard/module.desktop
src/modules/illume-keyboard/keyboards/Makefile
src/modules/illume-keyboard/dicts/Makefile
src/modules/illume-keyboard/images/Makefile
src/modules/illume-indicator/Makefile
src/modules/illume-indicator/module.desktop
src/modules/illume-indicator/images/Makefile
src/modules/illume-kbd-toggle/Makefile
src/modules/illume-kbd-toggle/module.desktop
src/modules/illume-mode-toggle/Makefile
src/modules/illume-mode-toggle/module.desktop
src/modules/illume-bluetooth/Makefile
src/modules/illume-bluetooth/module.desktop
src/modules/syscon/Makefile
src/modules/syscon/module.desktop
src/modules/everything/Makefile
src/modules/everything/module.desktop
src/modules/everything/everything.pc
src/modules/systray/Makefile
src/modules/systray/module.desktop
src/modules/comp/Makefile
src/modules/comp/module.desktop
src/modules/shot/Makefile
src/modules/shot/module.desktop
src/modules/backlight/Makefile
src/modules/backlight/module.desktop
src/preload/Makefile
data/Makefile
data/images/Makefile
data/input_methods/Makefile
data/themes/Makefile
data/themes/images/Makefile
data/other/Makefile
data/xsession/Makefile
data/xsession/enlightenment.desktop
data/tools/Makefile
data/etc/Makefile
data/etc/sysactions.conf
data/icons/Makefile
data/backgrounds/Makefile
doc/Makefile
doc/Doxyfile
doc/e.dox
config/Makefile
config/default/Makefile
config/standard/Makefile
config/illume/Makefile
$po_makefile_in
])

# report
txt_strip() {
  echo "[$]@" | sed -e 's/^[[ \t]]*\([[^ \t]]*\)[[ \t]]*$/\1/g'
}

echo
cat << SUMMARY_EOF
Summary:
 * project.........: $PACKAGE $VERSION
 * prefix..........: $(txt_strip $prefix)
 * CFLAGS..........: $(txt_strip $CFLAGS)
 * LDFLAGS.........: $(txt_strip $LDFLAGS)
SUMMARY_EOF
echo
cat << DEVICE_EOF
Preferred Backends:
 * device..........: $device_backend
 * hal_mount...........: $((test x$e_cv_want_mount_hal != xno) && echo enabled)
 * udisks_mount........: $((test x$e_cv_want_mount_udisks != xno) && echo enabled)
 * eeze_mount..........: $((test x$e_cv_want_mount_eeze != xno) && echo enabled)
DEVICE_EOF
echo

UNUSED_MODS=""
for mod in $UNUSED_OPTIONAL_MODULES; do
  UNUSED_MODS="$UNUSED_MODS${COLOR_HRED}$mod${COLOR_END} "
done

cat << MODULES_EOF
Modules:
 * disabled........: $(txt_strip $UNUSED_MODS)
MODULES_EOF
echo

cat << DOCS_EOF
Documentation:
 * enabled.........: ${build_doc}
DOCS_EOF
if test "x${build_doc}" = "xyes" ; then
  echo " Building..........: make doc"
fi
echo

cat << COMPILE_EOF
Compilation........: make (or gmake)
COMPILE_EOF
echo

cat << INSTALL_EOF
Installation.......: make all install (as root if needed, with 'su' or 'sudo')
INSTALL_EOF
echo
