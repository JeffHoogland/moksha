#include "e.h"
#include <assert.h>

typedef struct _CFModule CFModule;
typedef struct _CFType CFType;
typedef struct _CFTypes CFTypes;

struct _CFModule
{
   const char *short_name, *name, *comment;
   const char *icon, *orig_path;
   E_Module *module;
   Evas_Object *end;
   int idx;
   Eina_Bool enabled : 1;
   Eina_Bool selected : 1;
};

struct _CFType
{
   const char *key, *name, *icon;
   Eina_Hash *modules_hash; /* just used before constructing list */
   Eina_List *modules; /* sorted and ready to be used */
};

struct _E_Config_Dialog_Data
{
   Evas *evas;
   Evas_Object *l_modules;
   Evas_Object *b_load, *b_unload;
   Evas_Object *o_desc;
   Eina_List *types;
   struct 
     {
        Eina_List *loaded, *unloaded;
        Ecore_Idler *idler;
     } selected;

   /* just exists while loading: */
   Eina_List *modules_paths;
   Eina_List *modules_path_current;
   Eina_Hash *types_hash;
   Ecore_Timer *data_delay;
   Ecore_Idler *data_loader;
   Eina_Bool did_modules_list;
};

struct _CFTypes
{
   size_t key_len;
   const char *key, *name, *icon;
};

/* pre defined types (used to specify icon and i18n name) */
static const CFTypes _types[] =
{
#define _CFT(k, n, i)				\
  {sizeof(k) - 1, k, n, i}
  _CFT("appearance", N_("Appearance"), "preferences-appearance"),
  _CFT("config", N_("Settings"), "preferences-system"),
  _CFT("fileman", N_("File Manager"), "system-file-manager"),
  _CFT("shelf", N_("Shelf"), "preferences-desktop-shelf"),
  _CFT("system", N_("System"), "system"),
  _CFT("everything", N_("Everything Launcher"), "system-run"),
#undef _CFT
  {0, NULL, NULL, NULL}
};

/* local function protos */
static Eina_Bool _fill_data(E_Config_Dialog_Data *cfdata);
static void _cftype_free(CFType *cft);

static void _widget_list_populate(E_Config_Dialog_Data *cfdata);
static void _widget_list_selection_changed(void *data, Evas_Object *obj);

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static void _btn_cb_unload(void *data, void *data2);
static void _btn_cb_load(void *data, void *data2);

