#include "e.h"

static void _e_configure_free(E_Configure *eco);
static void _e_configure_cb_del_req(E_Win *win);
static void _e_configure_cb_resize(E_Win *win);
static void _e_configure_cb_close(void *data, void *data2);
static E_Configure_Category *_e_configure_category_add(E_Configure *eco, char *label, char *icon);
static void _e_configure_category_cb(void *data);
static void _e_configure_item_add(E_Configure_Category *cat, char *label, char *icon, E_Config_Dialog *(*func) (E_Container *con));
static void _e_configure_item_cb(void *data);
static void _e_configure_focus_cb(void *data, Evas_Object *obj);
static void _e_configure_keydown_cb(void *data, Evas *e, Evas_Object *obj, void *event);
static void _e_configure_fill_cat_list(void *data);

static E_Configure *_e_configure = NULL;

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
   of = e_widget_framelist_add(eco->evas, _("Configuration Items"), 1);
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
	       free(ci->cb);
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
   e_object_del(E_OBJECT(eco));
}

static E_Configure_Category *
_e_configure_category_add(E_Configure *eco, char *label, char *icon)
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
_e_configure_item_add(E_Configure_Category *cat, char *label, char *icon, E_Config_Dialog *(*func) (E_Container *con))
{
   E_Configure_Item *ci;
   E_Configure_CB *cb;
   
   if ((!cat) || (!label)) return;
   
   ci = E_NEW(E_Configure_Item, 1);
   cb = E_NEW(E_Configure_CB, 1);
   cb->eco = cat->eco;
   cb->func = func;
   ci->cb = cb;
   ci->label = evas_stringshare_add(label);
   if (icon)
     ci->icon = evas_stringshare_add(icon);
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
   cb->func(cb->eco->con);
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

   eco = data;
   if (!eco) return;

   evas_event_freeze(evas_object_evas_get(eco->cat_list));
   edje_freeze();
   e_widget_ilist_freeze(eco->cat_list);
   
   /* Add "Categories" & "Items" Here */
   cat = _e_configure_category_add(eco, _("Appearance"), "enlightenment/appearance");
   _e_configure_item_add(cat, _("Wallpaper"), "enlightenment/background", e_int_config_wallpaper);
   _e_configure_item_add(cat, _("Theme"), "enlightenment/themes", e_int_config_theme);
   _e_configure_item_add(cat, _("Colors"), "enlightenment/colors", e_int_config_color_classes);
   _e_configure_item_add(cat, _("Fonts"), "enlightenment/fonts", e_int_config_fonts);
   _e_configure_item_add(cat, _("Borders"), "enlightenment/windows", e_int_config_borders);
   _e_configure_item_add(cat, _("Icon Theme"), "enlightenment/icon_theme", e_int_config_icon_themes);
   _e_configure_item_add(cat, _("Mouse Cursor"), "enlightenment/mouse", e_int_config_cursor);
   _e_configure_item_add(cat, _("Transitions"), "enlightenment/transitions", e_int_config_transitions);
   _e_configure_item_add(cat, _("Startup"), "enlightenment/startup", e_int_config_startup);

   cat = _e_configure_category_add(eco, _("Applications"), "enlightenment/applications");
   _e_configure_item_add(cat, _("IBar Applications"), "enlightenment/ibar_applications", e_int_config_apps_ibar);   
   _e_configure_item_add(cat, _("Restart Applications"), "enlightenment/restart_applications", e_int_config_apps_restart);
   _e_configure_item_add(cat, _("Startup Applications"), "enlightenment/startup_applications", e_int_config_apps_startup);
   
   cat = _e_configure_category_add(eco, _("Screen"), "enlightenment/screen_setup");
   _e_configure_item_add(cat, _("Virtual Desktops"), "enlightenment/desktops", e_int_config_desks);
   _e_configure_item_add(cat, _("Screen Resolution"), "enlightenment/screen_resolution", e_int_config_display);
   _e_configure_item_add(cat, _("Screen Lock"), "enlightenment/desklock", e_int_config_desklock);
   
   cat = _e_configure_category_add(eco, _("Keyboard & Mouse"), "enlightenment/behavior");
   _e_configure_item_add(cat, _("Key Bindings"), "enlightenment/keys", e_int_config_keybindings);
   _e_configure_item_add(cat, _("Mouse Bindings"), "enlightenment/mouse_clean", e_int_config_mousebindings);

   cat = _e_configure_category_add(eco, _("Windows"), "enlightenment/windows");
   _e_configure_item_add(cat, _("Window Display"), "enlightenment/windows", e_int_config_window_display);
   _e_configure_item_add(cat, _("Window Focus"), "enlightenment/focus", e_int_config_focus);
   _e_configure_item_add(cat, _("Window Manipulation"), "enlightenment/window_manipulation", e_int_config_window_manipulation);

   cat = _e_configure_category_add(eco, _("Menus"), "enlightenment/menus");
#ifdef ENABLE_FAVORITES
   _e_configure_item_add(cat, _("Application Menus"), "enlightenment/applications", e_int_config_apps);
#else
   _e_configure_item_add(cat, _("Applications Menu"), "enlightenment/applications", e_int_config_apps);
#endif
   _e_configure_item_add(cat, _("Menu Settings"), "enlightenment/menu_settings", e_int_config_menus);

   cat = _e_configure_category_add(eco, _("Language"), "enlightenment/intl");
   _e_configure_item_add(cat, _("Language Settings"), "enlightenment/intl", e_int_config_intl);
   _e_configure_item_add(cat, _("Input Method Settings"), "enlightenment/imc", e_int_config_imc);

   cat = _e_configure_category_add(eco, _("Advanced"), "enlightenment/advanced");
   _e_configure_item_add(cat, _("Configuration Dialogs"), "enlightenment/configuration", e_int_config_cfgdialogs);
   _e_configure_item_add(cat, _("Performance"), "enlightenment/performance", e_int_config_performance);   
   _e_configure_item_add(cat, _("Window List"), "enlightenment/winlist", e_int_config_winlist);
   _e_configure_item_add(cat, _("Run Command"), "enlightenment/run", e_int_config_exebuf);
   _e_configure_item_add(cat, _("Search Directories"), "enlightenment/directories", e_int_config_paths);
   _e_configure_item_add(cat, _("File Icons"), "enlightenment/file_icons", e_int_config_mime);

   cat = _e_configure_category_add(eco, _("Extensions"), "enlightenment/extensions");
   _e_configure_item_add(cat, _("Modules"), "enlightenment/modules", e_int_config_modules);
   _e_configure_item_add(cat, _("Shelves"), "enlightenment/shelf", e_int_config_shelf);

   /* FIXME: we should have a way for modules to hook in here and add their own entries 
    * 
    * cat = _e_configure_category_add(eco, _("Extension Configuration"), "enlightenment/extension_config");
    */
   
   e_widget_ilist_go(eco->cat_list);
   e_widget_min_size_get(eco->cat_list, &mw, &mh);
   e_widget_min_size_set(eco->cat_list, mw, mh);
   e_widget_ilist_thaw(eco->cat_list);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(eco->cat_list));
}
