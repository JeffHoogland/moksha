#ifndef E_CONFIG_H
#define E_CONFIG_H

#include "e.h"

/*
 * Define small subsets of the whole config for defining data types for
 * loading from the databases.
 */
typedef struct _E_Config_Desktops E_Config_Desktops;
struct _E_Config_Desktops
{
   int                 count;
   int                 scroll;
   int                 scroll_sticky;
   int                 resist;
   int                 speed;
   int                 width;
   int                 height;
};

typedef struct _E_Config_Guides E_Config_Guides;
struct _E_Config_Guides
{
   float               x;
   float               y;
   int                 location;
};

typedef struct _E_Config_Menu E_Config_Menu;
struct _E_Config_Menu
{
   int                 resist;
   int                 speed;
};

typedef struct _E_Config_Move E_Config_Move;
struct _E_Config_Move
{
   int                 resist;
   int                 win_resist;
   int                 desk_resist;
};

typedef struct _E_Config_Window E_Config_Window;
struct _E_Config_Window
{
   int                 move_mode;
   int                 focus_mode;
   int                 auto_raise;
   float               raise_delay;
   int                 resize_mode;
   int                 place_mode;
};

typedef struct _E_Config E_Config;
struct _E_Config
{
   Evas_List          *actions;
   Evas_List          *grabs;
   Evas_List          *match;

   E_Config_Desktops  *desktops;
   E_Config_Guides    *guides;
   E_Config_Menu      *menu;
   E_Config_Move      *move;
   E_Config_Window    *window;
};

extern E_Config     *config_data;

char               *e_config_get(char *type);
void                e_config_init(void);
void                e_config_set_user_dir(char *dir);
char               *e_config_user_dir(void);

#endif
