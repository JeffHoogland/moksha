#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config Config;
typedef struct _Config_Item Config_Item;

struct _Config
{
   E_Module *module;
   Eina_List *tasks;
   Eina_List *borders;
   Eina_List *items;
   E_Menu *menu;
   Eina_List *handlers;
   E_Config_Dialog *config_dialog;
};

struct _Config_Item
{
   const char *id;
   int show_all;
   int minw, minh;
   Eina_Bool icon_only;
   Eina_Bool text_only;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

void _config_tasks_module(Config_Item *ci);
void _tasks_config_updated(Config_Item *ci);
extern Config *tasks_config;

/**
 * @addtogroup Optional_Gadgets
 * @{
 *
 * @defgroup Module_Tasks Tasks (Task Bar)
 *
 * Shows windows as buttons in a box.
 *
 * @see Module_IBox
 *
 * @}
 */
#endif
