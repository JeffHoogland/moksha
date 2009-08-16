/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

/* TODO
   - watch plugin directories
   - get plugins from ~/.e/e/everything_plugins
 */

#include "e_mod_main.h"

/* actual module specifics */
static void  _e_mod_action_exebuf_cb(E_Object *obj, const char *params);
static int   _e_mod_run_defer_cb(void *data);
static void  _e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void  _e_mod_menu_add(void *data, E_Menu *m);

/* static E_Module *conf_module = NULL; */
static E_Int_Menu_Augmentation *maug = NULL;

static Eina_Array  *plugins = NULL;
static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;

Config *evry_conf = NULL;

/* module setup */
EAPI E_Module_Api e_modapi =
  {
    E_MODULE_API_VERSION,
    "Everything"
  };


static Eina_Bool list_cb(Eina_Module *m, void *data)
{
   if (eina_module_load(m))
     return EINA_TRUE;

   return EINA_FALSE;
}



EAPI void *
e_modapi_init(E_Module *m)
{
   Eina_List *files;
   char buf[4096], dir[4096];
   char *file;
   E_Action *act;
   
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

   /* evry_conf->history = eina_hash_string_superfast_new(NULL); */

   evry_conf->width = 380;
   evry_conf->height = 235;
   evry_conf->scroll_animate = 0;
   /* conf_module = m; */
   evry_init();

   eina_module_init();

   snprintf(dir, sizeof(dir), "%s/enlightenment/everything_plugins", e_prefix_lib_get());
   files = ecore_file_ls(dir);

   EINA_LIST_FREE(files, file)
     {
	snprintf(buf, sizeof(buf), "%s/%s/%s", dir, file, MODULE_ARCH);

	if (ecore_file_is_dir(buf))
	  plugins = eina_module_list_get(plugins, buf, 1, &list_cb, NULL);

	free(file);
     }

   /* add module supplied action */
   act = e_action_add("everything");
   if (act)
     {
	act->func.go = _e_mod_action_exebuf_cb;
	e_action_predef_name_set(_("Launch"), _("Run Everything Dialog"), "everything",
				 NULL, NULL, 0);
	evry_conf->action_show = act;
     }
   
   maug = e_int_menus_menu_augmentation_add("main/1", _e_mod_menu_add, NULL, NULL, NULL);

   e_configure_registry_category_add("extensions", 80, _("Extensions"), NULL, "preferences-extensions");
   e_configure_registry_item_add("extensions/run_everything", 40, _("Run Everything"), NULL, "system-run", evry_config_dialog);

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
   if (evry_conf->action_show)
     {
	e_action_predef_name_del(_("Launch"), _("Run Everything Dialog"));
	e_action_del("everything");
     }

   evry_shutdown();
   /* conf_module = NULL; */
   eina_list_free(evry_conf->plugins);
   eina_list_free(evry_conf->actions);

   if (plugins)
     {
	eina_module_list_unload(plugins);
	eina_module_list_flush(plugins);
	eina_array_free(plugins);
     }

   eina_module_shutdown();

   while ((cfd = e_config_dialog_get("E", "_config_everything_dialog"))) e_object_del(E_OBJECT(cfd));
   e_configure_registry_item_del("extensions/run_everything");
   e_configure_registry_category_del("extensions");

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


static int
_evry_cb_plugin_sort(const void *data1, const void *data2)
{
   const Evry_Plugin *p1 = data1;
   const Evry_Plugin *p2 = data2;
   return p1->config->priority - p2->config->priority;
}

EAPI void
evry_plugin_register(Evry_Plugin *plugin, int priority)
{
   Eina_List *l;
   Plugin_Config *pc;
   Eina_Bool found = 0;

   evry_conf->plugins = eina_list_append(evry_conf->plugins, plugin);

   EINA_LIST_FOREACH(evry_conf->plugins_conf, l, pc)
     {
	if (pc->name && plugin->name && !strcmp(pc->name, plugin->name))
	  {
	     found = 1;
	     break;
	  }
     }

   if (!found)
     {
	pc = E_NEW(Plugin_Config, 1);
	pc->name = eina_stringshare_add(plugin->name);
	pc->enabled = 1;
	pc->priority = priority ? priority : 100;;

	evry_conf->plugins_conf = eina_list_append(evry_conf->plugins_conf, pc);
     }

   /* if (plugin->trigger && !pc->trigger)
    *   pc->trigger = eina_stringshare_add(plugin->trigger); */

   plugin->config = pc;

   evry_conf->plugins = eina_list_sort(evry_conf->plugins,
				       eina_list_count(evry_conf->plugins),
				       _evry_cb_plugin_sort);

   /* TODO sorting, initialization, etc */
}

EAPI void
evry_plugin_unregister(Evry_Plugin *plugin)
{
   evry_conf->plugins = eina_list_remove(evry_conf->plugins, plugin);
   /* cleanup */
}

EAPI void
evry_action_register(Evry_Action *action)
{
   evry_conf->actions = eina_list_append(evry_conf->actions, action);
   /* TODO sorting, initialization, etc */
}

EAPI void
evry_action_unregister(Evry_Action *action)
{
   evry_conf->actions = eina_list_remove(evry_conf->actions, action);
   /* cleanup */
}
