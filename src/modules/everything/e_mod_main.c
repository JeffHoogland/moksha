/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

/* TODO
   - watch plugin directories
   - get plugins from ~/.e/e/everything_plugins
 */

#include "e_mod_main.h"

/* #undef DBG
 * #define DBG(...) ERR(__VA_ARGS__) */

#define CONFIG_VERSION 9

/* actual module specifics */
static void _e_mod_action_cb(E_Object *obj, const char *params);
static int  _e_mod_run_defer_cb(void *data);
static void _e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_mod_menu_add(void *data, E_Menu *m);
static void _config_init(void);
static void _config_free(void);

static E_Int_Menu_Augmentation *maug = NULL;
static E_Action *act = NULL;

/* static Eina_Array  *plugins = NULL; */
static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;

EAPI int _e_module_evry_log_dom = -1;


EAPI Evry_Config *evry_conf = NULL;

EAPI int EVRY_EVENT_ITEM_SELECT;
EAPI int EVRY_EVENT_ITEM_CHANGED;
EAPI int EVRY_EVENT_ITEMS_UPDATE;

/* module setup */
EAPI E_Module_Api e_modapi =
  {
    E_MODULE_API_VERSION,
    "Everything"
  };

EAPI void *
e_modapi_init(E_Module *m)
{
   Eina_List *files;
   char buf[4096], dir[4096];
   char *file;

   /* snprintf(buf, sizeof(buf), "%s/.e/e/config/%s/module.everything",
    * 	    e_user_homedir_get(), e_config_profile_get());
    * ecore_file_mkdir(buf); */

   _e_module_evry_log_dom = eina_log_domain_register
     ("e_module_everything", EINA_LOG_DEFAULT_COLOR);

   if(_e_module_evry_log_dom < 0)
     {
	EINA_LOG_ERR
	  ("impossible to create a log domain for everything module");
	return NULL;
     }

   _config_init();
   evry_history_init();

   view_thumb_init();
   view_help_init();
   evry_plug_clipboard_init();

   /* add module supplied action */
   act = e_action_add("everything");
   if (act)
     {
	act->func.go = _e_mod_action_cb;
	e_action_predef_name_set(_("Everything"),
				 _("Show Everything Dialog"),
				 "everything", "", NULL, 0);
     }

   maug = e_int_menus_menu_augmentation_add("main/1",
					    _e_mod_menu_add,
					    NULL, NULL, NULL);

   e_configure_registry_category_add("extensions", 80, _("Extensions"),
				     NULL, "preferences-extensions");

   e_configure_registry_item_add("extensions/run_everything", 40,
				 _("Run Everything"),
				 NULL, "system-run",
				 evry_config_dialog);
   evry_init();

   if (!EVRY_EVENT_ITEMS_UPDATE)
     EVRY_EVENT_ITEMS_UPDATE = ecore_event_type_new();
   if (!EVRY_EVENT_ITEM_SELECT)
     EVRY_EVENT_ITEM_SELECT = ecore_event_type_new();
   if (!EVRY_EVENT_ITEM_CHANGED)
     EVRY_EVENT_ITEM_CHANGED = ecore_event_type_new();
   
   e_module_delayed_set(m, 0);

   /* make sure module is loaded before others */
   e_module_priority_set(m, -1000);

   e_datastore_set("everything_loaded", "");
   
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_Config_Dialog *cfd;
   Evry_Plugin *p;
   Evry_Action *a;
   
   evry_shutdown();

   /* remove module-supplied menu additions */
   if (maug)
     {
   	e_int_menus_menu_augmentation_del("main/1", maug);
   	maug = NULL;
     }
   /* remove module-supplied action */
   if (act)
     {
	e_action_predef_name_del(_("Everything"), _("Show Everything Dialog"));
	e_action_del("everything");
     }

   view_thumb_shutdown();
   view_help_shutdown();
   evry_plug_clipboard_shutdown();

   EINA_LIST_FREE(evry_conf->plugins, p)
     evry_plugin_free(p, 1); 

   EINA_LIST_FREE(evry_conf->actions, a)
     evry_action_free(a); 

   while ((cfd = e_config_dialog_get("E", "_config_everything_dialog")))
     e_object_del(E_OBJECT(cfd));

   e_configure_registry_item_del("extensions/run_everything");
   e_configure_registry_category_del("extensions");

   _config_free();
   evry_history_free();

   /* Clean EET */
   E_CONFIG_DD_FREE(conf_item_edd);
   E_CONFIG_DD_FREE(conf_edd);

   e_datastore_del("everything_loaded");
   
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.everything", conf_edd, evry_conf);

   return 1;
}

