/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Config_Data 
{
   const char *title, *icon, *dialog, *filename;
} E_Config_Data;

struct _E_Config_Dialog_Data 
{
   E_Config_Data *data;
   Evas_Object *o_all, *o_sel;
   Evas_Object *o_add, *o_del;
   Evas_Object *o_up, *o_down;
   Eina_List *apps;
};

/* local protos */
static E_Config_Dialog *_create_dialog(E_Container *con, E_Config_Data *data);
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Eina_List *_load_menu(const char *path);
static Eina_List *_load_order(const char *path);
static void _fill_apps(E_Config_Dialog_Data *cfdata);
static void _fill_list(E_Config_Dialog_Data *cfdata);
static int _cb_sort_desks(Efreet_Desktop *d1, Efreet_Desktop *d2);
static void _all_list_cb_change(void *data, Evas_Object *obj);
static void _sel_list_cb_change(void *data, Evas_Object *obj);
static void _all_list_cb_selected(void *data);
static void _sel_list_cb_selected(void *data);
static void _cb_add(void *data, void *data2);
static void _cb_del(void *data, void *data2);
static void _cb_up(void *data, void *data2);
static void _cb_down(void *data, void *data2);
static int _save_menu(E_Config_Dialog_Data *cfdata);
static int _save_order(E_Config_Dialog_Data *cfdata);

EAPI E_Config_Dialog *
e_int_config_apps_favs(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Data *data;
   char buf[4096];

   snprintf(buf, sizeof(buf), "%s/.e/e/applications/menu/favorite.menu", 
	    e_user_homedir_get());
   data = E_NEW(E_Config_Data, 1);
   data->title = eina_stringshare_add(_("Favorites Menu"));
   data->dialog = eina_stringshare_add("_config_apps_favs_dialog");
   data->icon = eina_stringshare_add("user-bookmarks");
   data->filename = eina_stringshare_add(buf);

   return _create_dialog(con, data);
}

EAPI E_Config_Dialog *
e_int_config_apps_add(E_Container *con, const char *params __UNUSED__) 
{
   E_Desktop_Edit *ed;
   Efreet_Desktop *de = NULL;
   char path[PATH_MAX];
   const char *desktop_dir;

   desktop_dir = e_user_desktop_dir_get();
   if (desktop_dir)
     {
	int i;

	for (i = 1; i < 65536; i++)
	  {
	     snprintf(path, sizeof(path), "%s/_new_app-%i.desktop",
		      desktop_dir, i);
	     if (!ecore_file_exists(path))
	       {
		  de = efreet_desktop_empty_new(path);
		  break;
	       }
	  }
	if (!de)
	  {
	     snprintf(path, sizeof(path), "%s/_rename_me-%i.desktop",
		      desktop_dir, (int)ecore_time_get());
	     de = efreet_desktop_empty_new(NULL);
	  }
     }
   else
     de = efreet_desktop_empty_new(NULL);
   if (!de) return NULL;
   ed = e_desktop_edit(con, de);
   if (!ed) return NULL;
   return ed->cfd;
}

EAPI E_Config_Dialog *
e_int_config_apps_ibar(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Data *data;
   char buf[4096];

   snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/default/.order", 
	    e_user_homedir_get());
   data = E_NEW(E_Config_Data, 1);
   data->title = eina_stringshare_add(_("IBar Applications"));
   data->dialog = eina_stringshare_add("_config_apps_ibar_dialog");
   data->icon = eina_stringshare_add("preferences-applications-ibar");
   data->filename = eina_stringshare_add(buf);

   return _create_dialog(con, data);
}

EAPI E_Config_Dialog *
e_int_config_apps_ibar_other(E_Container *con, const char *path) 
{
   E_Config_Data *data;

   if (!path) return NULL;
   data = E_NEW(E_Config_Data, 1);
   data->title = eina_stringshare_add(_("IBar Applications"));
   data->dialog = eina_stringshare_add("_config_apps_ibar_dialog");
   data->icon = eina_stringshare_add("preferences-applications-ibar");
   data->filename = eina_stringshare_add(path);

   return _create_dialog(con, data);
}

EAPI E_Config_Dialog *
e_int_config_apps_startup(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Data *data;
   char buf[4096];

   snprintf(buf, sizeof(buf), "%s/.e/e/applications/startup/.order", 
	    e_user_homedir_get());
   data = E_NEW(E_Config_Data, 1);
   data->title = eina_stringshare_add(_("Startup Applications"));
   data->dialog = eina_stringshare_add("_config_apps_startup_dialog");
   data->icon = eina_stringshare_add("preferences-applications-startup");
   data->filename = eina_stringshare_add(buf);

   return _create_dialog(con, data);
}

