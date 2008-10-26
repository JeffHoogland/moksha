# get rid of that stupid cache mechanism
rm -f config.cache

AC_INIT(enlightenment, 0.16.999.050, enlightenment-devel@lists.sourceforge.net)
AC_PREREQ(2.52)
AC_CONFIG_SRCDIR(configure.ac)
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_ISC_POSIX

AM_INIT_AUTOMAKE(1.6 dist-bzip2)
AM_CONFIG_HEADER(config.h)

AC_C_BIGENDIAN
AC_PROG_CC
AM_PROG_CC_STDC
AM_PROG_CC_C_O
AC_HEADER_STDC
AC_C_CONST
AM_ENABLE_SHARED
AM_PROG_LIBTOOL
AC_FUNC_ALLOCA
AC_C___ATTRIBUTE__

AC_CHECK_FUNCS(setenv)
AC_CHECK_FUNCS(unsetenv)

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
AC_CHECK_HEADERS(security/pam_appl.h, [
    LIBS="$LIBS -lpam"
    have_pam=yes
  AC_DEFINE(HAVE_PAM, 1, [PAM Authentication Support])])

# Setting have_valgrind to "no" seems pointless, but we just need to
# put something in as the 4th parameter, so configure doesn't abort
# when valgrind.pc isn't found.
#have_valgrind="no"
#PKG_CHECK_MODULES(VALGRIND, valgrind >= 2.4.0, have_valgrind=yes, have_valgrind=no)
#AC_ARG_ENABLE(valgrind,
#  [  --enable-valgrind               enable valgrind support], [
#      if test x"$enableval" = x"yes" ; then
#        AC_MSG_RESULT(yes)
#        have_valgrind="yes"
#      else
#        AC_MSG_RESULT(no)
#	have_valgrind="no"
#        VALGRIND_CFLAGS=""
#        VALGRIND_LIBS=""
#      fi
#  ], [
#    have_valgrind=$have_valgrind
#  ]
#)

#if test x$have_valgrind = "xyes"; then
#  AC_DEFINE(HAVE_VALGRIND, 1, [Valgrind support])
#fi

AC_SUBST(VALGRIND_CFLAGS)
AC_SUBST(VALGRIND_LIBS)

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

#ALL_LINGUAS="bg de eo es fi fr ja pl pt ru zh_CN hu sl it cs da sk sv nb nl zh_TW ko"
#ALL_LINGUAS="bg de eo es fi fr ja pt ru zh_CN hu sl it cs da sk sv nb nl zh_TW"
# a lot of .po's are broken
ALL_LINGUAS="bg de eo es fi fr ja ru zh_CN hu sl it zh_TW ko pt_BR ca fr_CH"
AC_SUBST(ALL_LINGUAS)

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.12.1])
if test "x$LIBINTL" = "x"; then
  LIBINTL="$INTLLIBS"
fi

AC_CHECK_LIB(dl, dlopen, dlopen_libs=-ldl)
AC_SUBST(dlopen_libs)

AC_MSG_CHECKING([whether to enable Files menu item])
AC_ARG_ENABLE([files],
   AS_HELP_STRING([--enable-files],[enable Files menu item @<:@yes@:>@]),
   [e_cv_enable_files=$enableval],
   AC_CACHE_VAL([e_cv_enable_files], [e_cv_enable_files=yes]))
if test "[${e_cv_enable_files}]" = yes; then
   AC_DEFINE_UNQUOTED([ENABLE_FILES],[1],[enable Files menu item])
fi
AC_MSG_RESULT([$e_cv_enable_files])

PKG_CHECK_MODULES(E, [
  evas
  ecore
  ecore-x
  ecore-evas
  ecore-con
  ecore-ipc
  ecore-job
  ecore-file
  eet >= 1.0.1
  edje
  efreet
  efreet-mime
  eina-0
])
requirements_e="evas ecore ecore-x ecore-evas ecore-con ecore-ipc ecore-job ecore-file eet edje efreet efreet-mime eina-0"

