
AC_DEFUN([AC_C___ATTRIBUTE__],
[
  AC_MSG_CHECKING(for __attribute__)
  AC_CACHE_VAL(ac_cv___attribute__, [
  AC_TRY_COMPILE([#include <stdlib.h>],
  [int func(int x); int foo(int x __attribute__ ((unused))) { exit(1); }],
  ac_cv___attribute__=yes, ac_cv___attribute__=no)])
  if test "$ac_cv___attribute__" = "yes"; then
    AC_DEFINE(HAVE___ATTRIBUTE__, 1, [Define to 1 if your compiler has __attribute__])
  fi
  AC_MSG_RESULT($ac_cv___attribute__)
])

