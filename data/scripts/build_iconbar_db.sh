#!/bin/sh

DB=$1"/.e_iconbar.db"
BIT=$1"/.e_iconbar.bits.db"

setup ()
{
  PREFIX="/usr/local"
  SYSIC="/usr/share/pixmaps"
  USRIC=$HOME"/stuff/icons"
  NUM=0
  rm -f $DB
}

finish ()
{
  edb_ed $DB add "/icons/count" int $NUM
}

icon ()
{
  e_img_import "$1" $DB":""/icons/"$NUM"/image"
  edb_ed $DB add "/icons/"$NUM"/exec" str "$2"
  NUM=$[ $NUM + 1 ];
}

setup

icon $SYSIC"/gnome-term.png" "Eterm"
icon $SYSIC"/gnome-ccdesktop.png" "sylpheed"
icon $SYSIC"/netscape.png" "netscape"
icon $USRIC"/mozilla.png" "mozilla"
icon $SYSIC"/gnome-irc.png" "xchat"
icon $SYSIC"/gnome-gimp.png" "gimp"
icon $SYSIC"/gnome-mixer.png" "/home/raster/.desktop/FixMix"
icon $SYSIC"/mc/gnome-audio.png" "xmms"
icon $USRIC"star_office.png" "/home/raster/.desktop/Office"

finish

cp -f $PREFIX"/share/enlightenment/data/iconbar/iconbar_bottom.bits.db" $BIT
