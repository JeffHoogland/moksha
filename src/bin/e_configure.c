#include "e.h"

typedef struct _E_Configure_CB E_Configure_CB;
typedef struct _E_Configure_Category E_Configure_Category;
typedef struct _E_Configure_Item E_Configure_Item;

typedef struct _E_Configure_Cat E_Configure_Cat;
typedef struct _E_Configure_It E_Configure_It;

static void _e_configure_free(E_Configure *eco);
static void _e_configure_cb_del_req(E_Win *win);
static void _e_configure_cb_resize(E_Win *win);
static void _e_configure_cb_close(void *data, void *data2);
static E_Configure_Category *_e_configure_category_add(E_Configure *eco, const char *label, const char *icon);
static void _e_configure_category_cb(void *data);
static void _e_configure_item_add(E_Configure_Category *cat, const char *label, const char *icon, const char *path);
static void _e_configure_item_cb(void *data);
static void _e_configure_focus_cb(void *data, Evas_Object *obj);
static void _e_configure_keydown_cb(void *data, Evas *e, Evas_Object *obj, void *event);
static void _e_configure_fill_cat_list(void *data);

struct _E_Configure_CB
{
   E_Configure *eco;
   const char *path;
};

struct _E_Configure_Category
{
   E_Configure *eco;
   const char *label;
   
   Evas_List *items;
};

struct _E_Configure_Item
{
   E_Configure_CB *cb;
   
   const char *label;
   const char *icon;
};



struct _E_Configure_Cat
{
   const char *cat;
   int         pri;
   const char *label;
   const char *icon_file;
   const char *icon;
   Evas_List  *items;
};

struct _E_Configure_It
{
   const char        *item;
   int                pri;
   const char        *label;
   const char        *icon_file;
   const char        *icon;
   E_Config_Dialog *(*func) (E_Container *con, const char *params);
};

static E_Configure *_e_configure = NULL;
static Evas_List *_e_configure_registry = NULL;

EAPI void
e_configure_registry_item_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon, E_Config_Dialog *(*func) (E_Container *con, const char *params))
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
   
   for (l = _e_configure_registry; l; l = l->next)
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
e_configure_registry_item_del(const char *path)
{
   Evas_List *l;
   char *cat;
   const char *item;
   
   /* path is "category/item" */
   cat = ecore_file_dir_get(path);
   if (!cat) return;
   item = ecore_file_file_get(path);
   for (l = _e_configure_registry; l; l = l->next)
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
   
   ecat = E_NEW(E_Configure_Cat, 1);
   if (!ecat) return;
   
   ecat->cat = evas_stringshare_add(path);
   ecat->pri = pri;
   ecat->label = evas_stringshare_add(label);
   if (icon_file) ecat->icon_file = evas_stringshare_add(icon_file);
   if (icon) ecat->icon = evas_stringshare_add(icon);
   for (l = _e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat2;
	
	ecat2 = l->data;
	if (ecat2->pri > ecat->pri)
	  {
	     _e_configure_registry = evas_list_prepend_relative_list(_e_configure_registry, ecat, l);
	     return;
	  }
     }
   _e_configure_registry = evas_list_append(_e_configure_registry, ecat);
}

EAPI void
e_configure_registry_category_del(const char *path)
{
   Evas_List *l;
   char *cat;
   
   cat = ecore_file_dir_get(path);
   if (!cat) return;
   for (l = _e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat;
	
	ecat = l->data;
        if (!strcmp(cat, ecat->cat))
	  {
	     if (ecat->items) goto done;
	     _e_configure_registry = evas_list_remove_list(_e_configure_registry, l);
	     evas_stringshare_del(ecat->cat);
	     evas_stringshare_del(ecat->label);
	     if (ecat->icon) evas_stringshare_del(ecat->icon);
	     free(ecat);
	     goto done;
	  }
     }
   done:
   free(cat);
}

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
   for (l = _e_configure_registry; l; l = l->next)
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
		  printf("%s == %s\n", item, eci->item);
		  if (!strcmp(item, eci->item))
		    {
		       if (eci->func) eci->func(con, params);
		       goto done;
		    }
	       }
	     goto done;
	  }
     }
   done:
   free(cat);
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
   for (l = _e_configure_registry; l; l = l->next)
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

