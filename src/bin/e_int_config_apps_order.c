#include "e.h"

struct _E_Config_Dialog_Data 
{
   Evas_Object *o_apps, *o_list;
   Evas_Object *o_add, *o_del;   
   char *list, *app;
};

typedef struct _E_Config_Once 
{
   const char *title;
   const char *icon;
   const char *dialog;
   E_Order *order;
} E_Config_Once;

static E_Config_Dialog *_create_config_dialog(E_Container *con, E_Config_Once *once);
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _fill_apps(E_Config_Dialog_Data *cfdata);
static void _fill_list(E_Config_Once *once, E_Config_Dialog_Data *cfdata);
static void _apps_cb_selected(void *data);
static void _list_cb_selected(void *data);
static void _cb_add(void *data, void *data2);
static void _cb_del(void *data, void *data2);

EAPI E_Config_Dialog *
e_int_config_apps_ibar(E_Container *con) 
{
   E_Config_Once *once;
   char buf[4096];
   
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/default/.order",
	    e_user_homedir_get());

   once = E_NEW(E_Config_Once, 1);
   once->title = _("IBar Applications");
   once->icon = "enlightenment/ibar_applications";
   once->dialog = "_config_apps_ibar_dialog";
   once->order = e_order_new(buf);
   
   return _create_config_dialog(con, once);
}

EAPI E_Config_Dialog *
e_int_config_apps_ibar_other(E_Container *con, const char *path) 
{
   E_Config_Once *once;

   once = E_NEW(E_Config_Once, 1);
   once->title = _("IBar Applications");
   once->icon = "enlightenment/ibar_applications";
   once->dialog = "_config_apps_ibar_dialog";
   once->order = e_order_new(path);
   
   return _create_config_dialog(con, once);
}

EAPI E_Config_Dialog *
e_int_config_apps_startup(E_Container *con) 
{
   E_Config_Once *once;
   char buf[4096];
   
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/startup/.order",
	    e_user_homedir_get());

   once = E_NEW(E_Config_Once, 1);
   once->title = _("Startup Applications");
   once->icon = "enlightenment/startup_applications";
   once->dialog = "_config_apps_startup_dialog";
   once->order = e_order_new(buf);
   
   return _create_config_dialog(con, once);
}

EAPI E_Config_Dialog *
e_int_config_apps_restart(E_Container *con) 
{
   E_Config_Once *once;
   char buf[4096];
   
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/restart/.order",
	    e_user_homedir_get());

   once = E_NEW(E_Config_Once, 1);
   once->title = _("Restart Applications");
   once->icon = "enlightenment/restart_applications";
   once->dialog = "_config_apps_restart_dialog";
   once->order = e_order_new(buf);
   
   return _create_config_dialog(con, once);
}

/* Private Functions */
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
   
   cfd = e_config_dialog_new(con, once->title, "E", once->dialog, 
			     once->icon, 0, v, once);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_Config_Once *o;
   
   o = cfd->data;
   E_FREE(o);
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   E_Config_Once *once;
   Evas_Object *ot, *of, *ob;
   
   once = cfd->data;
   ot = e_widget_table_add(evas, 0);

   of = e_widget_framelist_add(evas, _("All Applications"), 0);
   ob = e_widget_ilist_add(evas, 24, 24, &(cfdata->app));
   cfdata->o_apps = ob;
   _fill_apps(cfdata);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 4, 1, 0, 1, 0);
   
   ob = e_widget_button_add(evas, _("Add"), "widget/add", _cb_add, cfdata, once);
   cfdata->o_add = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_table_object_append(ot, ob, 1, 1, 1, 1, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Delete"), "widget/del", _cb_del, cfdata, once);
   cfdata->o_del = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_table_object_append(ot, ob, 1, 2, 1, 1, 1, 0, 1, 0);

   of = e_widget_framelist_add(evas, (char *)once->title, 0);
   ob = e_widget_ilist_add(evas, 24, 24, &(cfdata->list));
   cfdata->o_list = ob;
   _fill_list(cfd->data, cfdata);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 2, 0, 1, 4, 1, 0, 1, 0);
   
   return ot;
}

