dnl Process this file with autoconf to produce a configure script.

AC_INIT
AC_CONFIG_SRCDIR([configure.in])

ENLIGHTENMENT_MAJOR=0
ENLIGHTENMENT_MINOR=17
ENLIGHTENMENT_MICRO=pre_0
ENLIGHTENMENT_VERSION=$ENLIGHTENMENT_MAJOR.$ENLIGHTENMENT_MINOR.$ENLIGHTENMENT_MICRO

VERSION=$ENLIGHTENMENT_VERSION

AM_INIT_AUTOMAKE(enlightenment, $VERSION)
AM_CONFIG_HEADER(config.h)

AC_DEFINE_UNQUOTED(ENLIGHTENMENT_VERSION, "$ENLIGHTENMENT_VERSION")
AC_SUBST(ENLIGHTENMENT_VERSION)
AC_DEFINE_UNQUOTED(ENLIGHTENMENT_MAJOR, "$ENLIGHTENMENT_MAJOR")
AC_SUBST(ENLIGHTENMENT_MAJOR)
AC_DEFINE_UNQUOTED(ENLIGHTENMENT_MINOR, "$ENLIGHTENMENT_MINOR")
AC_SUBST(ENLIGHTENMENT_MINOR)
AC_DEFINE_UNQUOTED(ENLIGHTENMENT_MICRO, "$ENLIGHTENMENT_MICRO")
AC_SUBST(ENLIGHTENMENT_MICRO)

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_C_CONST
AM_ENABLE_SHARED
AM_PROG_LIBTOOL

AM_WITH_DMALLOC

dnl Add the languages which your application supports here.
ALL_LINGUAS=""
AM_GNU_GETTEXT

dnl Set PACKAGE_LOCALE_DIR in config.h.
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${ac_default_prefix}/${DATADIRNAME}/locale")
else
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${prefix}/${DATADIRNAME}/locale")
fi

dnl Set PACKAGE_DATA_DIR in config.h.
if test "x${datadir}" = 'x${prefix}/share'; then
  if test "x${prefix}" = "xNONE"; then
    AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${ac_default_prefix}/share/${PACKAGE}")
  else
    AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${prefix}/share/${PACKAGE}")
  fi
else
  AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${datadir}/${PACKAGE}")
fi

dnl Set PACKAGE_BIN_DIR in config.h.
if test "x${datadir}" = 'x${prefix}/bin'; then
  if test "x${prefix}" = "xNONE"; then
    AC_DEFINE_UNQUOTED(PACKAGE_BIN_DIR, "${ac_default_prefix}/bin")
  else
    AC_DEFINE_UNQUOTED(PACKAGE_BIN_DIR, "${prefix}/bin")
  fi
else
  AC_DEFINE_UNQUOTED(PACKAGE_BIN_DIR, "${bindir}")
fi

dnl Set PACKAGE_LIB_DIR in config.h.
if test "x${datadir}" = 'x${prefix}/lib'; then
  if test "x${prefix}" = "xNONE"; then
    AC_DEFINE_UNQUOTED(PACKAGE_LIB_DIR, "${ac_default_prefix}/lib")
  else
    AC_DEFINE_UNQUOTED(PACKAGE_LIB_DIR, "${prefix}/lib")
  fi
else
  AC_DEFINE_UNQUOTED(PACKAGE_LIB_DIR, "${libdir}")
fi

dnl Set PACKAGE_SOURCE_DIR in config.h.
packagesrcdir=`cd $srcdir && pwd`
AC_DEFINE_UNQUOTED(PACKAGE_SOURCE_DIR, "${packagesrcdir}")

dnl Use -Wall if we have gcc.
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl

dnl Check for Imlib2.
AC_ARG_WITH(imlib2,
        [  --with-imlib2=DIR          use imlib2 in <DIR>],
        [CFLAGS="$CFLAGS -I$withval/include"
         LIBS="-L$withval/lib $LIBS"])

AC_PATH_GENERIC(imlib2,, [
    AC_SUBST(imlib2_libs)
    AC_SUBST(imlib2_cflags)],
    AC_MSG_ERROR(Cannot find imlib2: Is imlib2-config in path?))
imlib2_libs=`imlib2-config --libs`
imlib2_cflags=`imlib2-config --cflags`

dnl Check for Evas.
AC_ARG_WITH(evas,
        [  --with-evas=DIR          use evas in <DIR>],
        [CFLAGS="$CFLAGS -I$withval/include"
         LIBS="-L$withval/lib $LIBS"])

AC_PATH_GENERIC(evas,, [
    AC_SUBST(evas_libs)
    AC_SUBST(evas_cflags)],
    AC_MSG_ERROR(Cannot find evas: Is evas-config in path?))
evas_libs=`evas-config --libs`
evas_cflags=`evas-config --cflags`

dnl Check for Edb.
AC_ARG_WITH(edb,
        [  --with-edb=DIR          use edb in <DIR>],
        [CFLAGS="$CFLAGS -I$withval/include"
         LIBS="-L$withval/lib $LIBS"])

AC_PATH_GENERIC(edb,, [
    AC_SUBST(edb_libs)
    AC_SUBST(edb_cflags)],
    AC_MSG_ERROR(Cannot find edb: Is edb-config in path?))
edb_libs=`edb-config --libs`
edb_cflags=`edb-config --cflags`

