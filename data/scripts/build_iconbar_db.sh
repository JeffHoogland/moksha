#!/bin/sh

# db to create
DB="./.e_iconbar.db"

# replace ICONDIR with where you keep icons, or just use full paths below
setup ()
{
	NUM=0
	ICONDIR="/usr/share/pixmaps"
	E_PREFIX="/usr/local"
	rm -f $DB
}

finish ()
{
	edb_ed $DB add "/ib/num"	int $NUM
}

# usage: icon 'image_path' 'exec'
icon ()
{
	edb_ed $DB add "/ib/"$NUM"/icon"	str "$1"
	edb_ed $DB add "/ib/"$NUM"/exec"        str "$2"
	NUM=$[ $NUM + 1 ];
}

config ()
{
	edb_ed $DB add "/ib/"$1	int $2
}

str ()
{
	edb_ed $DB add "/ib/"$1	str $2
}
#################################################
##
##  Define Icons here
##

setup

# title is put at top or left of bar, this may be removed in future
# vline/hline are the images that are repeated as borders for the bar / scroll region (for vert / horiz. bars).

str 'image/title' $E_PREFIX'/share/enlightenment/data/images/ib_title.png'
str 'image/vline' $E_PREFIX'/share/enlightenment/data/images/vline.png'
str 'image/hline' $E_PREFIX'/share/enlightenment/data/images/hline.png'

# 0 width / height makes it equal view's width / height, negative values for left/top go from bottowm right corner. scroll_w is used for both horizontal and vertical bars (i guess it should be renamed scroll_thikness or something).

config 'geom/w' 0  
config 'geom/h' 75
config 'geom/top' -75 
config 'geom/left' 0
config 'geom/scroll_w' 16
config 'geom/horizontal' 1
config 'scroll_when_less' 0

# change icondir above, or replace with full pathname

icon $ICONDIR'/gnome-term.png' 'Eterm'
icon $ICONDIR'/gnome-ccdesktop.png' 'sylpheed'
icon $ICONDIR'/netscape.png' 'netscape'
icon $ICONDIR'/gnome-irc.png' 'xchat'

icon $ICONDIR'/gnome-gimp.png' 'gimp'
icon $ICONDIR'/mc/gnome-audio.png' 'xmms'
icon $ICONDIR'/home/raster/stuff/icons/star_office.png' '/home/raster/.desktop/Office'

#icon $ICONDIR'/write.png' 'abiword'
#icon $ICONDIR'/spreadsheet.png' 'gnumeric'

#icon $ICONDIR'/synth.png' 'SpiralSynth'
#icon $ICONDIR'/modsynth.png' 'SpiralSynthModular'
#icon $ICONDIR'/loops.png' 'SpiralLoops'
#icon $ICONDIR'/drakconf.png' 'gnomecc'
#icon $ICONDIR'/floppy.png' 'NULL'
#icon $ICONDIR'/trash.png' 'NULL'

finish

