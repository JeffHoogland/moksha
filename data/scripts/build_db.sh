# enumerated variables
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

# actions defining how to react to things
DB="./actions.db"
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
edb_ed $DB add "/actions/count"       int $NUM

# basic settings
DB="./settings.db"
edb_ed $DB add "/focus/mode   "      int $FOCUS_POINTER
edb_ed $DB add "/move/resist"        int 1
edb_ed $DB add "/move/resist/desk"   int 24
edb_ed $DB add "/move/resist/win"    int 12
edb_ed $DB add "/menu/scroll/resist" int 5
edb_ed $DB add "/menu/scroll/speed"  int 12

# what events on windows are "grabbed" by the window manager
DB="./grabs.db"
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

