#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "config.h"
#include <e.h>
#define E_CONNMAN_I_KNOW_THIS_API_IS_SUBJECT_TO_CHANGE 1
#include <E_Connman.h>
#include <eina_log.h>

#define MOD_CONF_VERSION 2

extern int _e_connman_log_dom;
#define DBG(...) EINA_LOG_DOM_DBG(_e_connman_log_dom, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(_e_connman_log_dom, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_e_connman_log_dom, __VA_ARGS__)

typedef struct E_Connman_Instance       E_Connman_Instance;
typedef struct E_Connman_Module_Context E_Connman_Module_Context;
typedef struct E_Connman_Service        E_Connman_Service;
typedef struct E_Connman_Technology     E_Connman_Technology;

struct E_Connman_Instance
{
   E_Connman_Module_Context *ctxt;
   E_Gadcon_Client          *gcc;
   E_Gadcon_Popup           *popup;
   E_Menu                   *menu;

   /* used by popup */
   int                       offline_mode;
   const char               *service_path;
   Eina_Bool                 first_selection;

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

struct E_Connman_Service
{
                             EINA_INLIST;
   E_Connman_Module_Context *ctxt;
   E_Connman_Element        *element;
   const char               *path;
   const char               *name;
   const char               *type;
   const char               *mode;
   const char               *state;
   const char               *error;
   const char               *security;
   const char               *ipv4_method;
   const char               *ipv4_address;
   const char               *ipv4_netmask;
   unsigned char             strength;
   Eina_Bool                 favorite : 1;
   Eina_Bool                 auto_connect : 1;
   Eina_Bool                 pass_required : 1;
};

struct E_Connman_Technology
{
                             EINA_INLIST;
   E_Connman_Module_Context *ctxt;
   E_Connman_Element        *element;
   const char               *path;
   const char               *name;
   const char               *type;
   const char               *state;
};

struct E_Connman_Module_Context
{
   Eina_List       *instances;
   E_Config_Dialog *conf_dialog;

   struct st_connman_actions
   {
      E_Action *toggle_offline_mode;
   } actions;

   struct
   {
      Ecore_Event_Handler *manager_in;
      Ecore_Event_Handler *manager_out;
      Ecore_Event_Handler *mode_changed;
   } event;

   struct
   {
      Ecore_Poller *default_service_changed;
      Ecore_Poller *manager_changed;
   } poller;

   Eina_Bool                has_manager : 1;
   Eina_Bool                offline_mode;
   Eina_Bool                offline_mode_pending;
   const char              *technology;
   const E_Connman_Service *default_service;
   Eina_Inlist             *services;
   Eina_Inlist             *technologies;
};

EAPI extern E_Module_Api e_modapi;
EAPI void       *e_modapi_init(E_Module *m);
EAPI int         e_modapi_shutdown(E_Module *m);
EAPI int         e_modapi_save(E_Module *m);

const char      *e_connman_theme_path(void);
E_Config_Dialog *e_connman_config_dialog_new(E_Container              *con,
                                             E_Connman_Module_Context *ctxt);
void             _connman_toggle_offline_mode(E_Connman_Module_Context *ctxt);
Evas_Object     *_connman_service_new_list_item(Evas              *evas,
                                                E_Connman_Service *service);

static inline void
_connman_dbus_error_show(const char      *msg,
                         const DBusError *error)
{
   const char *name;

   if ((!error) || (!dbus_error_is_set(error)))
     return;

   name = error->name;
   if (strncmp(name, "org.moblin.connman.Error.",
               sizeof("org.moblin.connman.Error.") - 1) == 0)
     name += sizeof("org.moblin.connman.Error.") - 1;

   e_util_dialog_show(_("Connman Server Operation Failed"),
                      _("Could not execute remote operation:<br>"
                        "%s<br>"
                        "Server Error <hilight>%s:</hilight> %s"),
                      msg, name, error->message);
}

static inline void
_connman_operation_error_show(const char *msg)
{
   e_util_dialog_show(_("Connman Operation Failed"),
                      _("Could not execute local operation:<br>%s"),
                      msg);
}

static inline E_Connman_Service *
_connman_ctxt_find_service_stringshare(const E_Connman_Module_Context *ctxt,
                                       const char                     *service_path)
{
   E_Connman_Service *itr;

   EINA_INLIST_FOREACH(ctxt->services, itr)
   if (itr->path == service_path)
     return itr;

   return NULL;
}

static inline E_Connman_Technology *
_connman_ctxt_technology_find_stringshare(const E_Connman_Module_Context *ctxt,
                                          const char                     *path)
{
   E_Connman_Technology *t;

   EINA_INLIST_FOREACH(ctxt->technologies, t)
   if (t->path == path)
     return t;

   return NULL;
}

#endif
