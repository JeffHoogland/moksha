/*
    * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
*/

#include "e.h"

typedef struct _CFModule 
{
   const char *short_name, *name, *comment;
   const char *icon, *orig_path;
   int enabled, selected;
} CFModule;

struct _E_Config_Dialog_Data 
{
   Evas_Object *o_avail, *o_loaded;
   Evas_Object *b_load, *b_unload;
   Evas_Object *b_about, *b_config;
   Evas_Object *o_desc;
};

static void        *_create_data  (E_Config_Dialog *cfd);
static void         _fill_data    (E_Config_Dialog_Data *cfdata);
static void         _free_data    (E_Config_Dialog *cfd, 
				   E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create (E_Config_Dialog *cfd, Evas *evas, 
				   E_Config_Dialog_Data *cfdata);

/* Private Function Protos */
static void _load_modules          (const char *dir);
static int  _modules_list_cb_sort  (void *data1, void *data2);
static void _fill_avail_list       (E_Config_Dialog_Data *cfdata);
static void _fill_loaded_list      (E_Config_Dialog_Data *cfdata);
static void _avail_list_cb_change  (void *data, Evas_Object *obj);
static void _loaded_list_cb_change (void *data, Evas_Object *obj);
static void _btn_cb_unload         (void *data, void *data2);
static void _btn_cb_load           (void *data, void *data2);
static void _btn_cb_about          (void *data, void *data2);
static void _btn_cb_config         (void *data, void *data2);
static int  _upd_hdl_cb            (void *data, int type, void *event);

/* Hash callback Protos */
static Evas_Bool _modules_hash_cb_free   (Evas_Hash *hash __UNUSED__, 
					  const char *key __UNUSED__, 
					  void *data, void *fdata __UNUSED__);
static Evas_Bool _modules_hash_cb_unsel  (Evas_Hash *hash __UNUSED__, 
					  const char *key __UNUSED__, 
					  void *data, void *fdata __UNUSED__);
static Evas_Bool _modules_hash_cb_load   (Evas_Hash *hash __UNUSED__, 
					  const char *key __UNUSED__, 
					  void *data, void *fdata __UNUSED__);
static Evas_Bool _modules_hash_cb_unload (Evas_Hash *hash __UNUSED__, 
					  const char *key __UNUSED__, 
					  void *data, void *fdata __UNUSED__);
static Evas_Bool _modules_hash_cb_about  (Evas_Hash *hash __UNUSED__, 
					  const char *key __UNUSED__, 
					  void *data, void *fdata __UNUSED__);
static Evas_Bool _modules_hash_cb_config (Evas_Hash *hash __UNUSED__, 
					  const char *key __UNUSED__, 
					  void *data, void *fdata __UNUSED__);

static Evas_Hash *modules = NULL;
static Evas_List *modules_list = NULL;
Ecore_Event_Handler *upd_hdl = NULL;

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
   
   cfd = e_config_dialog_new(con, _("Module Settings"), 
			     "E", "_config_modules_dialog", 
			     "enlightenment/modules", 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
   return cfd;
}

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
   Evas_List *mdirs = NULL, *l = NULL;

   if (!cfdata) return;
   mdirs = e_path_dir_list_get(path_modules);
   for (l = mdirs; l; l = l->next) 
     {
	E_Path_Dir *epd;
	
	epd = l->data;
	if (!epd) continue;
	if (!ecore_file_is_dir(epd->dir)) continue;
	_load_modules(epd->dir);
     }
   if (l) evas_list_free(l);
   if (mdirs) e_path_dir_list_free(mdirs);
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (upd_hdl) ecore_event_handler_del(upd_hdl);
   upd_hdl = NULL;
   
   if (modules) 
     {
	evas_hash_foreach(modules, _modules_hash_cb_free, NULL);
	evas_hash_free(modules);
	modules = NULL;
     }
   while (modules_list) 
     {
	char *m;
	
	m = modules_list->data;
	modules_list = evas_list_remove_list(modules_list, modules_list);
	free(m);
     }
   
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *ot, *of, *ow;

   ot = e_widget_table_add(evas, 0);
   
   of = e_widget_frametable_add(evas, _("Available Modules"), 0);
   ow = e_widget_ilist_add(evas, 24, 24, NULL);
   cfdata->o_avail = ow;
   e_widget_ilist_multi_select_set(ow, 1);
   e_widget_on_change_hook_set(ow, _avail_list_cb_change, cfdata);
   _fill_avail_list(cfdata);
   e_widget_frametable_object_append(of, ow, 0, 0, 1, 1, 1, 1, 1, 1);
   ow = e_widget_textblock_add(evas);
   cfdata->o_desc = ow;
   e_widget_textblock_markup_set(ow, "Description: Unavailable.");
   e_widget_frametable_object_append(of, ow, 0, 1, 1, 1, 1, 1, 1, 1);
   ow = e_widget_button_add(evas, _("Load Module"), NULL, _btn_cb_load, 
			    cfdata, NULL);
   cfdata->b_load = ow;
   e_widget_disabled_set(ow, 1);
   e_widget_frametable_object_append(of, ow, 0, 2, 1, 1, 1, 1, 1, 0);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);
   
   of = e_widget_frametable_add(evas, _("Loaded Modules"), 0);
   ow = e_widget_ilist_add(evas, 24, 24, NULL);
   cfdata->o_loaded = ow;
   e_widget_ilist_multi_select_set(ow, 1);
   e_widget_on_change_hook_set(ow, _loaded_list_cb_change, cfdata);
   _fill_loaded_list(cfdata);
   e_widget_frametable_object_append(of, ow, 0, 0, 2, 1, 1, 1, 1, 1);
   ow = e_widget_button_add(evas, _("Unload Module"), NULL, _btn_cb_unload, 
			    cfdata, NULL);
   cfdata->b_unload = ow;
   e_widget_disabled_set(ow, 1);
   e_widget_frametable_object_append(of, ow, 0, 2, 2, 1, 1, 1, 1, 0);
   ow = e_widget_button_add(evas, _("About"), NULL, _btn_cb_about, 
			    NULL, NULL);
   cfdata->b_about = ow;
   e_widget_disabled_set(ow, 1);
   e_widget_frametable_object_append(of, ow, 0, 1, 1, 1, 1, 0, 0, 0);
   ow = e_widget_button_add(evas, _("Configure"), NULL, _btn_cb_config, 
			    NULL, NULL);
   cfdata->b_config = ow;
   e_widget_disabled_set(ow, 1);
   e_widget_frametable_object_append(of, ow, 1, 1, 1, 1, 1, 0, 0, 0);
   
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);
   
   /* Setup Module update Handler */
   if (upd_hdl) ecore_event_handler_del(upd_hdl);
   upd_hdl = NULL;
   upd_hdl = ecore_event_handler_add(E_EVENT_MODULE_UPDATE, _upd_hdl_cb, cfdata);
   
   return ot;
}

