# enumerated variables - don't touch these
ACT_MOUSE_IN=0
ACT_MOUSE_OUT=1
ACT_MOUSE_CLICK=2
ACT_MOUSE_DOUBLE=3
ACT_MOUSE_TRIPLE=4
ACT_MOUSE_UP=5
ACT_MOUSE_CLICKED=6
ACT_MOUSE_MOVE=7
ACT_KEY_DOWN=8
ACT_KEY_UP=9

MOD_ANY=-1
MOD_NONE=0
MOD_SHIFT=1
MOD_CTRL=2
MOD_ALT=4
MOD_WIN=8
MOD_CTRL_ALT=$[ $MOD_CTRL + $MOD_ALT ];

FOCUS_POINTER=0
FOCUS_SLOPPY=1
FOCUS_CLICK=2

WINDOW_MODE_OPAQUE=0
WINDOW_MODE_BORDER=1
WINDOW_MODE_BOX=2
WINDOW_MODE_TECHNICAL=3

WINDOW_PLACE_MANUAL=0
WINDOW_PLACE_SMART=1
WINDOW_PLACE_MIDDLE=2
WINDOW_PLACE_CASCADE=3
WINDOW_PLACE_RANDOM=4

DESKTOP_COUNT=8
DESKTOP_SCROLL=0
DESKTOP_SCROLL_STICKY=0
DESKTOP_RESIST=3
DESKTOP_SPEED=30
DESKTOP_WIDTH=2
DESKTOP_HEIGHT=2

GUIDES_LOC_WIN=0
GUIDES_LOC_SCR=1

DB="./behavior.db"

# actions defining how to react to things
NUM=0
edb_ed $DB add "/actions/"$NUM"/name"      str "Title_Bar"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Raise"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICK  
edb_ed $DB add "/actions/"$NUM"/button"    int 1
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_NONE
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Title_Bar"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Move"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICK  
edb_ed $DB add "/actions/"$NUM"/button"    int 1
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_NONE
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Title_Bar"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Shade"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_DOUBLE
edb_ed $DB add "/actions/"$NUM"/button"    int 1
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_NONE
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Resize"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Resize"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICK
edb_ed $DB add "/actions/"$NUM"/button"    int 1
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_NONE
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Resize_Horizontal"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Resize_Horizontal"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICK
edb_ed $DB add "/actions/"$NUM"/button"    int 1
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_NONE
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Resize_Vertical"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Resize_Vertical"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICK
edb_ed $DB add "/actions/"$NUM"/button"    int 1
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_NONE
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Window_Grab"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Raise"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICK
edb_ed $DB add "/actions/"$NUM"/button"    int 1
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Window_Grab"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Move"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICK
edb_ed $DB add "/actions/"$NUM"/button"    int 1
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Window_Grab"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Resize"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICK
edb_ed $DB add "/actions/"$NUM"/button"    int 2
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Window_Grab"
edb_ed $DB add "/actions/"$NUM"/action"    str "Menu"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICK
edb_ed $DB add "/actions/"$NUM"/button"    int 3
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Resize"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Move"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICK
edb_ed $DB add "/actions/"$NUM"/button"    int 3
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_NONE
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Resize_Horizontal"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Move"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICK
edb_ed $DB add "/actions/"$NUM"/button"    int 3
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_NONE
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Resize_Vertical"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Move"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICK
edb_ed $DB add "/actions/"$NUM"/button"    int 3
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_NONE
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Close"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Close"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICKED
edb_ed $DB add "/actions/"$NUM"/button"    int 1
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_NONE
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Close"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Kill"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICKED
edb_ed $DB add "/actions/"$NUM"/button"    int 3
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_NONE
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Max_Size"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Max_Size"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICKED
edb_ed $DB add "/actions/"$NUM"/button"    int 1
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_NONE
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Iconify"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Iconify"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICKED
edb_ed $DB add "/actions/"$NUM"/button"    int 1
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_NONE
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Menu"
edb_ed $DB add "/actions/"$NUM"/action"    str "Menu"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICKED
edb_ed $DB add "/actions/"$NUM"/button"    int 1
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_NONE
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Window_Place"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Move"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_MOUSE_CLICK  
edb_ed $DB add "/actions/"$NUM"/button"    int 1
edb_ed $DB add "/actions/"$NUM"/key"       str ""
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_NONE
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Restart"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "End"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_CTRL_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Exit"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "Delete"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_CTRL_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Max_Size"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "m"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_CTRL_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Close"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "x"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_CTRL_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Shade"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "r"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_CTRL_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Raise"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "Up"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_CTRL_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Lower"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "Down"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_CTRL_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Stick"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "a"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_CTRL_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Desktop"
edb_ed $DB add "/actions/"$NUM"/params"    str "0"
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "F1"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Desktop"
edb_ed $DB add "/actions/"$NUM"/params"    str "1"
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "F2"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Desktop"
edb_ed $DB add "/actions/"$NUM"/params"    str "2"
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "F3"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Desktop"
edb_ed $DB add "/actions/"$NUM"/params"    str "3"
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "F4"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Desktop"
edb_ed $DB add "/actions/"$NUM"/params"    str "4"
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "F5"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Desktop"
edb_ed $DB add "/actions/"$NUM"/params"    str "5"
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "F6"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Desktop"
edb_ed $DB add "/actions/"$NUM"/params"    str "6"
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "F7"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Desktop"
edb_ed $DB add "/actions/"$NUM"/params"    str "7"
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "F8"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Desktop_Relative"
edb_ed $DB add "/actions/"$NUM"/params"    str "1"
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "Right"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_CTRL_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Desktop_Relative"
edb_ed $DB add "/actions/"$NUM"/params"    str "-1"
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "Left"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_CTRL_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Execute"
edb_ed $DB add "/actions/"$NUM"/params"    str "Eterm"
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "Insert"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_CTRL_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/"$NUM"/name"      str "Key_Binding"
edb_ed $DB add "/actions/"$NUM"/action"    str "Window_Next"
edb_ed $DB add "/actions/"$NUM"/params"    str ""
edb_ed $DB add "/actions/"$NUM"/event"     int $ACT_KEY_DOWN
edb_ed $DB add "/actions/"$NUM"/button"    int 0
edb_ed $DB add "/actions/"$NUM"/key"       str "Tab"
edb_ed $DB add "/actions/"$NUM"/modifiers" int $MOD_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/actions/count"       int $NUM

