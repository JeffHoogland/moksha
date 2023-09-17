##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_define([v_maj], [0])
m4_define([v_min], [4])
m4_define([v_mic], [1])

m4_define([v_rev], m4_esyscmd([(git rev-list --count HEAD 2>/dev/null || echo 0) | tr -d '\n']))dnl
##--   When released, remove the dnl on the below line
dnl m4_undefine([v_rev])
m4_define([relname], [0.4.0])
##--   When doing snapshots - change soname. remove dnl on below line
dnl m4_define([relname], [ver-0.4])
dnl m4_define([v_rel], [-release relname])
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_ifdef([v_rev], [m4_define([v_ver], [v_maj.v_min.v_mic.v_rev])], [m4_define([v_ver], [v_maj.v_min.v_mic])])
m4_define([lt_cur], m4_eval(v_maj + v_min))
m4_define([lt_rev], v_mic)
m4_define([lt_age], v_min)
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##

AC_INIT([enlightenment],[v_ver],[moksha@bodhilinux.com])
AC_PREREQ([2.71])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AC_CONFIG_HEADERS([config.h])
AH_TOP([
#ifndef EFL_CONFIG_H__
#define EFL_CONFIG_H__
])
AH_BOTTOM([
#endif /* EFL_CONFIG_H__ */
])

AM_INIT_AUTOMAKE([1.11 dist-bzip2 -Wno-portability])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_USE_SYSTEM_EXTENSIONS
m4_version_prereq(2.70, [AC_PROG_CC], [m4_warn([obsolete],
[AC_PROG_CC_C99 is obsolete; use AC_PROG_CC])dnl
AC_REQUIRE(AC_PROG_CC)])
AC_C_BIGENDIAN
AM_PROG_CC_C_O
AC_C_CONST
AC_FUNC_ALLOCA
AC_PROG_MKDIR_P
AC_C___ATTRIBUTE__

LT_INIT([disable-static])

##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_ifdef([v_rev], , [m4_define([v_rev], [0])])
m4_ifdef([v_rel], , [m4_define([v_rel], [])])
AC_DEFINE_UNQUOTED(VMAJ, [v_maj], [Major version])
AC_DEFINE_UNQUOTED(VMIN, [v_min], [Minor version])
AC_DEFINE_UNQUOTED(VMIC, [v_mic], [Micro version])
AC_DEFINE_UNQUOTED(VREV, [v_rev], [Revison])
version_info="lt_cur:lt_rev:lt_age"
release_info="v_rel"
AC_SUBST(version_info)
AC_SUBST(release_info)
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##

##--   For debugging consider removing the define below
AC_DEFINE_UNQUOTED(MOKSHA_RELEASE_BUILD, [1], [enable for release])

release=relname
AC_SUBST(release)

PKG_PROG_PKG_CONFIG

EFL_CHECK_PATH_MAX

AC_CHECK_FUNCS(setenv)
AC_CHECK_FUNCS(unsetenv)
AC_CHECK_FUNCS(clearenv)
AC_CHECK_HEADERS(features.h)

case "$host_os" in
   darwin*)
      AC_DEFINE([environ], [(*_NSGetEnviron())],
                ["apple doesn't follow POSIX in this case."])
      ;;
   *)
      ;;
esac

AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#define _GNU_SOURCE 1
#include <unistd.h>
    ]], [[
extern char **environ;
    ]])],[
AC_DEFINE(HAVE_ENVIRON, 1, [Have environ var])
    ],[])


efl_version="1.20.0"
AC_SUBST(efl_version)

PKG_CHECK_MODULES([EFL], [efl],
[
			EFL_VERSION=$($PKG_CONFIG --modversion efl)
			AC_SUBST(EFL_VERSION)
			EFL_MINOR=$(echo "[$EFL_VERSION]" | cut -d. -f2)
			AC_SUBST(EFL_MINOR)
	  ])
AC_DEFINE_UNQUOTED(EFL_VERSION, ["$EFL_VERSION"], [Determine EFL version])
AC_DEFINE_UNQUOTED(EFL_MINOR, [$EFL_MINOR], [Determine EFL minor version])

# Detect systemd user session directory properly
EFL_PKG_CHECK_VAR([USER_SESSION_DIR], [systemd >= 192], [systemduserunitdir],
	[have_systemd_user_session="yes"], [have_systemd_user_session="no"])