/* Private Functions */
static void 
_load_modules(const char *dir) 
{
   Ecore_List *files = NULL;
   char *mod;

   files = ecore_file_ls(dir);
   if (!files) return;

   ecore_list_first_goto(files);
   while ((mod = ecore_list_next(files))) 
     {
	char buf[4096];
	Efreet_Desktop *desktop;
	CFModule *module;

	snprintf(buf, sizeof(buf), "%s/%s/module.desktop", dir, mod);
	if (!ecore_file_exists(buf)) continue;
	desktop = efreet_desktop_get(buf);
	if (!desktop) continue;
	if (evas_hash_find(modules, desktop->name)) 
	  {
	     efreet_desktop_free(desktop);
	     continue;
	  }
	
	module = E_NEW(CFModule, 1);
	module->short_name = evas_stringshare_add(mod);
	if (desktop->name) module->name = evas_stringshare_add(desktop->name);
	if (desktop->icon) module->icon = evas_stringshare_add(desktop->icon);
	if (desktop->comment) module->comment = evas_stringshare_add(desktop->comment);
	if (desktop->orig_path) 
	  module->orig_path = evas_stringshare_add(desktop->orig_path);
	if (e_module_find(mod)) module->enabled = 1;
	
	modules = evas_hash_direct_add(modules, 
				       evas_stringshare_add(desktop->name), 
				       module);
	modules_list = evas_list_append(modules_list, strdup(desktop->name));
	efreet_desktop_free(desktop);
     }
   free(mod);
   if (files) ecore_list_destroy(files);
   
   if (modules_list) 
     modules_list = evas_list_sort(modules_list, -1, _modules_list_cb_sort);
}