edb_ed $DB add "/desktops/count"           int   $DESKTOP_COUNT
edb_ed $DB add "/desktops/scroll"          int   $DESKTOP_SCROLL
edb_ed $DB add "/desktops/scroll_sticky"   int   $DESKTOP_SCROLL_STICKY
edb_ed $DB add "/desktops/resist"          int   $DESKTOP_RESIST
edb_ed $DB add "/desktops/speed"           int   $DESKTOP_SPEED
edb_ed $DB add "/desktops/width"           int   $DESKTOP_WIDTH
edb_ed $DB add "/desktops/height"          int   $DESKTOP_HEIGHT

# basic settings
edb_ed $DB add "/move/resist"             int   1
edb_ed $DB add "/move/resist/desk"        int   24
edb_ed $DB add "/move/resist/win"         int   12
edb_ed $DB add "/menu/scroll/resist"      int   5
edb_ed $DB add "/menu/scroll/speed"       int   12
edb_ed $DB add "/window/focus/mode"       int   $FOCUS_POINTER
edb_ed $DB add "/window/raise/auto"       int    0
edb_ed $DB add "/window/raise/delay"      float 0.5
edb_ed $DB add "/window/move/mode"        int   $WINDOW_MODE_OPAQUE
edb_ed $DB add "/window/resize/mode"      int   $WINDOW_MODE_BOX
edb_ed $DB add "/window/place/mode"       int   $WINDOW_PLACE_SMART
edb_ed $DB add "/guides/display/x"        float 0.5
edb_ed $DB add "/guides/display/y"        float 0.5
edb_ed $DB add "/guides/display/location" int   $GUIDES_LOC_SCR

# what events on windows are "grabbed" by the window manager
NUM=0
edb_ed $DB add "/grabs/"$NUM"/button"       int 1
edb_ed $DB add "/grabs/"$NUM"/modifiers"    int $MOD_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/grabs/"$NUM"/button"       int 2
edb_ed $DB add "/grabs/"$NUM"/modifiers"    int $MOD_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/grabs/"$NUM"/button"       int 3
edb_ed $DB add "/grabs/"$NUM"/modifiers"    int $MOD_ALT
NUM=$[ $NUM + 1 ];
edb_ed $DB add "/grabs/count"       int $NUM