AM_CONDITIONAL([HAVE_SYSTEMD_USER_SESSION], [test "x${have_systemd_user_session}" = "xyes"])
AC_SUBST([USER_SESSION_DIR])

AC_CHECK_HEADERS([sys/timerfd.h sys/ptrace.h arpa/inet.h netinet/in.h])

dnl AC_CHECK_HEADERS(X11/extensions/shape.h,, AC_MSG_ERROR([Cannot find X11/extensions/shape.h. Make sure your CFLAGS environment variable contains include lines for the location of this file]))

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
  AS_HELP_STRING([--enable-pam], [enable PAM support @<:@default=detect@:>@]),
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

dnl     defines HAVE_STRDUP if strdup is available
AC_CHECK_FUNCS([strdup])
dnl     defines HAVE_STRNDUP if strndup is available
AC_CHECK_FUNCS([strndup])
AC_CHECK_HEADERS([malloc.h])
dnl     defines HAVE_MALLOC_TRIM if malloc_trim is available
AC_CHECK_FUNCS([malloc_trim])

MODULE_ARCH="$host_os-$host_cpu"
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
  AS_HELP_STRING([--enable-simple-x11], [enable simple x11 linking]),
  [ want_evas_simple_x11=$enableval ]
)
    
dnl x_dir=""
dnl x_cflags=""
dnl x_libs=""
dnl AC_PATH_XTRA
dnl AC_CHECK_HEADER(X11/X.h,
dnl   [
dnl     if test "x$want_evas_simple_x11" = "xyes"; then
dnl       x_libs="${x_libs} -lX11 -lXext"
dnl     else
dnl       x_dir=${x_dir:-/usr/X11R6}
dnl       x_cflags=${x_cflags:--I${x_includes:-$x_dir/include}}
dnl       x_libs="${x_libs:--L${x_libraries:-$x_dir/lib}} -lX11 -lXext"
dnl     fi
dnl   ],[
dnl     AC_MSG_ERROR([Cannot find X headers and libraries.])
dnl   ]
dnl )
dnl AC_SUBST(x_cflags)
dnl AC_SUBST(x_libs)

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
AM_GNU_GETTEXT_VERSION([0.19])
])

m4_ifdef([AC_GNU_GETTEXT], [
AC_GNU_GETTEXT([external], [need-ngettext])
po_makefile_in=po/Makefile.in
have_po="yes"
],[
m4_ifdef([AM_GNU_GETTEXT], [
AM_GNU_GETTEXT([external], [need-ngettext])
po_makefile_in=po/Makefile.in
have_po="yes"
],[
have_po="no"
])
])
AC_SUBST(LTLIBINTL)
if test "x$LIBINTL" = "x"; then
  LIBINTL="$INTLLIBS"
fi

if test "x${POSUB}" = "x" ; then
   have_po="no"
fi

AM_CONDITIONAL([HAVE_PO], [test "x${have_po}" = "xyes"])

AC_CHECK_LIB(dl, dlopen, dlopen_libs=-ldl)
AC_SUBST(dlopen_libs)

AC_MSG_CHECKING([whether to install sysactions.conf])
AC_ARG_ENABLE([install-sysactions],
  AS_HELP_STRING([--disable-install-sysactions],[disable installing /etc/enlightenment/sysactions.conf @<:@default=enabled@:>@]),
    [e_cv_enable_install_sysactions=$enableval],
    AC_CACHE_VAL([e_cv_enable_install_sysactions], [e_cv_enable_install_sysactions=yes]))
AC_MSG_RESULT([$e_cv_enable_install_sysactions])
AM_CONDITIONAL(INSTALL_SYSACTIONS, test x${e_cv_enable_install_sysactions} = xyes)

AC_MSG_CHECKING([whether to install wizard desktop files])
AC_ARG_ENABLE([install-wizard-desktop],
  AS_HELP_STRING([--disable-wizard-desktop],[disable installing home Desktop files @<:@default=disabled@:>@]),
    [e_cv_enable_install_wizard_desktop=$enableval],
    AC_CACHE_VAL([e_cv_enable_install_wizard_desktop], [e_cv_enable_install_wizard_desktop=no]))
