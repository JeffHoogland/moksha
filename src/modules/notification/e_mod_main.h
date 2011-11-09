#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define HAVE_EDBUS 1
#include <e.h>
#include <E_Notification_Daemon.h>
#include "config.h"

#define MOD_CFG_FILE_EPOCH 0x0002
#define MOD_CFG_FILE_GENERATION 0x0006
#define MOD_CFG_FILE_VERSION					\
((MOD_CFG_FILE_EPOCH << 16) | MOD_CFG_FILE_GENERATION)

#undef  MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))


typedef enum   _Popup_Corner Popup_Corner;
typedef struct _Config Config;
typedef struct _Config_Item Config_Item;
typedef struct _Instance Instance;
typedef struct _Popup_Data Popup_Data;
typedef struct _Notification_Box Notification_Box;
typedef struct _Notification_Box_Icon Notification_Box_Icon;

enum _Popup_Corner
  {
    CORNER_TL,
    CORNER_TR,
    CORNER_BL,
    CORNER_BR
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

  struct
  {
    Eina_Bool presentation;
    Eina_Bool offline;
  } last_config_mode;
  
  Eina_List  *instances;
  Eina_List  *n_box;
  Eina_List  *config_dialog;
  E_Menu     *menu;
  Eina_List  *handlers;
  Eina_List  *items;
  Eina_List  *popups;
  int         next_id;

  Ecore_Timer *initial_mode_timer;
  E_Notification_Daemon *daemon;
};

struct _Config_Item 
{
  const char *id;
  int show_label;
  int show_popup;
  int focus_window;
  int store_low;
  int store_normal;
  int store_critical;
};

struct _Instance 
{
  E_Gadcon_Client  *gcc;
  Notification_Box *n_box;
  Config_Item      *ci;
};


struct _Notification_Box
{
  const char     *id;
  Instance       *inst;
  Evas_Object    *o_box;
  Evas_Object    *o_empty;
  Eina_List      *icons;
};

struct _Notification_Box_Icon
{
  Notification_Box *n_box;
  unsigned int      n_id;
  const char       *label;
  Evas_Object      *o_holder;
  Evas_Object      *o_icon;
  Evas_Object      *o_holder2;
  Evas_Object      *o_icon2;
  E_Border         *border;
  E_Notification   *notif;

  int             popup;
  Ecore_Timer    *mouse_in_timer;
};

struct _Popup_Data
{
  E_Notification *notif;
  E_Popup *win;
  Evas *e;
  Evas_Object *theme;
  const char  *app_name;
  Evas_Object *app_icon;
  Ecore_Timer *timer;
  E_Zone *zone;
};


int  notification_popup_notify(E_Notification *n, unsigned int replaces_id, const char *appname);
void notification_popup_shutdown(void);
void notification_popup_close(unsigned int id);

void notification_box_notify(E_Notification *n, unsigned int replaces_id, unsigned int id);
void notification_box_shutdown(void);
void notification_box_del(const char *id);
void notification_box_visible_set(Notification_Box *b, Eina_Bool visible);
Notification_Box *notification_box_get(const char *id, Evas *evas);
Config_Item *notification_box_config_item_get(const char *id);
void notification_box_orient_set(Notification_Box *b, int horizontal);
void notification_box_cb_obj_moveresize(void *data, Evas *e, Evas_Object *obj, void *event_info);
Eina_Bool  notification_box_cb_border_remove(void *data, int type, E_Event_Border_Remove *ev);

EAPI extern E_Module_Api e_modapi;
EAPI void  *e_modapi_init(E_Module *m);
EAPI int    e_modapi_shutdown(E_Module *m);
EAPI int    e_modapi_save(E_Module *m);

void _gc_orient    (E_Gadcon_Client *gcc, E_Gadcon_Orient orient);

void config_notification_box_module(Config_Item *ci);

E_Config_Dialog *e_int_config_notification_module(E_Container *con, 
						  const char *params __UNUSED__);

extern E_Module *notification_mod;
extern Config   *notification_cfg;
extern const E_Gadcon_Client_Class _gc_class;

#endif
