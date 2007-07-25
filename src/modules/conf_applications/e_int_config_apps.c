/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* TODO: This should be modified to handle any other fdo menu editing. */

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _fill_categories(E_Config_Dialog_Data *cfdata);
static void _fill_apps(E_Config_Dialog_Data *cfdata);
static void _fill_list(E_Config_Dialog_Data *cfdata);
static void _category_cb_selected(void *data);
static void _apps_cb_selected(void *data);
static void _list_cb_selected(void *data);
static void _cb_categories(void *data, void *data2);
static void _cb_add(void *data, void *data2);
static void _cb_del(void *data, void *data2);
static void _cb_up(void *data, void *data2);
static void _cb_down(void *data, void *data2);
static int _cb_desktop_name_sort(Efreet_Desktop *a, Efreet_Desktop *b);

static int _save_menu(E_Config_Dialog_Data *cfdata);
static int _save_order(E_Config_Dialog_Data *cfdata);
static Ecore_List *_load_menu(const char *path);
static Ecore_List *_load_order(const char *path);

typedef struct _E_Config_Once 
{
   const char *title;
   const char *icon;
   const char *dialog;
   char *filename;
} E_Config_Once;

static E_Config_Dialog *_create_config_dialog(E_Container *con, E_Config_Once *once);

struct _E_Config_Dialog_Data 
{
   E_Config_Once *once;

   Evas_Object *o_apps, *o_list;
   Evas_Object *o_add, *o_del, *o_categories;
   Evas_Object *o_up, *o_down;
   Ecore_List *apps;
   const char *category;
   int category_n;
   
   char *fav, *app;
};

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
   return (E_Config_Dialog *)ed;
}

EAPI E_Config_Dialog *
e_int_config_apps_favs(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Once *once;
   char buf[4096];

   snprintf(buf, sizeof(buf), "%s/.e/e/applications/menu/favorite.menu", 
	    e_user_homedir_get());

   once = E_NEW(E_Config_Once, 1);
   once->title = _("Favorites Menu");
   once->dialog = "_config_apps_favs_dialog";
   once->icon = "enlightenment/favorites";
   once->filename = strdup(buf);
   return _create_config_dialog(con, once);
}

EAPI E_Config_Dialog *
e_int_config_apps_ibar(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Once *once;
   char buf[4096];
   
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/default/.order",
	    e_user_homedir_get());

   once = E_NEW(E_Config_Once, 1);
   once->title = _("IBar Applications");
   once->icon = "enlightenment/ibar_applications";
   once->dialog = "_config_apps_ibar_dialog";
   once->filename = strdup(buf);
   
   return _create_config_dialog(con, once);
}

EAPI E_Config_Dialog *
e_int_config_apps_ibar_other(E_Container *con, const char *path) 
{
   E_Config_Once *once;

   if (!path) return NULL;
   once = E_NEW(E_Config_Once, 1);
   once->title = _("IBar Applications");
   once->icon = "enlightenment/ibar_applications";
   once->dialog = "_config_apps_ibar_dialog";
   once->filename = strdup(path);
   
   return _create_config_dialog(con, once);
}

EAPI E_Config_Dialog *
e_int_config_apps_startup(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Once *once;
   char buf[4096];
   
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/startup/.order",
	    e_user_homedir_get());

   once = E_NEW(E_Config_Once, 1);
   once->title = _("Startup Applications");
   once->icon = "enlightenment/startup_applications";
   once->dialog = "_config_apps_startup_dialog";
   once->filename = strdup(buf);
   
   return _create_config_dialog(con, once);
}

EAPI E_Config_Dialog *
e_int_config_apps_restart(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Once *once;
   char buf[4096];
   
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/restart/.order",
	    e_user_homedir_get());

   once = E_NEW(E_Config_Once, 1);
   once->title = _("Restart Applications");
   once->icon = "enlightenment/restart_applications";
   once->dialog = "_config_apps_restart_dialog";
   once->filename = strdup(buf);
   
   return _create_config_dialog(con, once);
}

