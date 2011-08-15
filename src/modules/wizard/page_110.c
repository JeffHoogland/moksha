/* Setup if we need connman? */
#include "e.h"
#include "e_mod_main.h"
#ifdef HAVE_ECONNMAN             
#include <E_Connman.h>
#endif

static Ecore_Event_Handler *handler = NULL;
static Ecore_Timer *connman_timeout = NULL;

static void
_recommend_connman(E_Wizard_Page *pg)
{
   Evas_Object *o, *of, *ob;
   
   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("Network Management"));

   of = e_widget_framelist_add(pg->evas, 
                               _("Connman network service not found"), 0);
   
   ob = e_widget_label_add
      (pg->evas, _("Install Connman for network management support"));
   e_widget_framelist_object_append(of, ob);
   evas_object_show(ob);
   
   e_widget_list_object_append(o, of, 0, 0, 0.5);
   evas_object_show(ob);
   evas_object_show(of);
   
   e_wizard_page_show(o);
   pg->data = of;
   
   e_wizard_button_next_enable_set(1);
}

static Eina_Bool
_connman_in(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   if (handler)
     {
        ecore_event_handler_del(handler);
        handler = NULL;
     }
   if (connman_timeout)
     {
        ecore_timer_del(connman_timeout);
        connman_timeout = NULL;
     }
   e_wizard_button_next_enable_set(1);
   e_wizard_next();
   return EINA_TRUE;
}

static Eina_Bool
_connman_timeout(void *data)
{
   E_Wizard_Page *pg = data;
   E_Config_Module *em;
   Eina_List *l;
   
   EINA_LIST_FOREACH(e_config->modules, l, em)
     {
        if (!em->name) continue;
        if (!strcmp(em->name, "connman"))
          {
             e_config->modules = eina_list_remove_list
                (e_config->modules, l);
             if (em->name) eina_stringshare_del(em->name);
             free(em);
             break;
          }
     }
   e_config_save_queue();
   connman_timeout = NULL;
   if (handler)
     {
        ecore_event_handler_del(handler);
        handler = NULL;
     }
   _recommend_connman(pg);
   return EINA_FALSE;
}

EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_shutdown(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   int hav_connman = 0;
   E_DBus_Connection *c;
   
   c = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (c)
     {
        if (e_connman_system_init(c))
          {
#ifdef HAVE_ECONNMAN             
             handler = ecore_event_handler_add
                (E_CONNMAN_EVENT_MANAGER_IN, _connman_in, NULL);
             if (connman_timeout) ecore_timer_del(connman_timeout);
             connman_timeout = ecore_timer_add(2.0, _connman_timeout, pg);
             hav_connman = 1;
             e_wizard_button_next_enable_set(0);
#endif             
          }
     }
   if (!hav_connman)
     {
        E_Config_Module *em;
        Eina_List *l;
        
        EINA_LIST_FOREACH(e_config->modules, l, em)
          {
             if (!em->name) continue;
             if (!strcmp(em->name, "connman"))
               {
                  e_config->modules = eina_list_remove_list
                     (e_config->modules, l);
                  if (em->name) eina_stringshare_del(em->name);
                  free(em);
                  break;
               }
          }
        e_config_save_queue();
        _recommend_connman(pg);
     }
   e_wizard_title_set(_("Checking to see if Connman exists"));
   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}

EAPI int
wizard_page_hide(E_Wizard_Page *pg __UNUSED__)
{
   if (handler)
     {
        ecore_event_handler_del(handler);
        handler = NULL;
     }
   if (connman_timeout)
     {
        ecore_timer_del(connman_timeout);
        connman_timeout = NULL;
     }
   if (pg->data) evas_object_del(pg->data);
   return 1;
}

EAPI int
wizard_page_apply(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}
