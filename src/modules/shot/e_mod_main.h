#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

//~ #ifdef ENABLE_NLS
//~ # include <libintl.h>
//~ # define D_(string) dgettext(PACKAGE, string)
//~ #else
//~ # define bindtextdomain(domain,dir)
//~ # define bind_textdomain_codeset(domain,codeset)
//~ # define D_(string) (string)
//~ #endif

/* Macros used for config file versioning */
/* You can increment the EPOCH value if the old configuration is not
 * compatible anymore, it creates an entire new one.
 * You need to increment GENERATION when you add new values to the
 * configuration file but is not needed to delete the existing conf  */
#define MOD_CONFIG_FILE_EPOCH 0x0002
#define MOD_CONFIG_FILE_GENERATION 0x008d
#define MOD_CONFIG_FILE_VERSION \
   ((MOD_CONFIG_FILE_EPOCH << 16) | MOD_CONFIG_FILE_GENERATION)

/* More mac/def; Define your own. What do you need ? */
#define CONN_DEVICE_ETHERNET 0

/* We create a structure config for our module, and also a config structure
 * for every item element (you can have multiple gadgets for the same module) */

typedef struct _Config Config;
typedef struct _Config_Item Config_Item;

struct _Config 
{
   E_Module *module;
   E_Config_Dialog *cfd;
   Eina_List *conf_items;
   int count, notify, version, full_dialog, mode_dialog; 
   
   const char *viewer;
   const char *path;
   unsigned char view_enable;
   double delay, pict_quality;
};

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

/* Function for calling the module's Configuration Dialog */
E_Config_Dialog *e_int_config_shot_module(E_Container *con, const char *params);

extern Config *shot_conf;

#endif