AC_MSG_RESULT([$e_cv_enable_install_wizard_desktop])
AM_CONDITIONAL(INSTALL_WIZARD_DESKTOP, test x${e_cv_enable_install_wizard_desktop} = xyes)

AC_MSG_CHECKING([whether to enable Files menu item])
AC_ARG_ENABLE([files],
  AS_HELP_STRING([--disable-files],[disable Files menu item @<:@default=enabled@:>@]),
    [e_cv_enable_files=$enableval],
    AC_CACHE_VAL([e_cv_enable_files], [e_cv_enable_files=yes]))
if test "[${e_cv_enable_files}]" = yes; then
  AC_DEFINE_UNQUOTED([ENABLE_FILES],[1],[enable Files menu item])
fi

AC_MSG_RESULT([$e_cv_enable_bodhi])
AC_MSG_CHECKING([whether to enable Bodhi menu item])
AC_ARG_ENABLE([bodhi],
  AS_HELP_STRING([--disable-bodhi],[disable bodhi menu item @<:@default=enabled@:>@]),
    [e_cv_enable_bodhi=$enableval],
    AC_CACHE_VAL([e_cv_enable_bodhi], [e_cv_enable_bodhi=yes]))
if test "[${e_cv_enable_bodhi}]" = yes; then
  AC_DEFINE_UNQUOTED([ENABLE_BODHI],[1],[enable Bodhi menu item])
fi
AC_MSG_RESULT([$e_cv_enable_bodhi])

AC_ARG_ENABLE([device-hal],
  AS_HELP_STRING([--enable-device-hal],[enable hal device backend @<:@default=disabled@:>@]),
  [e_cv_want_device_hal=$enableval],
  [e_cv_want_device_hal=no])

AC_MSG_CHECKING([whether the HAL device backend is enabled])
AC_MSG_RESULT([${e_cv_want_device_hal}])

AC_ARG_ENABLE([device-udev],
  AS_HELP_STRING([--enable-device-udev],[enable udev device backend @<:@default=enabled@:>@]),
  [e_cv_want_device_udev=$enableval],
  [e_cv_want_device_udev=yes])

AC_MSG_CHECKING([whether the UDEV device backend is enabled])
AC_MSG_RESULT([${e_cv_want_device_udev}])

if test "x$e_cv_want_device_udev" != "xno";then
  PKG_CHECK_MODULES([EEZE], [eeze >= ${efl_version} ecore >= ${efl_version} eina >= ${efl_version}],
    [
      device_backend=eeze
      AC_DEFINE_UNQUOTED([HAVE_EEZE],[1],[enable udev support])
    ])
fi

AC_MSG_CHECKING([which device backend to use])
AC_MSG_RESULT($device_backend)

AM_CONDITIONAL([HAVE_EEZE], [test "x${device_backend}" = "xeeze"])
test "x${device_backend}" = "xeeze" || AC_DEFINE_UNQUOTED([HAVE_HAL],[1],[enable HAL support])


DBUS_MOUNT_CONFIG=1
dbus_mount="dbus-1"

if test "x${e_cv_want_mount_udisks}" = "xno" && test "x${e_cv_want_mount_hal}" = "xno" ; then
  DBUS_MOUNT_CONFIG=0
  dbus_mount=
  udisks_mount=
  hal_mount=
fi

AC_ARG_ENABLE([mount-eeze],
  AS_HELP_STRING([--enable-mount-eeze],[enable eeze mounting support @<:@default=disabled@:>@]),
  [e_cv_want_mount_eeze=$enableval],
  [e_cv_want_mount_eeze=no])

AC_MSG_CHECKING([whether the Eeze mounting support is enabled])
AC_MSG_RESULT([${e_cv_want_mount_eeze}])

eeze_mount=
EEZE_MOUNT_CONFIG=0
if test "x$e_cv_want_mount_eeze" != "xno" && test -z "$dbus_mount" ; then
      CPPFLAGS_save="$CPPFLAGS"
      LIBS_save="$LIBS"
      CPPFLAGS="$EEZE_CFLAGS $CPPFLAGS"
      LIBS="$EEZE_LIBS $LIBS"
      AC_CHECK_LIB([eeze], [eeze_disk_function],
        [
           eeze_mount="eeze >= $efl_version ecore-con >= $efl_version"
           EEZE_MOUNT_CONFIG=1
           AC_DEFINE_UNQUOTED([HAVE_EEZE_MOUNT], [1], [enable eeze mounting])
           AC_MSG_NOTICE([eeze mounting enabled])
        ],
        [
           AC_MSG_NOTICE([eeze mounting disabled])
           e_cv_want_mount_eeze=no
        ]
      )
      CPPFLAGS="$CPPFLAGS_save"
      LIBS="$LIBS_save"
