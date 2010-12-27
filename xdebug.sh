#!/usr/bin/env bash


debugger_default="b"
geo_default="800x600+0+0"


echo
xserver="Xephyr"
if [ `type $xserver &>/dev/null; echo $?` -ne 0 ]; then
	xserver="Xnest"
	if [ `type $xserver &>/dev/null; echo $?` -ne 0 ]; then
		echo "Please install either Xephyr (recommended) or Xnest!"
		exit 1
	fi
fi


for arg in $@; do
   option=`echo "'$arg'" | cut -d'=' -f1 | tr -d "'"`
   value=`echo "'$arg'" | cut -d'=' -f2- | tr -d "'"`
   if [ "$value" == "$option" ]; then
      value=""
   fi

   case $option in
      "--dbg-display")
         if [ -z "$value" ]; then
            echo "Missing value for $option= !"
			exit 1
         fi
		 dbg_display=$value
         ;;
	  "--dbg-geo")
         if [ -z "$value" ]; then
            echo "Missing value for $option= !"
			exit 1
         fi
		 dbg_geo=$value
         ;;
      "--dbg-mode")
         if [ -z "$value" ]; then
            echo "Missing value for $option= !"
			exit 1
         fi
		 debugger=$value
         ;;
	  "--dbg-ecore-errors")  export ECORE_ERROR_ABORT=1 ;;
	  "--dbg-ecore-noclean") export ECORE_NOCLEAN=1 ;;
      "--dbg-redraw")        export REDRAW_DEBUG=1 ;;
	  "--help")
	     echo "Usage: $0 [DEBUG-OPTION] ..."
		 echo "      --dbg-display=<NUMBER>    = set the used display number"
		 echo "      --dbg-geo=<WxH+X+Y>       = set xserver geometry"
		 echo "      --dbg-mode=<CHAR>         = b: text debugger with auto backtrace (gdb)"
		 echo "                                  c: curses debugger (cgdb)"
		 echo "                                  d: GUI debugger (ddd)"
		 echo "                                  e: no debugging"
		 echo "                                  g: text debugger (gdb)"
		 echo "                                  l: leak check (valgrind)"
		 echo "                                  m: memory check (valgrind)"
		 echo "                                  p: memory profiling (memprof)"
		 echo "                                  r: raster's memory profiling (memprof_raster)"
		 echo "                                  s: show syscalls (strace)"
		 echo "                                  v: GUI memory check (valkyrie)"
		 echo "      --dbg-ecore-errors        = to cause ecore to abort on errors"
		 echo "      --dbg-ecore-noclean       = to cause ecore to not unload modules"
		 echo "      --dbg-redraw              = to cause redraw to happen slovly and obviously"
		 echo "      --help                    = wysiwyg"
		 echo
	     echo "Usage: $0 [ENLIGHTENMENT-OPTION] ..."
		 enlightenment --help
		 exit 0
	     ;;
      *) enlightenment_args="$enlightenment_args $arg" ;;
   esac
done


if [ -z "$dbg_display" ]; then 
   if [ -z "$DISPLAY" ]; then
      echo "Couldn't read your \$DISPLAY env variable, are you running X?"
	  exit 1
   fi

   dcnt=`echo "$DISPLAY" | tr -d ':' | cut -d '.' -f1`
   dbg_display=$(($dcnt+1))
fi
if [ -z "$dbg_geo" ]; then
	dbg_geo=$geo_default
fi
if [ -z "$debugger" ]; then
	debugger=$debugger_default
fi
case $debugger in
   "b")
      tmpfile=`mktemp`
	  if [ -z "$tmpfile" ]; then
         echo "Can't create tmp file!"
		 exit 1
      fi
      echo -e "run\nbt\nq\ny" > $tmpfile
      debugcmd="gdb -x $tmpfile --args"
	  ;;
   "c") debugcmd="cgdb" ;;
   "d") debugcmd="ddd -display $DISPLAY --args" ;;
   "e") debugcmd="" ;;
   "g") debugcmd="gdb --args" ;;
   "l") debugcmd="valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --log-file=valgrind_log" ;;
   "m") debugcmd="valgrind --tool=memcheck --log-file=valgrind_log" ;;
   "p") debugcmd="memprof --display=$DISPLAY" ;;
   "r") debugcmd="memprof_raster --display=$DISPLAY" ;;
   "s") debugcmd="strace -F -o strace_log" ;;
   "v") debugcmd="valkyrie -display $DISPLAY" ;;
esac


echo "- DISPLAY: $dbg_display"
echo "- XSERVER GEOMETRY: $dbg_geo"
echo -n "- DEBUGMODE: "
if [ "$debugcmd" ]; then
	echo "$debugcmd"
else
	echo "NONE"
fi
if [ "$enlightenment_args" ]; then
	echo "- ENLIGHTENMENT ARGUMENTS: $enlightenment_args"
fi
if [ "$ECORE_ERROR_ABORT" ]; then
	echo "- ECORE ERROR ABORT"
fi
if [ "$ECORE_NOCLEAN" ]; then
	echo "- ECORE NOCLEAN"
fi
if [ "$REDRAW_DEBUG" ]; then
	echo "- REDRAW DEBUG"
fi
echo "======================================================"
echo
sleep 1


case $xserver in
	"Xephyr")	$xserver :$dbg_display -ac -screen $dbg_geo & ;;
	"Xnest")	$xserver :$dbg_display -ac -geometry $dbg_geo & ;;
esac
sleep 2 # Someone reported that it starts E before X has started properly.

export DISPLAY=":$dbg_display"
export E_START="enlightenment_start"
$debugcmd enlightenment $enlightenment_args

if [ "$tmpfile" ]; then
	rm "$tmpfile"
fi
killall -TERM $xserver
echo
