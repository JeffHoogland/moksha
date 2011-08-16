#include "e.h"

static void _e_configure_menu_module_item_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_configure_menu_add(void *data, E_Menu *m);
static void _e_configure_efreet_desktop_cleanup(void);
static void _e_configure_efreet_desktop_update(void);
static Eina_Bool _e_configure_cb_efreet_desktop_cache_update(void *data, int type, void *event);
static void _e_configure_registry_item_full_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon, E_Config_Dialog *(*func) (E_Container *con, const char *params), void (*generic_func) (E_Container *con, const char *params), Efreet_Desktop *desktop, const char *params);
static void _e_configure_registry_item_free(E_Configure_It *eci);

static void _configure_job(void *data);
static Eina_Bool _configure_init_timer(void *data);

EAPI Eina_List *e_configure_registry = NULL;

static Eina_List *handlers = NULL;
static E_Int_Menu_Augmentation *maug = NULL;
static Ecore_Job *update_job = NULL;

static struct {
   void (*func) (const void *data, E_Container *con, const char *params, Efreet_Desktop *desktop);
   const char *data;
} custom_desktop_exec = { NULL, NULL };

EINTERN void
e_configure_init(void)
{
   e_configure_registry_category_add("extensions", 90, _("Extensions"), NULL, "preferences-extensions");
   e_configure_registry_item_add("extensions/modules", 10, _("Modules"), NULL, "preferences-plugin", e_int_config_modules);

   maug = e_int_menus_menu_augmentation_add_sorted
     ("config/1", _("Modules"), _e_configure_menu_add, NULL, NULL, NULL);

   if (update_job)
      {
         ecore_job_del(update_job);
         update_job = NULL;
      }
   ecore_timer_add(0.0, _configure_init_timer, NULL);
}

EAPI void
e_configure_registry_call(const char *path, E_Container *con, const char *params)
{
   E_Configure_Cat *ecat;
   Eina_List *l;
   char *cat;
   const char *item;

   /* path is "category/item" */
   cat = ecore_file_dir_get(path);
   if (!cat) return;
   item = ecore_file_file_get(path);
   EINA_LIST_FOREACH(e_configure_registry, l, ecat)
     if (!strcmp(cat, ecat->cat))
       {
	  E_Configure_It *eci;
	  Eina_List *ll;

	  EINA_LIST_FOREACH(ecat->items, ll, eci)
	    if (!strcmp(item, eci->item))
	      {
		 if (!params) params = eci->params;
		 
		 if (eci->func) eci->func(con, params);
		 else if (eci->generic_func) eci->generic_func(con, params);
		 else if (eci->desktop)
		   {
		      if (custom_desktop_exec.func)
			custom_desktop_exec.func(custom_desktop_exec.data,
						 con, params, eci->desktop);
		      else
			e_exec(e_util_zone_current_get(con->manager),
			       eci->desktop, NULL, NULL, "config");
		   }
		 break;
	      }
	  break;
       }
   free(cat);
}

EAPI void
e_configure_registry_item_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon, E_Config_Dialog *(*func) (E_Container *con, const char *params))
{
   _e_configure_registry_item_full_add(path, pri, label, icon_file, icon, func, NULL, NULL, NULL);
}

EAPI void
e_configure_registry_generic_item_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon, void (*generic_func) (E_Container *con, const char *params))
{
   _e_configure_registry_item_full_add(path, pri, label, icon_file, icon, NULL, generic_func, NULL, NULL);
}

EAPI void
e_configure_registry_item_params_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon, E_Config_Dialog *(*func) (E_Container *con, const char *params), const char *params)
{
   _e_configure_registry_item_full_add(path, pri, label, icon_file, icon, func, NULL, NULL, params);
}