EAPI E_Config_Dialog *
e_int_config_apps_restart(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Data *data;
   char buf[4096];

   snprintf(buf, sizeof(buf), "%s/.e/e/applications/restart/.order", 
	    e_user_homedir_get());
   data = E_NEW(E_Config_Data, 1);
   data->title = eina_stringshare_add(_("Restart Applications"));
   data->dialog = eina_stringshare_add("_config_apps_restart_dialog");
   data->icon = eina_stringshare_add("preferences-applications-restart");
   data->filename = eina_stringshare_add(buf);

   return _create_dialog(con, data);
}

/* local functions */
static E_Config_Dialog *
_create_dialog(E_Container *con, E_Config_Data *data) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", data->dialog)) 
     {
	if (data->title) eina_stringshare_del(data->title);
	if (data->dialog) eina_stringshare_del(data->dialog);
	if (data->icon) eina_stringshare_del(data->icon);
	if (data->filename) eina_stringshare_del(data->filename);
	E_FREE(data);
	return NULL;
     }

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   cfd = e_config_dialog_new(con, data->title, "E", data->dialog, 
			     data->icon, 0, v, data);
   e_dialog_resizable_set(cfd->dia, 1);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Data *data;
   const char *ext;

   if (!(data = cfd->data)) return NULL;
   if (!data->filename) return NULL;

   ext = strrchr(data->filename, '.');
   if (!ext) return NULL;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->data = data;
   if (!strcmp(ext, ".menu"))
     cfdata->apps = _load_menu(data->filename);
   else if (!strcmp(ext, ".order"))
     cfdata->apps = _load_order(data->filename);

   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_Config_Data *data;
   Efreet_Desktop *desktop;

   data = cfdata->data;
   if (data) 
     {
	if (data->title) eina_stringshare_del(data->title);
	if (data->dialog) eina_stringshare_del(data->dialog);
	if (data->icon) eina_stringshare_del(data->icon);
	if (data->filename) eina_stringshare_del(data->filename);
	E_FREE(data);
     }
   EINA_LIST_FREE(cfdata->apps, desktop)
     efreet_desktop_free(desktop);
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *ot, *ow, *bt;

   o = e_widget_list_add(evas, 0, 1);
   ot = e_widget_frametable_add(evas, _("All Applications"), 0);
   ow = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_multi_select_set(ow, 1);
   e_widget_on_change_hook_set(ow, _all_list_cb_change, cfdata);
   cfdata->o_all = ow;
   _fill_apps(cfdata);
   e_widget_frametable_object_append(ot, ow, 0, 0, 1, 1, 1, 1, 1, 1);
   ow = e_widget_button_add(evas, _("Add"), "list-add", _cb_add, 
			    cfdata, NULL);
   cfdata->o_add = ow;
   e_widget_disabled_set(ow, 1);
   e_widget_frametable_object_append(ot, ow, 0, 1, 1, 1, 1, 0, 1, 0);
   e_widget_list_object_append(o, ot, 1, 1, 0.5);

   ot = e_widget_frametable_add(evas, _("Selected Applications"), 0);
   ow = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_multi_select_set(ow, 1);
   e_widget_on_change_hook_set(ow, _sel_list_cb_change, cfdata);
   cfdata->o_sel = ow;
   _fill_list(cfdata);
   e_widget_frametable_object_append(ot, ow, 0, 0, 1, 1, 1, 1, 1, 1);
   bt = e_widget_table_add(evas, 0);
   ow = e_widget_button_add(evas, _("Up"), "go-up", _cb_up, 
			    cfdata, NULL);
   cfdata->o_up = ow;
   e_widget_disabled_set(ow, 1);
   e_widget_table_object_append(bt, ow, 0, 0, 1, 1, 1, 0, 1, 0);
   ow = e_widget_button_add(evas, _("Down"), "go-down", _cb_down, 
			    cfdata, NULL);
   cfdata->o_down = ow;
   e_widget_disabled_set(ow, 1);
   e_widget_table_object_append(bt, ow, 1, 0, 1, 1, 1, 0, 1, 0);
   e_widget_frametable_object_append(ot, bt, 0, 1, 1, 1, 1, 0, 1, 0);
   ow = e_widget_button_add(evas, _("Delete"), "list-remove", _cb_del, 
			    cfdata, NULL);
   cfdata->o_del = ow;
   e_widget_disabled_set(ow, 1);
   e_widget_frametable_object_append(ot, ow, 0, 2, 1, 1, 1, 0, 1, 0);
   e_widget_list_object_append(o, ot, 1, 1, 0.5);

   return o;
}

