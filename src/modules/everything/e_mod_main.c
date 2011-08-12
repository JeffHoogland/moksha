#include "e_mod_main.h"

static void _e_mod_action_cb(E_Object *obj, const char *params);
static void _e_mod_action_cb_edge(E_Object *obj, const char *params, E_Event_Zone_Edge *ev);
static Eina_Bool  _e_mod_run_defer_cb(void *data);
static void _e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_mod_menu_add(void *data, E_Menu *m);
static void _config_init(void);
static void _config_free(void);
static Eina_Bool  _cleanup_history(void *data);
static void _evry_type_init(const char *type);
static Eina_List *_evry_types = NULL;
static E_Int_Menu_Augmentation *maug = NULL;
static E_Action *act = NULL;
static Ecore_Timer *cleanup_timer;
static const char *module_icon = NULL;
static E_Config_DD *conf_edd = NULL;
static E_Config_DD *plugin_conf_edd = NULL;
static int _e_module_evry_log_dom = -1;

Evry_API *evry = NULL;
Evry_Config *evry_conf = NULL;
int _evry_events[NUM_EVRY_EVENTS];

/* module setup */
EAPI E_Module_Api e_modapi =
  {
    E_MODULE_API_VERSION,
    "Everything"
  };


EAPI void *
e_modapi_init(E_Module *m)
{
   Eina_List *l;
   Evry_Module *em;
   char buf[4096];

   _e_module_evry_log_dom = eina_log_domain_register
     ("e_module_everything", EINA_LOG_DEFAULT_COLOR);

   if(_e_module_evry_log_dom < 0)
     {
	EINA_LOG_ERR
	  ("impossible to create a log domain for everything module");
	return NULL;
     }

   /* add module supplied action */
   act = e_action_add("everything");
   if (act)
     {
	act->func.go = _e_mod_action_cb;
	act->func.go_edge = _e_mod_action_cb_edge;
	e_action_predef_name_set
	  (_("Everything Launcher"),
	   _("Show Everything Launcher"),
	   "everything", "", NULL, 0);
     }

   maug = e_int_menus_menu_augmentation_add
     ("main/1", _e_mod_menu_add, NULL, NULL, NULL);

   e_configure_registry_category_add
     ("launcher", 80, _("Launcher"), NULL, "modules-launcher");

   snprintf(buf, sizeof(buf), "%s/e-module-everything.edj", e_module_dir_get(m));
   module_icon = eina_stringshare_add(buf);
   
   e_configure_registry_item_add
     ("launcher/run_everything", 40, _("Everything Configuration"),
      NULL, module_icon, evry_config_dialog);
   evry_init();

   _evry_type_init("NONE");
   _evry_type_init("FILE");
   _evry_type_init("DIRECTORY");
   _evry_type_init("APPLICATION");
   _evry_type_init("ACTION");
   _evry_type_init("PLUGIN");
   _evry_type_init("BORDER");
   _evry_type_init("TEXT");

   _config_init();
   
   _evry_events[EVRY_EVENT_ITEMS_UPDATE]     = ecore_event_type_new();
   _evry_events[EVRY_EVENT_ITEM_SELECTED]    = ecore_event_type_new();
   _evry_events[EVRY_EVENT_ITEM_CHANGED]     = ecore_event_type_new();
   _evry_events[EVRY_EVENT_ACTION_PERFORMED] = ecore_event_type_new();
   _evry_events[EVRY_EVENT_PLUGIN_SELECTED]  = ecore_event_type_new();
   
   evry = E_NEW(Evry_API, 1);
   evry->log_dom = _e_module_evry_log_dom;
#define SET(func) (evry->func = &evry_##func);
   SET(api_version_check);
   SET(item_new);
   SET(item_free);
   SET(item_ref);
   SET(plugin_new);
   SET(plugin_free);
   SET(plugin_register);
   SET(plugin_unregister);
   SET(plugin_update);
   SET(plugin_find);
   SET(action_new);
   SET(action_free);
   SET(action_register);
   SET(action_unregister);
   SET(action_find);
   SET(api_version_check);
   SET(type_register);
   SET(icon_mime_get);
   SET(icon_theme_get);
   SET(fuzzy_match);
   SET(util_exec_app);
   SET(util_url_escape);
   SET(util_url_unescape);
   SET(util_file_detail_set);
   SET(util_plugin_items_add);
   SET(util_md5_sum);
   SET(util_icon_get);
   SET(items_sort_func);
   SET(item_changed);
   SET(file_path_get);
   SET(file_url_get);
   SET(history_item_add);
   SET(history_types_get);
   SET(history_item_usage_set);
   SET(event_handler_add);
#undef SET

   evry_history_init();
   evry_plug_actions_init();
   evry_plug_apps_init(m);
   evry_plug_files_init(m);
   evry_plug_windows_init(m);
   evry_plug_settings_init(m);
   evry_plug_calc_init(m);
   e_datastore_set("evry_api", evry);

   EINA_LIST_FOREACH(e_datastore_get("evry_modules"), l, em)
     em->active = em->init(evry);

   evry_plug_collection_init();
   evry_plug_clipboard_init();
   evry_plug_text_init();
   evry_view_init();
   evry_view_help_init();
   evry_gadget_init();

   e_module_priority_set(m, -1000);
   e_module_delayed_set(m, 1);

   /* cleanup every hour :) */
   cleanup_timer = ecore_timer_add(3600, _cleanup_history, NULL);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_Config_Dialog *cfd;
   const char *t;
   Eina_List *l;
   Evry_Module *em;

   EINA_LIST_FOREACH(e_datastore_get("evry_modules"), l, em)
     em->shutdown();
   
   evry_plug_apps_shutdown();
   evry_plug_files_shutdown();
   evry_plug_settings_shutdown();
   evry_plug_windows_shutdown();
   evry_plug_calc_shutdown();
   evry_plug_clipboard_shutdown();
   evry_plug_text_shutdown();
   evry_plug_collection_shutdown();
   evry_plug_actions_shutdown();
   evry_view_shutdown();
   evry_view_help_shutdown();
   evry_gadget_shutdown();
   evry_shutdown();

   e_datastore_del("evry_api");
   E_FREE(evry);
   evry = NULL;

   _config_free();
   evry_history_free();

   EINA_LIST_FREE(_evry_types, t)
     eina_stringshare_del(t);

   e_configure_registry_item_del("launcher/run_everything");
   e_configure_registry_category_del("launcher");

   while ((cfd = e_config_dialog_get("E", "_config_everything_dialog")))
     e_object_del(E_OBJECT(cfd));

   if (act)
     {
	e_action_predef_name_del(_("Everything Launcher"),
				 _("Show Everything Dialog"));
	e_action_del("everything");
     }

   if (maug)
     {
   	e_int_menus_menu_augmentation_del("main/1", maug);
   	maug = NULL;
     }

   if (module_icon)
     eina_stringshare_del(module_icon);
   
   /* Clean EET */
   E_CONFIG_DD_FREE(conf_edd);
   E_CONFIG_DD_FREE(plugin_conf_edd);

   if (cleanup_timer)
     ecore_timer_del(cleanup_timer);

#ifdef CHECK_REFS
   Evry_Item *it;
   EINA_LIST_FREE(_refd, it)
     printf("%d %s\n", it->ref, it->label);
#endif

   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.everything", conf_edd, evry_conf);

   evry_plug_apps_save();
   evry_plug_files_save();
   evry_plug_settings_save();
   evry_plug_windows_save();
   evry_plug_calc_save();
   
   return 1;
}

