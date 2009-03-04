#include "e.h"

typedef struct _E_Configure E_Configure;
typedef struct _E_Configure_CB E_Configure_CB;
typedef struct _E_Configure_Category E_Configure_Category;
typedef struct _E_Configure_Item E_Configure_Item;

#define E_CONFIGURE_TYPE 0xE0b01014

struct _E_Configure
{
   E_Object e_obj_inherit;
   
   E_Container *con;
   E_Win *win;
   Evas *evas;
   Evas_Object *edje;
   
   Evas_Object *o_list;
   Evas_Object *cat_list;
   Evas_Object *item_list;
   Evas_Object *close;
   
   Eina_List *cats;
   Ecore_Event_Handler *mod_hdl;
};

struct _E_Configure_CB
{
   E_Configure *eco;
   const char *path;
};

struct _E_Configure_Category
{
   E_Configure *eco;
   const char *label;
   
   Eina_List *items;
};

struct _E_Configure_Item
{
   E_Configure_CB *cb;
   
   const char *label;
   const char *icon;
};

static void _e_configure_free(E_Configure *eco);
static void _e_configure_cb_del_req(E_Win *win);
static void _e_configure_cb_resize(E_Win *win);
static void _e_configure_cb_close(void *data, void *data2);
static E_Configure_Category *_e_configure_category_add(E_Configure *eco, const char *label, const char *icon);
static void _e_configure_category_cb(void *data, void *data2);
static void _e_configure_item_add(E_Configure_Category *cat, const char *label, const char *icon, const char *path);
static void _e_configure_item_cb(void *data);
static void _e_configure_focus_cb(void *data, Evas_Object *obj);
static void _e_configure_keydown_cb(void *data, Evas *e, Evas_Object *obj, void *event);
static void _e_configure_fill_cat_list(void *data);
static int  _e_configure_module_update_cb(void *data, int type, void *event);

static E_Configure *_e_configure = NULL;

EAPI void
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
	return;
     }
   
   if (!con) 
     {
	man = e_manager_current_get();
	if (!man) return;
	con = e_container_current_get(man);
	if (!con)
	  con = e_container_number_get(man, 0);
	if (!con) return;
     }

   eco = E_OBJECT_ALLOC(E_Configure, E_CONFIGURE_TYPE, _e_configure_free);
   if (!eco) return;
   eco->win = e_win_new(con);
   if (!eco->win) 
     {
	free(eco);
	return;
     }
   eco->win->data = eco;
   eco->con = con;
   eco->evas = e_win_evas_get(eco->win);

   /* Event Handler for Module Updates */
   eco->mod_hdl = ecore_event_handler_add(E_EVENT_MODULE_UPDATE, 
					  _e_configure_module_update_cb, eco);

   e_win_title_set(eco->win, _("Settings"));
   e_win_name_class_set(eco->win, "E", "_configure");
   e_win_dialog_set(eco->win, 0);
   e_win_delete_callback_set(eco->win, _e_configure_cb_del_req);
   e_win_resize_callback_set(eco->win, _e_configure_cb_resize);
   e_win_centered_set(eco->win, 1);

   eco->edje = edje_object_add(eco->evas);
   e_theme_edje_object_set(eco->edje, "base/theme/configure", 
			   "e/widgets/configure/main");
   edje_object_part_text_set(eco->edje, "e.text.title", 
			   _("Settings"));

   eco->o_list = e_widget_list_add(eco->evas, 0, 0);
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
   eco->cat_list = e_widget_toolbar_add(eco->evas, 32 * e_scale, 32 * e_scale);
   e_widget_toolbar_scrollable_set(eco->cat_list, 1);
   /***--- fill ---***/
   _e_configure_fill_cat_list(eco);
   e_widget_on_focus_hook_set(eco->cat_list, _e_configure_focus_cb, eco->win);
   e_widget_list_object_append(eco->o_list, eco->cat_list, 1, 0, 0.5);
   /* Item List */
   eco->item_list = e_widget_ilist_add(eco->evas, 32 * e_scale, 32 * e_scale, NULL);
   e_widget_ilist_selector_set(eco->item_list, 1);
   e_widget_ilist_go(eco->item_list);
   e_widget_on_focus_hook_set(eco->item_list, _e_configure_focus_cb, eco->win);
   e_widget_min_size_get(eco->item_list, &mw, &mh);
   if (mw < (200 * e_scale)) mw = 200 * e_scale;
   if (mh < (120 * e_scale)) mh = 120 * e_scale;
   e_widget_min_size_set(eco->item_list, mw, mh);
   e_widget_list_object_append(eco->o_list, eco->item_list, 1, 1, 0.5);

   e_widget_min_size_get(eco->o_list, &mw, &mh);
   edje_extern_object_min_size_set(eco->o_list, mw, mh);

   /* Close Button */
   eco->close = e_widget_button_add(eco->evas, _("Close"), NULL, 
				    _e_configure_cb_close, eco, NULL);
   e_widget_on_focus_hook_set(eco->close, _e_configure_focus_cb, eco->win);
   e_widget_min_size_get(eco->close, &mw, &mh);
   edje_extern_object_min_size_set(eco->close, mw, mh);
   edje_object_part_swallow(eco->edje, "e.swallow.button", eco->close);
   
   edje_object_size_min_calc(eco->edje, &ew, &eh);
   e_win_size_min_set(eco->win, ew, eh);
   e_util_win_auto_resize_fill(eco->win);

   evas_object_show(eco->edje);
   e_win_show(eco->win);
   e_win_border_icon_set(eco->win, "enlightenment/configuration");

   /* Preselect "Appearance" */
   e_widget_focus_set(eco->cat_list, 1);
   e_widget_toolbar_item_select(eco->cat_list, 0);

   if (eco->cats)
     {
	E_Configure_Category *cat;
	
	cat = eco->cats->data;
	_e_configure_category_cb(cat, NULL);
     }
   
   _e_configure = eco;
}