static int 
_modules_list_cb_sort(void *data1, void *data2) 
{
   if (!data1) return 1;
   if (!data2) return -1;
   return (strcmp((char *)data1, (char *)data2));
}

static void 
_fill_avail_list(E_Config_Dialog_Data *cfdata) 
{
   Evas *evas;
   Evas_List *l = NULL;
   Evas_Coord w;

   if (!cfdata) return;
   
   evas = evas_object_evas_get(cfdata->o_avail);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_avail);
   e_widget_ilist_clear(cfdata->o_avail);
   
   for (l = modules_list; l; l = l->next) 
     {
	CFModule *module = NULL;
	Evas_Object *ic = NULL;
	char *name, *icon, *path;
	char buf[4096];
	
	name = l->data;
	if (!name) continue;
	module = evas_hash_find(modules, name);
	if ((!module) || (module->enabled) || (!module->icon)) continue;
	icon = efreet_icon_path_find(e_config->icon_theme, 
				     module->icon, "24x24");
	if ((!icon) && (module->orig_path))
	  {
	     path = ecore_file_dir_get(module->orig_path);
	     snprintf(buf, sizeof(buf), "%s/%s.edj", path, module->icon);
	     icon = strdup(buf);
	     free(path);
	  }
	if (icon) 
	  {
	     ic = e_util_icon_add(icon, evas);
	     free(icon);
	  }
	if (module->name)
	  e_widget_ilist_append(cfdata->o_avail, ic, module->name, NULL, NULL, NULL);
	else if (module->short_name)
	  e_widget_ilist_append(cfdata->o_avail, ic, module->short_name, NULL, NULL, NULL);
     }
   
   e_widget_ilist_go(cfdata->o_avail);
   e_widget_min_size_get(cfdata->o_avail, &w, NULL);
   e_widget_min_size_set(cfdata->o_avail, w, 200);
   e_widget_ilist_thaw(cfdata->o_avail);
   edje_thaw();
   evas_event_thaw(evas);
   if (l) evas_list_free(l);
}

static void 
_fill_loaded_list(E_Config_Dialog_Data *cfdata) 
{
   Evas *evas;
   Evas_List *l = NULL;
   Evas_Coord w;

   if (!cfdata) return;
   
   evas = evas_object_evas_get(cfdata->o_loaded);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_loaded);
   e_widget_ilist_clear(cfdata->o_loaded);
   
   for (l = modules_list; l; l = l->next) 
     {
	CFModule *module = NULL;
	Evas_Object *ic = NULL;
	char *name, *icon, *path;
	char buf[4096];
	
	name = l->data;
	if (!name) continue;
	module = evas_hash_find(modules, name);
	if ((!module) || (!module->enabled) || (!module->icon)) continue;
	icon = efreet_icon_path_find(e_config->icon_theme, 
				     module->icon, "24x24");
	if ((!icon) && (module->orig_path))
	  {
	     path = ecore_file_dir_get(module->orig_path);
	     snprintf(buf, sizeof(buf), "%s/%s.edj", path, module->icon);
	     icon = strdup(buf);
	     free(path);
	  }
	if (icon) 
	  {
	     ic = e_util_icon_add(icon, evas);
	     free(icon);
	  }
	if (module->name)
	  e_widget_ilist_append(cfdata->o_loaded, ic, module->name, NULL, NULL, NULL);
	else if (module->short_name)
	  e_widget_ilist_append(cfdata->o_loaded, ic, module->short_name, NULL, NULL, NULL);
     }
   
   e_widget_ilist_go(cfdata->o_loaded);
   e_widget_min_size_get(cfdata->o_loaded, &w, NULL);
   e_widget_min_size_set(cfdata->o_loaded, w, 200);
   e_widget_ilist_thaw(cfdata->o_loaded);
   edje_thaw();
   evas_event_thaw(evas);
   if (l) evas_list_free(l);
}

