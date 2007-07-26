#include "e.h"

/* Dialog Protos */
static void *_create_data(E_Config_Dialog *cfd);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

/* Private Function Protos */
static void _load_modules(E_Config_Dialog_Data *cfdata);
static int _sort_modules(void *data1, void *data2);
static void _fill_all(E_Config_Dialog_Data *cfdata);
static void _fill_loaded(E_Config_Dialog_Data *cfdata);
static char *_get_icon(Efreet_Desktop *desk);
static E_Module *_get_module(E_Config_Dialog_Data *cfdata, const char *lbl);

/* Callbacks */
static void _cb_monitor(void *data, Ecore_File_Monitor *monitor, Ecore_File_Event event, const char *path);
static void _cb_mod_monitor(void *data, Ecore_File_Monitor *monitor, Ecore_File_Event event, const char *path);
static void _cb_dir_monitor(void *data, Ecore_File_Monitor *monitor, Ecore_File_Event event, const char *path);
static void _cb_all_change(void *data, Evas_Object *obj);
static void _cb_loaded_change(void *data, Evas_Object *obj);
static void _cb_load(void *data, void *data2);
static void _cb_unload(void *data, void *data2);
static void _cb_about(void *data, void *data2);
static void _cb_config(void *data, void *data2);
static int  _cb_mod_update(void *data, int type, void *event);

struct _E_Config_Dialog_Data 
{
   Evas_List *modules;
   Evas_Object *o_all, *o_loaded;
   Evas_Object *b_load, *b_unload;
   Evas_Object *b_about, *b_configure;
   
   Ecore_Event_Handler *hdl;
};

static Evas_List *monitors = NULL;
Ecore_File_Monitor *mod_mon, *dir_mon;

EAPI E_Config_Dialog *
e_int_config_modules(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_modules_dialog")) return NULL;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   
   cfd = e_config_dialog_new(con, _("Module Settings"), "E", 
			     "_config_modules_dialog", "enlightenment/modules",
			     0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
   return cfd;
}

/* Dialog Functions */
static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   Evas_List *l = NULL, *mdirs = NULL;
   
   /* Setup file monitors for module directories */
   mdirs = e_path_dir_list_get(path_modules);
   for (l = mdirs; l; l = l->next) 
     {
	E_Path_Dir *epd;
	Ecore_File_Monitor *mon;
	
	epd = l->data;
	if (!ecore_file_is_dir(epd->dir)) continue;
	mon = ecore_file_monitor_add(epd->dir, _cb_monitor, cfdata);
	monitors = evas_list_append(monitors, mon);
     }
   if (l) evas_list_free(l);
   if (mdirs) e_path_dir_list_free(mdirs);
   
   /* Load available modules */
   _load_modules(cfdata);
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   /* Remove module update event handler */
   if (cfdata->hdl) ecore_event_handler_del(cfdata->hdl);
   
   /* Remove file monitors for module directories */
   if (mod_mon) ecore_file_monitor_del(mod_mon);
   if (dir_mon) ecore_file_monitor_del(dir_mon);
   while (monitors) 
     {
	Ecore_File_Monitor *mon;
	
	mon = monitors->data;
	ecore_file_monitor_del(mon);
	monitors = evas_list_remove_list(monitors, monitors);
     }

   /* Free the stored list of modules */
   while (cfdata->modules) 
     cfdata->modules = evas_list_remove_list(cfdata->modules, cfdata->modules);
   
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   
   o = e_widget_table_add(evas, 0);
   
   of = e_widget_frametable_add(evas, _("Available Modules"), 0);
   ob = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_multi_select_set(ob, 1);
   e_widget_on_change_hook_set(ob, _cb_all_change, cfdata);
   cfdata->o_all = ob;
   _fill_all(cfdata);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 1, 1);
   ob = e_widget_button_add(evas, _("Load Module"), NULL, _cb_load, cfdata, NULL);
   cfdata->b_load = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 1, 0);
   e_widget_table_object_append(o, of, 0, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_frametable_add(evas, _("Loaded Modules"), 0);
   ob = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_multi_select_set(ob, 1);
   e_widget_on_change_hook_set(ob, _cb_loaded_change, cfdata);
   cfdata->o_loaded = ob;
   _fill_loaded(cfdata);
   e_widget_frametable_object_append(of, ob, 0, 0, 2, 1, 1, 1, 1, 1);
   
   ob = e_widget_button_add(evas, _("Unload Module"), NULL, _cb_unload, cfdata, NULL);
   cfdata->b_unload = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 2, 2, 1, 1, 1, 1, 0);

   ob = e_widget_button_add(evas, _("About"), NULL, _cb_about, cfdata, NULL);
   cfdata->b_about = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 0, 0, 0);
   ob = e_widget_button_add(evas, _("Configure"), NULL, _cb_config, cfdata, NULL);
   cfdata->b_configure = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 1, 1, 1, 1, 1, 0, 0, 0);
   
   e_widget_table_object_append(o, of, 1, 0, 1, 1, 1, 1, 1, 1);
   
   /* Setup Module update handler */
   if (cfdata->hdl) ecore_event_handler_del(cfdata->hdl);
   cfdata->hdl = ecore_event_handler_add(E_EVENT_MODULE_UPDATE, 
					 _cb_mod_update, cfdata);
   
   return o;   
}

