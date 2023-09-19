#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

/* Increment for Major Changes */
#define MOD_CONFIG_FILE_EPOCH      1
/* Increment for Minor Changes (ie: user doesn't need a new config) */
#define MOD_CONFIG_FILE_GENERATION 0
#define MOD_CONFIG_FILE_VERSION    ((MOD_CONFIG_FILE_EPOCH * 1000000) + MOD_CONFIG_FILE_GENERATION)

typedef struct _Config      Config;
typedef struct _Config_Item Config_Item;

struct _Config
{
   unsigned int     version;
   /* saved * loaded config values */
   Eina_List       *items;
   /* just config state */
   E_Module        *module;
   E_Config_Dialog *config_dialog;
   Eina_List       *instances;
   Eina_List       *handlers;
};

struct _Config_Item
{
   const char *id;
   const char *dir;
   int show_label;
   int eap_label;
   int lock_move;
   int dont_add_nonorder;
   int focus_flash;
   int control;
   unsigned char dont_track_launch;
   unsigned char dont_icon_menu_mouseover;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

void _ibar_config_update(Config_Item *ci);
void _config_ibar_module(Config_Item *ci);
extern Config *ibar_config;

/**
 * @addtogroup Optional_Gadgets
 * @{
 *
 * @defgroup Module_IBar IBar (Icon Launch Bar)
 *
 * Launches applications from an icon bar, usually placed on shelves.
 *
 * @}
 */

#endif
