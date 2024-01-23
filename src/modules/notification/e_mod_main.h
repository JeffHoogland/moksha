#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "e.h"

/* Enable/Disable debug messages */
// #define PRINT(...) do {} while (0)
#define PRINT(...) printf("\nNOTIFY: "__VA_ARGS__)

/* Increment for Major Changes */
#define MOD_CONFIG_FILE_EPOCH      2
/* Increment for Minor Changes (ie: user doesn't need a new config) */
#define MOD_CONFIG_FILE_GENERATION 3
#define MOD_CONFIG_FILE_VERSION    ((MOD_CONFIG_FILE_EPOCH * 1000000) + MOD_CONFIG_FILE_GENERATION)

/* Notification history list defines */
#define HIST_MIN    5
#define HIST_MAX   50
#define HIST_MAX_DIGITS 2

typedef struct _Hist_eet
{
  unsigned int version;
  char *path;
  Eina_List *history;
} Hist_eet;


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
    CORNER_BR,
    CORNER_R,
    CORNER_L,
    CORNER_T,
    CORNER_B
  };

typedef enum
{
   POPUP_DISPLAY_POLICY_FIRST,
   POPUP_DISPLAY_POLICY_CURRENT,
   POPUP_DISPLAY_POLICY_ALL,
   POPUP_DISPLAY_POLICY_MULTI
} Popup_Display_Policy;

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
  int mute;
  float timeout;
  Popup_Corner corner;
  Eina_List  *instances;

  int time_stamp;
  int show_app;
  int show_count;
  int reverse;
  Eina_Bool item_click;
  double item_length;
  double menu_items;
  double jump_delay;
  const char *blacklist;

  struct
  {
    Eina_Bool presentation;
    Eina_Bool offline;
  } last_config_mode;

  Ecore_Event_Handler  *handler;
  Eina_List  *popups;
  Hist_eet *  hist;
  unsigned int         next_id;
  int         new_item;

  //Ecore_Timer *initial_mode_timer;
  Ecore_Timer *jump_timer;
};

struct _Popup_Data
{
   unsigned id;
   E_Notification_Notify *notif;
   E_Popup *win;
   Evas *e;
   Evas_Object *theme;
   const char  *app_name;
   const char  *app_icon_image;
   Evas_Object *app_icon;
   Evas_Object *desktop_icon;
   Evas_Object *action_box;
   Eina_List *actions;
   Ecore_Timer *timer;
   Eina_Bool pending;
   E_Zone *zone;
};


struct _Popup_Items
{
  Eina_Stringshare *item_date_time; 
  Eina_Stringshare *item_app;
  Eina_Stringshare *item_icon;
  Eina_Stringshare *item_icon_img;
  Eina_Stringshare *item_title;
  Eina_Stringshare *item_body;
  E_Notification_Notify *notif;
  Eina_List *actions;
  unsigned int notif_id;
};


void notification_popup_notify(E_Notification_Notify *n, unsigned int id);
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
void             gadget_text(int number, Eina_Bool logo_jump);
void             free_menu_data(Popup_Items *items);
//

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
