/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config      Config;
typedef struct _Config_Item Config_Item;

#define PAGER_RESIZE_NONE 0
#define PAGER_RESIZE_HORZ 1
#define PAGER_RESIZE_VERT 2
#define PAGER_RESIZE_BOTH 3

#define PAGER_DESKNAME_NONE   0
#define PAGER_DESKNAME_TOP    1
#define PAGER_DESKNAME_BOTTOM 2
#define PAGER_DESKNAME_LEFT   3
#define PAGER_DESKNAME_RIGHT  4

struct _Config
{
   /* saved * loaded config values */
   unsigned int    popup;
   double          popup_speed;
   unsigned int    popup_urgent;
   unsigned int    popup_urgent_stick;
   double          popup_urgent_speed;
   unsigned int    show_desk_names;
   int             popup_act_height; /*keyaction popup */
   int             popup_height;    /* urgent/on-deskswitch popup*/
   unsigned int    drag_resist;
   unsigned int    btn_drag;
   unsigned int    btn_noplace;
   unsigned int    btn_desk;
   unsigned int    flip_desk;

   /* just config state */
   E_Module        *module;
   E_Config_Dialog *config_dialog;
   Evas_List       *instances;
   E_Menu          *menu;
   Evas_List       *handlers;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

void  _pager_cb_config_updated(void);
void _config_pager_module(Config_Item *ci);
extern Config *pager_config;

#endif
