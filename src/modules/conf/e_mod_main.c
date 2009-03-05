/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/***************************************************************************/
/**/
/* actual module specifics */

static void  _e_mod_action_conf_cb(E_Object *obj, const char *params);
static void  _e_mod_conf_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void  _e_mod_menu_add(void *data, E_Menu *m);

static E_Module *conf_module = NULL;
static E_Action *act = NULL;
static E_Int_Menu_Augmentation *maug = NULL;

/**/
/***************************************************************************/

/***************************************************************************/
/**/
/* gadget */
/***************************************************************************/
/**/
/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *client_class);
static void _cb_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info);
/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
     "configuration",
     {
        _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL
     },
   E_GADCON_CLIENT_STYLE_PLAIN
};

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   
   o = e_icon_add(gc->evas);
   e_util_icon_theme_set(o, "preferences-system");
   evas_object_show(o);
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = o;
   e_gadcon_client_util_menu_attach(gcc);

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP,
                                  _cb_mouse_up, NULL);
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   evas_object_del(gcc->o_base);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   Evas_Coord mw, mh;
   
   mw = 0, mh = 0;
   edje_object_size_min_get(gcc->o_base, &mw, &mh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(gcc->o_base, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   e_gadcon_client_aspect_set(gcc, mw, mh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class)
{
   return "Settings";
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas)
{
   Evas_Object *o;
   char buf[4096];
   
   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-conf.edj",
            e_module_dir_get(conf_module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class)
{
   return _gadcon_class.name;
}

static void
_cb_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Action *a;

   a = e_action_find("configuration");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
}
/**/
/***************************************************************************/

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Conf"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   conf_module = m;
   /* add module supplied action */
   act = e_action_add("configuration");
   if (act)
     {
	act->func.go = _e_mod_action_conf_cb;
	e_action_predef_name_set(_("Launch"), _("Settings Panel"), "configuration",
				 NULL, NULL, 0);
     }
   maug = e_int_menus_menu_augmentation_add("config/0", _e_mod_menu_add, NULL, NULL, NULL);
   e_module_delayed_set(m, 1);
   e_gadcon_provider_register(&_gadcon_class);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   e_configure_del();
   e_gadcon_provider_unregister(&_gadcon_class);
   /* remove module-supplied menu additions */
   if (maug)
     {
	e_int_menus_menu_augmentation_del("config/0", maug);
	maug = NULL;
     }
   /* remove module-supplied action */
   if (act)
     {
	e_action_predef_name_del(_("Launch"), _("Settings Panel"));
	e_action_del("configuration");
	act = NULL;
     }
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
_e_mod_action_conf_cb(E_Object *obj, const char *params)
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
   if (zone) e_configure_show(zone->container);
}

/* menu item callback(s) */
static void 
_e_mod_conf_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   e_configure_show(m->zone->container);
}

/* menu item add hook */
static void
_e_mod_menu_add(void *data, E_Menu *m)
{
   E_Menu_Item *mi;
   
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Settings Panel"));
   e_util_menu_item_theme_icon_set(mi, "preferences-system");
   e_menu_item_callback_set(mi, _e_mod_conf_cb, NULL);
}
