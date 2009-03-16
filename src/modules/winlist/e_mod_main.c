/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/***************************************************************************/
/**/
/* actual module specifics */

static void _e_mod_action_winlist_cb(E_Object *obj, const char *params);
static void _e_mod_action_winlist_mouse_cb(E_Object *obj, const char *params, Ecore_Event_Mouse_Button *ev);
static void _e_mod_action_winlist_key_cb(E_Object *obj, const char *params, Ecore_Event_Key *ev);

static E_Module *conf_module = NULL;
static E_Action *act = NULL;

/**/
/***************************************************************************/

/***************************************************************************/
/**/

/**/
/***************************************************************************/

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Winlist"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   conf_module = m;
   e_winlist_init();
   /* add module supplied action */
   act = e_action_add("winlist");
   if (act)
     {
	act->func.go = _e_mod_action_winlist_cb;
	act->func.go_mouse = _e_mod_action_winlist_mouse_cb;
	act->func.go_key = _e_mod_action_winlist_key_cb;
	e_action_predef_name_set(_("Window : List"), _("Next Window"), "winlist",
				 "next", NULL, 0);
	e_action_predef_name_set(_("Window : List"), _("Previous Window"),
				 "winlist", "prev", NULL, 0);
     }
   e_module_delayed_set(m, 1);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   /* remove module-supplied action */
   if (act)
     {
	e_action_predef_name_del(_("Window : List"), _("Previous Window"));
	e_action_predef_name_del(_("Window : List"), _("Next Window"));
	e_action_del("winlist");
	act = NULL;
     }
   e_winlist_shutdown();
   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}

/* action callback */
static void
_e_mod_action_winlist_cb(E_Object *obj, const char *params)
{
   E_Zone *zone = NULL;
   
   if (obj)
     {
	if (obj->type == E_MANAGER_TYPE)
	  zone = e_util_zone_current_get((E_Manager *)obj);
	else if (obj->type == E_CONTAINER_TYPE)
	  zone = e_util_zone_current_get(((E_Container *)obj)->manager);
	else if (obj->type == E_ZONE_TYPE)
	  zone = e_util_zone_current_get(((E_Zone *)obj)->container->manager);
	else
	  zone = e_util_zone_current_get(e_manager_current_get());
     }
   if (!zone) zone = e_util_zone_current_get(e_manager_current_get());
   if (zone)
     {
	if (params)
	  {
	     if (!strcmp(params, "next"))
	       {
		  if (!e_winlist_show(zone))
		    e_winlist_next();
	       }
	     else if (!strcmp(params, "prev"))
	       {
		  if (!e_winlist_show(zone))
		    e_winlist_prev();
	       }
	  }
	else
	  {
	     if (!e_winlist_show(zone))
	       e_winlist_next();
	  }
     }
}

static void
_e_mod_action_winlist_mouse_cb(E_Object *obj, const char *params, Ecore_Event_Mouse_Button *ev)
{
   E_Zone *zone = NULL;
   
   if (obj)
     {
	if (obj->type == E_MANAGER_TYPE)
	  zone = e_util_zone_current_get((E_Manager *)obj);
	else if (obj->type == E_CONTAINER_TYPE)
	  zone = e_util_zone_current_get(((E_Container *)obj)->manager);
	else if (obj->type == E_ZONE_TYPE)
	  zone = e_util_zone_current_get(((E_Zone *)obj)->container->manager);
	else
	  zone = e_util_zone_current_get(e_manager_current_get());
     }
   if (!zone) zone = e_util_zone_current_get(e_manager_current_get());
   if (zone)
     {
	if (params)
	  {
	     if (!strcmp(params, "next"))
	       {
		  if (e_winlist_show(zone))
		    e_winlist_modifiers_set(ev->modifiers);
		  else
		    e_winlist_next();
	       }
	     else if (!strcmp(params, "prev"))
	       {
		  if (e_winlist_show(zone))
		    e_winlist_modifiers_set(ev->modifiers);
		  else
		    e_winlist_prev();
	       }
	  }
	else
	  {
	     if (e_winlist_show(zone))
	       e_winlist_modifiers_set(ev->modifiers);
	     else
	       e_winlist_next();
	  }
     }
}

static void
_e_mod_action_winlist_key_cb(E_Object *obj, const char *params, Ecore_Event_Key *ev)
{
   E_Zone *zone = NULL;
   
   if (obj)
     {
	if (obj->type == E_MANAGER_TYPE)
	  zone = e_util_zone_current_get((E_Manager *)obj);
	else if (obj->type == E_CONTAINER_TYPE)
	  zone = e_util_zone_current_get(((E_Container *)obj)->manager);
	else if (obj->type == E_ZONE_TYPE)
	  zone = e_util_zone_current_get(((E_Zone *)obj)->container->manager);
	else
	  zone = e_util_zone_current_get(e_manager_current_get());
     }
   if (!zone) zone = e_util_zone_current_get(e_manager_current_get());
   if (zone)
     {
	if (params)
	  {
	     if (!strcmp(params, "next"))
	       {
		  if (e_winlist_show(zone))
		    e_winlist_modifiers_set(ev->modifiers);
		  else
		    e_winlist_next();
	       }
	     else if (!strcmp(params, "prev"))
	       {
		  if (e_winlist_show(zone))
		    e_winlist_modifiers_set(ev->modifiers);
		  else
		    e_winlist_prev();
	       }
	  }
	else
	  {
	     if (e_winlist_show(zone))
	       e_winlist_modifiers_set(ev->modifiers);
	     else
	       e_winlist_next();
	  }
     }
}
