dnl _XTERM_COLORS
define([_XTERM_COLORS],
[
        # Check for XTerm and define some colors
        if test "x$TERM" = "xxterm" -o "x$TERM" = "xscreen"; then
                COLOR_PREF="\0033\0133"
                COLOR_H="${COLOR_PREF}1m"
                COLOR_HGREEN="${COLOR_PREF}1;32m"
                COLOR_HRED="${COLOR_PREF}1;31m"
                COLOR_GREEN="${COLOR_PREF}32m"
                COLOR_RED="${COLOR_PREF}31m"
                COLOR_YELLOW="${COLOR_PREF}1;33m"
                COLOR_END="${COLOR_PREF}0m"
        else
                COLOR_H=""
                COLOR_HGREEN=""
                COLOR_HRED=""
                COLOR_GREEN=""
                COLOR_RED=""
                COLOR_YELLOW=""
                COLOR_END=""
        fi
])

dnl AC_E_CHECK_PKG(name, lib [>= version], [action-if, [action-not]])
dnl   improved version of PKG_CHECK_MODULES, it does the same checking
dnl   and defines HAVE_[name]=yes/no and also exports
dnl   [name]_CFLAGS and [name]_LIBS.
dnl
dnl   if action-not isn't provided, AC_MSG_ERROR will be used.
dnl
dnl   Checks:
dnl       lib >= version
dnl
dnl   Provides:
dnl       - HAVE_[name]=yes|no
dnl       - [name]_CFLAGS: if HAVE_[name]=yes
dnl       - [name]_LIBS: if HAVE_[name]=yes
dnl       - [name]_VERSION: if HAVE_[name]=yes
dnl
AC_DEFUN([AC_E_CHECK_PKG],
[
# ----------------------------------------------------------------------
# BEGIN: Check library with pkg-config: $1 (pkg-config=$2)
#

        PKG_CHECK_MODULES([$1], [$2],
                          [
                                HAVE_[$1]=yes
                                [pkg_name]=$(echo "[$2]" | cut -d\   -f1)
                                [$1]_VERSION=$($PKG_CONFIG --modversion $pkg_name)
                                AC_SUBST([$1]_VERSION)
                                ifelse([$3], , :, [$3])
                          ],
                          [
                                HAVE_[$1]=no
                                ifelse([$4], , AC_MSG_ERROR(you need [$2] development installed!), AC_MSG_RESULT(no); [$4])
                          ])
        AM_CONDITIONAL(HAVE_[$1], test x$HAVE_[$1] = xyes)
        AC_SUBST(HAVE_[$1])
        if test x$HAVE_[$1] = xyes; then
                AC_DEFINE_UNQUOTED(HAVE_[$1], 1, Package [$1] ($2) found.)
        fi

#
# END: Check library with pkg-config: $1 (pkg-config=$2)
# ----------------------------------------------------------------------
])

dnl AC_E_OPTIONAL_MODULE(name, [initial-status, [check-if-enabled]])
dnl   Defines configure argument --<enable|disable>-[name] to enable an
dnl   optional module called 'name'.
dnl
dnl   If initial-status is true, then it's enabled by default and option
dnl   will be called --disable-[name], otherwise it's disabled and option
dnl   is --enable-[name].
dnl
dnl   If module is enabled, then check-if-enabled will be executed. This
dnl   may change the contents of shell variable NAME (uppercase version of
dnl   name, with underscores instead of dashed) to something different than
dnl   "true" to disable module.
dnl
dnl Parameters:
dnl     - name: module name to use. It will be converted to have dashes (-)
dnl          instead of underscores, and will be in lowercase.
dnl     - initial-status: true or false, states if module is enabled or
dnl          disabled by default.
dnl     - check-if-enabled: macro to be expanded inside check for enabled
dnl           module.
dnl
dnl Provides:
dnl     - USE_MODULE_[name]=true|false [make, shell]
dnl     - USE_MODULE_[name]=1 if enabled [config.h]
dnl
AC_DEFUN([AC_E_OPTIONAL_MODULE],
[
# ----------------------------------------------------------------------
# BEGIN: Check for optional module: $1 (default: $2)
#
        m4_pushdef([MODNAME], [m4_bpatsubst(m4_toupper([$1]), -, _)])dnl
        m4_pushdef([modname_opt], [m4_bpatsubst(m4_tolower([$1]), _, -)])
        m4_pushdef([INITVAL], [m4_default([$2], [false])])dnl
        m4_pushdef([ENABLE_HELP], AS_HELP_STRING([--enable-modname_opt],
                               [enable optional module modname_opt. Default is disabled.])
                )dnl
        m4_pushdef([DISABLE_HELP], AS_HELP_STRING([--disable-modname_opt],
                               [disable optional module modname_opt. Default is enabled.])
                )dnl
        m4_pushdef([HELP_STR], m4_if(INITVAL, [true], [DISABLE_HELP], [ENABLE_HELP]))dnl
        m4_pushdef([NOT_INITVAL], m4_if(INITVAL, [true], [false], [true]))dnl

        USING_MODULES=1

        MODNAME=INITVAL
        AC_ARG_ENABLE(modname_opt, HELP_STR, [MODNAME=${enableval:-NOT_INITVAL}])
        if test x[$]MODNAME = xyes || test x[$]MODNAME = x1; then
                MODNAME=true
        fi
        if test x[$]MODNAME = xno || test x[$]MODNAME = x0; then
                MODNAME=false
        fi

        USE_MODULE_[]MODNAME=[$]MODNAME

        _XTERM_COLORS

        # Check list for optional module $1
        if test x[$]MODNAME = xtrue; then
                ifelse([$3], , , [
echo
echo "checking optional module modname_opt:"
# BEGIN: User checks
$3
# END: User checks
if test x[$]MODNAME = xfalse; then
        echo -e "optional module modname_opt ${COLOR_HRED}failed${COLOR_END} checks."
else
        echo -e "optional module modname_opt passed checks."
fi
echo
])

                if test x[$]MODNAME = xfalse; then
                        echo -e "${COLOR_YELLOW}Warning:${COLOR_END} optional module ${COLOR_H}modname_opt${COLOR_END} disabled by extra checks."
                fi
        fi

        # Check if user checks succeeded
        if test x[$]MODNAME = xtrue; then
                [OPTIONAL_MODULES]="$[OPTIONAL_MODULES] modname_opt"
                AC_DEFINE_UNQUOTED(USE_MODULE_[]MODNAME, 1, Use module modname_opt)
        else
                [UNUSED_OPTIONAL_MODULES]="$[UNUSED_OPTIONAL_MODULES] modname_opt"
        fi

        AM_CONDITIONAL(USE_MODULE_[]MODNAME, test x[$]MODNAME = xtrue)
        AC_SUBST(USE_MODULE_[]MODNAME)

        m4_popdef([HELP_STR])dnl
        m4_popdef([DISABLE_HELP])dnl
        m4_popdef([ENABLE_HELP])dnl
        m4_popdef([INITVAL])dnl
        m4_popdef([MODNAME])
#
# END: Check for optional module: $1 ($2)
# ----------------------------------------------------------------------
])
