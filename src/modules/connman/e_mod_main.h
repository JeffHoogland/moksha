/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "config.h"
#include <e.h>
#include <E_Connman.h>

#define MOD_CONF_VERSION 2


typedef struct E_Connman_Instance E_Connman_Instance;
typedef struct E_Connman_Module_Context E_Connman_Module_Context;
typedef struct E_Connman_Service E_Connman_Service;

struct E_Connman_Instance
{
   E_Connman_Module_Context *ctxt;
   E_Gadcon_Client *gcc;
   E_Gadcon_Popup *popup;
   E_Menu *menu;

   /* used by popup */
   int offline_mode;
   const char *service_path;
   Eina_Bool first_selection;

   struct
   {
      Evas_Object *gadget;
      Evas_Object *list;
      Evas_Object *offline_mode;
      Evas_Object *button;
      Evas_Object *table;
      struct
      {
         Ecore_X_Window win;
         Ecore_Event_Handler *mouse_up;
         Ecore_Event_Handler *key_down;
      } input;
   } ui;

   E_Gadcon_Popup *tip;
   Evas_Object *o_tip;
};

struct E_Connman_Service
{
   EINA_INLIST;
   E_Connman_Module_Context *ctxt;
   E_Connman_Element *element;
   const char *path;
   const char *name;
   const char *type;
   const char *mode;
   const char *state;
   const char *error;
   const char *security;
   const char *ipv4_method;
   const char *ipv4_address;
   const char *ipv4_netmask;
   unsigned char strength;
   Eina_Bool favorite:1;
   Eina_Bool auto_connect:1;
   Eina_Bool pass_required:1;
};

struct E_Connman_Module_Context
{
   Eina_List *instances;

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

   Eina_Bool has_manager:1;
   bool offline_mode;
   bool offline_mode_pending;
   const char *technology;
   const E_Connman_Service *default_service;
   Eina_Inlist *services;
};

EAPI extern E_Module_Api e_modapi;
EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

#endif