EAPI void
e_configure_registry_item_del(const char *path)
{
   E_Configure_Cat *ecat;
   Eina_List *l;
   const char *item;
   char *cat;

   /* path is "category/item" */
   cat = ecore_file_dir_get(path);
   if (!cat) return;
   item = ecore_file_file_get(path);

   EINA_LIST_FOREACH(e_configure_registry, l, ecat)
     if (!strcmp(cat, ecat->cat))
       {
	  E_Configure_It *eci;
	  Eina_List *ll;

	  EINA_LIST_FOREACH(ecat->items, ll, eci)
	    if (!strcmp(item, eci->item))
	      {
		 ecat->items = eina_list_remove_list(ecat->items, ll);

                 _e_configure_registry_item_free(eci);
		 break;
	      }
	  break;
       }
   free(cat);
}

EAPI void
e_configure_registry_category_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon)
{
   E_Configure_Cat *ecat2;
   E_Configure_Cat *ecat;
   Eina_List *l;

   /* if it exists - ignore this */
   EINA_LIST_FOREACH(e_configure_registry, l, ecat2)
     if (!strcmp(ecat2->cat, path)) return;

   ecat = E_NEW(E_Configure_Cat, 1);
   if (!ecat) return;

   ecat->cat = eina_stringshare_add(path);
   ecat->pri = pri;
   ecat->label = eina_stringshare_add(label);
   if (icon_file) ecat->icon_file = eina_stringshare_add(icon_file);
   if (icon) ecat->icon = eina_stringshare_add(icon);
   EINA_LIST_FOREACH(e_configure_registry, l, ecat2)
     if (ecat2->pri > ecat->pri)
       {
	  e_configure_registry = 
            eina_list_prepend_relative_list(e_configure_registry, ecat, l);
	  return;
       }
   e_configure_registry = eina_list_append(e_configure_registry, ecat);
}

EAPI void
e_configure_registry_category_del(const char *path)
{
   E_Configure_Cat *ecat;
   Eina_List *l;
   char *cat;

   cat = ecore_file_dir_get(path);
   if (!cat) return;
   EINA_LIST_FOREACH(e_configure_registry, l, ecat)
     if (!strcmp(cat, ecat->cat))
       {
	  if (ecat->items) break;
	  e_configure_registry = eina_list_remove_list(e_configure_registry, l);
	  eina_stringshare_del(ecat->cat);
	  eina_stringshare_del(ecat->label);
	  if (ecat->icon) eina_stringshare_del(ecat->icon);
	  if (ecat->icon_file) eina_stringshare_del(ecat->icon_file);
	  free(ecat);
	  break;
       }
   free(cat);
}

EAPI void
e_configure_registry_custom_desktop_exec_callback_set(void (*func) (const void *data, E_Container *con, const char *params, Efreet_Desktop *desktop), const void *data)
{
   custom_desktop_exec.func = func;
   custom_desktop_exec.data = data;
}

EAPI int
e_configure_registry_exists(const char *path)
{
   E_Configure_Cat *ecat;
   Eina_List *l;
   char *cat;
   const char *item;
   int ret = 0;

   /* path is "category/item" */
   cat = ecore_file_dir_get(path);
   if (!cat) return 0;
   item = ecore_file_file_get(path);
   EINA_LIST_FOREACH(e_configure_registry, l, ecat)
     if (!strcmp(cat, ecat->cat))
       {
	  E_Configure_It *eci;
	  Eina_List *ll;

	  if (!item)
	    {
	       ret = 1;
	       break;
	    }
	  EINA_LIST_FOREACH(ecat->items, ll, eci)
	    if (!strcmp(item, eci->item))
	      {
		 ret = 1;
		 break;
	      }
	  break;
       }

   free(cat);
   return ret;
}

static void
_e_configure_menu_module_item_cb(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   e_int_config_modules(m->zone->container, NULL);
}

static void
_e_configure_menu_add(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Modules"));
   e_util_menu_item_theme_icon_set(mi, "preferences-plugin");
   e_menu_item_callback_set(mi, _e_configure_menu_module_item_cb, NULL);
}