/***************************************************************************/

Ecore_Event_Handler *
evry_event_handler_add(int type, Eina_Bool (*func) (void *data, int type, void *event), const void *data)
{
   return ecore_event_handler_add(_evry_events[type], func, data);
}


Evry_Type
evry_type_register(const char *type)
{
   const char *t = eina_stringshare_add(type);
   Evry_Type ret = NUM_EVRY_TYPES;
   const char *i;
   Eina_List *l;

   EINA_LIST_FOREACH(_evry_types, l, i)
     {
	if (i == t) break;
	ret++;
     }

   if(!l)
     {
	_evry_types = eina_list_append(_evry_types, t);
	return ret;
     }
   eina_stringshare_del(t);

   return ret;
}

static void
_evry_type_init(const char *type)
{
   const char *t = eina_stringshare_add(type);
   _evry_types = eina_list_append(_evry_types, t);
}

const char *
evry_type_get(Evry_Type type)
{
   const char *ret = eina_list_nth(_evry_types, type);
   if (!ret)
     return eina_stringshare_add("");

   return ret;
}

int evry_api_version_check(int version)
{
   if (EVRY_API_VERSION == version)
     return 1;

   ERR("module API is %d, required is %d", version, EVRY_API_VERSION);

   return 0;
}


static int
_evry_cb_view_sort(const void *data1, const void *data2)
{
   const Evry_View *v1 = data1;
   const Evry_View *v2 = data2;
   return v1->priority - v2->priority;
}