EAPI void 
e_configure_del(void) 
{
   if (_e_configure) 
     {
	if (_e_configure->mod_hdl) 
	  ecore_event_handler_del(_e_configure->mod_hdl);
	_e_configure->mod_hdl = NULL;
	e_object_del(E_OBJECT(_e_configure));
	_e_configure = NULL;
     }
}

static void 
_e_configure_free(E_Configure *eco) 
{
   if (_e_configure->mod_hdl)
     ecore_event_handler_del(_e_configure->mod_hdl);
   _e_configure->mod_hdl = NULL;
   _e_configure = NULL;
   while (eco->cats) 
     {
	E_Configure_Category *cat;
	
	cat = eco->cats->data;
	if (!cat) continue;
	if (cat->label)
	  eina_stringshare_del(cat->label);
	
	while (cat->items) 
	  {
	     E_Configure_Item *ci;
	     
	     ci = cat->items->data;
	     if (!ci) continue;
	     if (ci->label)
	       eina_stringshare_del(ci->label);
	     if (ci->icon)
	       eina_stringshare_del(ci->icon);
	     if (ci->cb)
	       {
		  if (ci->cb->path)
		    eina_stringshare_del(ci->cb->path);
		  free(ci->cb);
	       }
	     cat->items = eina_list_remove_list(cat->items, cat->items);
	     E_FREE(ci);
	  }
	eco->cats = eina_list_remove_list(eco->cats, eco->cats);
	E_FREE(cat);
     }
   evas_object_del(eco->close);
   evas_object_del(eco->cat_list);
   evas_object_del(eco->item_list);
   evas_object_del(eco->o_list);
   evas_object_del(eco->edje);
   e_object_del(E_OBJECT(eco->win));
   E_FREE(eco);
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
   cat->label = eina_stringshare_add(label);
   if (icon) 
     {
	if (e_util_edje_icon_check(icon)) 
	  {
	     o = edje_object_add(eco->evas);
	     e_util_edje_icon_set(o, icon);
	  }
	else
	  o = e_util_icon_add(icon, eco->evas);
     }
   eco->cats = eina_list_append(eco->cats, cat);

   e_widget_toolbar_item_append(eco->cat_list, o, label, _e_configure_category_cb, cat, NULL);
   return cat;
}

static void 
_e_configure_category_cb(void *data, void *data2) 
{
   E_Configure_Category *cat;
   E_Configure *eco;
   Eina_List *l;
   Evas_Coord w, h;
   
   cat = data;
   if (!cat) return;
   eco = cat->eco;

   evas_event_freeze(evas_object_evas_get(eco->item_list));
   edje_freeze();
   e_widget_ilist_freeze(eco->item_list);
   e_widget_ilist_clear(eco->item_list);
   /***--- fill ---***/
   for (l = cat->items; l; l = l->next) 
     {
	E_Configure_Item *ci;
	Evas_Object *o = NULL;
	
	ci = l->data;
	if (!ci) continue;
	if (ci->icon) 
	  {
	     o = e_icon_add(eco->evas);
	     if (!e_util_icon_theme_set(o, ci->icon))
	       {
		  evas_object_del(o);
		  o = e_util_icon_add(ci->icon, eco->evas);
	       }
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
   cb->path = eina_stringshare_add(path);
   ci->cb = cb;
   ci->label = eina_stringshare_add(label);
   if (icon) ci->icon = eina_stringshare_add(icon);
   cat->items = eina_list_append(cat->items, ci);
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
   Eina_List *l;

   eco = data;
   if (!eco) return;

   evas_event_freeze(evas_object_evas_get(eco->cat_list));
   edje_freeze();

   for (l = e_configure_registry; l; l = l->next)
     {
	Eina_List *ll;
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
   
   e_widget_min_size_get(eco->cat_list, &mw, &mh);
   e_widget_min_size_set(eco->cat_list, mw, mh);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(eco->cat_list));
}

static int 
_e_configure_module_update_cb(void *data, int type, void *event) 
{
   E_Configure *eco;
   int sel = 0;

   if (!(eco = data)) return 1;
   if (!eco->cat_list) return 1;
   sel = e_widget_ilist_selected_get(eco->cat_list);
   _e_configure_fill_cat_list(eco);
   e_widget_ilist_selected_set(eco->cat_list, sel);
   return 1;
}