static void
_configure_job(void *data __UNUSED__)
{
   _e_configure_efreet_desktop_update();
   update_job = NULL;
}

static Eina_Bool
_configure_init_timer(void *data __UNUSED__)
{
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
         (EFREET_EVENT_DESKTOP_CACHE_UPDATE, _e_configure_cb_efreet_desktop_cache_update, NULL));
   if (update_job) ecore_job_del(update_job);
   update_job = ecore_job_add(_configure_job, NULL);
   return EINA_FALSE;
}

static void
_e_configure_efreet_desktop_cleanup(void)
{
   Eina_List *l;
   E_Configure_Cat *ecat;

//   printf("_e_configure_efreet_desktop_cleanup\n");
   /* remove anything with a desktop entry */
   EINA_LIST_FOREACH(e_configure_registry, l, ecat)
     {
	E_Configure_It *eci;
	Eina_List *ll, *ln;

	EINA_LIST_FOREACH_SAFE(ecat->items, ll, ln, eci)
	   if (eci->desktop)
	     {
                _e_configure_registry_item_free(eci);
		ecat->items = eina_list_remove_list(ecat->items, ll);
	     }
     }
}

static void
_e_configure_efreet_desktop_update(void)
{
   Eina_List *settings_desktops, *system_desktops;
   Efreet_Desktop *desktop;
   Eina_List *l;
   char buf[1024];

   /* get desktops */
   settings_desktops = efreet_util_desktop_category_list("Settings");
   system_desktops = efreet_util_desktop_category_list("System");
   if ((!settings_desktops) && (!system_desktops))
     {
	EINA_LIST_FREE(settings_desktops, desktop)
	   efreet_desktop_free(desktop);
	EINA_LIST_FREE(system_desktops, desktop)
	   efreet_desktop_free(desktop);
	return;
     }

   /* get ones in BOTH lists */
   EINA_LIST_FOREACH(settings_desktops, l, desktop)
     {
	char *s;
	char *cfg_cat_name;
	const char *cfg_cat_icon;
	char *cfg_cat;
	char *cfg_cat_cfg;
	const char *cfg_icon;
	char *label;
	int cfg_pri;
        int dopref;
        
        dopref = 0;
	cfg_cat = NULL;
	cfg_icon = NULL;
	cfg_cat_cfg = NULL;
	cfg_pri = 1000;
	cfg_cat_name = NULL;
	cfg_cat_icon = NULL;
	label = NULL;
	if (!eina_list_data_find(system_desktops, desktop))
          {
             /* settings desktop but not in system -> put in preferened */
             dopref = 1;
          }
	if (desktop->x)
	  {
	     cfg_cat_cfg = eina_hash_find(desktop->x, "X-Enlightenment-Config-Category");
	     s = eina_hash_find(desktop->x, "X-Enlightenment-Config-Priority");
	     if (s) cfg_pri = atoi(s);
	     cfg_cat_name = eina_hash_find(desktop->x, "X-Enlightenment-Config-Category-Name");
	     cfg_cat_icon = eina_hash_find(desktop->x, "X-Enlightenment-Config-Category-Icon");
	     if ((cfg_cat_icon) && (cfg_cat_icon[0] != '/'))
               cfg_cat_icon = efreet_icon_path_find(e_config->icon_theme,
                                                    cfg_cat_icon, 64);
	  }
	if (desktop->icon)
	  {
	     if (desktop->icon[0] == '/')
	       cfg_icon = desktop->icon;
	     else
	       cfg_icon = efreet_icon_path_find(e_config->icon_theme,
						desktop->icon, 64);
	  }
	if (desktop->name) label = desktop->name;
	else if (desktop->generic_name) label = desktop->generic_name;
	else label = "???";
        if (!cfg_cat_cfg)
          {
             const char *ic = cfg_cat_icon;
             
             if (dopref)
               {
                  snprintf(buf, sizeof(buf), "preferences/%s", label);
                  if (!ic) ic = "preferences-preferences";
                  e_configure_registry_category_add("preferences", 900,
                                                    _("Preferences"),
                                                    NULL, ic);
               }
             else
               {
                  snprintf(buf, sizeof(buf), "system/%s", label);
                  if (!ic) ic = "preferences-system";
                  e_configure_registry_category_add("system", 1000, 
                                                    _("System"),
                                                    NULL, ic);
               }
             cfg_cat_cfg = buf;
          }
        else
          {
             cfg_cat = ecore_file_dir_get(cfg_cat_cfg);
             if (!cfg_cat) cfg_cat = strdup(cfg_cat_cfg);
             if (cfg_cat)
               {
                  if (!cfg_cat_name) cfg_cat_name = cfg_cat;
                  e_configure_registry_category_add(cfg_cat,
                                                    1000, cfg_cat_name,
                                                    NULL, cfg_cat_icon);
                  free(cfg_cat);
                  cfg_cat = NULL;
               }
          }
        _e_configure_registry_item_full_add(cfg_cat_cfg, cfg_pri, label,
                                            NULL, cfg_icon,
                                            NULL, NULL, desktop, NULL);
     }
   EINA_LIST_FREE(settings_desktops, desktop)
      efreet_desktop_free(desktop);
   EINA_LIST_FREE(system_desktops, desktop)
      efreet_desktop_free(desktop);
}

