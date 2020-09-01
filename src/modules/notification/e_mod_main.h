#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "e.h"
#include <E_Notification_Daemon.h>

/* Increment for Major Changes */
#define MOD_CONFIG_FILE_EPOCH      2
/* Increment for Minor Changes (ie: user doesn't need a new config) */
#define MOD_CONFIG_FILE_GENERATION 1
#define MOD_CONFIG_FILE_VERSION    ((MOD_CONFIG_FILE_EPOCH * 1000000) + MOD_CONFIG_FILE_GENERATION)

typedef enum   _Popup_Corner Popup_Corner;
typedef struct _Config Config;
typedef struct _Popup_Data Popup_Data;
typedef struct _Popup_Items Popup_Items;

typedef struct _Instance Instance;

enum _Popup_Corner
  {
    CORNER_TL,
    CORNER_TR,
    CORNER_BL,
    CORNER_BR
  };
  
struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_notif;
   E_Menu *menu;
};

struct _Config 
{
  E_Config_Dialog *cfd;

  int version;
  int show_low;
  int show_normal;
  int show_critical;
  int force_timeout;
  int ignore_replacement;
  int dual_screen;
  float timeout;
  Popup_Corner corner;
  Eina_List  *instances;
  
  int time_stamp;
  int show_app;
  int reverse;
  Eina_Bool clicked_item;
  double item_length;
  double menu_items;
  const char *blacklist;
  
  struct
  {
    Eina_Bool presentation;
    Eina_Bool offline;
  } last_config_mode;
  
  Ecore_Event_Handler  *handler;
  Eina_List  *popups;
  Eina_List  *popup_items;
  int         next_id;
  Eina_Bool   new_item;

  Ecore_Timer *initial_mode_timer;
  E_Notification_Daemon *daemon;
};

struct _Popup_Data
{
  E_Notification *notif;
  E_Popup *win;
  Evas *e;
  Evas_Object *theme;
  const char  *app_name;
  const char  *app_icon_image;
  Evas_Object *app_icon;
  Ecore_Timer *timer;
  E_Zone *zone;
};

struct _Popup_Items
{
  char *item_date_time;
  char *item_app;
  char *item_icon;
  char *item_icon_img;
  char *item_title;
  char *item_body;
};


int  notification_popup_notify(E_Notification *n, unsigned int replaces_id, const char *appname);
void notification_popup_shutdown(void);
void notification_popup_close(unsigned int id);

EAPI extern E_Module_Api e_modapi;
EAPI void  *e_modapi_init(E_Module *m);
EAPI int    e_modapi_shutdown(E_Module *m);
EAPI int    e_modapi_save(E_Module *m);

//~ void _gc_orient    (E_Gadcon_Client *gcc, E_Gadcon_Orient orient);

E_Config_Dialog *e_int_config_notification_module(E_Container *con, const char *params);

extern E_Module *notification_mod;
extern Config   *notification_cfg;
void             gadget_text(Eina_List *list);
void             free_menu_data(Popup_Items *items);
int              write_history(Eina_List *popup_items);


/**
 * @addtogroup Optional_Monitors
 * @{
 *
 * @defgroup Module_Notification Notification (Notify-OSD)
 *
 * Presents notifications on screen as an unobtrusive popup. It
 * implements the Notify-OSD and FreeDesktop.org standards.
 *
 * @see http://www.galago-project.org/specs/notification/0.9/
 * @see https://wiki.ubuntu.com/NotifyOSD
 * @}
 */

#endif
