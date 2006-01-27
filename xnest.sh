#!/bin/sh

#export REDRAW_DEBUG=1        # To cause redraw- to happen slovly and obviously.
#export ECORE_ERROR_ABORT=1   # To cause ecore to abort on errers.

Xnest :1 -ac &

sleep 2   # Someone reported that it starts E before X has started properly.

# Comment out all but one of these.
DISPLAY=:1 ; enlightenment &   # Just run it.
#DISPLAY=:1 ; xterm -e gdb -x gdb.txt enlightenment   # Run it with the text based debugger.
#DISPLAY=:1 ; ddd -geometry 550x450+5+25 enlightenment &   # Run it with the GUI debugger.
#DISPLAY=:1 ; valgrind --tool=memcheck --log-file=valgrind_log enlightenment &   # Run it with valgrind.
