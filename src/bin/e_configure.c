#include "e.h"

static void _e_configure_efreet_desktop_update(void);
static int _e_configure_cb_efreet_desktop_list_change(void *data, int type, void *event);
static int _e_configure_cb_efreet_desktop_change(void *data, int type, void *event);
static void _e_configure_registry_item_full_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon, E_Config_Dialog *(*func) (E_Container *con, const char *params), void (*generic_func) (E_Container *con, const char *params), Efreet_Desktop *desktop);

Evas_List *e_configure_registry = NULL;

static Evas_List *handlers = NULL;

EAPI void
e_configure_init(void)
{
   e_configure_registry_category_add("extensions", 90, _("Extensions"), NULL, "enlightenment/extensions");
   e_configure_registry_item_add("extensions/modules", 10, _("Modules"), NULL, "enlightenment/modules", e_int_config_modules);

   handlers = evas_list_append
     (handlers, ecore_event_handler_add
         (EFREET_EVENT_DESKTOP_LIST_CHANGE, _e_configure_cb_efreet_desktop_list_change, NULL));
   handlers = evas_list_append
     (handlers, ecore_event_handler_add
         (EFREET_EVENT_DESKTOP_CHANGE, _e_configure_cb_efreet_desktop_change, NULL));
//   _e_configure_efreet_desktop_update();
}

static void
_e_configure_efreet_desktop_update(void)
{
   Ecore_List *settings_desktops, *system_desktops;
   Efreet_Desktop *desktop;
   Evas_List *l, *ll, *remove_items = NULL, *remove_cats = NULL;
   char buf[1024];

   /* remove anything with a desktop entry */
   for (l = e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat;
	
	ecat = l->data;
	for (ll = ecat->items; ll; ll = ll->next)
	  {
	     E_Configure_It *eci;
	     
	     eci = ll->data;
	     if (eci->desktop)
	       {
		  snprintf(buf, sizeof(buf), "%s/%s", ecat->cat, eci->item);
		  remove_items = evas_list_append(remove_items, strdup(buf));
		  remove_cats = evas_list_append(remove_cats, strdup(ecat->cat));
	       }
	  }
     }
   while (remove_items)
     {
	e_configure_registry_item_del(remove_items->data);
	free(remove_items->data);
	remove_items = evas_list_remove_list(remove_items, remove_items);
     }
   while (remove_cats)
     {
	e_configure_registry_category_del(remove_cats->data);
	free(remove_cats->data);
	remove_cats = evas_list_remove_list(remove_cats, remove_cats);
     }
   
   /* get desktops */
   settings_desktops = efreet_util_desktop_category_list("Settings");
   system_desktops = efreet_util_desktop_category_list("System");
   if ((!settings_desktops) || (!system_desktops)) return;
   
   /* get ones in BOTH lists */
   ecore_list_first_goto(settings_desktops);
   while ((desktop = ecore_list_next(settings_desktops)))
     {
	char *s;
	char *cfg_cat_name;
	char *cfg_cat_icon;
	char *cfg_cat;
	char *cfg_cat_cfg;
	char *cfg_icon;
	char *label;
	int cfg_pri;
	
	if (!ecore_list_goto(system_desktops, desktop)) continue;
	cfg_cat = NULL;
	cfg_icon = NULL;
	cfg_cat_cfg = NULL;
	cfg_pri = 1000;
	cfg_cat_name = NULL;
	cfg_cat_icon = NULL;
	label = NULL;
	if (desktop->x)
	  {
	     cfg_cat_cfg = ecore_hash_get(desktop->x, "X-Enlightenment-Config-Category");
	     s = ecore_hash_get(desktop->x, "X-Enlightenment-Config-Priority");
	     if (s) cfg_pri = atoi(s);
	     cfg_cat_name = ecore_hash_get(desktop->x, "X-Enlightenment-Config-Category-Name");
	     cfg_cat_icon = ecore_hash_get(desktop->x, "X-Enlightenment-Config-Category-Icon");
	     if (cfg_cat_icon)
	       {
		  if (cfg_cat_icon[0] == '/')
		    cfg_cat_icon = strdup(cfg_cat_icon);
		  else
		    cfg_cat_icon = efreet_icon_path_find(e_config->icon_theme,
							 cfg_cat_icon, 64);
	       }
	  }
	if (desktop->icon)
	  {
	     if (desktop->icon[0] == '/')
	       cfg_icon = strdup(desktop->icon);
	     else
	       cfg_icon = efreet_icon_path_find(e_config->icon_theme,
						desktop->icon, 64);
	  }
	if (desktop->name) label = desktop->name;
	else if (desktop->generic_name) label = desktop->generic_name;
	else label = "???";
	if (!cfg_cat_cfg)
	  {
	     char *ic;
	     
	     snprintf(buf, sizeof(buf), "system/%s", label);
	     cfg_cat_cfg = buf;
	     ic = cfg_cat_icon;
	     if (!ic) ic = "enlightenment/system";
	     e_configure_registry_category_add("system",
					       1000, _("System"),
					       NULL, 
					       ic);
	  }
	else
	  {
	     cfg_cat = ecore_file_dir_get(cfg_cat_cfg);
	     if (!cfg_cat) cfg_cat = strdup(cfg_cat_cfg);
	     if (cfg_cat)
	       {
		  if (!cfg_cat_name)
		    cfg_cat_name = cfg_cat;
		  e_configure_registry_category_add(cfg_cat,
						    1000, cfg_cat_name,
						    NULL, 
						    cfg_cat_icon);
		  free(cfg_cat);
		  cfg_cat = NULL;
	       }
	  }
	_e_configure_registry_item_full_add(cfg_cat_cfg, cfg_pri, label,
					    NULL, cfg_icon,
					    NULL, NULL,
					    desktop);
	if (cfg_icon) free(cfg_icon);
	if (cfg_cat_icon) free(cfg_cat_icon);
     }
}

