#!/bin/sh

#Xephyr :1 -noreset -ac -br -dpi 284 -screen 480x640x16 &
Xephyr :1 -noreset -ac -br -dpi 284 -screen 480x640 &
#Xephyr :1 -noreset -ac -br -dpi 284 -screen 640x480x16 &
#Xephyr :1 -noreset -ac -br -dpi 284 -screen 640x480 &
#Xephyr :1 -noreset -ac -br -dpi 181 -screen 320x320x16 &
#Xephyr :1 -noreset -ac -br -dpi 181 -screen 320x320 &
#Xephyr :1 -noreset -ac -br -dpi 186 -screen 272x480x16 &
#Xephyr :1 -noreset -ac -br -dpi 186 -screen 272x480 &
#Xephyr :1 -noreset -ac -br -dpi 142 -screen 240x320x16 &
#Xephyr :1 -noreset -ac -br -dpi 142 -screen 240x320 &

sleep 1
export DISPLAY=:1
unset E_RESTART E_START E_IPC_SOCKET E_START_TIME
#E_CONF_PROFILE=default enlightenment_start
enlightenment_start -profile illume