void
evry_view_register(Evry_View *view, int priority)
{
   view->priority = priority;

   evry_conf->views = eina_list_append(evry_conf->views, view);

   evry_conf->views = eina_list_sort(evry_conf->views,
				     eina_list_count(evry_conf->views),
				     _evry_cb_view_sort);
}

void
evry_view_unregister(Evry_View *view)
{
   evry_conf->views = eina_list_remove(evry_conf->views, view);
}

/***************************************************************************/

static Eina_Bool
_cleanup_history(void *data __UNUSED__)
{
   /* evrything is active */
   if (evry_hist)
     return ECORE_CALLBACK_RENEW;

   /* cleanup old entries */
   evry_history_free();
   evry_history_init();

   return ECORE_CALLBACK_RENEW;
}

static void
_config_init()
{
   Plugin_Config *pc, *pcc;

#undef T
#undef D
#define T Plugin_Config
#define D plugin_conf_edd
   plugin_conf_edd = E_CONFIG_DD_NEW("Plugin_Config", Plugin_Config);
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, enabled, INT);
   E_CONFIG_VAL(D, T, priority, INT);
   E_CONFIG_VAL(D, T, trigger, STR);
   E_CONFIG_VAL(D, T, trigger_only, INT);
   E_CONFIG_VAL(D, T, view_mode, INT);
   E_CONFIG_VAL(D, T, aggregate, INT);
   E_CONFIG_VAL(D, T, top_level, INT);
   E_CONFIG_VAL(D, T, min_query, INT);
   E_CONFIG_LIST(D, T, plugins, plugin_conf_edd);
#undef T
#undef D
#define T Evry_Config
#define D conf_edd
   conf_edd = E_CONFIG_DD_NEW("Config", Evry_Config);
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_VAL(D, T, height, INT);
   E_CONFIG_VAL(D, T, edge_width, INT);
   E_CONFIG_VAL(D, T, edge_height, INT);
   E_CONFIG_VAL(D, T, rel_x, DOUBLE);
   E_CONFIG_VAL(D, T, rel_y, DOUBLE);
   E_CONFIG_VAL(D, T, scroll_animate, INT);
   E_CONFIG_VAL(D, T, scroll_speed, DOUBLE);
   E_CONFIG_VAL(D, T, hide_input, INT);
   E_CONFIG_VAL(D, T, hide_list, INT);
   E_CONFIG_VAL(D, T, quick_nav, INT);
   E_CONFIG_VAL(D, T, view_mode, INT);
   E_CONFIG_VAL(D, T, view_zoom, INT);
   E_CONFIG_VAL(D, T, cycle_mode, INT);
   E_CONFIG_VAL(D, T, history_sort_mode, INT);
   E_CONFIG_LIST(D, T, conf_subjects, plugin_conf_edd);
   E_CONFIG_LIST(D, T, conf_actions, plugin_conf_edd);
   E_CONFIG_LIST(D, T, conf_objects, plugin_conf_edd);
   E_CONFIG_LIST(D, T, conf_views,   plugin_conf_edd);
   E_CONFIG_LIST(D, T, collections,  plugin_conf_edd);
   E_CONFIG_VAL(D, T, first_run, UCHAR);
#undef T
#undef D
   evry_conf = e_config_domain_load("module.everything", conf_edd);

   if (evry_conf && !e_util_module_config_check
       (_("Everything Module"), evry_conf->version,
	MOD_CONFIG_FILE_EPOCH, MOD_CONFIG_FILE_VERSION))
     _config_free();

   if (!evry_conf)
     {
	evry_conf = E_NEW(Evry_Config, 1);
	evry_conf->version = (MOD_CONFIG_FILE_EPOCH << 16);
     }

