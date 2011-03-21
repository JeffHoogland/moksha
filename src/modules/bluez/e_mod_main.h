#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "config.h"
#include <e.h>
#include <E_Bluez.h>
#include <eina_log.h>

#define MOD_CONF_VERSION 2

extern int _e_bluez_log_dom;
#define DBG(...) EINA_LOG_DOM_DBG(_e_bluez_log_dom, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(_e_bluez_log_dom, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_e_bluez_log_dom, __VA_ARGS__)

typedef struct E_Bluez_Instance        E_Bluez_Instance;
typedef struct E_Bluez_Instance_Device E_Bluez_Instance_Device;
typedef struct E_Bluez_Module_Context  E_Bluez_Module_Context;

struct E_Bluez_Instance
{
   E_Bluez_Module_Context *ctxt;
   E_Gadcon_Client        *gcc;
   E_Gadcon_Popup         *popup;
   E_Menu                 *menu;

   /* used by popup */
   int                     powered;
   Eina_Bool               first_selection;
   const char             *address;
   const char             *alias;

   Eina_List              *devices;
   E_Bluez_Element        *adapter;
   double                  last_scan;
   Eina_Bool               discovering : 1;
   Eina_Bool               powered_pending : 1;
   Eina_Bool               discoverable : 1;

   struct
   {
      Evas_Object *gadget;
      Evas_Object *list;
      Evas_Object *powered;
      Evas_Object *button;
      Evas_Object *control;
      struct
      {
         Ecore_X_Window       win;
         Ecore_Event_Handler *mouse_up;
         Ecore_Event_Handler *key_down;
      } input;
   } ui;

   E_Gadcon_Popup  *tip;
   Evas_Object     *o_tip;

   E_Config_Dialog *conf_dialog;
};

struct E_Bluez_Instance_Device
{
   const char *address;
   const char *alias;
   // TODO (and also show list icon!): Eina_Bool paired:1;
   // TODO (and also show list icon!): Eina_Bool trusted:1;
   // TODO (and also show list icon!): Eina_Bool connected:1;
   // TODO ... class, icon
};

struct E_Bluez_Module_Context
{
   Eina_List  *instances;
   const char *default_adapter;

   struct
   {
      E_DBus_Connection *conn;
      E_DBus_Interface  *iface;
      E_DBus_Object     *obj;
      Eina_List         *pending;
   } agent;

   struct
   {
      E_Action *toggle_powered;
   } actions;

   struct
   {
      Ecore_Event_Handler *manager_in;
      Ecore_Event_Handler *manager_out;
      Ecore_Event_Handler *device_found;
      Ecore_Event_Handler *element_updated;
   } event;

   struct
   {
      Ecore_Poller *manager_changed;
   } poller;

   Eina_Bool has_manager : 1;
};

EAPI extern E_Module_Api e_modapi;
EAPI void       *e_modapi_init(E_Module *m);
EAPI int         e_modapi_shutdown(E_Module *m);
EAPI int         e_modapi_save(E_Module *m);

const char      *e_bluez_theme_path(void);
E_Config_Dialog *e_bluez_config_dialog_new(E_Container      *con,
                                           E_Bluez_Instance *inst);
void             _bluez_toggle_powered(E_Bluez_Instance *inst);

static inline void
_bluez_dbus_error_show(const char      *msg,
                       const DBusError *error)
{
   const char *name;

   if ((!error) || (!dbus_error_is_set(error)))
     return;

   name = error->name;
   if (strncmp(name, "org.bluez.Error.",
               sizeof("org.bluez.Error.") - 1) == 0)
     name += sizeof("org.bluez.Error.") - 1;

   e_util_dialog_show(_("Bluez Server Operation Failed"),
                      _("Could not execute remote operation:<br>"
                        "%s<br>"
                        "Server Error <hilight>%s:</hilight> %s"),
                      msg, name, error->message);
}

static inline void
_bluez_operation_error_show(const char *msg)
{
   e_util_dialog_show(_("Bluez Operation Failed"),
                      _("Could not execute local operation:<br>%s"),
                      msg);
}

#endif
