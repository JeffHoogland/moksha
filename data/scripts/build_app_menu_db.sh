#!/bin/sh

# this is the databse to create
DB="./apps_menu.db"

# shell function calls that setup, cleanup and build menus in  a menus database
setup ()
{
  NUM=0
  ENTRYNUM=0
  MENUNUM=0
  rm -f $DB
}

finish ()
{
  edb_ed $DB add "/menu/count" int $MENUNUM
}

menu ()
{
  ENTRYNUM=0
  MENUNUM=$1;
  M1=$[ $MENUNUM + 1 ];
  if [ $M1 -gt $NUM ]; then 
    NUM=$M1;
  fi
}

end_menu ()
{
  edb_ed $DB add "/menu/"$MENUNUM"/count" int $ENTRYNUM
}

entry ()
{
  if [ $1 = "text" ]; then
    edb_ed $DB add "/menu/"$MENUNUM"/"$ENTRYNUM"/text"      str "$2"
    if [ "$3" = "sub" ]; then
      edb_ed $DB add "/menu/"$MENUNUM"/"$ENTRYNUM"/submenu" int $4
    else
      edb_ed $DB add "/menu/"$MENUNUM"/"$ENTRYNUM"/command" str "$3"
    fi
    
    
  else if [ $1 = "icon" ]; then
    edb_ed $DB add "/menu/"$MENUNUM"/"$ENTRYNUM"/icon"      str "$2"
    if [ "$3" = "sub" ]; then
      edb_ed $DB add "/menu/"$MENUNUM"/"$ENTRYNUM"/submenu" int $4
    else
      edb_ed $DB add "/menu/"$MENUNUM"/"$ENTRYNUM"/command" str "$3"
    fi
    
    
  else if [ $1 = "both" ]; then
    edb_ed $DB add "/menu/"$MENUNUM"/"$ENTRYNUM"/text"      str "$2"
    edb_ed $DB add "/menu/"$MENUNUM"/"$ENTRYNUM"/icon"      str "$3"
    if [ "$4" = "sub" ]; then
      edb_ed $DB add "/menu/"$MENUNUM"/"$ENTRYNUM"/submenu" int $5
    else
      edb_ed $DB add "/menu/"$MENUNUM"/"$ENTRYNUM"/command" str "$4"
    fi
    
    
  else if [ $1 = "separator" ]; then
    edb_ed $DB add "/menu/"$MENUNUM"/"$ENTRYNUM"/separator" int 1
    
    
  fi; fi; fi; fi
  ENTRYNUM=$[ $ENTRYNUM + 1 ];
}




###############################################################################
##
## Menus are defined here
##
## IF you want to edit anything - edit this - it's REALLY simple.

setup

menu 0
entry both 'Eterm' '/usr/share/pixmaps/gnome-eterm.png' 'Eterm'
entry both 'Netscape' '/usr/share/pixmaps/netscape.png' 'netscape'
entry both 'TkRat' '/usr/share/pixmaps/gnome-ccdesktop.png' 'tkrat'
entry both 'X Chat' '/usr/share/pixmaps/gnome-irc.png' 'xchat'
entry both 'XMMS' '/usr/share/pixmaps/mc/gnome-audio.png' 'xmms'
entry both 'The GIMP' '/usr/share/pixmaps/gimp.png' 'gimp'
entry separator
entry both 'XTerm' '/usr/share/pixmaps/gnome-term.png' 'xterm'
entry both 'XMag' '/usr/share/pixmaps/gnome-applets.png' 'xmag'
entry separator
entry text 'Network' 'sub' 1
entry text 'System' 'sub' 2
end_menu

menu 1
entry text 'Ethernet On' 'sudo -S /sbin/ifup eth0'
entry text 'Ethernet Off' 'sudo -S /sbin/ifdown eth0'
end_menu

menu 2
entry text 'Shut Down' 'sudo -S /sbin/shutdown -h now'
entry text 'Reboot' 'sudo -S /sbin/shutdown -r now'
end_menu

finish
