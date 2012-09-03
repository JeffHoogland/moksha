#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "config.h"
#include <e.h>
#include <eina_log.h>

#include "E_Connman.h"

#define MOD_CONF_VERSION 3

extern int _e_connman_log_dom;

typedef struct E_Connman_Instance E_Connman_Instance;
typedef struct E_Connman_Module_Context E_Connman_Module_Context;

struct E_Connman_Instance
{
   E_Connman_Module_Context *ctxt;
   E_Gadcon_Client *gcc;
   E_Gadcon_Popup *popup;

   /* used by popup */
   int offline_mode;
   const char *service_path;

   struct
     {
        Evas_Object *gadget;
        Evas_Object *list;
        Evas_Object *offline_mode;
        Evas_Object *button;
        Evas_Object *table;

        struct
          {
             Ecore_X_Window       win;
             Ecore_Event_Handler *mouse_up;
             Ecore_Event_Handler *key_down;
          } input;
     } ui;

   E_Gadcon_Popup *tip;
   Evas_Object    *o_tip;
};

struct E_Connman_Module_Context
{
   Eina_List *instances;
   E_Config_Dialog *conf_dialog;

   struct st_connman_actions
     {
        E_Action *toggle_offline_mode;
     } actions;

   struct
     {
        Ecore_Event_Handler *manager_in;
        Ecore_Event_Handler *manager_out;
     } event;

   struct
   {
      Ecore_Poller *default_service_changed;
      Ecore_Poller *manager_changed;
   } poller;

   Eina_Bool has_manager;
   Eina_Bool offline_mode;
};

EAPI extern E_Module_Api e_modapi;
EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

const char *e_connman_theme_path(void);
E_Config_Dialog *e_connman_config_dialog_new(E_Container *con,
                                             E_Connman_Module_Context *ctxt);

/**
 * @addtogroup Optional_Devices
 * @{
 *
 * @defgroup Module_Connman ConnMan (Connection Manager)
 *
 * Controls network connections for ethernet, wifi, 3G, GSM and
 * bluetooth (PAN).
 *
 * @see http://connman.net/
 * @}
 */

#endif