static void
_config_init()
{
#undef T
#undef D
#define T Plugin_Config
#define D conf_item_edd
   conf_item_edd = E_CONFIG_DD_NEW("Plugin_Config", Plugin_Config);
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, trigger, STR);
   E_CONFIG_VAL(D, T, min_query, INT);
   E_CONFIG_VAL(D, T, loaded, INT);
   E_CONFIG_VAL(D, T, enabled, INT);
   E_CONFIG_VAL(D, T, priority, INT);
#undef T
#undef D
   
#define T Evry_Config
#define D conf_edd
   conf_edd = E_CONFIG_DD_NEW("Config", Evry_Config);
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_VAL(D, T, height, INT);
   E_CONFIG_VAL(D, T, rel_x, DOUBLE);
   E_CONFIG_VAL(D, T, rel_y, DOUBLE);
   E_CONFIG_VAL(D, T, scroll_animate, INT);
   E_CONFIG_VAL(D, T, scroll_speed, DOUBLE);
   E_CONFIG_VAL(D, T, hide_input, INT);
   E_CONFIG_VAL(D, T, hide_list, INT);
   E_CONFIG_VAL(D, T, quick_nav, INT);
   E_CONFIG_VAL(D, T, cmd_terminal, STR);
   E_CONFIG_VAL(D, T, cmd_sudo, STR);
   E_CONFIG_VAL(D, T, view_mode, INT);
   E_CONFIG_VAL(D, T, view_zoom, INT);
   E_CONFIG_VAL(D, T, cycle_mode, INT);
   E_CONFIG_VAL(D, T, history_sort_mode, INT);
   E_CONFIG_LIST(D, T, conf_subjects, conf_item_edd);
   E_CONFIG_LIST(D, T, conf_actions, conf_item_edd);
   E_CONFIG_LIST(D, T, conf_objects, conf_item_edd);
   E_CONFIG_LIST(D, T, conf_views,   conf_item_edd);
#undef T
#undef D
   evry_conf = e_config_domain_load("module.everything", conf_edd);

   if (evry_conf && evry_conf->version == 7)
     {
	evry_conf->scroll_speed = 10.0;
	evry_conf->version = 8;
     }

   if (evry_conf && evry_conf->version == 8)
     {
	evry_conf->width = 460;
	evry_conf->height = 310;
	evry_conf->rel_y = 0.25;
	evry_conf->scroll_animate = 1;
	evry_conf->version = CONFIG_VERSION;
     }

   if (evry_conf && evry_conf->version != CONFIG_VERSION)
     {
	_config_free();
	evry_conf = NULL;
     }
   
   if (!evry_conf)
     {
	evry_conf = E_NEW(Evry_Config, 1);
	evry_conf->version = CONFIG_VERSION;
	evry_conf->rel_x = 0.5;
	evry_conf->rel_y = 0.25;
	evry_conf->width = 460;
	evry_conf->height = 310;
	evry_conf->scroll_animate = 1;
	evry_conf->scroll_speed = 10.0;
	evry_conf->hide_input = 0;
	evry_conf->hide_list = 0;
	evry_conf->quick_nav = 1;
	evry_conf->cmd_terminal = eina_stringshare_add("/usr/bin/xterm");
	evry_conf->cmd_sudo = eina_stringshare_add("/usr/bin/gksudo --preserve-env");
	evry_conf->view_mode = 0;
	evry_conf->view_zoom = 0;
	evry_conf->cycle_mode = 0;
	evry_conf->history_sort_mode = 0;
     }
}