dnl Check for Ebits.
AC_ARG_WITH(ebits,
        [  --with-ebits=DIR          use ebits in <DIR>],
        [CFLAGS="$CFLAGS -I$withval/include"
         LIBS="-L$withval/lib $LIBS"])

AC_PATH_GENERIC(ebits,, [
    AC_SUBST(ebits_libs)
    AC_SUBST(ebits_cflags)],
    AC_MSG_ERROR(Cannot find ebits: Is ebits-config in path?))
ebits_libs=`ebits-config --libs`
ebits_cflags=`ebits-config --cflags`

dnl Check for Ecore.
AC_ARG_WITH(ecore,
        [  --with-ecore=DIR          use ecore in <DIR>],
        [CFLAGS="$CFLAGS -I$withval/include"
         LIBS="-L$withval/lib $LIBS"])

AC_PATH_GENERIC(ecore,, [
    AC_SUBST(ecore_libs)
    AC_SUBST(ecore_cflags)],
    AC_MSG_ERROR(Cannot find ecore: Is ecore-config in path?))
ecore_libs=`ecore-config --libs`
ecore_cflags=`ecore-config --cflags`

dnl Check for Efsd.
AC_ARG_WITH(efsd,
        [  --with-efsd=DIR          use efsd in <DIR>],
        [CFLAGS="$CFLAGS -I$withval/include"
         LIBS="-L$withval/lib $LIBS"])

AC_PATH_GENERIC(efsd,, [
    AC_SUBST(efsd_libs)
    AC_SUBST(efsd_cflags)],
    AC_MSG_ERROR(Cannot find efsd: Is efsd-config in path?))
efsd_libs=`efsd-config --libs`
efsd_cflags=`efsd-config --cflags`

dnl Check for Edb.
AC_ARG_WITH(ebg,
        [  --with-ebg=DIR          use ebg in <DIR>],
        [CFLAGS="$CFLAGS -I$withval/include"
         LIBS="-L$withval/lib $LIBS"])

AC_PATH_GENERIC(ebg,, [
    AC_SUBST(ebg_libs)
    AC_SUBST(ebg_cflags)],
    AC_MSG_ERROR(Cannot find ebg: Is ebg-config in path?))
ebg_libs=`ebg-config --libs`
ebg_cflags=`ebg-config --cflags`

dnl Check for ferite.
AC_ARG_WITH(ferite,
        [  --with-ferite=DIR          use ferite in <DIR>],
        [CFLAGS="$CFLAGS -I$withval/include"
         LIBS="-L$withval/lib $LIBS"])

AC_PATH_GENERIC(ferite,, [
    AC_SUBST(ferite_libs)
	AC_SUBST(ferite_cflags)
	AC_DEFINE(USE_FERITE)
    ferite_libs=`ferite-config --libs`
    ferite_cflags=`ferite-config --cflags`
    AC_PATH_PROG(FEBUILDER, builder,,)
    AC_SUBST(FEBUILDER)
	ferite=yes],[
	echo "Not building ferite support"
	ferite=no])
  
AM_CONDITIONAL(USE_FERITE, test "x$ferite" = "xyes")

AC_SUBST(CP_OPTIONS)

if test "x$ferite" = "xyes"; then
  pushd $packagesrcdir/src >& /dev/null
    echo -n "... building ferite binding c code ... ";
    rm -f e_ferite_gen_*;
    $FEBUILDER -m e_ferite_gen -i e_ferite_register -d -c e_ferite.fec;
    echo "done.";
  popd >& /dev/null
fi

CFLAGS=$imlib2_cflags
LIBS=$imlib2_libs
dnl Check for db loader.
AC_TRY_RUN([
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <Imlib2.h>

int main(int argc, char **argv) {
Imlib_Image im;
int width;

im = imlib_load_image("$packagesrcdir/data/test/test.db:/test/image");
if (!im) exit(-1);
imlib_context_set_image(im);
width = imlib_image_get_width();
if (width != 7) exit(-1);
exit(0);
return 0;
argv=NULL;
argc=0;
}
],
[echo "checking for imlib2 db loader... yes"],
[
echo "checking for imlib2 db loader... no"
AC_MSG_ERROR(Cannot detect db loader: Have you installed imlib2_loaders?)
],
AC_MSG_ERROR(Sorry. Enlightenment cannot be cross-compiled.))


dnl Checking for Perl:
AC_PATH_PROG(PERL,perl,0)
AC_SUBST(PERL)


dnl Look for jade for sgml translations.
AC_ARG_WITH(dbsheets,
       [  --with-dbsheets=DIR     use DIR to specify your DocBook stylesheets installation path.],
       DB_STYLESHEETS="$withval", DB_STYLESHEETS="/usr/lib/sgml/stylesheet/dsssl/docbook/nwalsh")
AC_SUBST(DB_STYLESHEETS)
AC_PATH_PROG(JADE, jade)
AM_CONDITIONAL(HAVE_JADE, test "x$JADE" != "x" && test -d "$DB_STYLESHEETS")


AC_CONFIG_FILES([
Makefile
src/Makefile
lib/Makefile
client/Makefile
intl/Makefile
po/Makefile.in
tools/Makefile
doc/Makefile
doc/kernel-doc
doc/html-customizations.dsl
data/scripts/build_iconbar_db.sh
])
AC_CONFIG_COMMANDS([default],[[
chmod +x doc/kernel-doc
]],[[]])
AC_OUTPUT