EAPI E_Configure *
e_configure_show(E_Container *con) 
{
   E_Configure *eco;
   E_Manager *man;
   Evas_Coord ew, eh, mw, mh;
   Evas_Object *o, *of;
   Evas_Modifier_Mask mask;
   
   if (_e_configure) 
     {
	E_Zone *z, *z2;
	
	eco = _e_configure;
	z = e_util_zone_current_get(e_manager_current_get());
	z2 = eco->win->border->zone;
	e_win_show(eco->win);
	e_win_raise(eco->win);
	if (z->container == z2->container)
	  e_border_desk_set(eco->win->border, e_desk_current_get(z));
	else 
	  {
	     if (!eco->win->border->sticky)
	       e_desk_show(eco->win->border->desk);
	     ecore_x_pointer_warp(z2->container->win,
				  z2->x + (z2->w / 2), z2->y + (z2->h / 2));
	  }
	e_border_unshade(eco->win->border, E_DIRECTION_DOWN);
	return NULL;
     }
   
   if (!con) 
     {
	man = e_manager_current_get();
	if (!man) return NULL;
	con = e_container_current_get(man);
	if (!con)
	  con = e_container_number_get(man, 0);
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
   eco->evas = e_win_evas_get(eco->win);
   
   e_win_title_set(eco->win, _("Enlightenment Configuration"));
   e_win_name_class_set(eco->win, "E", "_configure");
   e_win_dialog_set(eco->win, 1);
   e_win_delete_callback_set(eco->win, _e_configure_cb_del_req);
   e_win_resize_callback_set(eco->win, _e_configure_cb_resize);
   e_win_centered_set(eco->win, 1);

   eco->edje = edje_object_add(eco->evas);
   e_theme_edje_object_set(eco->edje, "base/theme/configure", "e/widgets/configure/main");

   eco->o_list = e_widget_list_add(eco->evas, 1, 1);
   edje_object_part_swallow(eco->edje, "e.swallow.content", eco->o_list);

   /* Event Obj for keydown */
   o = evas_object_rectangle_add(eco->evas);
   mask = 0;
   evas_object_key_grab(o, "Tab", mask, ~mask, 0);
   mask = evas_key_modifier_mask_get(e_win_evas_get(eco->win), "Shift");
   evas_object_key_grab(o, "Tab", mask, ~mask, 0);
   mask = 0;
   evas_object_key_grab(o, "Return", mask, ~mask, 0);
   mask = 0;
   evas_object_key_grab(o, "KP_Enter", mask, ~mask, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN, _e_configure_keydown_cb, eco->win);
   
   /* Category List */
   of = e_widget_framelist_add(eco->evas, _("Categories"), 1);
   eco->cat_list = e_widget_ilist_add(eco->evas, 32, 32, NULL);
   _e_configure_fill_cat_list(eco);
   e_widget_on_focus_hook_set(eco->cat_list, _e_configure_focus_cb, eco->win);
   e_widget_framelist_object_append(of, eco->cat_list);
   e_widget_list_object_append(eco->o_list, of, 1, 1, 0.5);
   
   /* Item List */
   of = e_widget_framelist_add(eco->evas, _("Items"), 1);
   eco->item_list = e_widget_ilist_add(eco->evas, 32, 32, NULL);
   e_widget_ilist_selector_set(eco->item_list, 1);
   e_widget_ilist_go(eco->item_list);
   e_widget_on_focus_hook_set(eco->item_list, _e_configure_focus_cb, eco->win);
   e_widget_framelist_object_append(of, eco->item_list);   
   e_widget_list_object_append(eco->o_list, of, 1, 1, 0.5);
   
   /* Close Button */
   eco->close = e_widget_button_add(eco->evas, _("Close"), NULL, 
				    _e_configure_cb_close, eco, NULL);
   e_widget_on_focus_hook_set(eco->close, _e_configure_focus_cb, eco->win);
   e_widget_min_size_get(eco->close, &mw, &mh);
   edje_extern_object_min_size_set(eco->close, mw, mh);
   edje_object_part_swallow(eco->edje, "e.swallow.button", eco->close);
   
   edje_object_size_min_calc(eco->edje, &ew, &eh);
   e_win_resize(eco->win, ew, eh);
   e_win_size_min_set(eco->win, ew, eh);

   evas_object_show(eco->edje);
   e_win_show(eco->win);
   e_win_border_icon_set(eco->win, "enlightenment/configuration");

   /* Preselect "Appearance" */
   e_widget_focus_set(eco->cat_list, 1);
   e_widget_ilist_selected_set(eco->cat_list, 0);
   
   _e_configure = eco;
   return eco;
}

EAPI void
e_configure_init(void)
{
   /* FIXME: hardcoded - need to move these into modules - except modules config */
//   e_configure_registry_category_add("appearance", 10, _("Appearance"), NULL, "enlightenment/appearance");
//   e_configure_registry_item_add("appearance/wallpaper", 10, _("Wallpaper"), NULL, "enlightenment/background", e_int_config_wallpaper);
//   e_configure_registry_item_add("appearance/theme", 20, _("Theme"), NULL, "enlightenment/themes", e_int_config_theme);
//   e_configure_registry_item_add("appearance/colors", 30, _("Colors"), NULL, "enlightenment/colors", e_int_config_color_classes);
//   e_configure_registry_item_add("appearance/fonts", 40, _("Fonts"), NULL, "enlightenment/fonts", e_int_config_fonts);
//   e_configure_registry_item_add("appearance/borders", 50, _("Borders"), NULL, "enlightenment/windows", e_int_config_borders);
//   e_configure_registry_item_add("appearance/icon_theme", 60, _("Icon Theme"), NULL, "enlightenment/icon_theme", e_int_config_icon_themes);
//   e_configure_registry_item_add("appearance/mouse_cursor", 70, _("Mouse Cursor"), NULL, "enlightenment/mouse", e_int_config_cursor);
//   e_configure_registry_item_add("appearance/transitions", 80, _("Transitions"), NULL, "enlightenment/transitions", e_int_config_transitions);
//   e_configure_registry_item_add("appearance/startup", 90, _("Startup"), NULL, "enlightenment/startup", e_int_config_startup);
   
//   e_configure_registry_category_add("applications", 20, _("Applications"), NULL, "enlightenment/applications");
//   e_configure_registry_item_add("applications/new_application", 10, _("New Application"), NULL, "enlightenment/add_application", e_int_config_apps_add);
//   e_configure_registry_item_add("applications/ibar_applications", 20, _("IBar Applications"), NULL, "enlightenment/ibar_applications", e_int_config_apps_ibar);
//   e_configure_registry_item_add("applications/restart_applications", 30, _("Restart Applications"), NULL, "enlightenment/restart_applications", e_int_config_apps_restart);
//   e_configure_registry_item_add("applications/startup_applications", 40, _("Startup Applications"), NULL, "enlightenment/startup_applications", e_int_config_apps_startup);
   
//   e_configure_registry_category_add("screen", 30, _("Screen"), NULL, "enlightenment/screen_setup");
//   e_configure_registry_item_add("screen/virtual_desktops", 10, _("Virtual Desktops"), NULL, "enlightenment/desktops", e_int_config_desks);
//   e_configure_registry_item_add("screen/screen_resolution", 20, _("Screen Resolution"), NULL, "enlightenment/screen_resolution", e_int_config_display);
//   e_configure_registry_item_add("screen/screen_lock", 30, _("Screen Lock"), NULL, "enlightenment/desklock", e_int_config_desklock);
//   e_configure_registry_item_add("screen/screen_saver", 40, _("Screen Saver"), NULL, "enlightenment/screensaver", e_int_config_screensaver);
//   e_configure_registry_item_add("screen/power_management", 50, _("Power Management"), NULL, "enlightenment/power_management", e_int_config_dpms);
   
//   e_configure_registry_category_add("keyboard_and_mouse", 40, _("Keyboard & Mouse"), NULL, "enlightenment/behavior");
//   e_configure_registry_item_add("keyboard_and_mouse/key_bindings", 10, _("Key Bindings"), NULL, "enlightenment/keys", e_int_config_keybindings);
//   e_configure_registry_item_add("keyboard_and_mouse/mouse_bindings", 20, _("Mouse Bindings"), NULL, "enlightenment/mouse_clean", e_int_config_mousebindings);
//   e_configure_registry_item_add("keyboard_and_mouse/mouse_acceleration", 30, _("Mouse Acceleration"), NULL, "enlightenment/mouse_clean", e_int_config_mouse);
   
//   e_configure_registry_category_add("windows", 50, _("Windows"), NULL, "enlightenment/windows");
//   e_configure_registry_item_add("windows/window_display", 10, _("Window Display"), NULL, "enlightenment/windows", e_int_config_window_display);
//   e_configure_registry_item_add("windows/window_focus", 20, _("Window Focus"), NULL, "enlightenment/focus", e_int_config_focus);
//   e_configure_registry_item_add("windows/window_manipulation", 30, _("Window Manipulation"), NULL, "enlightenment/window_manipulation", e_int_config_window_manipulation);
   
   e_configure_registry_category_add("menus", 60, _("Menus"), NULL, "enlightenment/menus");
////   e_configure_registry_item_add("menus/favorites_menu", 10, _("Favorites Menu"), NULL, "enlightenment/favorites", e_int_config_apps_favs);   
#if 0
////   e_configure_registry_item_add("menus/applications_menu", 20, _("Application Menus"), NULL, "enlightenment/applications", e_int_config_apps);
#endif
   e_configure_registry_item_add("menus/menu_settings", 30, _("Menu Settings"), NULL, "enlightenment/menu_settings", e_int_config_menus);
   e_configure_registry_item_add("menus/client_list_menu", 40, _("Client List Menu"), NULL, "enlightenment/windows", e_int_config_clientlist);
   
   e_configure_registry_category_add("advanced", 80, _("Advanced"), NULL, "enlightenment/advanced");
   e_configure_registry_item_add("advanced/dialogs", 10, _("Dialogs"), NULL, "enlightenment/configuration", e_int_config_dialogs);
   e_configure_registry_item_add("advanced/performance", 20, _("Performance"), NULL, "enlightenment/performance", e_int_config_performance);   
   e_configure_registry_item_add("advanced/window_list", 30, _("Window List"), NULL, "enlightenment/winlist", e_int_config_winlist);
   e_configure_registry_item_add("advanced/run_command", 40, _("Run Command"), NULL, "enlightenment/run", e_int_config_exebuf);
   e_configure_registry_item_add("advanced/search_directories", 50, _("Search Directories"), NULL, "enlightenment/directories", e_int_config_paths);
   e_configure_registry_item_add("advanced/file_icons", 60, _("File Icons"), NULL, "enlightenment/file_icons", e_int_config_mime);
   
   e_configure_registry_category_add("extensions", 90, _("Extensions"), NULL, "enlightenment/extensions");
   e_configure_registry_item_add("extensions/modules", 10, _("Modules"), NULL, "enlightenment/modules", e_int_config_modules);
//   e_configure_registry_item_add("extensions/shelves", 20, _("Shelves"), NULL, "enlightenment/shelf", e_int_config_shelf);

   /* internal calls - not in config dialog but accessible from other code
    * that knows these config dialogs exist and how to interact. they require
    * parameters to be passed and will not work without them being set and
    * set properly
    */
//   e_configure_registry_category_add("internal", -1, _("Internal"), NULL, "enlightenment/internal");
//   e_configure_registry_item_add("internal/borders_border", -1, _("Border"), NULL, "enlightenment/windows", e_int_config_borders_border);
//   e_configure_registry_item_add("internal/wallpaper_desk", -1, _("Wallpaper"), NULL, "enlightenment/windows", e_int_config_wallpaper_desk);
//   e_configure_registry_item_add("internal/desk", -1, _("Desk"), NULL, "enlightenment/windows", e_int_config_desk);
//   e_configure_registry_item_add("internal/ibar_other", -1, _("IBar Other"), NULL, "enlightenment/windows", e_int_config_apps_ibar_other);
}

static void 
_e_configure_free(E_Configure *eco) 
{
   _e_configure = NULL;
   while (eco->cats) 
     {
	E_Configure_Category *cat;
	
	cat = eco->cats->data;
	if (!cat) continue;
	if (cat->label)
	  evas_stringshare_del(cat->label);
	
	while (cat->items) 
	  {
	     E_Configure_Item *ci;
	     
	     ci = cat->items->data;
	     if (!ci) continue;
	     if (ci->label)
	       evas_stringshare_del(ci->label);
	     if (ci->icon)
	       evas_stringshare_del(ci->icon);
	     if (ci->cb)
	       {
		  if (ci->cb->path)
		    evas_stringshare_del(ci->cb->path);
		  free(ci->cb);
	       }
	     cat->items = evas_list_remove_list(cat->items, cat->items);
	     E_FREE(ci);
	  }
	eco->cats = evas_list_remove_list(eco->cats, eco->cats);
	E_FREE(cat);
     }
   evas_object_del(eco->close);
   evas_object_del(eco->cat_list);
   evas_object_del(eco->item_list);
   evas_object_del(eco->o_list);
   evas_object_del(eco->edje);
   e_object_del(E_OBJECT(eco->win));
   free(eco);
}

static void 
_e_configure_cb_del_req(E_Win *win) 
{
   E_Configure *eco;
   
   eco = win->data;
   if (!eco) return;
   e_object_del(E_OBJECT(eco));
}

static void 
_e_configure_cb_resize(E_Win *win) 
{
   E_Configure *eco;
   Evas_Coord w, h;

   eco = win->data;
   if (!eco) return;
   ecore_evas_geometry_get(win->ecore_evas, NULL, NULL, &w, &h);
   evas_object_resize(eco->edje, w, h);
}

static void 
_e_configure_cb_close(void *data, void *data2) 
{
   E_Configure *eco;
   
   eco = data;
   if (!eco) return;
   e_util_defer_object_del(E_OBJECT(eco));
}

static E_Configure_Category *
_e_configure_category_add(E_Configure *eco, const char *label, const char *icon)
{
   Evas_Object *o = NULL;
   E_Configure_Category *cat;
   
   if (!eco) return NULL;
   if (!label) return NULL;

   cat = E_NEW(E_Configure_Category, 1);
   cat->eco = eco;
   cat->label = evas_stringshare_add(label);
   if (icon) 
     {
	o = edje_object_add(eco->evas);
	e_util_edje_icon_set(o, icon);
     }
   eco->cats = evas_list_append(eco->cats, cat);

   e_widget_ilist_append(eco->cat_list, o, label, _e_configure_category_cb, cat, NULL);
   return cat;
}

static void 
_e_configure_category_cb(void *data) 
{
   E_Configure_Category *cat;
   E_Configure *eco;
   Evas_List *l;
   Evas_Coord w, h;
   
   cat = data;
   if (!cat) return;
   eco = cat->eco;

   evas_event_freeze(evas_object_evas_get(eco->item_list));
   edje_freeze();
   e_widget_ilist_freeze(eco->item_list);
   e_widget_ilist_clear(eco->item_list);
   for (l = cat->items; l; l = l->next) 
     {
	E_Configure_Item *ci;
	Evas_Object *o = NULL;
	
	ci = l->data;
	if (!ci) continue;
	if (ci->icon) 
	  {
	     o = edje_object_add(eco->evas);
	     e_util_edje_icon_set(o, ci->icon);
	  }
	e_widget_ilist_append(eco->item_list, o, ci->label, _e_configure_item_cb, ci, NULL);
     }
   e_widget_ilist_go(eco->item_list);
   e_widget_min_size_get(eco->item_list, &w, &h);
   e_widget_min_size_set(eco->item_list, w, h);
   e_widget_ilist_thaw(eco->item_list);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(eco->item_list));
}