else
  AC_MSG_NOTICE([eeze mounting disabled])
fi

AC_SUBST([EEZE_MOUNT_CONFIG])
AC_SUBST([DBUS_MOUNT_CONFIG])
AM_CONDITIONAL([HAVE_HAL_MOUNT], [test -n "$hal_mount"])
AM_CONDITIONAL([HAVE_EEZE_MOUNT], [false])
test -n "$hal_mount" && AC_DEFINE_UNQUOTED([HAVE_HAL_MOUNT], [1], [enable HAL mounting])
test -n "$dbus_mount" && AC_DEFINE_UNQUOTED([HAVE_UDISKS_MOUNT], [1], [enable Udisks mounting])
AM_CONDITIONAL([HAVE_UDISKS_MOUNT], [test -n "$dbus_mount"])
AM_CONDITIONAL([HAVE_EEZE_MOUNT], [test -n "$eeze_mount"])




AC_MSG_RESULT([$e_cv_places_backend_e17])
AC_MSG_CHECKING([enable places e17 backend])
AC_ARG_ENABLE([places-e17],
  AS_HELP_STRING([--enable-places-e17],[enable places e17 backend @<:@default=disabled@:>@]),
    [e_cv_places_backend_e17=$enableval], [e_cv_places_backend_e17=no])
AC_MSG_RESULT([$e_cv_places_backend_e17])
 
AC_MSG_CHECKING([enable places udisks1 backend])
AC_ARG_ENABLE([places-udisks1],
  AS_HELP_STRING([--enable-places-udisks1],[enable places udisks1 backend with eldbus support @<:@default=disabled@:>@]),
    [e_cv_places_backend_udisks1=$enableval], [e_cv_places_backend_udisks1=no])
AC_MSG_RESULT([$e_cv_places_backend_udisks1]) 
 
 
e_cv_want_mount_udisks2=yes
if test "x$e_cv_places_backend_e17" = "xyes"; then
  if test "x$e_cv_places_backend_udisks1" = "xyes"; then
     AC_MSG_ERROR("Can not specify 2 udisks1 places backends", [255])
  fi
fi

AC_MSG_CHECKING([enable places udisks2 backend])  
AC_ARG_ENABLE([places-udisks2],
  AS_HELP_STRING([--enable-places-udisks2],[enable places udisks2 backend with eldbus support @<:@default=enabled@:>@]),
    [e_cv_places_backend_udisks2=yes], 
    [ 
      if test "x$e_cv_want_mount_udisks2" = "xyes" ; then
       e_cv_places_backend_udisks2=yes
      else
        e_cv_places_backend_udisks2=no
      fi ])
AC_MSG_RESULT([$e_cv_places_backend_udisks2]) 

UDISKS2_REQUIRED=2.7.2
    
if test "x$e_cv_places_backend_udisks2" = "xyes"; then
  if test -n "$force_places_backend_e17"  || test -n "$force_places_backend_udisks1"; then
    AC_MSG_ERROR("Can not specify more than one places backends", [255])
  fi
  e_cv_want_mount_udisks2=yes

  if test "x$e_cv_want_mount_udisks2" = "xyes"; then
    AC_DEFINE([PLACES_HAVE_UDISKS2], 1, [Define to 1 if libudisks2 is available])
  fi
fi

# doxygen program for documentation building

EFL_CHECK_DOXYGEN([build_doc="yes"], [build_doc="no"])

# explicit libs for each binary to reduce linkage requirements
PKG_CHECK_MODULES(E_REMOTE, [
  ecore >= ${efl_version}
  ecore-ipc >= ${efl_version}
  eet >= ${efl_version}
  eina >= ${efl_version}
])

PKG_CHECK_MODULES(E_START, [
  eina >= ${efl_version}
])

PKG_CHECK_MODULES(E_IMC, [
  ecore-file >= ${efl_version}
  eet >= ${efl_version}
  eina >= ${efl_version}
])