static E_Config_Dialog *
_create_config_dialog(E_Container *con, E_Config_Once *once)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", once->dialog)) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   
   cfd = e_config_dialog_new(con, once->title, "E", once->dialog, 
			     once->icon, 0, v, once);
   return cfd;
}

/* Private Functions */
static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Once *once;
   const char *ext;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   once = cfd->data;
   if (!once->filename) return NULL;

   ext = strrchr(once->filename, '.');
   if (!ext) return NULL;

   cfdata->once = once;
   if (!strcmp(ext, ".menu")) 
     cfdata->apps = _load_menu(once->filename);
   else if (!strcmp(ext, ".order"))
     cfdata->apps = _load_order(once->filename);
   
   return cfdata;
}

static Ecore_List *
_load_menu(const char *path)
{
   Efreet_Menu *menu, *entry;
   Ecore_List *apps;

   apps = ecore_list_new();

   menu = efreet_menu_parse(path);
   if (!menu) return apps;

   ecore_list_first_goto(menu->entries);
   while ((entry = ecore_list_next(menu->entries))) 
     {
	if (entry->type != EFREET_MENU_ENTRY_DESKTOP) continue;
	ecore_list_append(apps, entry->desktop);
     }
   efreet_menu_free(menu);
   return apps;
}

static Ecore_List *
_load_order(const char *path)
{
   E_Order *order;
   Evas_List *l;
   Ecore_List *apps;

   apps = ecore_list_new();
   order = e_order_new(path);
   for (l = order->desktops; l; l = l->next) 
     ecore_list_append(apps, l->data);

   return apps;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (cfdata->once)
     {
	E_FREE(cfdata->once->filename);
	E_FREE(cfdata->once);
     }

   if (cfdata->apps) ecore_list_destroy(cfdata->apps);
   if (cfdata->category) ecore_string_release(cfdata->category);
   E_FREE(cfdata->app);
   E_FREE(cfdata->fav);
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *ot, *of, *ob;
   
   ot = e_widget_table_add(evas, 0);

   of = e_widget_frametable_add(evas, _("Application Categories"), 0);
   ob = e_widget_button_add(evas, _("Categories"), "widget/categories", 
			    _cb_categories, cfdata, NULL);
   cfdata->o_categories = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 0, 1, 0);

   ob = e_widget_ilist_add(evas, 24, 24, &(cfdata->app));
   cfdata->o_apps = ob;
   _fill_categories(cfdata);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 1, 1);
   
   e_widget_table_object_append(ot, of, 0, 0, 1, 4, 1, 1, 1, 1);
  
   ob = e_widget_button_add(evas, _("Add"), "widget/add", _cb_add, cfdata, NULL);
   cfdata->o_add = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_table_object_append(ot, ob, 1, 1, 1, 1, 1, 0, 0, 0);

   ob = e_widget_button_add(evas, _("Delete"), "widget/del", _cb_del, cfdata, NULL);
   cfdata->o_del = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_table_object_append(ot, ob, 1, 2, 1, 1, 1, 0, 0, 0);

   of = e_widget_framelist_add(evas, cfdata->once->title, 0);
   ob = e_widget_ilist_add(evas, 24, 24, &(cfdata->fav));
   cfdata->o_list = ob;
   _fill_list(cfdata);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 2, 0, 1, 4, 1, 1, 1, 1);

   ob = e_widget_button_add(evas, _("Up"), "widget/up_arrow", 
			    _cb_up, cfdata, NULL);
   cfdata->o_up = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_table_object_append(ot, ob, 3, 1, 1, 1, 1, 0, 0, 0);

   ob = e_widget_button_add(evas, _("Down"), "widget/down_arrow", 
			    _cb_down, cfdata, NULL);
   cfdata->o_down = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_table_object_append(ot, ob, 3, 2, 1, 1, 1, 0, 0, 0);
   
   e_dialog_resizable_set(cfd->dia, 1);
   return ot;
}