static void
_e_configure_item_add(E_Configure_Category *cat, const char *label, const char *icon, const char *path)
{
   E_Configure_Item *ci;
   E_Configure_CB *cb;
   
   if ((!cat) || (!label)) return;
   
   ci = E_NEW(E_Configure_Item, 1);
   cb = E_NEW(E_Configure_CB, 1);
   cb->eco = cat->eco;
   cb->path = evas_stringshare_add(path);
   ci->cb = cb;
   ci->label = evas_stringshare_add(label);
   if (icon) ci->icon = evas_stringshare_add(icon);
   cat->items = evas_list_append(cat->items, ci);
}

static void 
_e_configure_item_cb(void *data) 
{
   E_Configure_Item *ci;
   E_Configure_CB *cb;
   
   ci = data;
   if (!ci) return;
   cb = ci->cb;
   if (cb->path) e_configure_registry_call(cb->path, cb->eco->con, NULL);
}

static void 
_e_configure_focus_cb(void *data, Evas_Object *obj) 
{
   E_Win *win;
   E_Configure *eco;
   
   win = data;
   eco = win->data;
   if (!eco) return;
   if (obj == eco->close) 
     {
	e_widget_focused_object_clear(eco->item_list);
	e_widget_focused_object_clear(eco->cat_list);
     }
   else if (obj == eco->item_list) 
     {
	e_widget_focused_object_clear(eco->cat_list);
	e_widget_focused_object_clear(eco->close);
     }
   else if (obj == eco->cat_list) 
     {
	e_widget_focused_object_clear(eco->item_list);
	e_widget_focused_object_clear(eco->close);
     }
}