static void 
_avail_list_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l = NULL;
   int i;
   
   cfdata = data;
   if (!cfdata) return;

   /* Loop the hash & unselect all */
   evas_hash_foreach(modules, _modules_hash_cb_unsel, NULL);
   
   /* Unselect all in loaded list & disable buttons */
   e_widget_ilist_unselect(cfdata->o_loaded);
   e_widget_disabled_set(cfdata->b_unload, 1);
   e_widget_disabled_set(cfdata->b_about, 1);
   e_widget_disabled_set(cfdata->b_config, 1);

   /* Make sure something is selected, else disable the load button */
   if (e_widget_ilist_selected_count_get(cfdata->o_avail) <= 0) 
     {
	e_widget_disabled_set(cfdata->b_load, 1);
	return;
     }

   for (i = 0, l = e_widget_ilist_items_get(cfdata->o_avail); l; l = l->next, i++) 
     {
	E_Ilist_Item *item = NULL;
	CFModule *module = NULL;
	const char *lbl;
	
	item = l->data;
	if ((!item) || (!item->selected)) continue;
	lbl = e_widget_ilist_nth_label_get(cfdata->o_avail, i);
	module = evas_hash_find(modules, lbl);
	if (!module) continue;
	if (module->comment)
		e_widget_textblock_markup_set(cfdata->o_desc, module->comment);
	else
		e_widget_textblock_markup_set(cfdata->o_desc, "Description: Unavailable.");
	module->selected = 1;
     }
   if (l) evas_list_free(l);
   e_widget_disabled_set(cfdata->b_load, 0);
}

static void 
_loaded_list_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l = NULL;
   int i, c;
   
   cfdata = data;
   if (!cfdata) return;

   /* Loop the hash & unselect all */
   evas_hash_foreach(modules, _modules_hash_cb_unsel, NULL);

   /* Unselect all in avail list & disable buttons */
   e_widget_ilist_unselect(cfdata->o_avail);
   e_widget_disabled_set(cfdata->b_load, 1);
   e_widget_disabled_set(cfdata->b_about, 1);
   e_widget_disabled_set(cfdata->b_config, 1);
   
   /* Make sure something is selected, else disable the buttons */
   c = e_widget_ilist_selected_count_get(cfdata->o_loaded);
   if (c <= 0) 
     {
	e_widget_disabled_set(cfdata->b_unload, 1);
	return;
     }

   for (i = 0, l = e_widget_ilist_items_get(cfdata->o_loaded); l; l = l->next, i++) 
     {
	E_Ilist_Item *item = NULL;
	E_Module *mod = NULL;
	CFModule *module = NULL;
	const char *lbl;
	
	item = l->data;
	if ((!item) || (!item->selected)) continue;
	lbl = e_widget_ilist_nth_label_get(cfdata->o_loaded, i);
	module = evas_hash_find(modules, lbl);
	if (!module) continue;
	module->selected = 1;
	if (c == 1) 
	  {
	     mod = e_module_find(module->short_name);
	     if (mod) 
	       {
		  if (mod->func.about)
		    e_widget_disabled_set(cfdata->b_about, 0);
		  if (mod->func.config)
		    e_widget_disabled_set(cfdata->b_config, 0);
	       }
	  }
     }
   if (l) evas_list_free(l);
   e_widget_disabled_set(cfdata->b_unload, 0);
}

static void 
_btn_cb_unload(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata) return;
   
   evas_hash_foreach(modules, _modules_hash_cb_unload, NULL);

   /* Loop the hash & unselect all */
   evas_hash_foreach(modules, _modules_hash_cb_unsel, NULL);
   e_widget_ilist_unselect(cfdata->o_loaded);
   e_widget_disabled_set(cfdata->b_unload, 1);
   e_widget_disabled_set(cfdata->b_about, 1);
   e_widget_disabled_set(cfdata->b_config, 1);

   _fill_avail_list(cfdata);
   _fill_loaded_list(cfdata);
}