static int
_e_configure_cb_efreet_desktop_list_change(void *data, int type, void *event)
{
   _e_configure_efreet_desktop_update();
   return 1;
}

static int
_e_configure_cb_efreet_desktop_change(void *data, int type, void *event)
{
   _e_configure_efreet_desktop_update();
   return 1;
}

static void
_e_configure_registry_item_full_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon, E_Config_Dialog *(*func) (E_Container *con, const char *params), void (*generic_func) (E_Container *con, const char *params), Efreet_Desktop *desktop)
{
   Evas_List *l;
   char *cat;
   const char *item;
   E_Configure_It *eci;
   
   /* path is "category/item" */
   cat = ecore_file_dir_get(path);
   if (!cat) return;
   item = ecore_file_file_get(path);
   eci = E_NEW(E_Configure_It, 1);
   if (!eci) goto done;
   
   eci->item = evas_stringshare_add(item);
   eci->pri = pri;
   eci->label = evas_stringshare_add(label);
   if (icon_file) eci->icon_file = evas_stringshare_add(icon_file);
   if (icon) eci->icon = evas_stringshare_add(icon);
   eci->func = func;
   eci->generic_func = generic_func;
   eci->desktop = desktop;
   if (eci->desktop) efreet_desktop_ref(eci->desktop);
   
   for (l = e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat;
	
	ecat = l->data;
	if (!strcmp(cat, ecat->cat))
	  {
	     Evas_List *ll;
	     
	     for (ll = ecat->items; ll; ll = ll->next)
	       {
		  E_Configure_It *eci2;
		  
		  eci2 = ll->data;
		  if (eci2->pri > eci->pri)
		    {
		       ecat->items = evas_list_prepend_relative_list(ecat->items, eci, ll);
		       goto done;
		    }
	       }
	     ecat->items = evas_list_append(ecat->items, eci);
	     goto done;
	  }
     }
   done:
   free(cat);
}

EAPI void
e_configure_registry_item_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon, E_Config_Dialog *(*func) (E_Container *con, const char *params))
{
   _e_configure_registry_item_full_add(path, pri, label, icon_file, icon, func, NULL, NULL);
}

EAPI void
e_configure_registry_generic_item_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon, void (*generic_func) (E_Container *con, const char *params))
{
   _e_configure_registry_item_full_add(path, pri, label, icon_file, icon, NULL, generic_func, NULL);
}

EAPI void
e_configure_registry_item_del(const char *path)
{
   Evas_List *l;
   char *cat;
   const char *item;
   
   /* path is "category/item" */
   cat = ecore_file_dir_get(path);
   if (!cat) return;
   item = ecore_file_file_get(path);
   for (l = e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat;
	
	ecat = l->data;
	if (!strcmp(cat, ecat->cat))
	  {
	     Evas_List *ll;
	     
	     for (ll = ecat->items; ll; ll = ll->next)
	       {
		  E_Configure_It *eci;
		  
		  eci = ll->data;
		  if (!strcmp(item, eci->item))
		    {
		       ecat->items = evas_list_remove_list(ecat->items, ll);
		       evas_stringshare_del(eci->item);
		       evas_stringshare_del(eci->label);
		       evas_stringshare_del(eci->icon);
		       if (eci->icon_file) evas_stringshare_del(eci->icon_file);
		       if (eci->desktop) efreet_desktop_free(eci->desktop);
		       free(eci);
		       goto done;
		    }
	       }
	     goto done;
	  }
     }
   done:
   free(cat);
}

