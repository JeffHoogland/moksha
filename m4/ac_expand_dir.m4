dnl AC_EXPAND_DIR(VARNAME, DIR)
dnl expands occurrences of ${prefix} and ${exec_prefix} in the given DIR,
dnl and assigns the resulting string to VARNAME
dnl example: AC_DEFINE_DIR(DATADIR, "$datadir")
dnl by Alexandre Oliva <oliva@dcc.unicamp.br>
AC_DEFUN([AC_EXPAND_DIR], [
	$1=$2
	$1=`(
	    test "x$prefix" = xNONE && prefix="$ac_default_prefix"
	    test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
	    eval echo \""[$]$1"\"
	    )`
])

