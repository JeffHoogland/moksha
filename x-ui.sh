#!/bin/sh

#Xephyr :1 -noreset -ac -br -dpi 142 -screen 240x320x16 &
#Xephyr :1 -noreset -ac -br -dpi 142 -screen 320x240x16 &
#Xephyr :1 -noreset -ac -br -dpi 186 -screen 272x480x16 &
#Xephyr :1 -noreset -ac -br -dpi 186 -screen 480x272x16 &
#Xephyr :1 -noreset -ac -br -dpi 181 -screen 320x320x16 &
#Xephyr :1 -noreset -ac -br -dpi 183 -screen 320x480x16 -host-cursor &
#Xephyr :1 -noreset -ac -br -dpi 183 -screen 480x320x16 -host-cursor &
#Xephyr :1 -noreset -ac -br -dpi 183 -screen 480x800x16 &
#Xephyr :1 -noreset -ac -br -dpi 183 -screen 800x480x16 &
#Xephyr :1 -noreset -ac -br -dpi 284 -screen 480x640x16 &
#Xephyr :1 -noreset -ac -br -dpi 284 -screen 640x480x16 -host-cursor &

#Xephyr :1 -noreset -ac -br -dpi 183 -screen 960x400x16 -host-cursor &

#Xephyr :1 -noreset -ac -br -dpi 284 -screen 480x800 -host-cursor &
#Xephyr :1 -noreset -ac -br -dpi 284 -screen 480x640 &
#Xephyr :1 -noreset -ac -br -dpi 284 -screen 640x480 &
#Xephyr :1 -noreset -ac -br -dpi 181 -screen 320x320 &
#Xephyr :1 -noreset -ac -br -dpi 186 -screen 272x480 &
Xephyr :1 -noreset -ac -br -dpi 142 -screen 240x320 &
#Xephyr :1 -noreset -ac -br -dpi 142 -screen 240x240 -host-cursor &

sleep 1
export DISPLAY=:1
#export E_CONF_PROFILE=illume-home
#valgrind --db-attach=yes --leak-check=full --leak-resolution=high --num-callers=32 \
#enlightenment \
unset E_RESTART E_START E_IPC_SOCKET E_START_TIME E_CONF_PROFILE
enlightenment_start \
-no-precache \
-i-really-know-what-i-am-doing-and-accept-full-responsibility-for-it \
-profile illume-home

#-fake-xinerama-screen 480x400+0+0 \
#$-fake-xinerama-screen 480x400+480+0