static void 
_btn_cb_load(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata) return;

   evas_hash_foreach(modules, _modules_hash_cb_load, NULL);

   /* Loop the hash & unselect all */
   evas_hash_foreach(modules, _modules_hash_cb_unsel, NULL);
   e_widget_ilist_unselect(cfdata->o_avail);
   e_widget_disabled_set(cfdata->b_load, 1);
   
   _fill_avail_list(cfdata);
   _fill_loaded_list(cfdata);
}

static void 
_btn_cb_about(void *data, void *data2) 
{
   evas_hash_foreach(modules, _modules_hash_cb_about, NULL);
}

static void 
_btn_cb_config(void *data, void *data2) 
{
   evas_hash_foreach(modules, _modules_hash_cb_config, NULL);
}

static int 
_upd_hdl_cb(void *data, int type, void *event) 
{
   E_Event_Module_Update *ev;
   E_Config_Dialog_Data *cfdata;

   if (type != E_EVENT_MODULE_UPDATE) return 1;
   cfdata = data;
   if (!cfdata) return 1;
   ev = event;
   _fill_avail_list(cfdata);
   _fill_loaded_list(cfdata);
   return 1;
}

/* Hash callback Functions */
static Evas_Bool 
_modules_hash_cb_free(Evas_Hash *hash __UNUSED__, const char *key __UNUSED__, 
		      void *data, void *fdata __UNUSED__) 
{
   CFModule *module;
   
   module = data;
   if (!module) return 1;
   if (module->short_name) evas_stringshare_del(module->short_name);
   if (module->name) evas_stringshare_del(module->name);
   if (module->icon) evas_stringshare_del(module->icon);
   if (module->comment) evas_stringshare_del(module->comment);
   if (module->orig_path) evas_stringshare_del(module->orig_path);
   E_FREE(module);
   return 1;
}

static Evas_Bool 
_modules_hash_cb_unsel(Evas_Hash *hash __UNUSED__, const char *key __UNUSED__, 
		       void *data, void *fdata __UNUSED__) 
{
   CFModule *module;
   
   module = data;
   if (!module) return 1;
   module->selected = 0;
   return 1;
}

static Evas_Bool 
_modules_hash_cb_load(Evas_Hash *hash __UNUSED__, const char *key __UNUSED__, 
		      void *data, void *fdata __UNUSED__) 
{
   CFModule *module;
   E_Module *mod;
   
   module = data;
   if ((!module) || (!module->selected)) return 1;
   mod = e_module_find(module->short_name);
   if (!mod) mod = e_module_new(module->short_name);
   if (!mod) return 1;
   module->enabled = e_module_enable(mod);
   return 1;
}

static Evas_Bool 
_modules_hash_cb_unload(Evas_Hash *hash __UNUSED__, const char *key __UNUSED__, 
			void *data, void *fdata __UNUSED__) 
{
   CFModule *module;
   E_Module *mod;
   
   module = data;
   if ((!module) || (!module->selected)) return 1;
   mod = e_module_find(module->short_name);
   if (mod) 
     {
	e_module_disable(mod);
	e_object_del(E_OBJECT(mod));
     }
   module->enabled = 0;
   return 1;
}

static Evas_Bool 
_modules_hash_cb_about(Evas_Hash *hash __UNUSED__, const char *key __UNUSED__, 
		       void *data, void *fdata __UNUSED__) 
{
   CFModule *module = NULL;
   E_Module *mod = NULL;
   
   module = data;
   if ((!module) || (!module->selected)) return 1;
   mod = e_module_find(module->short_name);
   if ((!mod) || (!mod->func.about)) return 1;
   mod->func.about(mod);
   return 1;
}

static Evas_Bool 
_modules_hash_cb_config(Evas_Hash *hash __UNUSED__, const char *key __UNUSED__, 
			void *data, void *fdata __UNUSED__) 
{
   CFModule *module = NULL;
   E_Module *mod = NULL;
   
   module = data;
   if ((!module) || (!module->selected)) return 1;
   mod = e_module_find(module->short_name);
   if ((!mod) || (!mod->func.config)) return 1;
   mod->func.config(mod);
   return 1;
}
