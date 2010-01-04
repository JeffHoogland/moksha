dnl Check for PATH_MAX in limits.h, and define a default value if not found
dnl This is a workaround for systems not providing PATH_MAX, like GNU/Hurd

dnl EFL_CHECK_PATH_MAX([DEFAULT_VALUE_IF_NOT_FOUND])
dnl
dnl If PATH_MAX is not defined in <limits.h>, defines it
dnl to DEFAULT_VALUE_IF_NOT_FOUND if it exists, or fallback
dnl to using 4096

AC_DEFUN([EFL_CHECK_PATH_MAX],
[

default_max=m4_default([$1], "4096")
AC_LANG_PUSH([C])

AC_MSG_CHECKING([for PATH_MAX in limit.h])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([[#include <limits.h>]],
	                 [[int i = PATH_MAX]])
	],
	AC_MSG_RESULT([yes]),
	[
	  AC_DEFINE_UNQUOTED([PATH_MAX],
	                     [${default_max}],
	                     [default value since PATH_MAX is not defined])
	  AC_MSG_RESULT([no: using ${default_max}])
	]
)

AC_LANG_POP([C])

])
dnl end of efl_path_max.m4
