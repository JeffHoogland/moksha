#!/bin/sh

#export REDRAW_DEBUG=1        # To cause redraw- to happen slovly and obviously.
#export ECORE_ERROR_ABORT=1   # To cause ecore to abort on errors.
#export EVAS_NOCLEAN=1        # To cause evas to not unload modules.

disp_num=":1"                 # Which display do you want the xnest to be on?

main=$DISPLAY
display=" -display $disp_num"
tmp=`mktemp` || exit 1
/bin/echo -e "run\nbt\nq\ny" > $tmp

case "$@" in
	"")	action="gdb -x $tmp" ; main=$disp_num ; display=""  ;;
	"-b")	action="gdb -x $tmp" ; main=$disp_num ; display=""  ;;
	"-c")	action="cgdb" ; main=$disp_num ; display=""  ;;
	"-d")	action="ddd -display $main" ; display="" ;;
	"-e")	action="" ;;
	"-g")	action="gdb" ; main=$disp_num ; display=""  ;;
	"-l")	action="valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --log-file=valgrind_log" ;;
	"-m")	action="valgrind --tool=memcheck --log-file=valgrind_log" ;;
	"-p")	action="memprof --display=$main" ; main=$disp_num ; display="" ;;
	"-r")	action="memprof_raster --display=$main" ; main=$disp_num ; display="" ;;
	"-s")	action="strace -F -o strace_log" ;;
	"-v")	action="valkyrie -display $main" ; main=$disp_num ; display="" ;;
	*)      echo -e "Usage : xnest.sh [option]"
		echo -e "\tdefault option is -b"
		echo -e "\t-b use text debugger with auto backtrace\tgdb"
		echo -e "\t-c use curses debugger\t\t\t\tcgdb"
		echo -e "\t-d use the GUI debugger\t\t\t\tddd"
		echo -e "\t-e enlightenment with no debugging"
		echo -e "\t-g use text debugger\t\t\t\tgdb"
		echo -e "\t-l leak check\t\t\t\t\tvalgrind"
		echo -e "\t-m memory check\t\t\t\t\tvalgrind"
		echo -e "\t-p memory profiling\t\t\t\tmemprof"
		echo -e "\t-r raster's memory profiling\t\t\tmemprof_raster"
		echo -e "\t-s show syscalls\t\t\t\tstrace"
		echo -e "\t-v GUI memory check\t\t\t\tvalkyrie"
		echo -e ""
		echo -e "You need to add \"-display $disp_num\" as the run arguments for the GUI debugger."
		echo -e "When you have finished with the text debugger, use the q command to quit."
		echo -e "The valgrind options will leave a log file with a name beginning with valgrind_log"
		exit
		;;
esac


Xnest $disp_num -ac &

sleep 2   # Someone reported that it starts E before X has started properly.

export DISPLAY=$main; export E_START="enlightenment_start"; $action enlightenment $display

rm -f $tmp
killall -TERM Xnest