static int 
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   int ret;
   char *ext;
 
   if (!cfdata->apps || !cfdata->once || !cfdata->once->filename) return 1;

   ext = strrchr(cfdata->once->filename, '.');
   if (!ext) return 0;

   if (!strcmp(ext, ".menu"))
     ret = _save_menu(cfdata);
   else if (!strcmp(ext, ".order"))
     ret = _save_order(cfdata);

   return ret;
}

static int
_save_menu(E_Config_Dialog_Data *cfdata)
{
   Efreet_Menu *menu;
   Efreet_Desktop *desk;
   int ret;

   menu = efreet_menu_new();
   ecore_list_first_goto(cfdata->apps);
   while ((desk = ecore_list_next(cfdata->apps)))
     efreet_menu_desktop_insert(menu, desk, -1);

   ret = efreet_menu_save(menu, cfdata->once->filename);
   efreet_menu_free(menu);
   return ret;
}

static int
_save_order(E_Config_Dialog_Data *cfdata)
{
   E_Order *order;
   Efreet_Desktop *desk;

   order = e_order_new(cfdata->once->filename);
   if (!order) return 0;
   e_order_clear(order);
   ecore_list_first_goto(cfdata->apps);
   while ((desk = ecore_list_next(cfdata->apps)))
     e_order_append(order, desk);

   //XXX need a way to free an E_Order
   return 1;
}

static void
_fill_categories(E_Config_Dialog_Data *cfdata) 
{
   Evas *evas;
   Evas_Coord w;
   Ecore_List *cats;
   char *category;

   evas = evas_object_evas_get(cfdata->o_apps);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_apps);
   e_widget_ilist_clear(cfdata->o_apps);

   cats = efreet_util_desktop_categories_list();
   ecore_list_sort(cats, ECORE_COMPARE_CB(strcmp), ECORE_SORT_MIN);
   ecore_list_first_goto(cats);
   while ((category = ecore_list_next(cats))) 
     e_widget_ilist_append(cfdata->o_apps, NULL, category, 
			   _category_cb_selected, cfdata, category);
   ecore_list_destroy(cats);

   e_widget_ilist_go(cfdata->o_apps);
   e_widget_min_size_get(cfdata->o_apps, &w, NULL);
   e_widget_min_size_set(cfdata->o_apps, w, 200);
   e_widget_ilist_thaw(cfdata->o_apps);
   edje_thaw();
   evas_event_thaw(evas);
}

static int
_cb_desktop_name_sort(Efreet_Desktop *a, Efreet_Desktop *b)
{
  return strcmp(a->name, b->name);
}

static void
_fill_apps(E_Config_Dialog_Data *cfdata) 
{
   Evas *evas;
   Evas_Coord w;

   Ecore_List *desktops;
   Efreet_Desktop *desktop;

   if (!cfdata->category) return;
   evas = evas_object_evas_get(cfdata->o_apps);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_apps);
   e_widget_ilist_clear(cfdata->o_apps);
   e_widget_ilist_header_append(cfdata->o_apps, NULL, cfdata->category);

   desktops = (Ecore_List *)efreet_util_desktop_category_list(cfdata->category);
   ecore_list_sort(desktops, ECORE_COMPARE_CB(_cb_desktop_name_sort), ECORE_SORT_MIN);
   ecore_list_first_goto(desktops);
   while ((desktop = ecore_list_next(desktops))) 
     {
	Evas_Object *icon;
	
	icon = e_util_desktop_icon_add(desktop, "24x24", evas);
	e_widget_ilist_append(cfdata->o_apps, icon, desktop->name,
			      _apps_cb_selected, cfdata, desktop->orig_path);
     }

   e_widget_ilist_go(cfdata->o_apps);
   e_widget_min_size_get(cfdata->o_apps, &w, NULL);
   e_widget_min_size_set(cfdata->o_apps, w, 200);
   e_widget_ilist_thaw(cfdata->o_apps);
   edje_thaw();
   evas_event_thaw(evas);
}