PKG_CHECK_MODULES(E_THUMB, [
  eina >= ${efl_version}
  evas >= ${efl_version}
  ecore >= ${efl_version}
  ecore-evas >= ${efl_version}
  edje >= ${efl_version}
  ecore-file >= ${efl_version}
  ecore-ipc >= ${efl_version}
  eet >= ${efl_version}
  efreet >= ${efl_version}
])

PKG_CHECK_MODULES(E_GRABBER, [
  eina >= ${efl_version}
  ecore >= ${efl_version}
  eet >= ${efl_version}
  efreet >= ${efl_version}
  ecore-x >= ${efl_version}
])

efm_requires="\
ecore >= $efl_version \
ecore-file >= $efl_version \
ecore-ipc >= $efl_version \
eet >= $efl_version \
efreet >= $efl_version \
eldbus >= $efl_version \
eina >= $efl_version"
efm_requires="$efm_requires $eeze_mount $dbus_mount"

PKG_CHECK_MODULES(E_FM, [$efm_requires])

PKG_CHECK_MODULES(E_FM_CMDLINE, [
  eina >= ${efl_version}
  ecore >= ${efl_version}
  ecore-file >= ${efl_version}
  dbus-1
])

PKG_CHECK_MODULES(E_FM_OP, [
  ecore >= ${efl_version}
  ecore-file >= ${efl_version}
  eina >= ${efl_version}
])

PKG_CHECK_MODULES(E_FM_OPEN, [
  eina >= ${efl_version}
  eet >= ${efl_version}
  ecore >= ${efl_version}
  ecore-ipc >= ${efl_version}
  ecore-con >= ${efl_version}
])

PKG_CHECK_MODULES(E_SYS, [
  eina >= ${efl_version}
])

PKG_CHECK_MODULES(E_INIT, [
  evas >= ${efl_version}
  ecore >= ${efl_version}
  ecore-evas >= ${efl_version}
  ecore-x >= ${efl_version}
  edje >= ${efl_version}
  ecore-ipc >= ${efl_version}
  eina >= ${efl_version}
])

PKG_CHECK_MODULES(E_ALERT, [
  xcb
  xcb-shape
  xcb-keysyms
  eina >= ${efl_version}
  ecore >= ${efl_version}
  ecore-ipc >= ${efl_version}
])

PKG_CHECK_MODULES(E, [
  evas >= ${efl_version}
  ecore >= ${efl_version}
  ecore-x >= ${efl_version}
  ecore-evas >= ${efl_version}
  ecore-input >= ${efl_version}
  ecore-input-evas >= ${efl_version}
  ecore-con >= ${efl_version}
  ecore-ipc >= ${efl_version}
  ecore-file >= ${efl_version}
  eet >= ${efl_version}
  edje >= ${efl_version}
  efreet >= ${efl_version}
  efreet-mime >= ${efl_version}
  efreet-trash >= ${efl_version}
  eina >= ${efl_version}
  dbus-1
  eldbus >= ${efl_version}
  eio >= ${efl_version}
  $hal_mount
  $eeze_mount
  $device_backend
])
requirements_e="\
evas >= ${efl_version} \
ecore >= ${efl_version} \
ecore-x >= ${efl_version} \
ecore-evas >= ${efl_version} \
ecore-con >= ${efl_version} \
ecore-ipc >= ${efl_version} \
ecore-file >= ${efl_version} \
eet >= ${efl_version} \
edje >= ${efl_version} \
efreet >= ${efl_version} \
efreet-mime >= ${efl_version} \
efreet-trash >= ${efl_version} \
eina >= ${efl_version} \
dbus-1 \
eldbus >= ${efl_version} \
eio >= ${efl_version} \
$hal_mount \
$eeze_mount \
$device_backend"


want_elementary=auto
have_elementary=no
AC_ARG_ENABLE(elementary,
  AS_HELP_STRING([--enable-elementary], [enable Elementary support @<:@default=detect@:>@]),
  [want_elementary=$enableval],
  [want_elementary=auto]
)
if test "x$want_elementary" != "xno"; then
   PKG_CHECK_MODULES(ELM, [elementary >= ${efl_version}],
                     [have_elementary=yes], [have_elementary=no])
   if test "x$want_elementary" = "xyes" -a "x$have_elementary" = "xno"; then
      AC_MSG_ERROR([Elementary support requested but it was not found])
   fi
   if test "x$have_elementary" = "xyes"; then
      AC_DEFINE([HAVE_ELEMENTARY], 1, "Have Elementary support")
      requirements_e="${requirements_e} elementary >= ${efl_version}"
   fi
