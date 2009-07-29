/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "e_mod_main.h"

/* actual module specifics */
static void  _e_mod_action_exebuf_cb(E_Object *obj, const char *params);
static int   _e_mod_run_defer_cb(void *data);
static void  _e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void  _e_mod_menu_add(void *data, E_Menu *m);

/* static E_Module *conf_module = NULL; */
static E_Action *act = NULL;
static E_Int_Menu_Augmentation *maug = NULL;

static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;
Config *evry_conf = NULL;

/* module setup */
EAPI E_Module_Api e_modapi =
  {
    E_MODULE_API_VERSION,
    "Everything"
  };

EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[4096];
   snprintf(buf, sizeof(buf), "%s/.e/e/config/%s/module.everything",
	    e_user_homedir_get(), e_config_profile_get());
   ecore_file_mkdir(buf);

   conf_item_edd = E_CONFIG_DD_NEW("Plugin_Config", Plugin_Config);
#undef T
#undef D
#define T Plugin_Config
#define D conf_item_edd
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, trigger, STR);
   E_CONFIG_VAL(D, T, min_query, INT);
   E_CONFIG_VAL(D, T, loaded, INT);
   E_CONFIG_VAL(D, T, enabled, INT);
   E_CONFIG_VAL(D, T, priority, INT);
   conf_edd = E_CONFIG_DD_NEW("Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_VAL(D, T, height, INT);
   E_CONFIG_VAL(D, T, rel_x, DOUBLE);
   E_CONFIG_VAL(D, T, rel_y, DOUBLE);
   E_CONFIG_VAL(D, T, scroll_animate, INT);
   E_CONFIG_VAL(D, T, scroll_speed, DOUBLE);
   E_CONFIG_LIST(D, T, plugins_conf, conf_item_edd);
#undef T
#undef D
   evry_conf = e_config_domain_load("module.everything", conf_edd);

   if (!evry_conf)
     {
	evry_conf = E_NEW(Config, 1);
	evry_conf->rel_x = 50.0;
	evry_conf->rel_y = 50.0;
	evry_conf->width = 400;
	evry_conf->height = 350;
	evry_conf->scroll_animate = 1;
	evry_conf->scroll_speed = 0.08;
     }
   
   /* conf_module = m; */
   evry_init();

   evry_plug_config_init();
   evry_plug_dir_browse_init();
   evry_plug_apps_init();
   evry_plug_tracker_init();
   evry_plug_border_init();
   evry_plug_border_act_init();
   evry_plug_calc_init();
   evry_plug_aspell_init();

   /* add module supplied action */
   act = e_action_add("everything");
   if (act)
     {
	act->func.go = _e_mod_action_exebuf_cb;
	e_action_predef_name_set(_("Launch"), _("Run Everything Dialog"), "everything",
				 NULL, NULL, 0);
     }

   maug = e_int_menus_menu_augmentation_add("main/1", _e_mod_menu_add, NULL, NULL, NULL);

   e_configure_registry_category_add("advanced", 80, _("Advanced"), NULL, "preferences-advanced");
   e_configure_registry_item_add("advanced/run_everything", 40, _("Run Everything"), NULL, "system-run", evry_config_dialog);
   
   e_module_delayed_set(m, 1);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_Config_Dialog *cfd;
   
   /* remove module-supplied menu additions */
   if (maug)
     {
   	e_int_menus_menu_augmentation_del("main/1", maug);
   	maug = NULL;
     }
   /* remove module-supplied action */
   if (act)
     {
	e_action_predef_name_del(_("Launch"), _("Run Everything Dialog"));
	e_action_del("everything");
	act = NULL;
     }

   evry_plug_config_shutdown();
   evry_plug_dir_browse_shutdown();
   evry_plug_apps_shutdown();
   evry_plug_tracker_shutdown();
   evry_plug_border_shutdown();
   evry_plug_border_act_shutdown();
   evry_plug_calc_shutdown();
   evry_plug_aspell_shutdown();

   evry_shutdown();
   /* conf_module = NULL; */

   while ((cfd = e_config_dialog_get("E", "_config_everything_dialog"))) e_object_del(E_OBJECT(cfd));
   e_configure_registry_item_del("advanced/run_everything");
   e_configure_registry_category_del("advanced");

   /* Clean EET */
   E_CONFIG_DD_FREE(conf_item_edd);
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.everything", conf_edd, evry_conf);
   return 1;
}

/* action callback */
static void
_e_mod_action_exebuf_cb(E_Object *obj, const char *params __UNUSED__)
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

   printf("zone %d %d\n", zone->x, zone->y);

   if (zone) evry_show(zone);
}

/* menu item callback(s) */
static int
_e_mod_run_defer_cb(void *data)
{
   E_Zone *zone;

   zone = data;
   if (zone) evry_show(zone);
   return 0;
}

static void
_e_mod_run_cb(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   ecore_idle_enterer_add(_e_mod_run_defer_cb, m->zone);
}

/* menu item add hook */
static void
_e_mod_menu_add(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Run Everything"));
   e_util_menu_item_theme_icon_set(mi, "system-run");
   e_menu_item_callback_set(mi, _e_mod_run_cb, NULL);
}