static int 
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   int ret = 0;
   char *ext;

   if ((!cfdata->data) || (!cfdata->data->filename)) return 0;
   ext = strrchr(cfdata->data->filename, '.');
   if (!ext) return 0;
   if (!strcmp(ext, ".menu"))
     ret = _save_menu(cfdata);
   else if (!strcmp(ext, ".order"))
     ret = _save_order(cfdata);

   return ret;
}

static Eina_List *
_load_menu(const char *path) 
{
   Efreet_Menu *menu, *entry;
   Eina_List *apps = NULL;
   Eina_List *l;

   apps = NULL;
   menu = efreet_menu_parse(path);
   if ((!menu) || (!menu->entries)) return apps;
   EINA_LIST_FOREACH(menu->entries, l, entry)
     {
	if (entry->type != EFREET_MENU_ENTRY_DESKTOP) continue;
	efreet_desktop_ref(entry->desktop);
	apps = eina_list_append(apps, entry->desktop);
     }
   efreet_menu_free(menu);
   return apps;
}

static Eina_List *
_load_order(const char *path) 
{
   E_Order *order = NULL;
   Eina_List *l = NULL;
   Eina_List *apps = NULL;

   apps = NULL;
   if (!path) return apps;
   order = e_order_new(path);
   if (!order) return apps;
   for (l = order->desktops; l; l = l->next) 
     {
	efreet_desktop_ref(l->data);
	apps = eina_list_append(apps, l->data);
     }
   /* FIXME: The test below will never be true ! */
   if (l) eina_list_free(l);
   e_object_del(E_OBJECT(order));
   return apps;
}

static void 
_fill_apps(E_Config_Dialog_Data *cfdata) 
{
   Eina_List *desks = NULL, *l = NULL;
   Efreet_Desktop *desk = NULL;
   Evas *evas;
   int w;

   l = NULL;

   evas = evas_object_evas_get(cfdata->o_all);
   desks = efreet_util_desktop_name_glob_list("*");

   desks = eina_list_sort(desks, 0, _cb_sort_desks);
   EINA_LIST_FREE(desks, desk)
	  {
	if (!eina_list_search_unsorted(l, _cb_sort_desks, desk))
	       {
		  efreet_desktop_ref(desk);
	     l = eina_list_append(l, desk);
	  }
	efreet_desktop_free(desk);
     }

   l = eina_list_sort(l, 0, _cb_sort_desks);

   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_all);
   e_widget_ilist_clear(cfdata->o_all);

   EINA_LIST_FREE(l, desk)
	  {
	     Evas_Object *icon = NULL;

	     icon = e_util_desktop_icon_add(desk, 24, evas);
	     e_widget_ilist_append(cfdata->o_all, icon, desk->name, 
				   _all_list_cb_selected, cfdata, desk->orig_path);
	efreet_desktop_free(desk);
     }

   e_widget_ilist_go(cfdata->o_all);
   e_widget_ilist_thaw(cfdata->o_all);
   edje_thaw();
   evas_event_thaw(evas);
   e_widget_min_size_get(cfdata->o_all, &w, NULL);
   e_widget_min_size_set(cfdata->o_all, w, 240);
}

static void 
_fill_list(E_Config_Dialog_Data *cfdata) 
{
   Efreet_Desktop *desk = NULL;
   Evas *evas;
   int w;

   if (!cfdata->apps) return;
   evas = evas_object_evas_get(cfdata->o_sel);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_sel);
   e_widget_ilist_clear(cfdata->o_sel);

   EINA_LIST_FREE(cfdata->apps, desk)
	  {
	     Evas_Object *icon = NULL;

	     icon = e_util_desktop_icon_add(desk, 24, evas);
	     e_widget_ilist_append(cfdata->o_sel, icon, desk->name, 
				   _sel_list_cb_selected, cfdata, desk->orig_path);
	  }
   
   cfdata->apps = NULL;
   e_widget_ilist_go(cfdata->o_sel);
   e_widget_min_size_get(cfdata->o_sel, &w, NULL);
   e_widget_min_size_set(cfdata->o_sel, w, 240);
   e_widget_ilist_thaw(cfdata->o_sel);
   edje_thaw();
   evas_event_thaw(evas);
   e_widget_disabled_set(cfdata->o_del, 1);
}

