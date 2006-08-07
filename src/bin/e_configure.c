#include "e.h"

typedef struct _E_Configure_CB E_Configure_CB;

struct _E_Configure_CB
{
   E_Configure *eco;
   E_Config_Dialog *(*func) (E_Container *con);   
};

static void _e_configure_free(E_Configure *app);
static void _e_configure_cb_del_req(E_Win *win);
static void _e_configure_cb_resize(E_Win *win);
static void _e_configure_cb_standard(void *data);
static void _e_configure_cb_close(void *data, void *data2);

EAPI E_Configure *
e_configure_show(E_Container *con)
{
   E_Configure *eco;
   E_Manager *man;
   Evas_Coord ew, eh, mw, mh;
   
   if (!con)
     {
	man = e_manager_current_get();
	if (!man) return NULL;
	con = e_container_current_get(man);
	if (!con) con = e_container_number_get(man, 0);
	if (!con) return NULL;
     }
   
   eco = E_OBJECT_ALLOC(E_Configure, E_CONFIGURE_TYPE, _e_configure_free);
   if (!eco) return NULL;
   eco->win = e_win_new(con);   
   if (!eco->win)
     {
	free(eco);
	return NULL;
     }
   eco->win->data = eco;
   
   eco->con = con;
   
   e_win_title_set(eco->win, _("Enlightenment Configuration"));
   e_win_name_class_set(eco->win, "E", "_configure");
   e_win_dialog_set(eco->win, 1);
   eco->evas = e_win_evas_get(eco->win);
   e_win_delete_callback_set(eco->win, _e_configure_cb_del_req);
   e_win_resize_callback_set(eco->win, _e_configure_cb_resize);   
   e_win_centered_set(eco->win, 1);
      
   eco->edje = edje_object_add(eco->evas);
   e_theme_edje_object_set(eco->edje, "base/theme/configure",
			   "widgets/configure/main");
   
   /* 24 */
   eco->ilist = e_widget_ilist_add(eco->evas, 32, 32, NULL);
   e_widget_ilist_selector_set(eco->ilist, 1);
   e_widget_min_size_get(eco->ilist, &mw, &mh);
   edje_extern_object_min_size_set(eco->ilist, mw, mh);
   edje_object_part_swallow(eco->edje, "item", eco->ilist);
   edje_object_part_text_set(eco->edje, "title", _("Configuration Panel"));

   /* add items here */
   e_configure_header_item_add(eco, "enlightenment/appearance", _("Appearance"));
//   e_configure_standard_item_add(eco, "enlightenment/background", _("Wallpaper"), e_int_config_background);
   e_configure_standard_item_add(eco, "enlightenment/background", _("Wallpaper"), e_int_config_wallpaper);
   e_configure_standard_item_add(eco, "enlightenment/themes", _("Theme"), e_int_config_theme);   
   e_configure_standard_item_add(eco, "enlightenment/fonts", _("Fonts"), e_int_config_fonts);
   e_configure_standard_item_add(eco, "enlightenment/mouse", _("Mouse Cursor"), e_int_config_cursor);
   e_configure_standard_item_add(eco, "enlightenment/windows", _("Window Display"), e_int_config_window_display);
   e_configure_standard_item_add(eco, "enlightenment/shelf", _("Shelves"), e_int_config_shelf); 
   
   e_configure_header_item_add(eco, "enlightenment/screen_setup", _("Screen"));
   e_configure_standard_item_add(eco, "enlightenment/desktops", _("Virtual Desktops"), e_int_config_desks);
   e_configure_standard_item_add(eco, "enlightenment/screen_resolution", _("Screen Resolution"), e_int_config_display);
   e_configure_standard_item_add(eco, "enlightenment/desklock", _("Screen Lock"), e_int_config_desklock);
   
   e_configure_header_item_add(eco, "enlightenment/behavior", _("Behavior"));
   e_configure_standard_item_add(eco, "enlightenment/focus", _("Window Focus"), e_int_config_focus);
   e_configure_standard_item_add(eco, "enlightenment/keys", _("Key Bindings"), e_int_config_keybindings);
   e_configure_standard_item_add(eco, "enlightenment/menus", _("Menus"), e_int_config_menus);
   
   e_configure_header_item_add(eco, "enlightenment/misc", _("Miscellaneous"));
   e_configure_standard_item_add(eco, "enlightenment/performance", _("Performance"), e_int_config_performance);
   e_configure_standard_item_add(eco, "enlightenment/configuration", _("Configuration Dialogs"), e_int_config_cfgdialogs);
   
   e_configure_header_item_add(eco, "enlightenment/advanced", _("Advanced"));
   e_configure_standard_item_add(eco, "enlightenment/startup", _("Startup"), e_int_config_startup);
   e_configure_standard_item_add(eco, "enlightenment/winlist", _("Window List"), e_int_config_winlist);
   e_configure_standard_item_add(eco, "enlightenment/window_manipulation", _("Window Manipulation"), e_int_config_window_manipulation);
   e_configure_standard_item_add(eco, "enlightenment/run", _("Run Command"), e_int_config_exebuf);
   e_configure_standard_item_add(eco, "enlightenment/directories", _("Search Directories"), e_int_config_paths); 

   e_configure_header_item_add(eco, "enlightenment/extensions", _("Extensions"));
   e_configure_standard_item_add(eco, "enlightenment/modules", _("Modules"), e_int_config_modules);
   
   /* FIXME: we should have a way for modules to hook in here and add their
    * own entries
    * 
    * e_configure_header_item_add(eco, "enlightenment/extension_config", _("Extension Configuration"));
    */
   
   eco->close = e_widget_button_add(eco->evas, _("Close"), NULL, _e_configure_cb_close, eco, NULL);
   e_widget_min_size_get(eco->close, &mw, &mh);
   edje_extern_object_min_size_set(eco->close, mw, mh);
   edje_object_part_swallow(eco->edje, "button", eco->close);

   edje_object_size_min_calc(eco->edje, &ew, &eh);
   e_win_resize(eco->win, ew, eh);
   e_win_size_min_set(eco->win, ew, eh);

   evas_object_show(eco->ilist);
   evas_object_show(eco->close);
   evas_object_show(eco->edje);

   e_win_show(eco->win);
   eco->win->border->internal_icon = evas_stringshare_add("enlightenment/configuration");

   e_widget_focus_set(eco->ilist, 1);
   e_widget_ilist_go(eco->ilist);
   
   return eco;
}