static void 
_e_configure_keydown_cb(void *data, Evas *e, Evas_Object *obj, void *event) 
{
   Evas_Event_Key_Down *ev;
   E_Win *win;
   E_Configure *eco;
   
   ev = event;
   win = data;
   eco = win->data;

   if (!strcmp(ev->keyname, "Tab")) 
     {
	if (evas_key_modifier_is_set(evas_key_modifier_get(e_win_evas_get(win)), "Shift"))
	  {
	     if (e_widget_focus_get(eco->close)) 
	       e_widget_focus_set(eco->item_list, 0);
	     else if (e_widget_focus_get(eco->item_list))
	       e_widget_focus_set(eco->cat_list, 1);
	     else if (e_widget_focus_get(eco->cat_list))
	       e_widget_focus_set(eco->close, 0);
	  }
	else 
	  {
	     if (e_widget_focus_get(eco->close)) 
	       e_widget_focus_set(eco->cat_list, 1);
	     else if (e_widget_focus_get(eco->item_list))
	       e_widget_focus_set(eco->close, 0);
	     else if (e_widget_focus_get(eco->cat_list))
	       e_widget_focus_set(eco->item_list, 0);
	  }
     }
   else if (((!strcmp(ev->keyname, "Return")) || 
	    (!strcmp(ev->keyname, "KP_Enter")) || 
	    (!strcmp(ev->keyname, "space"))))
     {
	Evas_Object *o = NULL;

	if (e_widget_focus_get(eco->cat_list))
	  o = eco->cat_list;
	else if (e_widget_focus_get(eco->item_list))
	  o = eco->item_list;
	else if (e_widget_focus_get(eco->close))
	  o = eco->close;
	
	if (o) 
	  {
	     o = e_widget_focused_object_get(o);
	     if (!o) return;
	     e_widget_activate(o);
	  }
     }
}