static void
_fill_apps(E_Config_Dialog_Data *cfdata) 
{
   Evas *evas;
   Evas_Coord w;
   Efreet_Menu *menu;
 
   menu = efreet_menu_get();
   if (!menu) return;
   
   evas = evas_object_evas_get(cfdata->o_apps);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_apps);

   if (menu->entries) 
     {
	Efreet_Menu *entry;
	
	ecore_list_goto_first(menu->entries);
	while ((entry = ecore_list_next(menu->entries))) 
	  {
	     Efreet_Menu *sub;
	     
	     if (entry->type != EFREET_MENU_ENTRY_MENU) continue;
	     e_widget_ilist_header_append(cfdata->o_apps, NULL, entry->id);
	     if (!entry->entries) continue;
	     ecore_list_goto_first(entry->entries);
	     while ((sub = ecore_list_next(entry->entries))) 
	       {
		  Evas_Object *icon = NULL;
		  
		  if (sub->type != EFREET_MENU_ENTRY_DESKTOP) continue;
		  icon = e_util_icon_theme_icon_add(sub->icon, "24x24", evas);
		  e_widget_ilist_append(cfdata->o_apps, icon, sub->name, 
					_apps_cb_selected, cfdata, 
					sub->desktop->orig_path);
	       }
	  }
     }
   
   e_widget_ilist_go(cfdata->o_apps);
   e_widget_min_size_get(cfdata->o_apps, &w, NULL);
   e_widget_min_size_set(cfdata->o_apps, w, 200);
   e_widget_ilist_thaw(cfdata->o_apps);
   edje_thaw();
   evas_event_thaw(evas);

   efreet_menu_free(menu);
}

static void 
_fill_list(E_Config_Once *once, E_Config_Dialog_Data *cfdata) 
{
   Evas *evas;
   Evas_Coord w;
   Evas_List *l;
   
   evas = evas_object_evas_get(cfdata->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);

   for (l = once->order->desktops; l; l = l->next) 
     {
	Efreet_Desktop *desk;
	Evas_Object *icon = NULL;

	desk = l->data;
	if (!desk) continue;
	icon = e_util_desktop_icon_add(desk, "24x24", evas);
	e_widget_ilist_append(cfdata->o_list, icon, desk->name, 
			      _list_cb_selected, cfdata, desk->orig_path);
     }
   
   e_widget_ilist_go(cfdata->o_list);
   e_widget_min_size_get(cfdata->o_list, &w, NULL);
   e_widget_min_size_set(cfdata->o_list, w, 200);
   e_widget_ilist_thaw(cfdata->o_list);
   edje_thaw();
   evas_event_thaw(evas);
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
   
   cfdata = data;
   e_widget_disabled_set(cfdata->o_del, 0);
}

static void 
_cb_add(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Once *once;
   Evas_Object *icon = NULL;
   Efreet_Desktop *desk;
   Evas *evas;
   Evas_Coord w;

   cfdata = data;
   once = data2;

   if (e_widget_ilist_selected_get(cfdata->o_apps) < 0) return;
 
   evas = evas_object_evas_get(cfdata->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);
   
   desk = efreet_desktop_get(cfdata->app);
   if (!desk) return;

   e_util_desktop_icon_add(desk, "24x24", evas);
   e_widget_ilist_append(cfdata->o_list, icon, desk->name, 
			 _list_cb_selected, cfdata, cfdata->app);
   e_widget_ilist_go(cfdata->o_list);
   e_widget_min_size_get(cfdata->o_list, &w, NULL);
   e_widget_min_size_set(cfdata->o_list, w, 200);
   e_widget_ilist_thaw(cfdata->o_list);
   edje_thaw();
   evas_event_thaw(evas);
   
   e_order_append(once->order, desk);
}

static void 
_cb_del(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Once *once;
   Efreet_Desktop *desk;
   Evas *evas;
   Evas_Coord w;
   int num;
   
   cfdata = data;
   once = data2;
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
   
   desk = efreet_desktop_get(cfdata->list);
   if (!desk) return;
   e_order_remove(once->order, desk);
}
