dnl Copyright (C) 2010 Vincent Torri <vtorri at univ-evry dot fr>
dnl                and Albin Tonnerre <albin dot tonnerre at gmail dot com>
dnl That code is public domain and can be freely used or copied.

dnl Macro that checks if a compiler flag is supported by the compiler.

dnl Usage: EFL_COMPILER_FLAG(flag)
dnl flag is added to CFLAGS if supported.

AC_DEFUN([EFL_COMPILER_FLAG],
[

CFLAGS_save="${CFLAGS}"
CFLAGS="${CFLAGS} $1"
  
AC_LANG_PUSH([C])
AC_MSG_CHECKING([whether the compiler supports $1])

AC_COMPILE_IFELSE(
   [AC_LANG_PROGRAM([[]])],
   [have_flag="yes"],
   [have_flag="no"])
AC_MSG_RESULT([${have_flag}])

if test "x${have_flag}" = "xno" ; then
   CFLAGS="${CFLAGS_save}"
fi
AC_LANG_POP([C])

])

dnl Macro that checks if a linker flag is supported by the compiler.

dnl Usage: EFL_LINKER_FLAG(flag)
dnl flag is added to LDFLAGS if supported (will be passed to ld anyway).

AC_DEFUN([EFL_LINKER_FLAG],
[

LDFLAGS_save="${LDFLAGS}"
LDFLAGS="${LDFLAGS} $1"
  
AC_LANG_PUSH([C])
AC_MSG_CHECKING([whether the compiler supports $1])

AC_LINK_IFELSE(
   [AC_LANG_PROGRAM([[]])],
   [have_flag="yes"],
   [have_flag="no"])
AC_MSG_RESULT([${have_flag}])

if test "x${have_flag}" = "xno" ; then
   LDFLAGS="${LDFLAGS_save}"
fi
AC_LANG_POP([C])

])