EAPI void
e_configure_registry_category_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon)
{
   E_Configure_Cat *ecat;
   Evas_List *l;

   /* if it exists - ignore this */
   for (l = e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat2;
	
	ecat2 = l->data;
	if (!strcmp(ecat2->cat, path)) return;
     }
   
   ecat = E_NEW(E_Configure_Cat, 1);
   if (!ecat) return;
   
   ecat->cat = evas_stringshare_add(path);
   ecat->pri = pri;
   ecat->label = evas_stringshare_add(label);
   if (icon_file) ecat->icon_file = evas_stringshare_add(icon_file);
   if (icon) ecat->icon = evas_stringshare_add(icon);
   for (l = e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat2;
	
	ecat2 = l->data;
	if (ecat2->pri > ecat->pri)
	  {
	     e_configure_registry = evas_list_prepend_relative_list(e_configure_registry, ecat, l);
	     return;
	  }
     }
   e_configure_registry = evas_list_append(e_configure_registry, ecat);
}

EAPI void
e_configure_registry_category_del(const char *path)
{
   Evas_List *l;
   char *cat;
   
   cat = ecore_file_dir_get(path);
   if (!cat) return;
   for (l = e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat;
	
	ecat = l->data;
        if (!strcmp(cat, ecat->cat))
	  {
	     if (ecat->items) goto done;
	     e_configure_registry = evas_list_remove_list(e_configure_registry, l);
	     evas_stringshare_del(ecat->cat);
	     evas_stringshare_del(ecat->label);
	     if (ecat->icon) evas_stringshare_del(ecat->icon);
	     if (ecat->icon_file) evas_stringshare_del(ecat->icon_file);
	     free(ecat);
	     goto done;
	  }
     }
   done:
   free(cat);
}

static struct {
   void (*func) (const void *data, E_Container *con, const char *params, Efreet_Desktop *desktop);
   const char *data;
} custom_desktop_exec = { NULL, NULL };

EAPI void
e_configure_registry_call(const char *path, E_Container *con, const char *params)
{
   Evas_List *l;
   char *cat;
   const char *item;
   
   /* path is "category/item" */
   cat = ecore_file_dir_get(path);
   if (!cat) return;
   item = ecore_file_file_get(path);
   for (l = e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat;
	
	ecat = l->data;
	if (!strcmp(cat, ecat->cat))
	  {
	     Evas_List *ll;
	     
	     for (ll = ecat->items; ll; ll = ll->next)
	       {
		  E_Configure_It *eci;
		  
		  eci = ll->data;
		  if (!strcmp(item, eci->item))
		    {
		       if (eci->func) eci->func(con, params);
		       else if (eci->generic_func) eci->generic_func(con, params);
		       else if (eci->desktop)
			 {
			    if (custom_desktop_exec.func)
			      custom_desktop_exec.func(custom_desktop_exec.data,
						       con,
						       params,
						       eci->desktop);
			    else
			      e_exec(e_util_zone_current_get(con->manager),
				     eci->desktop, NULL, NULL, "config");
			 }
		       goto done;
		    }
	       }
	     goto done;
	  }
     }
   done:
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
   Evas_List *l;
   char *cat;
   const char *item;
   int ret = 0;
   
   /* path is "category/item" */
   cat = ecore_file_dir_get(path);
   if (!cat) return 0;
   item = ecore_file_file_get(path);
   for (l = e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat;
	
	ecat = l->data;
	if (!strcmp(cat, ecat->cat))
	  {
	     Evas_List *ll;

	     if (!item)
	       {
		  ret = 1;
		  goto done;
	       }
	     for (ll = ecat->items; ll; ll = ll->next)
	       {
		  E_Configure_It *eci;
		  
		  eci = ll->data;
		  if (!strcmp(item, eci->item))
		    {
		       ret = 1;
		       goto done;
		    }
	       }
	     goto done;
	  }
     }
   done:
   free(cat);
   return ret;
}
