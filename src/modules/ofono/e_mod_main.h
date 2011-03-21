#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "config.h"
#include <e.h>
#include <E_Ofono.h>
#include <eina_log.h>

#define MOD_CONF_VERSION 2

extern int _e_ofono_module_log_dom;
#define DBG(...)  EINA_LOG_DOM_DBG(_e_ofono_module_log_dom, __VA_ARGS__)
#define WRN(...)  EINA_LOG_DOM_WARN(_e_ofono_module_log_dom, __VA_ARGS__)
#define CRIT(...) EINA_LOG_DOM_CRIT(_e_ofono_module_log_dom, __VA_ARGS__)
#define ERR(...)  EINA_LOG_DOM_ERR(_e_ofono_module_log_dom, __VA_ARGS__)

typedef struct E_Ofono_Instance       E_Ofono_Instance;
typedef struct E_Ofono_Module_Context E_Ofono_Module_Context;

struct E_Ofono_Instance
{
   E_Ofono_Module_Context *ctxt;
   E_Gadcon_Client        *gcc;
   E_Gadcon_Popup         *popup;
   E_Menu                 *menu;

   struct
   {
      Evas_Object *gadget;
      Evas_Object *table;
      Evas_Object *name;
      Evas_Object *powered;
      struct
      {
         Ecore_X_Window       win;
         Ecore_Event_Handler *mouse_up;
         Ecore_Event_Handler *key_down;
      } input;
   } ui;

   E_Gadcon_Popup  *tip;
   Evas_Object     *o_tip;

   /* e_dbus ofono element pointers */
   E_Ofono_Element *modem_element;
   E_Ofono_Element *netreg_element;

   /* modem data */
   const char      *path;
   const char      *name;
   const char      *status;
   const char      *op;
   int              int_powered; /* used by popup */
   Eina_Bool        powered;
   uint8_t          strength;

   Eina_Bool        powered_pending : 1;
};

struct E_Ofono_Module_Context
{
   Eina_List *instances;

   struct
   {
      Ecore_Event_Handler *manager_in;
      Ecore_Event_Handler *manager_out;
      Ecore_Event_Handler *element_add;
      Ecore_Event_Handler *element_del;
      Ecore_Event_Handler *element_updated;
   } event;

   struct
   {
      Ecore_Poller *manager_changed;
   } poller;

   Eina_Bool has_manager : 1;
};

EAPI extern E_Module_Api e_modapi;
EAPI void  *e_modapi_init(E_Module *m);
EAPI int    e_modapi_shutdown(E_Module *m);
EAPI int    e_modapi_save(E_Module *m);

const char *e_ofono_theme_path(void);

static inline void
_ofono_dbus_error_show(const char      *msg,
                       const DBusError *error)
{
   const char *name;

   if ((!error) || (!dbus_error_is_set(error)))
     return;

   name = error->name;
   if (strncmp(name, "org.ofono.Error.", sizeof("org.ofono.Error.") - 1) == 0)
     name += sizeof("org.ofono.Error.") - 1;

   e_util_dialog_show(_("Ofono Server Operation Failed"),
                      _("Could not execute remote operation:<br>"
                        "%s<br>"
                        "Server Error <hilight>%s:</hilight> %s"),
                      msg, name, error->message);
}

static inline void
_ofono_operation_error_show(const char *msg)
{
   e_util_dialog_show(_("Ofono Operation Failed"),
                      _("Could not execute local operation:<br>%s"),
                      msg);
}

#endif