PKG_CHECK_MODULES(ECORE_IMF, [
  ecore-imf
  ecore-imf-evas
], [
  have_ecore_imf="yes"
  AC_DEFINE(HAVE_ECORE_IMF, 1, [Ecore IMF support])
], [ have_ecore_imf="no" ]
)

PKG_CHECK_MODULES(ECORE_TXT, [
  ecore-txt
], [
  have_temp="yes"
], [ have_temp="no"])

AM_CONDITIONAL(HAVE_TEMPERATURE, test x$have_temp = "xyes")
if test x$have_temp = "xyes"; then
  PKG_CHECK_MODULES(TEMPERATURE, [
    ecore
    ecore-file
  ])
fi

PKG_CHECK_MODULES(BATTERY, [
  ecore
  ecore-file
  ecore-con
])

PKG_CHECK_MODULES(E_DBUS, edbus, have_edbus=yes, have_edbus=no)
PKG_CHECK_MODULES(E_HAL, ehal, have_ehal=yes, have_ehal=no)

# this is used to conditionally build modules which depend on E_DBUS
AM_CONDITIONAL(HAVE_EDBUS, test x$have_edbus = "xyes")
define_e_dbus=""
if test x$have_edbus = "xyes"; then
  define_e_dbus="-DHAVE_EDBUS"
  requirements_e=$requirements_e" edbus"
  if test x$have_ehal = "xyes"; then
    AC_DEFINE(HAVE_EDBUS, 1, [E_Dbus support])
  fi
fi

e_libs=$E_LIBS" "$LIBINTL" "$fnmatch_libs" "$ECORE_IMF_LIBS" "
e_cflags="-DUSE_E_CONFIG_H "$define_e_dbus" "$E_CFLAGS" "$E_DBUS_CFLAGS" "$ECORE_IMF_CFLAGS" "
e_configflags="-DUSE_E_CONFIG_H "$define_e_dbus" "

AC_SUBST(e_libs)
AC_SUBST(e_cflags)
AC_SUBST(e_configflags)
AC_SUBST(requirements_e)

profile="SLOW_PC"
AC_ARG_WITH(profile,
[  --with-profile=TARGET_PROFILE    specify a target format profile of:
                                    LOWRES_PDA, MEDIUMRES_PDA, HIRES_PDA, 
				    SLOW_PC, MEDIUM_PC, FAST_PC ],
[
  v=$withval;
  profile=$v
  echo "  Enlightenment build profile set to "$profile;
],[
  profile="SLOW_PC"
])
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

PKG_CHECK_MODULES(EET, [eet >= 1.0.1])
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


dnl Check for Alsa
AC_ARG_ENABLE(alsa, AS_HELP_STRING([--enable-alsa],
		    [enable support for alsa(default=autodetect)]),
		    [ enable_alsa=$enableval ], [ enable_alsa=default ])