static int 
_cb_sort_desks(Efreet_Desktop *d1, Efreet_Desktop *d2) 
{
   if (!d1->name) return 1;
   if (!d2->name) return -1;
   return strcmp(d1->name, d2->name);
}

static void 
_all_list_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata = NULL;

   if (!(cfdata = data)) return;

   /* unselect anything in Sel List & disable buttons */
   e_widget_ilist_unselect(cfdata->o_sel);
   e_widget_disabled_set(cfdata->o_up, 1);
   e_widget_disabled_set(cfdata->o_down, 1);
   e_widget_disabled_set(cfdata->o_del, 1);
}

static void 
_sel_list_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata = NULL;

   if (!(cfdata = data)) return;

   /* unselect anything in All List & disable buttons */
   e_widget_ilist_unselect(cfdata->o_all);
   e_widget_disabled_set(cfdata->o_add, 1);
}

static void 
_all_list_cb_selected(void *data) 
{
   E_Config_Dialog_Data *cfdata = NULL;
   int sel;

   if (!(cfdata = data)) return;
   sel = e_widget_ilist_selected_count_get(cfdata->o_all);
   if (sel == 0)
     e_widget_disabled_set(cfdata->o_add, 1);
   else
     e_widget_disabled_set(cfdata->o_add, 0);
}

static void 
_sel_list_cb_selected(void *data) 
{
   E_Config_Dialog_Data *cfdata = NULL;
   int sel, count;

   if (!(cfdata = data)) return;
   e_widget_disabled_set(cfdata->o_del, 0);
   sel = e_widget_ilist_selected_get(cfdata->o_sel);
   count = e_widget_ilist_count(cfdata->o_sel);
   if (sel == 0)
     e_widget_disabled_set(cfdata->o_up, 1);
   else
     e_widget_disabled_set(cfdata->o_up, 0);
   if (sel < (count - 1))
     e_widget_disabled_set(cfdata->o_down, 0);
   else
     e_widget_disabled_set(cfdata->o_down, 1);
}

static void 
_cb_add(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata = NULL;
   Eina_List *l = NULL;
   Evas *evas;
   int w, i;

   if (!(cfdata = data)) return;
   evas = evas_object_evas_get(cfdata->o_all);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_all);

   for (i = 0, l = e_widget_ilist_items_get(cfdata->o_all); l; l = l->next, i++) 
     {
	E_Ilist_Item *item = NULL;
	Efreet_Desktop *desk = NULL;
	Evas_Object *icon = NULL;
	const char *lbl;

	if (!(item = l->data)) continue;
	if (!item->selected) continue;
	lbl = e_widget_ilist_nth_label_get(cfdata->o_all, i);
	if (!lbl) continue;
	desk = efreet_util_desktop_name_find(lbl);
	if (!desk) continue;
	icon = e_util_desktop_icon_add(desk, 24, evas);
	e_widget_ilist_append(cfdata->o_sel, icon, desk->name, 
			      _sel_list_cb_selected, cfdata, desk->orig_path);
     }

   e_widget_ilist_go(cfdata->o_sel);
   e_widget_min_size_get(cfdata->o_sel, &w, NULL);
   e_widget_min_size_set(cfdata->o_sel, w, 240);
   e_widget_ilist_thaw(cfdata->o_sel);
   e_widget_ilist_unselect(cfdata->o_all);
   edje_thaw();
   evas_event_thaw(evas);
}

static void 
_cb_del(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata = NULL;
   Eina_List *l = NULL;
   Evas *evas;
   int w;

   if (!(cfdata = data)) return;
   evas = evas_object_evas_get(cfdata->o_sel);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_sel);
   for (l = e_widget_ilist_items_get(cfdata->o_sel); l; l = l->next) 
     {
	int i = 0;

	i = e_widget_ilist_selected_get(cfdata->o_sel);
	if (i == -1) break;
	e_widget_ilist_remove_num(cfdata->o_sel, i);
     }
   e_widget_ilist_unselect(cfdata->o_sel);
   e_widget_ilist_go(cfdata->o_sel);
   e_widget_min_size_get(cfdata->o_sel, &w, NULL);
   e_widget_min_size_set(cfdata->o_sel, w, 240);
   e_widget_ilist_thaw(cfdata->o_sel);
   edje_thaw();
   evas_event_thaw(evas);
}