static Eina_Bool
_e_configure_cb_efreet_desktop_cache_update(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Efreet_Event_Cache_Update *ev = event;

   if (!ev->changed) return ECORE_CALLBACK_PASS_ON;

   _e_configure_efreet_desktop_cleanup();
   if (update_job) ecore_job_del(update_job);
   update_job = ecore_job_add(_configure_job, NULL);
   return ECORE_CALLBACK_PASS_ON;;
}

static void
_e_configure_registry_item_full_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon, E_Config_Dialog *(*func) (E_Container *con, const char *params), void (*generic_func) (E_Container *con, const char *params), Efreet_Desktop *desktop, const char *params)
{
   Eina_List *l;
   char *cat;
   const char *item;
   E_Configure_It *eci;
   E_Configure_Cat *ecat;

   /* path is "category/item" */
   cat = ecore_file_dir_get(path);
   if (!cat) return;
   item = ecore_file_file_get(path);
   eci = E_NEW(E_Configure_It, 1);
   if (!eci) goto done;

   eci->item = eina_stringshare_add(item);
   eci->pri = pri;
   eci->label = eina_stringshare_add(label);
   if (icon_file) eci->icon_file = eina_stringshare_add(icon_file);
   if (icon) eci->icon = eina_stringshare_add(icon);
   if (params) eci->params = eina_stringshare_add(params);
   eci->func = func;
   eci->generic_func = generic_func;
   eci->desktop = desktop;
   if (eci->desktop) efreet_desktop_ref(eci->desktop);

   EINA_LIST_FOREACH(e_configure_registry, l, ecat)
     if (!strcmp(cat, ecat->cat))
       {
	  E_Configure_It *eci2;
	  Eina_List *ll;

	  EINA_LIST_FOREACH(ecat->items, ll, eci2)
	    if (eci2->pri > eci->pri)
	      {
		 ecat->items = eina_list_prepend_relative_list(ecat->items, eci, ll);
		 goto done;
	      }
	  ecat->items = eina_list_append(ecat->items, eci);
	  goto done;
       }

 done:
   free(cat);
}

static void
_e_configure_registry_item_free(E_Configure_It *eci)
{
   eina_stringshare_del(eci->item);
   eina_stringshare_del(eci->label);
   eina_stringshare_del(eci->icon);
   if (eci->icon_file) eina_stringshare_del(eci->icon_file);
   if (eci->desktop) efreet_desktop_free(eci->desktop);
   if (eci->params) eina_stringshare_del(eci->params);
   free(eci);
}