static void
_fill_list(E_Config_Dialog_Data *cfdata) 
{
   Evas *evas;
   Evas_Coord w;
   Efreet_Desktop *desktop;

   if ((!cfdata->apps)) 
     {
	e_widget_min_size_set(cfdata->o_list, 200, 200);
	return;
     }

   evas = evas_object_evas_get(cfdata->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);

   ecore_list_first_goto(cfdata->apps);
   while ((desktop = ecore_list_next(cfdata->apps))) 
     {
	Evas_Object *icon = NULL;

	icon = e_util_desktop_icon_add(desktop, "24x24", evas);
	e_widget_ilist_append(cfdata->o_list, icon, desktop->name, 
			      _list_cb_selected, cfdata, desktop->orig_path);
     }

   e_widget_ilist_go(cfdata->o_list);
   e_widget_min_size_get(cfdata->o_list, &w, NULL);
   e_widget_min_size_set(cfdata->o_list, w, 200);
   e_widget_ilist_thaw(cfdata->o_list);
   edje_thaw();
   evas_event_thaw(evas);
}

static void 
_category_cb_selected(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;

   cfdata->category_n = e_widget_ilist_selected_get(cfdata->o_apps);
   if (cfdata->category) ecore_string_release(cfdata->category);
   cfdata->category = ecore_string_instance(cfdata->app);
   _fill_apps(cfdata);
   e_widget_disabled_set(cfdata->o_categories, 0);
}

static void 
_apps_cb_selected(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   e_widget_disabled_set(cfdata->o_add, 0);
}

static void 
_list_cb_selected(void *data) 
{
   E_Config_Dialog_Data *cfdata;
   int sel, count;
   
   cfdata = data;
   e_widget_disabled_set(cfdata->o_del, 0);

   count = e_widget_ilist_count(cfdata->o_list);
   sel = e_widget_ilist_selected_get(cfdata->o_list);
   if (sel == 0) 
     e_widget_disabled_set(cfdata->o_up, 1);
   else
     e_widget_disabled_set(cfdata->o_up, 0);
   if (sel < (count -1))
     e_widget_disabled_set(cfdata->o_down, 0);
   else
     e_widget_disabled_set(cfdata->o_down, 1);
}

static void 
_cb_categories(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
 
   cfdata = data;
   e_widget_disabled_set(cfdata->o_categories, 1);
   if (cfdata->category) ecore_string_release(cfdata->category);
   cfdata->category = NULL;
   _fill_categories(cfdata);
   e_widget_ilist_nth_show(cfdata->o_apps, cfdata->category_n, 1);
   cfdata->category_n = 0;
}

static void 
_cb_add(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_Object *icon;
   Efreet_Desktop *desk;
   Evas *evas;
   Evas_Coord w;
 
   cfdata = data;
   if (!cfdata->apps) return;
   if (e_widget_ilist_selected_get(cfdata->o_apps) < 0) return;
   desk = efreet_desktop_get(cfdata->app);
   if (!desk) return;

   evas = evas_object_evas_get(cfdata->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);
   icon = e_util_desktop_icon_add(desk, "24x24", evas);
   e_widget_ilist_append(cfdata->o_list, icon, desk->name, 
			 _list_cb_selected, cfdata, cfdata->app);
   e_widget_ilist_go(cfdata->o_list);
   e_widget_min_size_get(cfdata->o_list, &w, NULL);
   e_widget_min_size_set(cfdata->o_list, w, 200);
   e_widget_ilist_thaw(cfdata->o_list);
   edje_thaw();
   evas_event_thaw(evas);

   ecore_list_append(cfdata->apps, desk);
}