EAPI void
e_configure_standard_item_add(E_Configure *eco, char *icon, char *label, E_Config_Dialog *(*func) (E_Container *con))
{
   Evas_Object *o;
   E_Configure_CB *ecocb;
   
   o = edje_object_add(eco->evas);
   e_util_edje_icon_set(o, icon);
   ecocb = E_NEW(E_Configure_CB, 1);
   ecocb->eco = eco;
   ecocb->func = func;
   eco->cblist = evas_list_append(eco->cblist, ecocb);
   e_widget_ilist_append(eco->ilist, o, label, _e_configure_cb_standard, ecocb, NULL);
}

EAPI void
e_configure_header_item_add(E_Configure *eco, char *icon, char *label)
{
   Evas_Object *o;
   
   o = edje_object_add(eco->evas);
   e_util_edje_icon_set(o, icon);
   e_widget_ilist_header_append(eco->ilist, o, label);
}

/* local subsystem functions */
static void
_e_configure_free(E_Configure *eco)
{
   while (eco->cblist)
     {
	free(eco->cblist->data);
	eco->cblist = evas_list_remove_list(eco->cblist, eco->cblist);
     }
   evas_object_del(eco->edje);
   evas_object_del(eco->ilist);
   e_object_del(E_OBJECT(eco->win));
   free(eco);
}

static void
_e_configure_cb_del_req(E_Win *win)
{
   E_Configure *eco;

   eco = win->data;
   if (eco) e_object_del(E_OBJECT(eco));
}

static void
_e_configure_cb_resize(E_Win *win)
{
   Evas_Coord w, h;
   E_Configure *eco;
   
   ecore_evas_geometry_get(win->ecore_evas, NULL, NULL, &w, &h);
   eco = win->data;
   evas_object_resize(eco->edje, w, h);
}

static void
_e_configure_cb_standard(void *data)
{
   E_Configure_CB *ecocb;
   
   ecocb = data;
   ecocb->func(ecocb->eco->con);
}

static void
_e_configure_cb_close(void *data, void *data2) 
{
   E_Configure *eco;
   
   eco = data;
   if (eco) e_object_del(E_OBJECT(eco));
}