static void 
_cb_up(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata = NULL;
   Evas_Object *icon = NULL;
   Efreet_Desktop *desk = NULL;
   Evas *evas;
   const char *lbl;
   int sel, w;

   if (!(cfdata = data)) return;
   if (e_widget_ilist_selected_count_get(cfdata->o_sel) > 1) return;
   evas = evas_object_evas_get(cfdata->o_sel);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_sel);
   sel = e_widget_ilist_selected_get(cfdata->o_sel);
   lbl = e_widget_ilist_nth_label_get(cfdata->o_sel, sel);
   if (lbl) 
     {
	desk = efreet_util_desktop_name_find(lbl);
	if (desk) 
	  {
	     e_widget_ilist_remove_num(cfdata->o_sel, sel);
	     e_widget_ilist_go(cfdata->o_sel);
	     icon = e_util_desktop_icon_add(desk, 24, evas);
	     e_widget_ilist_prepend_relative(cfdata->o_sel, icon, desk->name, 
					     _sel_list_cb_selected, cfdata, 
					     desk->orig_path, (sel - 1));
	     e_widget_ilist_selected_set(cfdata->o_sel, (sel - 1));
	  }
     }
   e_widget_ilist_go(cfdata->o_sel);
   e_widget_min_size_get(cfdata->o_sel, &w, NULL);
   e_widget_min_size_set(cfdata->o_sel, w, 240);
   e_widget_ilist_thaw(cfdata->o_sel);
   edje_thaw();
   evas_event_thaw(evas);
}

static void 
_cb_down(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata = NULL;
   Evas_Object *icon = NULL;
   Efreet_Desktop *desk = NULL;
   Evas *evas;
   const char *lbl;
   int sel, w;

   if (!(cfdata = data)) return;
   if (e_widget_ilist_selected_count_get(cfdata->o_sel) > 1) return;
   evas = evas_object_evas_get(cfdata->o_sel);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_sel);
   sel = e_widget_ilist_selected_get(cfdata->o_sel);
   lbl = e_widget_ilist_nth_label_get(cfdata->o_sel, sel);
   if (lbl) 
     {
	desk = efreet_util_desktop_name_find(lbl);
	if (desk) 
	  {
	     e_widget_ilist_remove_num(cfdata->o_sel, sel);
	     e_widget_ilist_go(cfdata->o_sel);
	     icon = e_util_desktop_icon_add(desk, 24, evas);
	     e_widget_ilist_append_relative(cfdata->o_sel, icon, desk->name, 
					     _sel_list_cb_selected, cfdata, 
					     desk->orig_path, sel);
	     e_widget_ilist_selected_set(cfdata->o_sel, (sel + 1));
	  }
     }
   e_widget_ilist_go(cfdata->o_sel);
   e_widget_min_size_get(cfdata->o_sel, &w, NULL);
   e_widget_min_size_set(cfdata->o_sel, w, 240);
   e_widget_ilist_thaw(cfdata->o_sel);
   edje_thaw();
   evas_event_thaw(evas);
}

static int 
_save_menu(E_Config_Dialog_Data *cfdata) 
{
   Eina_List *l = NULL;
   Efreet_Menu *menu = NULL;
   int i, ret;

   menu = efreet_menu_new("Favorites");
   for (i = 0, l = e_widget_ilist_items_get(cfdata->o_sel); l; l = l->next, i++) 
     {
	E_Ilist_Item *item = NULL;
	Efreet_Desktop *desk = NULL;
	const char *lbl;

	if (!(item = l->data)) continue;
	lbl = e_widget_ilist_nth_label_get(cfdata->o_sel, i);
	if (!lbl) continue;
	desk = efreet_util_desktop_name_find(lbl);
	if (!desk) continue;
	efreet_menu_desktop_insert(menu, desk, -1);
     }
   ret = efreet_menu_save(menu, cfdata->data->filename);
   efreet_menu_free(menu);
   return ret;
}

static int 
_save_order(E_Config_Dialog_Data *cfdata) 
{
   Eina_List *l = NULL;
   E_Order *order = NULL;
   int i;

   order = e_order_new(cfdata->data->filename);
   if (!order) return 0;
   e_order_clear(order);
   for (i = 0, l = e_widget_ilist_items_get(cfdata->o_sel); l; l = l->next, i++) 
     {
	E_Ilist_Item *item = NULL;
	Efreet_Desktop *desk = NULL;
	const char *lbl;

	if (!(item = l->data)) continue;
	lbl = e_widget_ilist_nth_label_get(cfdata->o_sel, i);
	if (!lbl) continue;
	desk = efreet_util_desktop_name_find(lbl);
	if (!desk) continue;
	e_order_append(order, desk);
     }
   e_object_del(E_OBJECT(order));
   return 1;
}