/* Private Functions */
static void 
_load_modules(E_Config_Dialog_Data *cfdata) 
{
   Evas_List *l = NULL, *mdirs = NULL;

   if (!cfdata) return;

   /* Free the stored list of modules */
   while (cfdata->modules) 
     cfdata->modules = evas_list_remove_list(cfdata->modules, cfdata->modules);

   /* Get list of modules to sort */
   mdirs = e_path_dir_list_get(path_modules);
   for (l = mdirs; l; l = l->next) 
     {
	E_Path_Dir *epd;
	Ecore_List *dirs = NULL;
	char *mod;
	
	epd = l->data;
	if (!ecore_file_is_dir(epd->dir)) continue;
	dirs = ecore_file_ls(epd->dir);
	if (!dirs) continue;
	ecore_list_first_goto(dirs);
	while ((mod = ecore_list_next(dirs)))
	  {
	     E_Module *module;
	     char buf[4096];
	     
	     snprintf(buf, sizeof(buf), "%s/%s/module.desktop", epd->dir, mod);
	     if (!ecore_file_exists(buf)) continue;
	     module = e_module_find(mod);
	     if (!module) module = e_module_new(mod);
	     if (module)
	       cfdata->modules = evas_list_append(cfdata->modules, module);
	  }
	free(mod);
	ecore_list_destroy(dirs);
     }
   if (l) evas_list_free(l);
   if (mdirs) e_path_dir_list_free(mdirs);
   
   /* Sort the modules */
   if (cfdata->modules)
     cfdata->modules = evas_list_sort(cfdata->modules, -1, _sort_modules);
}

static int
_sort_modules(void *data1, void *data2) 
{
   E_Module *m1, *m2;
   
   if (!data1) return 1;
   if (!data2) return -1;
   m1 = data1;
   m2 = data2;
   return (strcmp(m1->name, m2->name));
}

static void 
_fill_all(E_Config_Dialog_Data *cfdata) 
{
   Evas *evas;
   Evas_List *l = NULL;
   Evas_Coord w;
   
   if (!cfdata->o_all) return;
   
   /* Freeze ilist */
   evas = evas_object_evas_get(cfdata->o_all);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_all);
   e_widget_ilist_clear(cfdata->o_all);
   
   /* Loop modules & load ilist */
   for (l = cfdata->modules; l; l = l->next) 
     {
	E_Module *mod = NULL;
	Efreet_Desktop *desk = NULL;
	Evas_Object *oc = NULL;
	char buf[4096];
	const char *icon;
	
	mod = l->data;
	if (!mod) continue;
	if (mod->enabled) continue;
	snprintf(buf, sizeof(buf), "%s/module.desktop", mod->dir);
	if (!ecore_file_exists(buf)) continue;
	desk = efreet_desktop_get(buf);
	if (!desk) continue;
	icon = _get_icon(desk);
	if (icon)
	  {
	     oc = e_util_icon_add(icon, evas);
	     free(icon);
	  }
	e_widget_ilist_append(cfdata->o_all, oc, desk->name, NULL, NULL, NULL);
	efreet_desktop_free(desk);
     }

   /* Unfreeze ilist */
   e_widget_ilist_go(cfdata->o_all);
   e_widget_min_size_get(cfdata->o_all, &w, NULL);
   e_widget_min_size_set(cfdata->o_all, w, 200);
   e_widget_ilist_thaw(cfdata->o_all);
   edje_thaw();
   evas_event_thaw(evas);
   
   e_widget_disabled_set(cfdata->b_load, 1);
}