static void
_config_free(void)
{
   Plugin_Config *pc;

   /* free config */
   if (evry_conf->cmd_terminal)
     eina_stringshare_del(evry_conf->cmd_terminal);
   EINA_LIST_FREE(evry_conf->conf_subjects, pc)
     {
	if (pc->name) eina_stringshare_del(pc->name);
	if (pc->trigger) eina_stringshare_del(pc->trigger);
	E_FREE(pc);
     }
   EINA_LIST_FREE(evry_conf->conf_actions, pc)
     {
	if (pc->name) eina_stringshare_del(pc->name);
	if (pc->trigger) eina_stringshare_del(pc->trigger);
	E_FREE(pc);
     }
   EINA_LIST_FREE(evry_conf->conf_objects, pc)
     {
	if (pc->name) eina_stringshare_del(pc->name);
	if (pc->trigger) eina_stringshare_del(pc->trigger);
	E_FREE(pc);
     }
   E_FREE(evry_conf);
}


/* action callback */

static int
_e_mod_run_defer_cb(void *data)
{
   E_Zone *zone;

   zone = data;
   if (zone) evry_show(zone, NULL);
   return 0;
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

   if (params && params[0])
     evry_show(zone, params);
   else
     evry_show(zone, NULL);
   
   /* FIXME popup flickers sometimes when deferes*/
   /* if (params && params[0])
    *   evry_show(zone, params);
    * else
    *   ecore_idle_enterer_add(_e_mod_run_defer_cb, zone); */
}

/* menu item callback(s) */
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


EAPI int evry_api_version_check(int version)
{
   if (EVRY_API_VERSION == version)
     return 1;

   ERR("module API is %d, required is %d", version, EVRY_API_VERSION);

   return 0;
}

static int
_evry_cb_plugin_sort(const void *data1, const void *data2)
{
   const Evry_Plugin *p1 = data1;
   const Evry_Plugin *p2 = data2;
   return p1->config->priority - p2->config->priority;
}

Evry_Plugin *
evry_plugin_new(Evry_Plugin *base, const char *name, const char *label, int type,
		const char *type_in, const char *type_out,
		Evry_Plugin *(*begin) (Evry_Plugin *p, const Evry_Item *item),
		void (*cleanup) (Evry_Plugin *p),
		int  (*fetch) (Evry_Plugin *p, const char *input),
		Evas_Object *(*icon_get) (Evry_Plugin *p, const Evry_Item *it, Evas *e),
		void (*cb_free) (Evry_Plugin *p))
{
   Evry_Plugin *p;

   if (base)
     p = base;
   else
     p = E_NEW(Evry_Plugin, 1);

   p->name = eina_stringshare_add(name);
   p->label = eina_stringshare_add(label);
   p->type = type;
   p->type_in  = (type_in  ? eina_stringshare_add(type_in)  : NULL);
   p->type_out = (type_out ? eina_stringshare_add(type_out) : NULL);
   /* p->trigger  = (trigger  ? eina_stringshare_add(trigger)  : NULL); */
   /* p->icon     = (icon     ? eina_stringshare_add(icon)     : NULL); */
   p->begin    = begin;
   p->cleanup  = cleanup;
   p->fetch    = fetch;
   p->icon_get = icon_get;
   /* p->action   = action; */
   p->aggregate = EINA_TRUE;
   p->async_fetch = EINA_FALSE;
   p->free   = cb_free;

   p->history = EINA_TRUE;
   p->view_mode = VIEW_MODE_NONE;
   
   DBG("%s", p->name);
   
   return p;
}

void
evry_plugin_free(Evry_Plugin *p, int free_pointer)
{
   evry_plugin_unregister(p);

   DBG("%s", p->name);
   
   if (p->name)     eina_stringshare_del(p->name);
   if (p->label)    eina_stringshare_del(p->label);
   if (p->type_in)  eina_stringshare_del(p->type_in);
   if (p->type_out) eina_stringshare_del(p->type_out);
   /* if (p->trigger)  eina_stringshare_del(p->trigger); */
   /* if (p->icon)     eina_stringshare_del(p->icon); */

   if (p->free)
     p->free(p);
   else if (!free_pointer)
     return;
   else
     E_FREE(p);
}