EAPI E_Config_Dialog *
e_int_config_modules(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd = NULL;
   E_Config_Dialog_View *v = NULL;

   if (e_config_dialog_find("E", "_config_modules_dialog")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;

   cfd = e_config_dialog_new(con, _("Module Settings"), 
			     "E", "_config_modules_dialog", 
			     "preferences-plugin", 0, v, NULL);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = E_NEW(E_Config_Dialog_Data, 1);

   if (!cfdata) return NULL;
   if (!_fill_data(cfdata))
     {
	E_FREE(cfdata);
	return NULL;
     }

   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   CFType *cft;

   if (cfdata->data_delay)
     ecore_timer_del(cfdata->data_delay);
   else if (cfdata->data_loader)
     ecore_idler_del(cfdata->data_loader);

   if (cfdata->modules_paths)
     e_path_dir_list_free(cfdata->modules_paths);
   if (cfdata->types_hash)
     eina_hash_free(cfdata->types_hash);

   EINA_LIST_FREE(cfdata->types, cft) _cftype_free(cft);

   eina_list_free(cfdata->selected.loaded);
   eina_list_free(cfdata->selected.unloaded);
   if (cfdata->selected.idler) ecore_idler_del(cfdata->selected.idler);
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *of, *ol;
   Evas_Coord w;

   cfdata->evas = e_win_evas_get(cfd->dia->win);

   of = e_widget_table_add(evas, 0);
   ol = e_widget_ilist_add(evas, 24, 24, NULL);
   cfdata->l_modules = ol;

   e_widget_size_min_get(cfdata->l_modules, &w, NULL);
   if (w < 200 * e_scale) w = (200 * e_scale);
   e_widget_size_min_set(cfdata->l_modules, w, (100 * e_scale));

   e_widget_ilist_multi_select_set(ol, EINA_TRUE);
   e_widget_on_change_hook_set(ol, _widget_list_selection_changed, cfdata);
   e_widget_table_object_append(of, ol, 0, 0, 2, 1, 1, 1, 1, 1);

   ol = e_widget_button_add(evas, _("Load"), NULL, _btn_cb_load, cfdata, NULL);
   cfdata->b_load = ol;
   e_widget_disabled_set(ol, 1);
   e_widget_table_object_append(of, ol, 0, 1, 1, 1, 1, 1, 1, 0);

   ol = e_widget_button_add(evas, _("Unload"), NULL, _btn_cb_unload, cfdata, NULL);
   cfdata->b_unload = ol;
   e_widget_disabled_set(ol, 1);
   e_widget_table_object_append(of, ol, 1, 1, 1, 1, 1, 1, 1, 0);

   ol = e_widget_textblock_add(evas);
   e_widget_size_min_set(ol, (200 * e_scale), 60 * e_scale);
   cfdata->o_desc = ol;
   e_widget_textblock_markup_set(ol, _("No modules selected."));
   e_widget_table_object_append(of, ol, 0, 2, 2, 1, 1, 0, 1, 0);

   e_dialog_resizable_set(cfd->dia, 1);
   e_util_win_auto_resize_fill(cfd->dia->win);
   e_win_centered_set(cfd->dia->win, 1);

   return of;
}

static CFModule *
_module_new(const char *short_name, const Efreet_Desktop *desk)
{
   CFModule *cfm = E_NEW(CFModule, 1);

   if (!cfm) return NULL;
   cfm->short_name = eina_stringshare_add(short_name);

   if (desk->name)
     cfm->name = eina_stringshare_add(desk->name);
   else
     cfm->name = eina_stringshare_ref(cfm->short_name);

   cfm->icon = eina_stringshare_add(desk->icon);
   cfm->comment = eina_stringshare_add(desk->comment);
   cfm->orig_path = eina_stringshare_add(desk->orig_path);
   return cfm;
}

static void
_module_free(CFModule *cfm)
{
   eina_stringshare_del(cfm->short_name);
   eina_stringshare_del(cfm->name);
   eina_stringshare_del(cfm->icon);
   eina_stringshare_del(cfm->comment);
   eina_stringshare_del(cfm->orig_path);
   E_FREE(cfm);
}

static void
_module_end_state_apply(CFModule *cfm)
{
   const char *sig;

   if (!cfm->end) return;
   sig = cfm->enabled ? "e,state,checked" : "e,state,unchecked";
   edje_object_signal_emit(cfm->end, sig, "e");
}

static CFType *
_cftype_new(const char *key, const char *name, const char *icon)
{
   CFType *cft = E_NEW(CFType, 1);

   if (!cft) return NULL;
   cft->key = eina_stringshare_add(key);
   cft->name = eina_stringshare_add(name);
   cft->icon = eina_stringshare_add(icon);
   return cft;
}

static void
_cftype_free(CFType *cft)
{
   CFModule *cfm;

   assert(cft->modules_hash == NULL); // must do it before calling this function
   EINA_LIST_FREE(cft->modules, cfm)
     _module_free(cfm);

   eina_stringshare_del(cft->key);
   eina_stringshare_del(cft->name);
   eina_stringshare_del(cft->icon);
   E_FREE(cft);
}

static CFType *
_cftype_new_from_key(const char *key)
{
   const CFTypes *itr;
   char name[1024], icon[1024];
   size_t key_len = strlen(key);

   for (itr = _types; itr->key_len > 0; itr++)
     {
	if (key_len != itr->key_len) continue;
	if (strcmp(itr->key, key) != 0) continue;
	return _cftype_new(itr->key, itr->name, itr->icon);
     }

   if ((key_len + 1) >= sizeof(name)) return NULL;
   if ((key_len + sizeof("enlightenment/")) >= sizeof(icon)) return NULL;

   memcpy(name, key, key_len + 1);
   name[0] = toupper(name[0]);

   memcpy(icon, "enlightenment/", sizeof("enlightenment/") - 1);
   memcpy(icon + sizeof("enlightenment/") - 1, key, key_len + 1);

   return _cftype_new(key, name, icon);
}

static void
_load_modules(const char *dir, Eina_Hash *types_hash)
{
   Eina_List *files;
   char modpath[PATH_MAX];
   char *mod;
   int modpathlen;

   modpathlen = snprintf(modpath, sizeof(modpath), "%s/", dir);
   if (modpathlen >= (int)sizeof(modpath)) return;

   files = ecore_file_ls(dir);
   EINA_LIST_FREE(files, mod)
     {
	Efreet_Desktop *desk;
	CFType *cft;
	CFModule *cfm;
	const char *type;
	Eina_Bool new_type;

	snprintf(modpath + modpathlen, sizeof(modpath) - modpathlen,
		 "%s/module.desktop", mod);
	if (!ecore_file_exists(modpath)) goto end_mod;
	if (!(desk = efreet_desktop_new(modpath))) goto end_mod;

	if (desk->x)
	  type = eina_hash_find(desk->x, "X-Enlightenment-ModuleType");
	else
	  type = NULL;
	if (!type) type = "shelf"; // todo: warn?

	cft = eina_hash_find(types_hash, type);
	if (cft)
	  {
	     new_type = EINA_FALSE;
	     if ((cft->modules_hash) &&
		 (eina_hash_find(cft->modules_hash, mod)))
	       goto end_desktop;
	  }
	else
	  {
	     cft = _cftype_new_from_key(type);
	     if (cft) new_type = EINA_TRUE;
	     else goto end_desktop;
	  }

	cfm = _module_new(mod, desk);
	if (!cfm)
	  {
	     if (new_type) _cftype_free(cft);
	     goto end_desktop;
	  }

	if (!cft->modules_hash)
	  cft->modules_hash = eina_hash_string_superfast_new(NULL);
	if (!cft->modules_hash)
	  {
	     if (new_type) _cftype_free(cft);
	     goto end_desktop;
	  }
	eina_hash_direct_add(cft->modules_hash, cfm->short_name, cfm);
	// TODO be paranoid about hash add failure, otherwise it will leak

	cft->modules = eina_list_append(cft->modules, cfm);
	// TODO be paranoid about list append failure, otherwise it will leak
	cfm->module = e_module_find(mod);
	if (cfm->module)
	  cfm->enabled = e_module_enabled_get(cfm->module);
	else
	  cfm->enabled = 0;

	if (new_type)
	  eina_hash_direct_add(types_hash, cft->key, cft);
	// TODO be paranoid about hash add failure, otherwise it will leak

     end_desktop:
	efreet_desktop_free(desk);
     end_mod:
	free(mod);
     }
}

static int
_modules_list_sort(const void *data1, const void *data2)
{
   const CFModule *m1 = data1, *m2 = data2;
   return strcmp(m1->name, m2->name);
}

static Eina_Bool
_types_list_create_foreach_cb(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata)
{
   E_Config_Dialog_Data *cfdata = fdata;
   CFType *cft = data;

   // otherwise it should not be here
   assert(cft->modules);
   assert(cft->modules_hash);

   eina_hash_free(cft->modules_hash);
   cft->modules_hash = NULL;

   cft->modules = eina_list_sort(cft->modules, -1, _modules_list_sort);
   cfdata->types = eina_list_append(cfdata->types, cft);
   // TODO be paranoid about list append failure, otherwise leaks memory
   return EINA_TRUE;
}

static int
_types_list_sort(const void *data1, const void *data2)
{
   const CFType *t1 = data1, *t2 = data2;
   return strcmp(t1->name, t2->name);
}

static Eina_Bool
_fill_data_loader_iterate(void *data)
{
   E_Config_Dialog_Data *cfdata = data;

   if (cfdata->modules_path_current)
     {
	E_Path_Dir *epd = cfdata->modules_path_current->data;
	cfdata->modules_path_current = cfdata->modules_path_current->next;

	if (ecore_file_is_dir(epd->dir))
	  _load_modules(epd->dir, cfdata->types_hash);
     }
   else if (!cfdata->did_modules_list)
     {
	cfdata->did_modules_list = EINA_TRUE;

	e_path_dir_list_free(cfdata->modules_paths);
	cfdata->modules_paths = NULL;

	eina_hash_foreach(cfdata->types_hash, 
                          _types_list_create_foreach_cb, cfdata);
	eina_hash_free(cfdata->types_hash);
	cfdata->types_hash = NULL;
	cfdata->types = eina_list_sort(cfdata->types, -1, _types_list_sort);
     }
   else
     {
	_widget_list_populate(cfdata);
	cfdata->data_loader = NULL;
	return ECORE_CALLBACK_CANCEL;
     }

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_fill_data_delayed(void *data)
{
   E_Config_Dialog_Data *cfdata = data;

   cfdata->data_loader = ecore_idler_add(_fill_data_loader_iterate, cfdata);
   cfdata->data_delay = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->types_hash = eina_hash_string_superfast_new(NULL);
   if (!cfdata->types_hash) return EINA_FALSE;

   cfdata->modules_paths = e_path_dir_list_get(path_modules);
   if (!cfdata->modules_paths)
     {
	eina_hash_free(cfdata->types_hash);
	cfdata->types_hash = NULL;
	return EINA_FALSE;
     }
   cfdata->modules_path_current = cfdata->modules_paths;
   cfdata->data_delay = ecore_timer_add(0.2, _fill_data_delayed, cfdata);
   return EINA_TRUE;
}

static void
_list_header_append(E_Config_Dialog_Data *cfdata, CFType *cft)
{
   Evas_Object *icon = e_icon_add(cfdata->evas);

   if (icon)
     {
	if (!e_util_icon_theme_set(icon, cft->icon))
	  {
	     evas_object_del(icon);
	     icon = NULL;
	  }
     }
   e_widget_ilist_header_append(cfdata->l_modules, icon, cft->name);
}

static void
_list_item_append(E_Config_Dialog_Data *cfdata, CFModule *cfm)
{
   Evas_Object *icon, *end;

   if (!cfm->icon)
     icon = NULL;
   else
     {
	icon = e_icon_add(cfdata->evas);
	if (icon)
	  {
	     if (!e_util_icon_theme_set(icon, cfm->icon))
	       {
		  if (cfm->orig_path)
		    {
		       char *dir = ecore_file_dir_get(cfm->orig_path);
		       char buf[PATH_MAX];

		       snprintf(buf, sizeof(buf), "%s/%s.edj", dir, cfm->icon);
		       free(dir);

		       e_icon_file_edje_set(icon, buf, "icon");
		    }
		  else
		    {
		       evas_object_del(icon);
		       icon = NULL;
		    }
	       }
	  }
     }

   end = edje_object_add(cfdata->evas);
   if (end)
     {
	if (e_theme_edje_object_set(end, "base/theme/widgets",
				     "e/widgets/ilist/toggle_end"))
	  {
	     cfm->end = end;
	     _module_end_state_apply(cfm);
	  }
	else
	  {
	     EINA_LOG_ERR("your theme is missing 'e/widgets/ilist/toggle_end'!");
	     evas_object_del(end);
	     end = NULL;
	  }
     }

   e_widget_ilist_append_full(cfdata->l_modules, icon, end, 
                              cfm->name, NULL, cfm, NULL);
}

static void
_widget_list_populate(E_Config_Dialog_Data *cfdata)
{
   CFType *cft;
   Eina_List *l_type;
   int idx = 0;

   // TODO postpone list fill to idler?

   evas_event_freeze(cfdata->evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->l_modules);
   e_widget_ilist_clear(cfdata->l_modules);

   EINA_LIST_FOREACH(cfdata->types, l_type, cft)
     {
	CFModule *cfm;
	Eina_List *l_module;

	_list_header_append(cfdata, cft);
	idx++;

	EINA_LIST_FOREACH(cft->modules, l_module, cfm)
	  {
	     _list_item_append(cfdata, cfm);
	     cfm->idx = idx;
	     idx++;
	  }
     }

   e_widget_ilist_go(cfdata->l_modules);
   e_widget_ilist_thaw(cfdata->l_modules);
   edje_thaw();
   evas_event_thaw(cfdata->evas);
}

static Eina_Bool
_widget_list_item_selected_postponed(void *data)
{
   E_Config_Dialog_Data *cfdata = data;
   const Eina_List *l;
   const E_Ilist_Item *it;
   unsigned int loaded = 0, unloaded = 0;
   CFModule *cfm = NULL;
   const char *description;

   eina_list_free(cfdata->selected.loaded);
   eina_list_free(cfdata->selected.unloaded);
   cfdata->selected.loaded = NULL;
   cfdata->selected.unloaded = NULL;

   EINA_LIST_FOREACH(e_widget_ilist_items_get(cfdata->l_modules), l, it)
     {
	if ((!it->selected) || (it->header)) continue;
	cfm = e_widget_ilist_item_data_get(it);

	if (cfm->enabled)
	  {
	     cfdata->selected.loaded = 
               eina_list_append(cfdata->selected.loaded, cfm);
	     loaded++;
	  }
	else
	  {
	     cfdata->selected.unloaded = 
               eina_list_append(cfdata->selected.unloaded, cfm);
	     unloaded++;
	  }
     }

   e_widget_disabled_set(cfdata->b_load, !unloaded);
   e_widget_disabled_set(cfdata->b_unload, !loaded);

   if ((cfm) && (loaded + unloaded == 1))
     description = cfm->comment;
   else if (loaded + unloaded > 1)
     description = _("More than one module selected.");
   else
     description = _("No modules selected.");

   e_widget_textblock_markup_set(cfdata->o_desc, description);

   cfdata->selected.idler = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_widget_list_selection_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;

   if (cfdata->selected.idler)
     ecore_idler_del(cfdata->selected.idler);
   cfdata->selected.idler = 
     ecore_idler_add(_widget_list_item_selected_postponed, cfdata);
}

static void
_btn_cb_unload(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   CFModule *cfm;

   EINA_LIST_FREE(cfdata->selected.loaded, cfm)
     {
	if (!cfm->module)
	  cfm->module = e_module_find(cfm->short_name);

	if (cfm->module)
	  {
	     e_module_disable(cfm->module);
	     cfm->enabled = e_module_enabled_get(cfm->module);
	  }

	// weird, but unselects it as it was already selected
	e_widget_ilist_multi_select(cfdata->l_modules, cfm->idx);
	_module_end_state_apply(cfm);
     }

   e_widget_disabled_set(cfdata->b_unload, 1);
}

static void
_btn_cb_load(void *data, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   CFModule *cfm;

   EINA_LIST_FREE(cfdata->selected.unloaded, cfm)
     {
	if (!cfm->module)
	  cfm->module = e_module_find(cfm->short_name);
	if (!cfm->module)
	  cfm->module = e_module_new(cfm->short_name);

	if (cfm->module)
	  {
	     e_module_enable(cfm->module);
	     cfm->enabled = e_module_enabled_get(cfm->module);
	  }

	// weird, but unselects it as it was already selected
	e_widget_ilist_multi_select(cfdata->l_modules, cfm->idx);
	_module_end_state_apply(cfm);
     }

   e_widget_disabled_set(cfdata->b_load, 1);
}
