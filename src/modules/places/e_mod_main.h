#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#ifdef ENABLE_NLS
# include <libintl.h>
# define D_(string) dgettext(PACKAGE, string)
#else
# define bindtextdomain(domain,dir)
# define bind_textdomain_codeset(domain,codeset)
# define D_(string) (string)
#endif

/* Macros used for config file versioning */
#define MOD_CONFIG_FILE_EPOCH 0x0001
#define MOD_CONFIG_FILE_GENERATION 0x008f
#define MOD_CONFIG_FILE_VERSION \
   ((MOD_CONFIG_FILE_EPOCH << 16) | MOD_CONFIG_FILE_GENERATION)

typedef struct _Config Config;
struct _Config 
{
   E_Module *module;
   E_Config_Dialog *cfd;

   Eina_List *conf_items;

   /* config file version */
   int version;

   const char *fm;
   unsigned char auto_mount;
   unsigned char boot_mount;
   unsigned char auto_open;
   unsigned char show_menu;
   unsigned char hide_header;

   unsigned char show_home;
   unsigned char show_desk;
   unsigned char show_trash;
   unsigned char show_root;
   unsigned char show_temp;
   unsigned char show_bookm;
};

typedef struct _Config_Item Config_Item;
struct _Config_Item 
{
   const char *id;

   int switch2;
};

typedef struct _Instance Instance;
struct _Instance 
{
   E_Gadcon_Client *gcc;
   Evas_Object *o_box;
   E_Menu *menu;
   Config_Item *conf_item;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

E_Config_Dialog *e_int_config_places_module(E_Container *con, const char *params);
void places_menu_augmentation(void);

extern Config *places_conf;
extern Eina_List *instances;

#endif
