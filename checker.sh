#!/bin/bash
# E17 checker script. Makes sure the user has the required programs, and
# abort if not.
# By Lyle (term) Kempler; same license applies to this as does the source
# code it accompanies.

# TODO:
# - Make it check the actual version, and abort if too old.
# - Make autogen.sh call it.

# Base programs.
M4=`which m4`
AUTOMAKE=`which automake`
AUTOCONF=`which autoconf`
LIBTOOL=`which libtool`
GETTEXT=`which gettext`

# Our libraries.
IMLIB2_CONFIG=`which imlib2-config`
EDB_CONFIG=`which edb-config`
EVAS_CONFIG=`which evas-config`
EFSD_CONFIG=`which efsd-config`
ECORE_CONFIG=`which ecore-config`
EBITS_CONFIG=`which ebits-config`
EWL_CONFIG=`which ewl-config`

if [ -n "$M4" ]
then
  echo -n "m4      : "
  $M4 --version
else
  echo "No m4 found! This is a requirement for building Enlightenment 0.17."
  # Information on where to get it goes here.
  echo
  exit 1
fi

if [ -n "$AUTOMAKE" ]
then
  echo -n "automake: "
  $AUTOMAKE --version | grep automake
else
  echo "No automake found! This is a requirement for building Enlightenment 0.17."
  # Information on where to get it goes here.
  echo
  exit 1
fi

if [ -n "$AUTOCONF" ]
then
  echo -n "autoconf: "
  $AUTOCONF --version | grep version
else
  echo "No autoconf found! This is a requirement for building Enlightenment 0.17."
  # Information on where to get it goes here.
  echo
  exit 1
fi

if [ -n "$LIBTOOL" ]
then
  echo -n "libtool : "
  $LIBTOOL --version
else
  echo "No libtool found! This is a requirement for building Enlightenment 0.17."
  # Information on where to get it goes here.
  echo
  exit 1
fi

if [ -n "$GETTEXT" ]
then
  echo -n "gettext : "
  $GETTEXT --version | grep gettext
else
  echo "No gettext found! This is a requirement for building Enlightenment 0.17."
  # Information on where to get it goes here.
  echo
  exit 1
fi

echo

if [ -n "$IMLIB2_CONFIG" ]
then
  echo -n "imlib2-config: "
  $IMLIB2_CONFIG --version
else
  echo "No imlib2-config found! This is a requirement for building Enlightenment 0.17."
  # Information on where to get it goes here.
  echo
  exit 1
fi

if [ -n "$EDB_CONFIG" ]
then
  echo -n "edb-config   : "
  $EDB_CONFIG --version
else
  echo "No edb-config found! This is a requirement for building Enlightenment 0.17."
  # Information on where to get it goes here.
  echo
  exit 1
fi

if [ -n "$EVAS_CONFIG" ]
then
  echo -n "evas-config  : "
  $EVAS_CONFIG --version
else
  echo "No evas-config found! This is a requirement for building Enlightenment 0.17."
  # Information on where to get it goes here.
  echo
  exit 1
fi

if [ -n "$EFSD_CONFIG" ]
then
  echo -n "efsd-config  : "
  $EFSD_CONFIG --version
else
  echo "No efsd-config found! This is a requirement for building Enlightenment 0.17."
  # Information on where to get it goes here.
  echo
  exit 1
fi

if [ -n "$ECORE_CONFIG" ]
then
  echo -n "ecore-config : "
  $ECORE_CONFIG --version
else
  echo "No ecore-config found! This is a requirement for building Enlightenment 0.17."
  # Information on where to get it goes here.
  echo
  exit 1
fi

if [ -n "$EBITS_CONFIG" ]
then
  echo -n "ebits-config : "
  $EBITS_CONFIG --version
else
  echo "No ebits-config found! This is a requirement for building Enlightenment 0.17."
  # Information on where to get it goes here.
  echo
  exit 1
fi

#if [ -n "$EWL_CONFIG" ]
#then
#  echo -n "ewl-config   : "
#  $EWL_CONFIG --version
#else
#  echo "No ewl-config found! This is a requirement for building Enlightenment 0.17."
#  # Information on where to get it goes here.
#  echo
#  exit 1
#fi

if [ "$1" != "autogen" ]
then
  echo
  echo "All requirements have been met! Happy building."
  echo
  exit 0
fi
