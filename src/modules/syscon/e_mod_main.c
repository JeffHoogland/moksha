/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/***************************************************************************/
/**/
/* actual module specifics */

static void  _e_mod_action_syscon_cb(E_Object *obj, const char *params);
static int   _e_mod_syscon_defer_cb(void *data);
static void  _e_mod_syscon_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void  _e_mod_menu_add(void *data, E_Menu *m);

static E_Module *conf_module = NULL;
static E_Action *act = NULL;
static E_Int_Menu_Augmentation *maug = NULL;

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
     "Syscon"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   conf_module = m;
   e_syscon_init();
   /* add module supplied action */
   act = e_action_add("syscon");
   if (act)
     {
	act->func.go = _e_mod_action_syscon_cb;
	e_action_predef_name_set(_("System"), _("System Control"), "syscon",
				 NULL, NULL, 0);
     }
   maug = e_int_menus_menu_augmentation_add("main/10", _e_mod_menu_add, NULL, NULL, NULL);
   e_module_delayed_set(m, 1);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   /* remove module-supplied menu additions */
   if (maug)
     {
	e_int_menus_menu_augmentation_del("main/10", maug);
	maug = NULL;
     }
   /* remove module-supplied action */
   if (act)
     {
	e_action_predef_name_del(_("System"), _("System Controls"));
	e_action_del("syscon");
	act = NULL;
     }
   e_syscon_shutdown();
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
_e_mod_action_syscon_cb(E_Object *obj, const char *params)
{
   E_Zone *zone = NULL;

   // params = syscon action + now:
   // desk_lock
   // logout
   // halt
   // reboot
   // suspend
   // hibernate
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
   if (zone) e_syscon_show(zone, params);
}

/* menu item callback(s) */
static int
_e_mod_syscon_defer_cb(void *data)
{
   E_Zone *zone;
   
   zone = data;
   if (zone) e_syscon_show(zone, NULL);
   return 0;
}

static void 
_e_mod_syscon_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   ecore_idle_enterer_add(_e_mod_syscon_defer_cb, m->zone);
}

/* menu item add hook */
static void
_e_mod_menu_add(void *data, E_Menu *m)
{
   E_Menu_Item *mi;
   
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("System"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/system");
   e_menu_item_callback_set(mi, _e_mod_syscon_cb, NULL);
}
