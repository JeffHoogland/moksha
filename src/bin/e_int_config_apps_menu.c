#include "e.h"

/* TODO: This should be modified to handle any other fdo menu editing. */

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _fill_apps(E_Config_Dialog_Data *cfdata);
static void _fill_list(E_Config_Dialog_Data *cfdata);
static void _apps_cb_selected(void *data);
static void _list_cb_selected(void *data);
static void _cb_add(void *data, void *data2);
static void _cb_del(void *data, void *data2);
static void _create_fav_menu(const char *path);

struct _E_Config_Dialog_Data 
{
   Evas_Object *o_apps, *o_list;
   Evas_Object *o_add, *o_del;
   Efreet_Menu *menu;
   
   char *fav, *app;
};

EAPI E_Config_Dialog *
e_int_config_apps_favs(E_Container *con) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_apps_favs_dialog")) return NULL;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   
   cfd = e_config_dialog_new(con, _("Favorites Menu"), "E", 
			     "_config_apps_favs_dialog", 
			     "enlightenment/favorites", 0, v, NULL);
   return cfd;
}

/* Private Functions */
static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   char buf[4096];
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/menu/favorite.menu", 
	    e_user_homedir_get());
   cfdata->menu = efreet_menu_parse(buf);
   if (!cfdata->menu) 
     {
	_create_fav_menu(buf);
	cfdata->menu = efreet_menu_parse(buf);
     }
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (cfdata->menu) efreet_menu_free(cfdata->menu);
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *ot, *of, *ob;
   
   ot = e_widget_table_add(evas, 0);

   of = e_widget_framelist_add(evas, _("All Applications"), 0);
   ob = e_widget_ilist_add(evas, 24, 24, &(cfdata->app));
   cfdata->o_apps = ob;
   _fill_apps(cfdata);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 4, 1, 0, 1, 0);
   
   ob = e_widget_button_add(evas, _("Add"), "widget/add", _cb_add, cfdata, NULL);
   cfdata->o_add = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_table_object_append(ot, ob, 1, 1, 1, 1, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Delete"), "widget/del", _cb_del, cfdata, NULL);
   cfdata->o_del = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_table_object_append(ot, ob, 1, 2, 1, 1, 1, 0, 1, 0);

   of = e_widget_framelist_add(evas, _("Favorites"), 0);
   ob = e_widget_ilist_add(evas, 24, 24, &(cfdata->fav));
   cfdata->o_list = ob;
   _fill_list(cfdata);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 2, 0, 1, 4, 1, 0, 1, 0);
   
   return ot;
}

static int 
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   char buf[4096];
 
   if (!cfdata->menu) return 1;

   snprintf(buf, sizeof(buf), "%s/.e/e/applications/menu/favorite.menu", 
	    e_user_homedir_get());
   efreet_menu_save(cfdata->menu, buf);
   return 1;
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
_fill_list(E_Config_Dialog_Data *cfdata) 
{
   Evas *evas;
   Evas_Coord w;
   Efreet_Menu *menu, *entry;

   menu = cfdata->menu;
   if ((!menu) || (!menu->entries))  
     {
	e_widget_min_size_set(cfdata->o_list, 200, 200);
	return;
     }

   evas = evas_object_evas_get(cfdata->o_list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->o_list);

   ecore_list_goto_first(menu->entries);
   while ((entry = ecore_list_next(menu->entries))) 
     {
	Evas_Object *icon = NULL;

	if (entry->type != EFREET_MENU_ENTRY_DESKTOP) continue;

	icon = e_util_icon_theme_icon_add(entry->icon, "24x24", evas);
	e_widget_ilist_append(cfdata->o_list, icon, entry->name, 
			      _list_cb_selected, cfdata, 
			      entry->desktop->orig_path);
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
   Evas_Object *icon;
   Efreet_Desktop *desk;
   Evas *evas;
   Evas_Coord w;
 
   cfdata = data;
   if (!cfdata->menu) return;
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

   efreet_menu_desktop_insert(cfdata->menu, desk, -1);
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
   if (!cfdata->menu) return;
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
   efreet_menu_desktop_remove(cfdata->menu, desk);
}

static void 
_create_fav_menu(const char *path) 
{
   FILE *f;
   
   if (ecore_file_exists(path)) return;
   
   f = fopen(path, "w");
   if (!f) return;
   
   fprintf(f, "<?xml version=\"1.0\"?>\n");
   fprintf(f, "<!DOCTYPE Menu PUBLIC \"-//freedesktop//DTD Menu 1.0//EN\" "
	   "\"http://standards.freedesktop.org/menu-spec/menu-1.0.dtd\">\n");
   fprintf(f, "<Menu>\n");
   fprintf(f, "  <Name>Favorites</Name>\n");
   fprintf(f, "  <DefaultAppDirs/>\n");
   fprintf(f, "  <DefaultDirectoryDirs/>\n");
   fprintf(f, "  <Layout>\n");
   fprintf(f, "    <Filename>xterm.desktop</Filename>\n");
   fprintf(f, "    <Filename>firefox.desktop</Filename>\n");
   fprintf(f, "    <Filename>xmms.desktop</Filename>\n");
   fprintf(f, "  </Layout>\n");
   fprintf(f, "  <Include>\n");
   fprintf(f, "    <Filename>xterm.desktop</Filename>\n");
   fprintf(f, "    <Filename>firefox.desktop</Filename>\n");
   fprintf(f, "    <Filename>xmms.desktop</Filename>\n");
   fprintf(f, "  </Include>\n");
   fprintf(f, "</Menu>\n");
   fclose(f);
}