Evry_Action *
evry_action_new(const char *name, const char *label, const char *type_in1, const char *type_in2,
		const char *icon,
		int  (*action) (Evry_Action *act),
		int (*check_item) (Evry_Action *act, const Evry_Item *it))
{
   Evry_Action *act = E_NEW(Evry_Action, 1);
   act->name = eina_stringshare_add(name);
   act->label = eina_stringshare_add(label);
   act->type_in1 = (type_in1 ? eina_stringshare_add(type_in1) : NULL);
   act->type_in2 = (type_in2 ? eina_stringshare_add(type_in2) : NULL);
   act->action = action;
   act->check_item = check_item;
   act->icon = (icon ? eina_stringshare_add(icon) : NULL);

   DBG("%s", name);
   
   return act;
}

void
evry_action_free(Evry_Action *act)
{
   evry_action_unregister(act);

   if (act->name)     eina_stringshare_del(act->name);
   if (act->label)    eina_stringshare_del(act->label);
   if (act->type_in1) eina_stringshare_del(act->type_in1);
   if (act->type_in2) eina_stringshare_del(act->type_in2);
   if (act->icon)     eina_stringshare_del(act->icon);

   if (act->free)
     act->free(act);
   else
     E_FREE(act);
}


/* TODO make int return */
void
evry_plugin_register(Evry_Plugin *p, int priority)
{
   Eina_List *l, *confs = NULL;
   Plugin_Config *pc;

   DBG("%s", p->name);
   
   evry_conf->plugins = eina_list_append(evry_conf->plugins, p);

   if (p->type == type_subject)
     confs = evry_conf->conf_subjects;
   else if (p->type == type_action)
     confs = evry_conf->conf_actions;
   else if (p->type == type_object)
     confs = evry_conf->conf_objects;

   EINA_LIST_FOREACH(confs, l, pc)
     if (pc->name && p->name && !strcmp(pc->name, p->name))
       break;

   if (!pc)
     {
	pc = E_NEW(Plugin_Config, 1);
	pc->name = eina_stringshare_add(p->name);
	pc->enabled = 1;
	pc->priority = priority ? priority : 100;;

	confs = eina_list_append(confs, pc);
	/* return NULL */
     }

   /* if (plugin->trigger && !pc->trigger)
    *   pc->trigger = eina_stringshare_add(plugin->trigger); */

   p->config = pc;
   evry_conf->plugins = eina_list_sort(evry_conf->plugins,
				       eina_list_count(evry_conf->plugins),
				       _evry_cb_plugin_sort);

   if (p->type == type_subject)
     evry_conf->conf_subjects = confs;
   else if (p->type == type_action)
     evry_conf->conf_actions = confs;
   else if (p->type == type_object)
     evry_conf->conf_objects = confs;

   if (p->type == type_subject)
     {
	char buf[256];
	snprintf(buf, sizeof(buf), "Show %s Plugin", p->name);

	e_action_predef_name_set(_("Everything"), buf,
				 "everything", p->name, NULL, 1);
     }
}

void
evry_plugin_unregister(Evry_Plugin *p)
{
   DBG("%s", p->name);
   
   evry_conf->plugins = eina_list_remove(evry_conf->plugins, p);

   if (p->type == type_subject)
     {
	char buf[256];
	snprintf(buf, sizeof(buf), "Show %s Plugin", p->name);

	e_action_predef_name_del(_("Everything"), buf);
     }
   /* cleanup */
}

void
evry_action_register(Evry_Action *action, int priority)
{
   action->priority = priority;
   evry_conf->actions = eina_list_append(evry_conf->actions, action);
   /* TODO sorting, initialization, etc */
}

void
evry_action_unregister(Evry_Action *action)
{
   evry_conf->actions = eina_list_remove(evry_conf->actions, action);
   /* cleanup */
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


