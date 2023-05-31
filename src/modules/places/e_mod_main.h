#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

/* Macros used for config file versioning */
#define MOD_CONFIG_FILE_EPOCH 0x0001
#define MOD_CONFIG_FILE_GENERATION 0x008f
#define MOD_CONFIG_FILE_VERSION \
   ((MOD_CONFIG_FILE_EPOCH << 16) | MOD_CONFIG_FILE_GENERATION)

/* Check for dbus messages error */
#define PLACES_ON_MSG_ERROR_RETURN(msg)                                     \
   do                                                                       \
   {                                                                        \
      const char *err;                                                      \
      if (eldbus_message_error_get(msg, NULL, &err))                        \
      {                                                                     \
         EINA_SAFETY_ERROR(err);                                            \
         return;                                                            \
      }                                                                     \
   }                                                                        \
   while (0)

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
   int alert_p; 
   int alert_timeout;
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
   Evas_Object *o_main;
   Evas_Object *o_icon;
   E_Gadcon_Popup *popup;
   Eina_Bool horiz;
   E_Menu *menu;
   Config_Item *conf_item;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m __UNUSED__);
EAPI int e_modapi_save(E_Module *m __UNUSED__);

E_Config_Dialog *e_int_config_places_module(E_Container *con, const char *params);
void places_menu_augmentation(void);

extern Config *places_conf;
extern Eina_List *instances;

/**
 * @ingroup Optional_Gadgets Optional_Devices Optional_Control
 * @{
 *
 * @defgroup Module_Places Places
 *
 * Manage the volumes device attached to the system.
 *
 * Controls mounting and umounting of drives.
 * 
 * @}
 */

#endif