AM_CONDITIONAL(HAVE_ALSA, false)
if test "x$enable_alsa" = "xdefault" || test "x$enable_alsa" = "xyes"; then
   PKG_CHECK_MODULES(ALSA, [alsa >= 1.0.8],
   [ SOUND_CFLAGS="$ALSA_CFLAGS -DHAVE_ALSA $SOUND_CFLAGS"
     SOUND_LDFLAGS="$ALSA_LIBS $SOUND_LDFLAGS"
     AM_CONDITIONAL(HAVE_ALSA, true)
     have_alsa=yes ],
     [ if test "x$enable_alsa" = "xyes"; then
          AC_MSG_ERROR([alsa library >= 1.0.8 not found])
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

AC_SUBST(SOUND_CFLAGS)
AC_SUBST(SOUND_LDFLAGS)

AC_OUTPUT([
Makefile
enlightenment.spec
enlightenment.pc
src/Makefile
src/bin/Makefile
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
src/modules/exebuf/Makefile
src/modules/exebuf/module.desktop
src/modules/winlist/Makefile
src/modules/winlist/module.desktop
src/modules/fileman/Makefile
src/modules/fileman/module.desktop
src/modules/wizard/Makefile
src/modules/wizard/module.desktop
src/modules/conf/Makefile
src/modules/conf/module.desktop
src/modules/conf_wallpaper/Makefile
src/modules/conf_wallpaper/module.desktop
src/modules/conf_theme/Makefile
src/modules/conf_theme/module.desktop
src/modules/conf_colors/Makefile
src/modules/conf_colors/module.desktop
src/modules/conf_fonts/Makefile
src/modules/conf_fonts/module.desktop
src/modules/conf_borders/Makefile
src/modules/conf_borders/module.desktop
src/modules/conf_icon_theme/Makefile
src/modules/conf_icon_theme/module.desktop
src/modules/conf_mouse_cursor/Makefile
src/modules/conf_mouse_cursor/module.desktop
src/modules/conf_transitions/Makefile
src/modules/conf_transitions/module.desktop
src/modules/conf_startup/Makefile
src/modules/conf_startup/module.desktop
src/modules/conf_intl/Makefile
src/modules/conf_intl/module.desktop
src/modules/conf_imc/Makefile
src/modules/conf_imc/module.desktop
src/modules/conf_profiles/Makefile
src/modules/conf_profiles/module.desktop
src/modules/msgbus_lang/Makefile
src/modules/msgbus_lang/module.desktop
src/modules/conf_engine/Makefile
src/modules/conf_engine/module.desktop
src/modules/conf_applications/Makefile
src/modules/conf_applications/module.desktop
src/modules/conf_desks/Makefile
src/modules/conf_desks/module.desktop
src/modules/conf_desk/Makefile
src/modules/conf_desk/module.desktop
src/modules/conf_display/Makefile
src/modules/conf_display/module.desktop
src/modules/conf_desklock/Makefile
src/modules/conf_desklock/module.desktop
src/modules/conf_screensaver/Makefile
src/modules/conf_screensaver/module.desktop
src/modules/conf_dpms/Makefile
src/modules/conf_dpms/module.desktop
src/modules/conf_shelves/Makefile
src/modules/conf_shelves/module.desktop
src/modules/conf_keybindings/Makefile
src/modules/conf_keybindings/module.desktop
src/modules/conf_mousebindings/Makefile
src/modules/conf_mousebindings/module.desktop
src/modules/conf_mouse/Makefile
src/modules/conf_mouse/module.desktop
src/modules/conf_window_display/Makefile
src/modules/conf_window_display/module.desktop
src/modules/conf_window_focus/Makefile
src/modules/conf_window_focus/module.desktop
src/modules/conf_window_remembers/Makefile
src/modules/conf_window_remembers/module.desktop
src/modules/conf_window_manipulation/Makefile
src/modules/conf_window_manipulation/module.desktop
src/modules/conf_menus/Makefile
src/modules/conf_menus/module.desktop
src/modules/conf_clientlist/Makefile
src/modules/conf_clientlist/module.desktop
src/modules/conf_dialogs/Makefile
src/modules/conf_dialogs/module.desktop
src/modules/conf_performance/Makefile
src/modules/conf_performance/module.desktop
src/modules/conf_winlist/Makefile
src/modules/conf_winlist/module.desktop
src/modules/conf_exebuf/Makefile
src/modules/conf_exebuf/module.desktop
src/modules/conf_paths/Makefile
src/modules/conf_paths/module.desktop
src/modules/conf_mime/Makefile
src/modules/conf_mime/module.desktop
src/modules/conf_interaction/Makefile
src/modules/conf_interaction/module.desktop
src/modules/conf_scale/Makefile
src/modules/conf_scale/module.desktop
src/modules/gadman/Makefile
src/modules/gadman/module.desktop
src/modules/mixer/Makefile
src/modules/mixer/module.desktop
src/modules/connman/Makefile
src/modules/connman/module.desktop
src/preload/Makefile
data/Makefile
data/images/Makefile
data/input_methods/Makefile
data/themes/Makefile
data/themes/images/Makefile
data/other/Makefile
data/xsession/Makefile
data/xsession/enlightenment.desktop
data/etc/Makefile
data/icons/Makefile
data/backgrounds/Makefile
doc/Makefile
config/Makefile
config/default/Makefile
config/standard/Makefile
config/minimalist/Makefile
config/scaleable/Makefile
config/netbook/Makefile
po/Makefile.in
])