static void 
_e_configure_fill_cat_list(void *data) 
{
   E_Configure *eco;
   Evas_Coord mw, mh;
   E_Configure_Category *cat;
   Evas_List *l;

   eco = data;
   if (!eco) return;

   evas_event_freeze(evas_object_evas_get(eco->cat_list));
   edje_freeze();
   e_widget_ilist_freeze(eco->cat_list);

   for (l = _e_configure_registry; l; l = l->next)
     {
	Evas_List *ll;
	E_Configure_Cat *ecat;
	
	ecat = l->data;
	if ((ecat->pri >= 0) && (ecat->items))
	  {
	     cat = _e_configure_category_add(eco, ecat->label, ecat->icon);
	     for (ll = ecat->items; ll; ll = ll->next)
	       {
		  E_Configure_It *eci;
		  char buf[1024];
		  
		  eci = ll->data;
		  if (eci->pri >= 0)
		    {
		       snprintf(buf, sizeof(buf), "%s/%s", ecat->cat, eci->item);
		       _e_configure_item_add(cat, eci->label, eci->icon, buf);
		    }
	       }
	  }
     }
   
   e_widget_ilist_go(eco->cat_list);
   e_widget_min_size_get(eco->cat_list, &mw, &mh);
   e_widget_min_size_set(eco->cat_list, mw, mh);
   e_widget_ilist_thaw(eco->cat_list);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(eco->cat_list));
}