static void 
_cb_del(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   Efreet_Desktop *desk;
   Evas *evas;
   Evas_Coord w;
   int num;
   
   cfdata = data;
   if (!cfdata->apps) return;
   num = e_widget_ilist_selected_get(cfdata->o_list);
   if (num < 0) return;

   evas = evas_object_evas_get(cfdata->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);
   e_widget_ilist_remove_num(cfdata->o_list, num);
   e_widget_ilist_go(cfdata->o_list);
   e_widget_min_size_get(cfdata->o_list, &w, NULL);
   e_widget_min_size_set(cfdata->o_list, w, 200);
   e_widget_ilist_thaw(cfdata->o_list);
   edje_thaw();
   evas_event_thaw(evas);

   desk = efreet_desktop_get(cfdata->fav);
   if (!desk) return;
   if (ecore_list_goto(cfdata->apps, desk))
     ecore_list_remove(cfdata->apps);
}

static void 
_cb_up(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   Efreet_Desktop *desk;
   Evas *evas;
   Evas_Object *icon;
   Evas_Coord w;
   int sel, i;

   cfdata = data;
   if (!cfdata->apps) return;
   
   sel = e_widget_ilist_selected_get(cfdata->o_list);
   if (sel < 0) return;

   desk = efreet_desktop_get(cfdata->fav);
   if (!desk) return;

   evas = evas_object_evas_get(cfdata->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);
   e_widget_ilist_remove_num(cfdata->o_list, sel);
   e_widget_ilist_go(cfdata->o_list);
   
   icon = e_util_desktop_icon_add(desk, "24x24", evas);
   e_widget_ilist_prepend_relative(cfdata->o_list, icon, desk->name, 
			 _list_cb_selected, cfdata, cfdata->fav, (sel - 1));
   e_widget_ilist_selected_set(cfdata->o_list, (sel - 1));
   e_widget_ilist_go(cfdata->o_list);
   e_widget_min_size_get(cfdata->o_list, &w, NULL);
   e_widget_min_size_set(cfdata->o_list, w, 200);
   e_widget_ilist_thaw(cfdata->o_list);
   edje_thaw();
   evas_event_thaw(evas);

   if (ecore_list_goto(cfdata->apps, desk)) 
     {
	i = ecore_list_index(cfdata->apps);
	ecore_list_remove(cfdata->apps);
	ecore_list_index_goto(cfdata->apps, (i - 1));
	ecore_list_insert(cfdata->apps, desk);
     }
}

static void 
_cb_down(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   Efreet_Desktop *desk;
   Evas *evas;
   Evas_Object *icon;
   Evas_Coord w;
   int sel, i;

   cfdata = data;
   if (!cfdata->apps) return;
   
   sel = e_widget_ilist_selected_get(cfdata->o_list);
   if (sel < 0) return;

   desk = efreet_desktop_get(cfdata->fav);
   if (!desk) return;

   evas = evas_object_evas_get(cfdata->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);
   e_widget_ilist_remove_num(cfdata->o_list, sel);
   e_widget_ilist_go(cfdata->o_list);
   
   icon = e_util_desktop_icon_add(desk, "24x24", evas);
   e_widget_ilist_append_relative(cfdata->o_list, icon, desk->name, 
			 _list_cb_selected, cfdata, cfdata->fav, sel);
   e_widget_ilist_selected_set(cfdata->o_list, (sel + 1));
   e_widget_ilist_go(cfdata->o_list);
   e_widget_min_size_get(cfdata->o_list, &w, NULL);
   e_widget_min_size_set(cfdata->o_list, w, 200);
   e_widget_ilist_thaw(cfdata->o_list);
   edje_thaw();
   evas_event_thaw(evas);

   if (ecore_list_goto(cfdata->apps, desk)) 
     {
	i = ecore_list_index(cfdata->apps);
	ecore_list_remove(cfdata->apps);
	ecore_list_index_goto(cfdata->apps, (i + 1));
	ecore_list_insert(cfdata->apps, desk);
     }
}
