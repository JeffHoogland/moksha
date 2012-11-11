/* Splash Screen */

#include "e.h"
#include "e_mod_main.h"

static Ecore_Event_Handler *_update_handler = NULL;
static Ecore_Timer *_next_timer = NULL;

EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_shutdown(E_Wizard_Page *pg __UNUSED__)
{
   if (_update_handler) ecore_event_handler_del(_update_handler);
   _update_handler = NULL;
   if (_next_timer) ecore_timer_del(_next_timer);
   _next_timer = NULL;
   return 1;
}

static Eina_Bool
_next_page(void *data __UNUSED__)
{
   _next_timer = NULL;
   if (_update_handler) ecore_event_handler_del(_update_handler);
   _update_handler = NULL;
   e_wizard_button_next_enable_set(1);
   e_wizard_next();
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_cb_desktops_update(void *data __UNUSED__, int ev_type __UNUSED__, void *ev __UNUSED__)
{
  if (_update_handler) ecore_event_handler_del(_update_handler);
   _update_handler = NULL;
   if (_next_timer) ecore_timer_del(_next_timer);
   _next_timer = ecore_timer_add(2.0, _next_page, NULL);
   return ECORE_CALLBACK_PASS_ON;
}

EAPI int
wizard_page_show(E_Wizard_Page *pg __UNUSED__)
{
   Evas_Object *o;

   e_wizard_title_set(_("Enlightenment"));
   e_wizard_button_next_enable_set(0);
   o = edje_object_add(pg->evas);
   e_theme_edje_object_set(o, "base/theme/wizard", "e/wizard/firstpage");
   e_wizard_page_show(o);

   _update_handler =
     ecore_event_handler_add(EFREET_EVENT_DESKTOP_CACHE_UPDATE,
                             _cb_desktops_update, NULL);

   /* advance in 15 sec anyway if no efreet update comes */
   _next_timer = ecore_timer_add(15.0, _next_page, NULL);
   return 1;
}

EAPI int
wizard_page_hide(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_apply(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