static void 
_fill_loaded(E_Config_Dialog_Data *cfdata) 
{
   Evas *evas;
   Evas_List *l = NULL;
   Evas_Coord w;
   
   if (!cfdata->o_loaded) return;

   /* Freeze ilist */
   evas = evas_object_evas_get(cfdata->o_loaded);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_loaded);
   e_widget_ilist_clear(cfdata->o_loaded);

   /* Loop modules & load ilist */
   for (l = cfdata->modules; l; l = l->next) 
     {
	E_Module *mod = NULL;
	Efreet_Desktop *desk = NULL;
	Evas_Object *oc = NULL;
	char buf[4096];
	const char *icon;
	
	mod = l->data;
	if (!mod) continue;
	if (!mod->enabled) continue;
	snprintf(buf, sizeof(buf), "%s/module.desktop", mod->dir);
	if (!ecore_file_exists(buf)) continue;
	desk = efreet_desktop_get(buf);
	if (!desk) continue;
	icon = _get_icon(desk);
	if (icon)
	  {
	     oc = e_util_icon_add(icon, evas);
	     free(icon);
	  }
	e_widget_ilist_append(cfdata->o_loaded, oc, desk->name, NULL, NULL, NULL);
	efreet_desktop_free(desk);
     }

   /* Unfreeze ilist */
   e_widget_ilist_go(cfdata->o_loaded);
   e_widget_min_size_get(cfdata->o_loaded, &w, NULL);
   e_widget_min_size_set(cfdata->o_loaded, w, 200);
   e_widget_ilist_thaw(cfdata->o_loaded);
   edje_thaw();
   evas_event_thaw(evas);
   
   e_widget_disabled_set(cfdata->b_unload, 1);
   e_widget_disabled_set(cfdata->b_about, 1);
   e_widget_disabled_set(cfdata->b_configure, 1);
}

static char *
_get_icon(Efreet_Desktop *desk) 
{
   char *icon;
   
   if (!desk) return NULL;
   if (desk->icon) 
     {
	icon = efreet_icon_path_find(e_config->icon_theme, desk->icon, "24x24");
	if (!icon) 
	  {
	     char *path;
	     char buf[4096];
	     
	     path = ecore_file_dir_get(desk->orig_path);
	     snprintf(buf, sizeof(buf), "%s/%s.edj", path, desk->icon);
	     icon = strdup(buf);
	     free(path);
	  }
	return icon;
     }
   return NULL;
}

static E_Module *
_get_module(E_Config_Dialog_Data *cfdata, const char *lbl) 
{
   Evas_List *l = NULL;
   E_Module *mod = NULL;
   
   if (!cfdata) return NULL;
   if (!lbl) return NULL;
   
   for (l = cfdata->modules; l; l = l->next) 
     {
	Efreet_Desktop *desk = NULL;
	char buf[4096];
	
	mod = l->data;
	if (!mod) continue;
	snprintf(buf, sizeof(buf), "%s/module.desktop", mod->dir);
	if (!ecore_file_exists(buf)) continue;
	desk = efreet_desktop_get(buf);
	if (!desk) continue;
	if (!strcmp(desk->name, lbl))
	  {
	     efreet_desktop_free(desk);
	     break;
	  }
	efreet_desktop_free(desk);
     }
   return mod;
}

/* Callbacks */
static void 
_cb_monitor(void *data, Ecore_File_Monitor *monitor, Ecore_File_Event event, const char *path) 
{
   E_Config_Dialog_Data *cfdata;
   const char *file;
   
   cfdata = data;
   if (!cfdata) return;
   
   switch (event) 
     {
      case ECORE_FILE_EVENT_CREATED_DIRECTORY:
	file = ecore_file_file_get(path);
	if (mod_mon) ecore_file_monitor_del(mod_mon);
	mod_mon = ecore_file_monitor_add(path, _cb_mod_monitor, cfdata);
	break;
      case ECORE_FILE_EVENT_DELETED_DIRECTORY:
	_load_modules(cfdata);
	_fill_all(cfdata);
	_fill_loaded(cfdata);
	break;
      default:
	break;
     }
}

static void 
_cb_mod_monitor(void *data, Ecore_File_Monitor *monitor, Ecore_File_Event event, const char *path) 
{
   E_Config_Dialog_Data *cfdata;
   const char *file;
   
   cfdata = data;
   if (!cfdata) return;
   
   switch (event) 
     {
      case ECORE_FILE_EVENT_CREATED_DIRECTORY:
	file = ecore_file_file_get(path);
	if (!e_util_glob_case_match(file, MODULE_ARCH)) break;
	if (dir_mon) ecore_file_monitor_del(dir_mon);
	dir_mon = ecore_file_monitor_add(path, _cb_dir_monitor, cfdata);
	break;
      default:
	break;
     }
}

static void 
_cb_dir_monitor(void *data, Ecore_File_Monitor *monitor, Ecore_File_Event event, const char *path) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata) return;
   
   switch (event) 
     {
      case ECORE_FILE_EVENT_CREATED_FILE:
	if (e_util_glob_case_match(path, "*.so")) 
	  {
	     ecore_file_monitor_del(dir_mon);
	     dir_mon = NULL;
	     if (mod_mon) ecore_file_monitor_del(mod_mon);
	     mod_mon = NULL;
	     _load_modules(cfdata);
	     _fill_all(cfdata);
	     _fill_loaded(cfdata);
	  }
	break;
      default:
	break;
     }
}