#define IFMODCFG(v) if ((evry_conf->version & 0xffff) < v) {
#define IFMODCFGEND }

   /* setup defaults */
   IFMODCFG(0x0001);
   evry_conf->rel_x = 0.5;
   evry_conf->rel_y = 0.43;
   evry_conf->width = 455;
   evry_conf->height = 430;
   evry_conf->scroll_animate = 1;
   evry_conf->scroll_speed = 10.0;
   evry_conf->hide_input = 0;
   evry_conf->hide_list = 0;
   evry_conf->quick_nav = 1;
   evry_conf->view_mode = VIEW_MODE_DETAIL;
   evry_conf->view_zoom = 0;
   evry_conf->cycle_mode = 0;
   evry_conf->history_sort_mode = 0;
   evry_conf->edge_width = 340;
   evry_conf->edge_height = 385;
   evry_conf->first_run = EINA_TRUE;

   pcc = E_NEW(Plugin_Config, 1);
   pcc->name = eina_stringshare_add("Start");
   pcc->enabled = EINA_FALSE;
   pcc->aggregate = EINA_FALSE;
   pcc->top_level = EINA_TRUE;
   pcc->view_mode = VIEW_MODE_THUMB;
   evry_conf->collections = eina_list_append(evry_conf->collections, pcc);

   pc = E_NEW(Plugin_Config, 1);
   pc->name = eina_stringshare_add("Windows");
   pc->enabled = EINA_TRUE;
   pc->view_mode = VIEW_MODE_NONE;
   pcc->plugins = eina_list_append(pcc->plugins, pc);

   pc = E_NEW(Plugin_Config, 1);
   pc->name = eina_stringshare_add("Settings");
   pc->enabled = EINA_TRUE;
   pc->view_mode = VIEW_MODE_NONE;
   pcc->plugins = eina_list_append(pcc->plugins, pc);

   pc = E_NEW(Plugin_Config, 1);
   pc->name = eina_stringshare_add("Files");
   pc->enabled = EINA_TRUE;
   pc->view_mode = VIEW_MODE_NONE;
   pcc->plugins = eina_list_append(pcc->plugins, pc);

   pc = E_NEW(Plugin_Config, 1);
   pc->name = eina_stringshare_add("Applications");
   pc->enabled = EINA_TRUE;
   pc->view_mode = VIEW_MODE_NONE;
   pcc->plugins = eina_list_append(pcc->plugins, pc);
   IFMODCFGEND;

   IFMODCFG(0x0003);
   evry_conf->width = 464;
   evry_conf->height = 366;
   IFMODCFGEND;
   
   evry_conf->version = MOD_CONFIG_FILE_VERSION;
}

static void
_config_free(void)
{
   Plugin_Config *pc, *pc2;

   EINA_LIST_FREE(evry_conf->collections, pc)
     EINA_LIST_FREE(pc->plugins, pc2)
     {
	IF_RELEASE(pc2->name);
	IF_RELEASE(pc2->trigger);
	E_FREE(pc2);
     }

   EINA_LIST_FREE(evry_conf->conf_subjects, pc)
     {
	IF_RELEASE(pc->name);
	IF_RELEASE(pc->trigger);
	E_FREE(pc);
     }
   EINA_LIST_FREE(evry_conf->conf_actions, pc)
     {
	IF_RELEASE(pc->name);
	IF_RELEASE(pc->trigger);
	E_FREE(pc);
     }
   EINA_LIST_FREE(evry_conf->conf_objects, pc)
     {
	IF_RELEASE(pc->name);
	IF_RELEASE(pc->trigger);
	E_FREE(pc);
     }

   E_FREE(evry_conf);
}

/***************************************************************************/
/* action callback */

static Ecore_Idle_Enterer *_idler = NULL;
static const char *_params = NULL;

static Eina_Bool
_e_mod_run_defer_cb(void *data)
{
   E_Zone *zone;

   zone = data;
   if (zone) evry_show(zone, E_ZONE_EDGE_NONE, _params);

   _idler = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_mod_action_cb(E_Object *obj, const char *params)
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

   if (!zone) return;

   IF_RELEASE(_params);
   if (params && params[0])
     _params = eina_stringshare_add(params);
   /* if (zone) evry_show(zone, _params); */

   if (_idler) ecore_idle_enterer_del(_idler);
  _idler = ecore_idle_enterer_add(_e_mod_run_defer_cb, zone);
}

static void
_e_mod_action_cb_edge(E_Object *obj  __UNUSED__, const char *params, E_Event_Zone_Edge *ev)
{
   IF_RELEASE(_params);
   if (params && params[0])
     _params = eina_stringshare_add(params);

   if (_idler) ecore_idle_enterer_del(_idler);

   evry_show(ev->zone, ev->edge, _params);
}

/* menu item callback(s) */
static void
_e_mod_run_cb(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   IF_RELEASE(_params);
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