fi
AM_CONDITIONAL([HAVE_ELEMENTARY], [test "x${want_elementary}" = "xyes"])

want_emotion=auto
have_emotion=no
AC_ARG_ENABLE(emotion,
  AS_HELP_STRING([--enable-emotion], [enable emotion support @<:@default=detect@:>@]),
  [want_emotion=$enableval],
  [want_emotion=auto]
)
if test "x$want_emotion" != "xno"; then
   PKG_CHECK_MODULES(EMOTION, [emotion >= ${efl_version}],
                     [have_emotion=yes], [have_emotion=no])
   if test "x$want_emotion" = "xyes" -a "x$have_emotion" = "xno"; then
      AC_MSG_ERROR([emotion support requested but it was not found])
   fi
   if test "x$have_emotion" = "xyes"; then
      AC_DEFINE([HAVE_EMOTION], 1, "Have emotion support")
      requirements_e="${requirements_e} emotion >= ${efl_version}"
   fi
fi

PKG_CHECK_MODULES(E_OPEN, [
  eina >= ${efl_version}
  ecore >= ${efl_version}
  ecore-file >= ${efl_version}
  efreet >= ${efl_version}
  efreet-mime >= ${efl_version}
])

AC_E_CHECK_PKG(ECORE_IMF, [ ecore-imf >= ${efl_version} ecore-imf-evas >= ${efl_version} ], [], [:])

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

PKG_CHECK_MODULES(EET, [eet >= ${efl_version}])
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
  AC_E_CHECK_PKG(BATTERY, [ ecore >= $efl_version ecore-file >= $efl_version ecore-con >= $efl_version eina >= $efl_version ], [], [BATTERY=false])
])

have_openbsd="no"
BATTERY_LDFLAGS=
case "$host_os" in
  darwin*)
    BATTERY_LDFLAGS="-framework Foundation -framework IOKit"
  ;;
  openbsd*)
    have_openbsd="yes"
  ;;
esac

UDISKS_REQUIRED=1.0.5
UDISKS2_REQUIRED=2.7.2
AM_CONDITIONAL(HAVE_PLACES, false)
define([CHECK_MODULE_PLACES],
[
  if test "x$e_cv_want_mount_udisks2" = "xyes"; then
    AC_E_CHECK_PKG(PLACES, [ eldbus >= $efl_version udisks2 >= $UDISKS2_REQUIRED ], [],
    [
    PLACES=false
    AC_MSG_NOTICE([Install libudisks2-dev or eldbus-dev or change places options])])
  fi
])

AC_SUBST(BATTERY_LDFLAGS)
AM_CONDITIONAL([HAVE_OPENBSD], [test "x${have_openbsd}" = "xyes"])

AM_CONDITIONAL(HAVE_TEMPERATURE, false)
define([CHECK_MODULE_TEMPERATURE],
[
  AC_E_CHECK_PKG(TEMPERATURE, [ ecore >= $efl_version ecore-file >= $efl_version eina >= $efl_version ], [], [TEMPERATURE=false])
])