static void 
_cb_all_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata) return;
   e_widget_disabled_set(cfdata->b_load, 0);
}

static void 
_cb_loaded_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;
   E_Module *mod;
   const char *lbl;
   int count, idx;
   
   cfdata = data;
   if (!cfdata) return;
   count = e_widget_ilist_selected_count_get(cfdata->o_loaded);
   e_widget_disabled_set(cfdata->b_about, 1);
   e_widget_disabled_set(cfdata->b_configure, 1);
   e_widget_disabled_set(cfdata->b_unload, 0);
   if (count == 1) 
     {
	idx = e_widget_ilist_selected_get(cfdata->o_loaded);
	lbl = e_widget_ilist_nth_label_get(cfdata->o_loaded, idx);
	mod = _get_module(cfdata, lbl);
	if (!mod) return;
	if (mod->func.about) e_widget_disabled_set(cfdata->b_about, 0);
	if (mod->func.config) e_widget_disabled_set(cfdata->b_configure, 0);
     }
}

static void 
_cb_load(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l = NULL;
   int idx;
   
   cfdata = data;
   if (!cfdata) return;
   
   /* Check that something is selected */
   idx = e_widget_ilist_selected_get(cfdata->o_all);
   if (idx < 0) 
     {
	e_widget_ilist_unselect(cfdata->o_all);
	e_widget_disabled_set(cfdata->b_load, 1);
	return;
     }
   
   /* Loop the selected items, loading modules which were asked for */
   for (idx = 0, l = e_widget_ilist_items_get(cfdata->o_all); l; l = l->next, idx++) 
     {
	E_Ilist_Item *item = NULL;
	E_Module *mod = NULL;
	const char *lbl;
	
	item = l->data;
	if (!item) continue;
	if (!item->selected) continue;
	lbl = e_widget_ilist_nth_label_get(cfdata->o_all, idx);
	mod = _get_module(cfdata, lbl);
	if ((mod) && (!mod->enabled)) 
	  e_module_enable(mod);
     }
   if (l) evas_list_free(l);
}

static void 
_cb_unload(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l = NULL;
   int idx;
   
   cfdata = data;
   if (!cfdata) return;
   
   /* Check that something is selected */
   idx = e_widget_ilist_selected_get(cfdata->o_loaded);
   if (idx < 0) 
     {
	e_widget_ilist_unselect(cfdata->o_loaded);
	e_widget_disabled_set(cfdata->b_unload, 1);
	return;
     }

   /* Loop the selected items, unloading modules which were asked for */
   for (idx = 0, l = e_widget_ilist_items_get(cfdata->o_loaded); l; l = l->next, idx++) 
     {
	E_Ilist_Item *item = NULL;
	E_Module *mod = NULL;
	const char *lbl;
	
	item = l->data;
	if (!item) continue;
	if (!item->selected) continue;
	lbl = e_widget_ilist_nth_label_get(cfdata->o_loaded, idx);
	mod = _get_module(cfdata, lbl);
	if ((mod) && (mod->enabled)) e_module_disable(mod);
     }
   if (l) evas_list_free(l);
}

static void 
_cb_about(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   E_Module *mod = NULL;
   const char *lbl;
   int idx;
   
   cfdata = data;
   if (!cfdata) return;
   idx = e_widget_ilist_selected_get(cfdata->o_loaded);
   lbl = e_widget_ilist_nth_label_get(cfdata->o_loaded, idx);
   mod = _get_module(cfdata, lbl);
   if (!mod) return;
   if ((mod) && (mod->func.about)) mod->func.about(mod);
}

static void 
_cb_config(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   E_Module *mod = NULL;
   const char *lbl;
   int idx;
   
   cfdata = data;
   if (!cfdata) return;
   idx = e_widget_ilist_selected_get(cfdata->o_loaded);
   lbl = e_widget_ilist_nth_label_get(cfdata->o_loaded, idx);
   mod = _get_module(cfdata, lbl);
   if (!mod) return;
   if ((mod) && (mod->func.config)) mod->func.config(mod);
}

static int 
_cb_mod_update(void *data, int type, void *event) 
{
   E_Event_Module_Update *ev;
   E_Config_Dialog_Data *cfdata;
   
   if (type != E_EVENT_MODULE_UPDATE) return 1;

   cfdata = data;
   ev = event;
   if (!cfdata) return 1;
   
   _fill_all(cfdata);
   _fill_loaded(cfdata);
   return 1;
}
