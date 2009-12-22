/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/* actual module specifics */

static void  _e_mod_action_conf_cb(E_Object *obj, const char *params);
static void  _e_mod_conf_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void  _e_mod_menu_add(void *data, E_Menu *m);

static E_Module *conf_module = NULL;
static E_Action *act = NULL;
static E_Int_Menu_Augmentation *maug = NULL;

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *client_class);
static void _cb_button_click(void *data, void *data2);

/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION, "configuration",
     {
        _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, 
        e_gadcon_site_is_not_toolbar
     },
   E_GADCON_CLIENT_STYLE_PLAIN
};

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o, *icon;
   E_Gadcon_Client *gcc;

   o = e_widget_button_add(gc->evas, NULL, NULL, 
                           _cb_button_click, NULL, NULL);

   icon = e_icon_add(evas_object_evas_get(o));
   e_util_icon_theme_set(icon, "preferences-system");
   e_widget_button_icon_set(o, icon);

   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = o;
   e_gadcon_client_util_menu_attach(gcc);

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
   return _("Settings");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas)
{
   Evas_Object *o;
   char buf[PATH_MAX];

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
_cb_button_click(void *data, void *data2) 
{
   E_Action *a;

   a = e_action_find("configuration");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
}

/* module setup */
EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Conf" };

EAPI void *
e_modapi_init(E_Module *m)
{
   conf_module = m;
   /* add module supplied action */
   act = e_action_add("configuration");
   if (act)
     {
	act->func.go = _e_mod_action_conf_cb;
	e_action_predef_name_set(_("Launch"), _("Settings Panel"), 
                                 "configuration", NULL, NULL, 0);
     }
   maug = 
     e_int_menus_menu_augmentation_add_sorted("config/0", _("Settings Panel"), 
                                              _e_mod_menu_add, NULL, NULL, NULL);
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
   if (zone && params)
      e_configure_registry_call(params, zone->container, params);
   else if (zone)
      e_configure_show(zone->container);
}

/* menu item callback(s) */
static void 
_e_mod_conf_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   e_configure_show(m->zone->container);
}

static void
_e_mod_mode_presentation_toggle(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi)
{
   e_config->mode.presentation = !e_config->mode.presentation;
   e_menu_item_toggle_set(mi, e_config->mode.presentation);
   e_config_mode_changed();
   e_config_save_queue();
}

static void
_e_mod_mode_offline_toggle(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi)
{
   e_config->mode.offline = !e_config->mode.offline;
   e_menu_item_toggle_set(mi, e_config->mode.offline);
   e_config_mode_changed();
   e_config_save_queue();
}

static void
_e_mod_submenu_modes_fill(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;

   mi = e_menu_item_new(m);
   e_menu_item_check_set(mi, 1);
   e_menu_item_toggle_set(mi, e_config->mode.presentation);
   e_menu_item_label_set(mi, _("Presentation"));
   e_util_menu_item_theme_icon_set(mi, "preferences-modes-presentation");
   e_menu_item_callback_set(mi, _e_mod_mode_presentation_toggle, NULL);

   mi = e_menu_item_new(m);
   e_menu_item_check_set(mi, 1);
   e_menu_item_toggle_set(mi, e_config->mode.offline);
   e_menu_item_label_set(mi, _("Offline"));
   e_util_menu_item_theme_icon_set(mi, "preferences-modes-offline");
   e_menu_item_callback_set(mi, _e_mod_mode_offline_toggle, NULL);

   e_menu_pre_activate_callback_set(m, NULL, NULL);
}

static E_Menu *
_e_mod_submenu_modes_get(void)
{
   E_Menu *m = e_menu_new();
   if (!m) return NULL;
   e_menu_pre_activate_callback_set(m, _e_mod_submenu_modes_fill, NULL);
   return m;
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

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Modes"));
   e_util_menu_item_theme_icon_set(mi, "preferences-modes");
   e_menu_item_submenu_set(mi, _e_mod_submenu_modes_get());
}