have_enotify=no
AM_CONDITIONAL([HAVE_ENOTIFY], [false])
AC_ARG_ENABLE([enotify],
  AS_HELP_STRING([--disable-enotify], [disable Enotify support @<:@default=detect@:>@]),
  [e_cv_want_enotify=$enableval],
  AC_CACHE_VAL([e_cv_want_enotify], [e_cv_want_enotify=yes])
)
if test "x$e_cv_want_enotify" != "xno" ; then
  AC_E_CHECK_PKG(ENOTIFY, [ eldbus >= $efl_version ],
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

AM_CONDITIONAL(HAVE_NOTIFICATION, false)
define([CHECK_MODULE_NOTIFICATION],
[
  if test "x$e_cv_want_enotify" = "xno" ; then
    NOTIFICATION=false
  fi
])

AM_CONDITIONAL(HAVE_ALSA, false)
define([CHECK_MODULE_MIXER],
[
  AC_E_CHECK_PKG(MIXER, [ libpulse-simple libpulse ], [], [MIXER=false])
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

have_wayland_clients=no
AC_ARG_ENABLE([wayland-clients],
  AS_HELP_STRING([--enable-wayland-clients],[enable wayland clients in composite module @<:@default=disabled@:>@]),
  [e_cv_want_wayland_clients=$enableval],
  [e_cv_want_wayland_clients=no])

AC_MSG_CHECKING([whether wayland client support is enabled])
AC_MSG_RESULT([${e_cv_want_wayland_clients}])

if test "x$e_cv_want_wayland_clients" != "xno";then
  PKG_CHECK_MODULES([WAYLAND], [wayland-server egl >= 7.10 glesv2 gl pixman-1 xcb-image],
    [
      have_wayland_clients=yes
      AC_DEFINE_UNQUOTED([HAVE_WAYLAND_CLIENTS],[1],[enable wayland client support])
    ],
    [have_wayland_clients=no])
else
  have_wayland_clients=no
fi

AM_CONDITIONAL([HAVE_WAYLAND_CLIENTS], [test "x${have_wayland_clients}" = "xyes"])


AC_E_OPTIONAL_MODULE([ibar], true)
AC_E_OPTIONAL_MODULE([clock], true)
AC_E_OPTIONAL_MODULE([pager], true)
AC_E_OPTIONAL_MODULE([battery], true, [CHECK_MODULE_BATTERY])
AC_E_OPTIONAL_MODULE([temperature], true, [CHECK_MODULE_TEMPERATURE])
AC_E_OPTIONAL_MODULE([notification], true, [CHECK_MODULE_NOTIFICATION])
AC_E_OPTIONAL_MODULE([cpufreq], true)
AC_E_OPTIONAL_MODULE([ibox], true)
AC_E_OPTIONAL_MODULE([start], true)
AC_E_OPTIONAL_MODULE([winlist], true)
AC_E_OPTIONAL_MODULE([fileman], true)
AC_E_OPTIONAL_MODULE([wizard], true)
AC_E_OPTIONAL_MODULE([conf], true)
AC_E_OPTIONAL_MODULE([conf_wallpaper2], false)
AC_E_OPTIONAL_MODULE([conf_theme], true, true)
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
AC_E_OPTIONAL_MODULE([syscon], true)
AC_E_OPTIONAL_MODULE([everything], true)
AC_E_OPTIONAL_MODULE([systray], true)
AC_E_OPTIONAL_MODULE([quickaccess], true)
AC_E_OPTIONAL_MODULE([shot], true)
AC_E_OPTIONAL_MODULE([backlight], true)
AC_E_OPTIONAL_MODULE([tasks], true)
AC_E_OPTIONAL_MODULE([conf_randr], false)
AC_E_OPTIONAL_MODULE([xkbswitch], true)
AC_E_OPTIONAL_MODULE([tiling], true)
AC_E_OPTIONAL_MODULE([access], false)
AC_E_OPTIONAL_MODULE([clipboard], true)
AC_E_OPTIONAL_MODULE([places], true,[CHECK_MODULE_PLACES])
AC_E_OPTIONAL_MODULE([classicmenu], true)

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
HALT="/sbin/shutdown -h now"
REBOOT="/sbin/shutdown -r now"
SUSPEND=""
HIBERNATE=""
case "$host_os" in
   freebsd*|pcbsd*)
      SUSPEND="acpiconf -s3"
      AC_MSG_NOTICE([use suspend method: $SUSPEND])
      HIBERNATE="acpiconf -s4"
      AC_MSG_NOTICE([use hibernate method: $HIBERNATE])
      ;;
   *)
      if test "x${have_systemd_user_session}" = "xyes"; then
         HALT="/usr/bin/systemctl poweroff"
         REBOOT="/usr/bin/systemctl reboot"
         SUSPEND="/usr/bin/systemctl suspend"
         HIBERNATE="/usr/bin/systemctl hibernate"
         AC_MSG_NOTICE([use systemctl for halt, reboot, suspend and hibernate])
      else
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
      fi
      ;;
esac

AC_SUBST(HALT)
AC_SUBST(REBOOT)
AC_SUBST(SUSPEND)
AC_SUBST(HIBERNATE)

m4_ifdef([v_mic],
   [
    EFL_COMPILER_FLAG([-Wshadow])
    EFL_COMPILER_FLAG([-Wall])
    EFL_COMPILER_FLAG([-W])
   ])

SUID_CFLAGS=-fPIE
SUID_LDFLAGS=-pie
AC_SUBST([SUID_CFLAGS])
AC_SUBST([SUID_LDFLAGS])

AC_CONFIG_FILES([
Makefile
enlightenment.spec
enlightenment.pc
src/Makefile
src/bin/Makefile
src/bin/e_fm_shared_types.h
src/bin/e_fm/Makefile
src/modules/Makefile
src/modules/ibar/module.desktop
src/modules/clock/module.desktop
src/modules/pager/module.desktop
src/modules/battery/module.desktop
src/modules/temperature/module.desktop
src/modules/notification/module.desktop
src/modules/cpufreq/module.desktop
src/modules/ibox/module.desktop
src/modules/start/module.desktop
src/modules/winlist/module.desktop
src/modules/fileman/module.desktop
src/modules/conf/module.desktop
src/modules/conf_wallpaper2/module.desktop
src/modules/conf_theme/module.desktop
src/modules/conf_intl/module.desktop
src/modules/conf_applications/module.desktop
src/modules/conf_display/module.desktop
src/modules/conf_shelves/module.desktop
src/modules/conf_keybindings/module.desktop
src/modules/conf_edgebindings/module.desktop
src/modules/conf_window_remembers/module.desktop
src/modules/conf_window_manipulation/module.desktop
src/modules/conf_menus/module.desktop
src/modules/conf_dialogs/module.desktop
src/modules/conf_performance/module.desktop
src/modules/conf_paths/module.desktop
src/modules/conf_interaction/module.desktop
src/modules/msgbus/module.desktop
src/modules/conf_randr/module.desktop
src/modules/gadman/module.desktop
src/modules/mixer/module.desktop
src/modules/connman/module.desktop
src/modules/syscon/module.desktop
src/modules/everything/module.desktop
src/modules/everything/everything.pc
src/modules/systray/module.desktop
src/modules/quickaccess/module.desktop
src/modules/shot/module.desktop
src/modules/backlight/module.desktop
src/modules/tasks/module.desktop
src/modules/xkbswitch/module.desktop
src/modules/tiling/module.desktop
src/modules/access/module.desktop
src/modules/clipboard/module.desktop
src/modules/places/module.desktop
src/modules/classicmenu/module.desktop
data/Makefile
data/images/Makefile
data/flags/Makefile
data/favorites/Makefile
data/desktop/Makefile
data/input_methods/Makefile
data/themes/Makefile
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
config/bodhi/Makefile
$po_makefile_in
])
AC_OUTPUT

# report
txt_strip() {
  echo "[$]@" | sed -e 's/^[[ \t]]*\([[^ \t]]*\)[[ \t]]*$/\1/g'
}

echo
echo "Summary:"
echo "   Enlightenment build profile set to $profile"
cat << SUMMARY_EOF
 * project.........: $PACKAGE $VERSION
 * prefix..........: $(txt_strip $prefix)
 * CFLAGS..........: $(txt_strip $CFLAGS)
 * LDFLAGS.........: $(txt_strip $LDFLAGS)
 * EFL.............: $EFL_VERSION
SUMMARY_EOF
echo
cat << DEVICE_EOF
Preferred Backends:
 * device...............: $device_backend
 * hal_mount............: $e_cv_want_mount_hal
 * udisks2_mount........: $e_cv_want_mount_udisks2
 * eeze_mount...........: $e_cv_want_mount_eeze
DEVICE_EOF
echo

cat << WIZARD_EOF
Install Wizard Desktop Files
 * disabled........: $e_cv_enable_install_wizard_desktop
WIZARD_EOF
echo

UNUSED_MODS=""
for mod in $UNUSED_OPTIONAL_MODULES; do
  UNUSED_MODS="$UNUSED_MODS$mod "
done

cat << MODULES_EOF
Modules:
 * disabled........: $(txt_strip $UNUSED_MODS)
MODULES_EOF
echo
echo "Places Backends":
if [[ "$UNUSED_MODS" == *"places"* ]]; then
 echo "  Module disabled"
 echo
fi
if test "x$e_cv_want_mount_udisks2" = "xyes"; then
   echo "* udisks2 backend with eldbus"
fi
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
