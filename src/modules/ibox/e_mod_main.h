/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config      Config;
typedef struct _Config_Item Config_Item;

struct _Config
{
   /* just config state */
   E_Module        *module;
   Evas_List       *instances;
   E_Menu          *menu;
   Evas_List       *handlers;
   Evas_List       *items;
   Evas_List	   *config_dialog;
};

struct _Config_Item 
{
   const char *id;
   int show_label;
   int show_zone;
   int show_desk;
   int icon_label;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

void _ibox_config_update(Config_Item *ci);
void _config_ibox_module(Config_Item *ci);
extern Config *ibox_config;

#endif
