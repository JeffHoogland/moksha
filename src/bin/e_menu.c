/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* TODO List:
 * 
 * * support change of menu items after realize
 * * support add/del of menu items after realize
 * * support text/color classes
 * * refcount menu up while looping thru and calling other fn's
 * * support alignment (x, y) as well as spawn direction
 * * need different menu style support for different menus
 * * add menu icon/title support
 * * use event timestamps not clock for "click and release" detect
 * * menu icons can set if/how they will be scaled
 * * support move/resize of "box" that spawned the menu
 * * add image item (label is replaced by image/icon)
 * * add generic evas object item type (label replaced by object)
 * * allow menus to stretch width/height to fit spawner widget/box
 * * allow menus to auto-shrink (horizontally) if forced to
 * * support auto left/right direction spawn
 * * support obscures to indicate offscreen/not visible menu parts
 */

/* local subsystem data types */
typedef struct _E_Menu_Category E_Menu_Category;

struct _E_Menu_Category
{
	void *data;
	Eina_List *callbacks;
};

/* local subsystem functions */
static void _e_menu_free                          (E_Menu *m);
static void _e_menu_item_free                     (E_Menu_Item *mi);
static void _e_menu_item_realize                  (E_Menu_Item *mi);
static void _e_menu_realize                       (E_Menu *m);
static void _e_menu_items_layout_update           (E_Menu *m);
static void _e_menu_item_unrealize                (E_Menu_Item *mi);
static void _e_menu_unrealize                     (E_Menu *m);
static void _e_menu_activate_internal             (E_Menu *m, E_Zone *zone);
static void _e_menu_deactivate_all                (void);
static void _e_menu_deactivate_above              (E_Menu *m);
static void _e_menu_submenu_activate              (E_Menu_Item *mi);
static void _e_menu_submenu_deactivate            (E_Menu_Item *mi);
static void _e_menu_reposition                    (E_Menu *m);
static int  _e_menu_active_call                   (void);
static int _e_menu_realize_call                   (E_Menu_Item *mi);
static void _e_menu_item_activate_next            (void);
static void _e_menu_item_activate_previous        (void);
static void _e_menu_item_activate_first           (void);
static void _e_menu_item_activate_last            (void);
static void _e_menu_item_activate_nth             (int n);
static void _e_menu_item_activate_char            (char * key_compose);
static void _e_menu_activate_next                 (void);
static void _e_menu_activate_previous             (void);
static void _e_menu_activate_first                (void);
static void _e_menu_activate_last                 (void);
#if 0
static void _e_menu_activate_nth                  (int n);
#endif
static E_Menu *_e_menu_active_get                 (void);
static E_Menu_Item *_e_menu_item_active_get       (void);
static Eina_List * _e_menu_list_item_active_get   (void);
static int  _e_menu_outside_bounds_get            (int xdir, int ydir);
static void _e_menu_scroll_by                     (int dx, int dy);
static void _e_menu_mouse_autoscroll_check        (void);
static void _e_menu_item_ensure_onscreen          (E_Menu_Item *mi);
static int  _e_menu_auto_place                    (E_Menu *m, int x, int y, int w, int h);
static void _e_menu_cb_intercept_item_move        (void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y);
static void _e_menu_cb_intercept_item_resize      (void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h);
static void _e_menu_cb_intercept_container_move   (void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y);
static void _e_menu_cb_intercept_container_resize (void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h);
static void _e_menu_cb_ecore_evas_resize          (Ecore_Evas *ee);
static void _e_menu_cb_item_in                    (void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_menu_cb_item_out                   (void *data, Evas *evas, Evas_Object *obj, void *event_info);
static int  _e_menu_cb_key_down                   (void *data, int type, void *event);
static int  _e_menu_cb_key_up                     (void *data, int type, void *event);
static int  _e_menu_cb_mouse_down                 (void *data, int type, void *event);
static int  _e_menu_cb_mouse_up                   (void *data, int type, void *event);
static int  _e_menu_cb_mouse_move                 (void *data, int type, void *event);
static int  _e_menu_cb_mouse_wheel                (void *data, int type, void *event);
static int  _e_menu_cb_scroll_animator            (void *data);
static int  _e_menu_cb_window_shape               (void *data, int ev_type, void *ev);

static void _e_menu_cb_item_submenu_post_default  (void *data, E_Menu *m, E_Menu_Item *mi);
static Evas_Bool _e_menu_categories_free_cb(const Evas_Hash *hash, const void *key, void *data, void *fdata);

/* local subsystem globals */
static Ecore_X_Window       _e_menu_win                 = 0;
static Eina_List           *_e_active_menus             = NULL;
static E_Menu_Item         *_e_active_menu_item         = NULL;
/*static Evas_Hash	   *_e_menu_category_items	= NULL;*/
static Evas_Hash	   *_e_menu_categories		= NULL;
static Ecore_X_Time         _e_menu_activate_time       = 0;
static int                  _e_menu_activate_floating   = 0;
static int                  _e_menu_activate_maybe_drag = 0;
static int                  _e_menu_activate_dragging   = 0;
static Ecore_Animator      *_e_menu_scroll_animator     = NULL;
static double               _e_menu_scroll_start        = 0.0;
static int                  _e_menu_x                   = 0;
static int                  _e_menu_y                   = 0;
static Ecore_X_Time         _e_menu_time                = 0;
static int                  _e_menu_autoscroll_x        = 0;
static int                  _e_menu_autoscroll_y        = 0;
static Ecore_Event_Handler *_e_menu_key_down_handler     = NULL;
static Ecore_Event_Handler *_e_menu_key_up_handler       = NULL;
static Ecore_Event_Handler *_e_menu_mouse_down_handler   = NULL;
static Ecore_Event_Handler *_e_menu_mouse_up_handler     = NULL;
static Ecore_Event_Handler *_e_menu_mouse_move_handler   = NULL;
static Ecore_Event_Handler *_e_menu_mouse_wheel_handler  = NULL;
static Ecore_Event_Handler *_e_menu_window_shape_handler = NULL;

/* externally accessible functions */
EAPI int
e_menu_init(void)
{
   _e_menu_key_down_handler     = ecore_event_handler_add(ECORE_X_EVENT_KEY_DOWN,          _e_menu_cb_key_down,    NULL);
   _e_menu_key_up_handler       = ecore_event_handler_add(ECORE_X_EVENT_KEY_UP,            _e_menu_cb_key_up,      NULL);
   _e_menu_mouse_down_handler   = ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_DOWN, _e_menu_cb_mouse_down,  NULL);
   _e_menu_mouse_up_handler     = ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_UP,   _e_menu_cb_mouse_up,    NULL);
   _e_menu_mouse_move_handler   = ecore_event_handler_add(ECORE_X_EVENT_MOUSE_MOVE,        _e_menu_cb_mouse_move,  NULL);
   _e_menu_mouse_wheel_handler  = ecore_event_handler_add(ECORE_X_EVENT_MOUSE_WHEEL,       _e_menu_cb_mouse_wheel, NULL);
   _e_menu_window_shape_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHAPE,     _e_menu_cb_window_shape, NULL);
   return 1;
}

EAPI int
e_menu_shutdown(void)
{
   E_FN_DEL(ecore_event_handler_del, _e_menu_key_down_handler);
   E_FN_DEL(ecore_event_handler_del, _e_menu_key_up_handler);
   E_FN_DEL(ecore_event_handler_del, _e_menu_mouse_down_handler);
   E_FN_DEL(ecore_event_handler_del, _e_menu_mouse_up_handler);
   E_FN_DEL(ecore_event_handler_del, _e_menu_mouse_move_handler);
   E_FN_DEL(ecore_event_handler_del, _e_menu_mouse_wheel_handler);
   E_FN_DEL(ecore_event_handler_del, _e_menu_window_shape_handler);

   while (_e_active_menus)
     {
	E_Menu *m;
	
	m = _e_active_menus->data;
	m->active = 0;
	_e_menu_unrealize(m);
	_e_active_menus = eina_list_remove_list(_e_active_menus, _e_active_menus);

	m->in_active_list = 0;
	e_object_unref(E_OBJECT(m));
     }
   _e_active_menus = NULL;
   if (_e_menu_categories)
     {
	evas_hash_foreach(_e_menu_categories, _e_menu_categories_free_cb, NULL);
	evas_hash_free(_e_menu_categories);
	_e_menu_categories = NULL;
     }
  
   return 1;
}

EAPI E_Menu *
e_menu_new(void)
{
   E_Menu *m;
   
   m = E_OBJECT_ALLOC(E_Menu, E_MENU_TYPE, _e_menu_free);
   if (!m) return NULL;
   m->cur.w = 1;
   m->cur.h = 1;
   m->category = NULL;
   return m;
}


EAPI void
e_menu_activate_key(E_Menu *m, E_Zone *zone, int x, int y, int w, int h, int dir)
{
   E_OBJECT_CHECK(m);
   E_OBJECT_TYPE_CHECK(m, E_MENU_TYPE);
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);
   _e_menu_activate_time = 0;
   _e_menu_activate_floating = 0;
   _e_menu_activate_internal(m, zone);
   if (!m->zone)
     {
	e_menu_deactivate(m);
	return;
     }
   switch (dir)
     {
      case E_MENU_POP_DIRECTION_LEFT:
	_e_menu_realize(m);
	m->cur.x = x - m->cur.w;
	m->cur.y = y;
	if ((m->cur.y + m->cur.h) > (m->zone->y + m->zone->h))
	  m->cur.y = y + h - m->cur.h;
	_e_menu_activate_first();
	break;
      case E_MENU_POP_DIRECTION_RIGHT:
	m->cur.x = x + w;
	m->cur.y = y ;
	_e_menu_activate_first();
	break;
      case E_MENU_POP_DIRECTION_UP:
	_e_menu_realize(m);
	m->cur.x = x;
	if ((m->cur.x + m->cur.w) > (m->zone->x + m->zone->w))
	  m->cur.x = x + w - m->cur.w;
	m->cur.y = y - m->cur.h;
	_e_menu_activate_last();
	break;
      case E_MENU_POP_DIRECTION_DOWN:
	_e_menu_realize(m);
	m->cur.x = x;
	if ((m->cur.x + m->cur.w) > (m->zone->x + m->zone->w))
	  m->cur.x = x + w - m->cur.w;
	m->cur.y = y + h;
	if ((m->cur.y + m->cur.h) > (m->zone->y + m->zone->h))
	  m->cur.y = y + h - m->cur.h;
	_e_menu_activate_first();
	break;
      case E_MENU_POP_DIRECTION_AUTO:
	  {
	     int pos;
	     
	     pos = _e_menu_auto_place(m, x, y, w, h);
	     if (pos == 4)
	       _e_menu_activate_last();
	     else
	       _e_menu_activate_first();
	  }
	break;
      default:
	m->cur.x = x + w;
	m->cur.y = y + h;
	_e_menu_activate_first();
	break;
     }
}

EAPI void
e_menu_activate_mouse(E_Menu *m, E_Zone *zone, int x, int y, int w, int h, int dir, Ecore_X_Time activate_time)
{
   E_Menu_Item *pmi;
   
   E_OBJECT_CHECK(m);
   E_OBJECT_TYPE_CHECK(m, E_MENU_TYPE);
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);
   _e_menu_activate_time = activate_time;
   _e_menu_activate_floating = 0;
   _e_menu_activate_internal(m, zone);
   if (!m->zone)
     {
	e_menu_deactivate(m);
	return;
     }
   switch (dir)
     {
      case E_MENU_POP_DIRECTION_LEFT:
	_e_menu_realize(m);
	m->cur.x = x - m->cur.w;
	m->cur.y = y;
	if ((m->cur.y + m->cur.h) > (m->zone->y + m->zone->h))
	  m->cur.y = y + h - m->cur.h;
	break;
      case E_MENU_POP_DIRECTION_RIGHT:
	m->cur.x = x + w;
	m->cur.y = y;
	break;
      case E_MENU_POP_DIRECTION_UP:
	_e_menu_realize(m);
	m->cur.x = x;
	if ((m->cur.x + m->cur.w) > (m->zone->x + m->zone->w))
	  m->cur.x = x + w - m->cur.w;
	m->cur.y = y - m->cur.h;
	break;
      case E_MENU_POP_DIRECTION_DOWN:
	_e_menu_realize(m);
	m->cur.x = x;
	if ((m->cur.x + m->cur.w) > (m->zone->x + m->zone->w))
	  m->cur.x = x + w - m->cur.w;
	m->cur.y = y + h;
	if ((m->cur.y + m->cur.h) > (m->zone->y + m->zone->h))
	  m->cur.y = y + h - m->cur.h;
	break;
      case E_MENU_POP_DIRECTION_AUTO:
	_e_menu_auto_place(m, x, y, w, h);
	break;
      default:
	m->cur.x = x + w;
	m->cur.y = y + h;
	break;
     }
   pmi = _e_menu_item_active_get();
   if (pmi) e_menu_item_active_set(pmi, 0);
}

EAPI void
e_menu_activate(E_Menu *m, E_Zone *zone, int x, int y, int w, int h, int dir)
{
   E_Menu_Item *pmi;

   E_OBJECT_CHECK(m);
   E_OBJECT_TYPE_CHECK(m, E_MENU_TYPE);
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);
   _e_menu_activate_time = 0;
   _e_menu_activate_floating = 0;
   _e_menu_activate_internal(m, zone);
   if (!m->zone)
     {
	e_menu_deactivate(m);
	return;
     }
   switch (dir)
     {
      case E_MENU_POP_DIRECTION_LEFT:
	_e_menu_realize(m);
	m->cur.x = x - m->cur.w;
	m->cur.y = y;
	if ((m->cur.y + m->cur.h) > (m->zone->y + m->zone->h))
	  m->cur.y = y + h - m->cur.h;
	_e_menu_activate_first();
	break;
      case E_MENU_POP_DIRECTION_RIGHT:
	m->cur.x = x + w;
	m->cur.y = y ;
	_e_menu_activate_first();
	break;
      case E_MENU_POP_DIRECTION_UP:
	_e_menu_realize(m);
	m->cur.x = x;
	if ((m->cur.x + m->cur.w) > (m->zone->x + m->zone->w))
	  m->cur.x = x + w - m->cur.w;
	m->cur.y = y - m->cur.h;
	_e_menu_activate_last();
	break;
      case E_MENU_POP_DIRECTION_DOWN:
	_e_menu_realize(m);
	m->cur.x = x;
	if ((m->cur.x + m->cur.w) > (m->zone->x + m->zone->w))
	  m->cur.x = x + w - m->cur.w;
	m->cur.y = y + h;
	if ((m->cur.y + m->cur.h) > (m->zone->y + m->zone->h))
	  m->cur.y = y + h - m->cur.h;
	_e_menu_activate_first();
	break;
      case E_MENU_POP_DIRECTION_AUTO:
	_e_menu_auto_place(m, x, y, w, h);
	break;
      default:
	m->cur.x = x + w;
	m->cur.y = y + h;
	break;
     }
   pmi = _e_menu_item_active_get();
   if (pmi) e_menu_item_active_set(pmi, 0);
}

EAPI void
e_menu_deactivate(E_Menu *m)
{
   E_OBJECT_CHECK(m);
   E_OBJECT_TYPE_CHECK(m, E_MENU_TYPE);
   m->cur.visible = 0;
   m->active = 0;
   if (m->post_deactivate_cb.func)
     m->post_deactivate_cb.func(m->post_deactivate_cb.data, m);
}

EAPI int
e_menu_freeze(E_Menu *m)
{
   E_OBJECT_CHECK_RETURN(m, 0);
   E_OBJECT_TYPE_CHECK_RETURN(m, E_MENU_TYPE, 0);
   m->frozen++;
   return m->frozen;
}

EAPI int
e_menu_thaw(E_Menu *m)
{
   E_OBJECT_CHECK_RETURN(m, 0);
   E_OBJECT_TYPE_CHECK_RETURN(m, E_MENU_TYPE, 0);
   m->frozen--;
   if (m->frozen < 0) m->frozen = 0;
   return m->frozen;
}

EAPI void
e_menu_title_set(E_Menu *m, char *title)
{
   E_OBJECT_CHECK(m);
   E_OBJECT_TYPE_CHECK(m, E_MENU_TYPE);
   if ((m->header.title) && (title) && (!strcmp(m->header.title, title)))
     return;
   if (m->header.title)
     {
	eina_stringshare_del(m->header.title);
	m->header.title = NULL;
     }
   if (title) m->header.title = eina_stringshare_add(title);
   else m->header.title = NULL;
   m->changed = 1;
}

EAPI void
e_menu_icon_file_set(E_Menu *m, char *icon)
{
   E_OBJECT_CHECK(m);
   E_OBJECT_TYPE_CHECK(m, E_MENU_TYPE);
   /* FIXME: support menu icons */
}

EAPI void
e_menu_category_set(E_Menu *m, char *category)
{
   E_OBJECT_CHECK(m);
   E_OBJECT_TYPE_CHECK(m, E_MENU_TYPE);
   if (m->category)
     {
	eina_stringshare_del(m->category);
	m->category = NULL;
     }
   if (category)
      m->category = eina_stringshare_add(category);
   else
      m->category = NULL;
   m->changed = 1;
}

EAPI void
e_menu_category_data_set(char *category, void *data)
{
   E_Menu_Category *cat;
   
   cat = evas_hash_find(_e_menu_categories, category);
   if (cat)
   	cat->data = data;
   else   /* if it isnt found create the new hash */
     {
	cat = calloc(1, sizeof(E_Menu_Category));
	cat->data = data;
	_e_menu_categories = evas_hash_add(_e_menu_categories, category, cat);
     }
}

EAPI E_Menu_Category_Callback *
e_menu_category_callback_add(char *category, void (*create) (E_Menu *m, void *category_data, void *data), void (*free) (void *data), void *data)
{
   E_Menu_Category *cat;
   E_Menu_Category_Callback *cb = NULL;
   
   cat = evas_hash_find(_e_menu_categories, category);
   if (!cat)   /* if it isnt found create the new hash */
     {
	cat = calloc(1, sizeof(E_Menu_Category));
	_e_menu_categories = evas_hash_add(_e_menu_categories, category, cat);
     }
   if (cat)
      {
         cb = calloc(1, sizeof(E_Menu_Category_Callback));
	 if (cb)
	    {
               cb->data = data;
               cb->create = create;
               cb->free = free;
	       cb->category = eina_stringshare_add(category);
               cat->callbacks = eina_list_append(cat->callbacks, cb);
	    }
      }
   return cb;
}

EAPI void
e_menu_category_callback_del(E_Menu_Category_Callback *cb)
{
   E_Menu_Category *cat;

   if (cb)
      {
         cat = evas_hash_find(_e_menu_categories, cb->category);
	 if (cat)
            cat->callbacks = eina_list_remove(cat->callbacks, cb);
         eina_stringshare_del(cb->category);
         free(cb);
      }
}

EAPI void
e_menu_pre_activate_callback_set(E_Menu *m, void (*func) (void *data, E_Menu *m), void *data)
{
   E_OBJECT_CHECK(m);
   E_OBJECT_TYPE_CHECK(m, E_MENU_TYPE);
   m->pre_activate_cb.func = func;
   m->pre_activate_cb.data = data;
}

EAPI void
e_menu_post_deactivate_callback_set(E_Menu *m, void (*func) (void *data, E_Menu *m), void *data)
{
   E_OBJECT_CHECK(m);
   E_OBJECT_TYPE_CHECK(m, E_MENU_TYPE);
   m->post_deactivate_cb.func = func;
   m->post_deactivate_cb.data = data;
}

EAPI E_Menu *
e_menu_root_get(E_Menu *m)
{
   E_Menu *ret;

   E_OBJECT_CHECK_RETURN(m, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(m, E_MENU_TYPE, NULL);
   ret = m;
   while ((ret->parent_item) && (ret->parent_item->menu))
     {
	ret = ret->parent_item->menu;
     }

   return ret;
}

EAPI E_Menu_Item *
e_menu_item_new(E_Menu *m)
{
   E_Menu_Item *mi;
   
   E_OBJECT_CHECK_RETURN(m, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(m, E_MENU_TYPE, NULL);
   mi = E_OBJECT_ALLOC(E_Menu_Item, E_MENU_ITEM_TYPE, _e_menu_item_free);
   mi->menu = m;
   mi->menu->items = eina_list_append(mi->menu->items, mi);
   mi->list_position = eina_list_last(mi->menu->items);
   return mi;
}

EAPI E_Menu_Item *
e_menu_item_nth(E_Menu *m, int n)
{
   E_OBJECT_CHECK_RETURN(m, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(m, E_MENU_TYPE, NULL);
   return (E_Menu_Item *)eina_list_nth(m->items, n);
}

EAPI int
e_menu_item_num_get(E_Menu_Item *mi)
{
   Eina_List *l;
   int i;
   
   E_OBJECT_CHECK_RETURN(mi, -1);
   E_OBJECT_CHECK_RETURN(mi->menu, -1);
   E_OBJECT_TYPE_CHECK_RETURN(mi, E_MENU_TYPE, -1);
   for (i = 0, l = mi->menu->items; l; l = l->next, i++)
     {
	E_Menu_Item *mi2;
	
	mi2 = l->data;
	if (mi2 == mi) return i;
     }
   return -1;
}

EAPI void
e_menu_item_icon_file_set(E_Menu_Item *mi, const char *icon)
{
   E_OBJECT_CHECK(mi);
   E_OBJECT_TYPE_CHECK(mi, E_MENU_ITEM_TYPE);
   if (((mi->icon) && (icon) && (!strcmp(icon, mi->icon))) ||
       ((!mi->icon) && (!icon))) 
     return;
   if (mi->icon) eina_stringshare_del(mi->icon);
   if (mi->icon_key) eina_stringshare_del(mi->icon_key);
   mi->icon = NULL;
   mi->icon_key = NULL;
   if (icon) mi->icon = eina_stringshare_add(icon);
   mi->changed = 1;
   mi->menu->changed = 1;
}

EAPI void
e_menu_item_icon_edje_set(E_Menu_Item *mi, const char *icon, const char *key)
{
   E_OBJECT_CHECK(mi);
   E_OBJECT_TYPE_CHECK(mi, E_MENU_ITEM_TYPE);
   if (((mi->icon) && (icon) && (!strcmp(icon, mi->icon))) ||
       ((!mi->icon) && (!icon)) || 
       ((key) && (mi->icon_key) && (!strcmp(key, mi->icon_key))))
     return;
   if (mi->icon) eina_stringshare_del(mi->icon);
   if (mi->icon_key) eina_stringshare_del(mi->icon_key);
   mi->icon = NULL;
   mi->icon_key = NULL;
   if (icon) mi->icon = eina_stringshare_add(icon);
   if (key) mi->icon_key = eina_stringshare_add(key);
   mi->changed = 1;
   mi->menu->changed = 1;
}

EAPI void
e_menu_item_label_set(E_Menu_Item *mi, const char *label)
{
   E_OBJECT_CHECK(mi);
   E_OBJECT_TYPE_CHECK(mi, E_MENU_ITEM_TYPE);
   if (((mi->label) && (label) && (!strcmp(label, mi->label))) ||
       ((!mi->label) && (!label))) 
     return;
   if (mi->label) eina_stringshare_del(mi->label);
   mi->label = NULL;
   if (label) mi->label = eina_stringshare_add(label);
   mi->changed = 1;
   mi->menu->changed = 1;
}

EAPI void
e_menu_item_submenu_set(E_Menu_Item *mi, E_Menu *sub)
{
   E_OBJECT_CHECK(mi);
   E_OBJECT_TYPE_CHECK(mi, E_MENU_ITEM_TYPE);
   if (mi->submenu) e_object_unref(E_OBJECT(mi->submenu));
   if (sub) e_object_ref(E_OBJECT(sub));
   mi->submenu = sub;
   mi->changed = 1;
   mi->menu->changed = 1;
}

EAPI void
e_menu_item_separator_set(E_Menu_Item *mi, int sep)
{
   E_OBJECT_CHECK(mi);
   E_OBJECT_TYPE_CHECK(mi, E_MENU_ITEM_TYPE);
   if (((mi->separator) && (sep)) ||
	((!mi->separator) && (!sep))) return;
   mi->separator = sep;
   mi->changed = 1;
   mi->menu->changed = 1;
}

EAPI void
e_menu_item_check_set(E_Menu_Item *mi, int chk)
{
   E_OBJECT_CHECK(mi);
   E_OBJECT_TYPE_CHECK(mi, E_MENU_ITEM_TYPE);
   if (((mi->check) && (chk)) ||
       ((!mi->check) && (!chk))) return;
   mi->check = chk;
   mi->changed = 1;
   mi->menu->changed = 1;
}

EAPI void
e_menu_item_radio_set(E_Menu_Item *mi, int rad)
{
   E_OBJECT_CHECK(mi);
   E_OBJECT_TYPE_CHECK(mi, E_MENU_ITEM_TYPE);
   if (((mi->radio) && (rad)) ||
       ((!mi->radio) && (!rad))) return;
   mi->radio = rad;
   mi->changed = 1;
   mi->menu->changed = 1;
}

EAPI void
e_menu_item_radio_group_set(E_Menu_Item *mi, int radg)
{
   E_OBJECT_CHECK(mi);
   E_OBJECT_TYPE_CHECK(mi, E_MENU_ITEM_TYPE);
   if (mi->radio_group == radg) return;
   mi->radio_group = radg;
   mi->changed = 1;
   mi->menu->changed = 1;
}

EAPI void
e_menu_item_toggle_set(E_Menu_Item *mi, int tog)
{
   E_OBJECT_CHECK(mi);
   E_OBJECT_TYPE_CHECK(mi, E_MENU_ITEM_TYPE);
   if (mi->separator) return;
   if (tog)
     {
	mi->toggle = 1;
	if (mi->bg_object)
	  edje_object_signal_emit(mi->bg_object, "e,state,on", "e");
	if (mi->icon_bg_object)
	  edje_object_signal_emit(mi->icon_bg_object, "e,state,on", "e");
	if (mi->label_object)
	  edje_object_signal_emit(mi->label_object, "e,state,on", "e");
	if (mi->submenu_object)
	  edje_object_signal_emit(mi->submenu_object, "e,state,on", "e");
	if (mi->toggle_object)
	  edje_object_signal_emit(mi->toggle_object, "e,state,on", "e");
	if (mi->menu->bg_object)
	  edje_object_signal_emit(mi->menu->bg_object, "e,state,on", "e");
     }
   else
     {
	mi->toggle = 0;
	if (mi->bg_object)
	  edje_object_signal_emit(mi->bg_object, "e,state,off", "e");
	if (mi->icon_bg_object)
	  edje_object_signal_emit(mi->icon_bg_object, "e,state,off", "e");
	if (mi->label_object)
	  edje_object_signal_emit(mi->label_object, "e,state,off", "e");
	if (mi->submenu_object)
	  edje_object_signal_emit(mi->submenu_object, "e,state,off", "e");
	if (mi->toggle_object)
	  edje_object_signal_emit(mi->toggle_object, "e,state,off", "e");
	if (mi->menu->bg_object)
	  edje_object_signal_emit(mi->menu->bg_object, "e,state,off", "e");
     }
   if (tog)
     {
	if (mi->radio)
	  {
	     Eina_List *l;
	     
	     for (l = mi->menu->items; l; l = l->next)
	       {
		  E_Menu_Item *mi2;
		  
		  mi2 = l->data;
		  if ((mi2 != mi) && 
		      (mi2->radio) && 
		      (mi2->radio_group == mi->radio_group))
		    e_menu_item_toggle_set(mi2, 0);
	       }
	  }
     }
}

EAPI int
e_menu_item_toggle_get(E_Menu_Item *mi)
{
   E_OBJECT_CHECK_RETURN(mi, 0);
   E_OBJECT_TYPE_CHECK_RETURN(mi, E_MENU_ITEM_TYPE, 0);
   return mi->toggle;
}

EAPI void
e_menu_item_callback_set(E_Menu_Item *mi,  void (*func) (void *data, E_Menu *m, E_Menu_Item *mi), void *data)
{
   E_OBJECT_CHECK(mi);
   E_OBJECT_TYPE_CHECK(mi, E_MENU_ITEM_TYPE);
   mi->cb.func = func;
   mi->cb.data = data;
}

EAPI void
e_menu_item_realize_callback_set(E_Menu_Item *mi,  void (*func) (void *data, E_Menu *m, E_Menu_Item *mi), void *data)
{
   E_OBJECT_CHECK(mi);
   E_OBJECT_TYPE_CHECK(mi, E_MENU_ITEM_TYPE);
   mi->realize_cb.func = func;
   mi->realize_cb.data = data;
}

EAPI void
e_menu_item_submenu_pre_callback_set(E_Menu_Item *mi,  void (*func) (void *data, E_Menu *m, E_Menu_Item *mi), void *data)
{
   E_OBJECT_CHECK(mi);
   E_OBJECT_TYPE_CHECK(mi, E_MENU_ITEM_TYPE);
   mi->submenu_pre_cb.func = func;
   mi->submenu_pre_cb.data = data;
   if (!mi->submenu_post_cb.func)
     mi->submenu_post_cb.func = _e_menu_cb_item_submenu_post_default;
}

EAPI void
e_menu_item_submenu_post_callback_set(E_Menu_Item *mi,  void (*func) (void *data, E_Menu *m, E_Menu_Item *mi), void *data)
{
   E_OBJECT_CHECK(mi);
   E_OBJECT_TYPE_CHECK(mi, E_MENU_ITEM_TYPE);
   mi->submenu_post_cb.func = func;
   mi->submenu_post_cb.data = data;
}

EAPI void
e_menu_item_drag_callback_set(E_Menu_Item *mi,  void (*func) (void *data, E_Menu *m, E_Menu_Item *mi), void *data)
{
   E_OBJECT_CHECK(mi);
   E_OBJECT_TYPE_CHECK(mi, E_MENU_ITEM_TYPE);
   mi->drag_cb.func = func;
   mi->drag_cb.data = data;
}

EAPI void
e_menu_item_active_set(E_Menu_Item *mi, int active)
{
   E_OBJECT_CHECK(mi);
   E_OBJECT_TYPE_CHECK(mi, E_MENU_ITEM_TYPE);
   if (mi->separator) return;
   if ((active) && (!mi->active))
     {
	E_Menu_Item *pmi;
	
	pmi = _e_menu_item_active_get();
	if (mi == pmi) return;
	if (pmi) e_menu_item_active_set(pmi, 0);
	mi->active = 1;
	_e_active_menu_item = mi;
	if (mi->bg_object)
	  edje_object_signal_emit(mi->bg_object, "e,state,selected", "e");
	if (mi->icon_bg_object)
	  edje_object_signal_emit(mi->icon_bg_object, "e,state,selected", "e");
	if (mi->label_object)
	  edje_object_signal_emit(mi->label_object, "e,state,selected", "e");
	if (mi->submenu_object)
	  edje_object_signal_emit(mi->submenu_object, "e,state,selected", "e");
	if (mi->toggle_object)
	  edje_object_signal_emit(mi->toggle_object, "e,state,selected", "e");
	if (mi->icon_key)
	  edje_object_signal_emit(mi->icon_object, "e,state,selected", "e");
	edje_object_signal_emit(mi->menu->bg_object, "e,state,selected", "e");
	_e_menu_submenu_activate(mi);
     }
   else if ((!active) && (mi->active))
     {
	mi->active = 0;
	_e_active_menu_item = NULL;
	if (mi->bg_object)
	  edje_object_signal_emit(mi->bg_object, "e,state,unselected", "e");
	if (mi->icon_bg_object)
	  edje_object_signal_emit(mi->icon_bg_object, "e,state,unselected", "e");
	if (mi->label_object)
	  edje_object_signal_emit(mi->label_object, "e,state,unselected", "e");
	if (mi->submenu_object)
	  edje_object_signal_emit(mi->submenu_object, "e,state,unselected", "e");
	if (mi->toggle_object)
	  edje_object_signal_emit(mi->toggle_object, "e,state,unselected", "e");
	if (mi->icon_key)
	  edje_object_signal_emit(mi->icon_object, "e,state,unselected", "e");
	edje_object_signal_emit(mi->menu->bg_object, "e,state,unselected", "e");
	_e_menu_submenu_deactivate(mi);
     }
}

EAPI void
e_menu_idler_before(void)
{
   /* when e goes "idle" this gets called so leave all our hard work till */
   /* idle time to avoid falling behind the user. just evaluate the high */
   /* level state machine */
   Eina_List *l, *removals = NULL, *tmp = NULL;

   /* add refcount to all menus we will work with */
   for (l = _e_active_menus; l; l = l->next)
     {
	tmp = eina_list_append(tmp, l->data);
	e_object_ref(E_OBJECT(l->data));
     }
   /* phase 1. hide all the menus that want to be hidden */
   for (l = _e_active_menus; l; l = l->next)
     {
	E_Menu *m;
	
	m = l->data;
	if ((!m->cur.visible) && (m->prev.visible))
	  {
	     m->prev.visible = m->cur.visible;
	     ecore_evas_hide(m->ecore_evas);
	     e_container_shape_hide(m->shape);
	  }
     }
   /* phase 2. move & reisze all the menus that want to moves/resized */
   for (l = _e_active_menus; l; l = l->next)
     {
	E_Menu *m;
	
	m = l->data;
	if (!m->realized) _e_menu_realize(m);
	if (m->realized)
	  {
	     if (((m->cur.w) != (m->prev.w)) ||
		 ((m->cur.h) != (m->prev.h)))
	       {
		  m->prev.w = m->cur.w;
		  m->prev.h = m->cur.h;
		  ecore_evas_resize(m->ecore_evas, m->cur.w, m->cur.h);
		  e_container_shape_resize(m->shape, m->cur.w, m->cur.h);
	       }
	     if (((m->cur.x) != (m->prev.x)) ||
		 ((m->cur.y) != (m->prev.y)))
	       {
		  m->prev.x = m->cur.x;
		  m->prev.y = m->cur.y;
		  ecore_evas_move(m->ecore_evas, m->cur.x, m->cur.y);
		  e_container_shape_move(m->shape, m->cur.x, m->cur.y);
	       }
	  }
     }
   /* phase 3. show all the menus that want to be shown */
   for (l = _e_active_menus; l; l = l->next)
     {
	E_Menu *m;
	
	m = l->data;
	if ((m->cur.visible) && (!m->prev.visible))
	  {
	     m->prev.visible = m->cur.visible;
	     ecore_evas_raise(m->ecore_evas);
	     ecore_evas_show(m->ecore_evas);
	     if (!m->shaped)
	       e_container_shape_show(m->shape);
	  }
     }
   /* phase 4. de-activate... */
   for (l = _e_active_menus; l; l = l->next)
     {
	E_Menu *m;
	
	m = l->data;
	if (!m->active)
	  {
	     _e_menu_unrealize(m);
	     removals = eina_list_append(removals, m);
	  }
     }
   while (removals)
     {
	E_Menu *m;
	
	m = removals->data;
	removals = eina_list_remove(removals, m);
	if (m->in_active_list)
	  {
	     _e_active_menus = eina_list_remove(_e_active_menus, m);
	     m->in_active_list = 0;
	     e_object_unref(E_OBJECT(m));
	  }
     }
   /* phase 5. shapes... */
   for (l = _e_active_menus; l; l = l->next)
     {
	E_Menu *m;
	
	m = l->data;
	if (m->need_shape_export)
	  {
	     Ecore_X_Rectangle *rects, *orects;
	     int num;
	     
	     rects = ecore_x_window_shape_rectangles_get(m->evas_win, &num);
	     if (rects)
	       {
                 int changed;
		  
		  changed = 1;
		  if ((num == m->shape_rects_num) && (m->shape_rects))
		    {
		       int i;
		       
		       orects = m->shape_rects;
		       for (i = 0; i < num; i++)
			 {
			    if ((orects[i].x != rects[i].x) ||
				(orects[i].y != rects[i].y) ||
				(orects[i].width != rects[i].width) ||
				(orects[i].height != rects[i].height))
			      {
				 changed = 1;
				 break;
			      }
			 }
		       changed = 0;
		    }
		  if (changed)
		    {
		       E_FREE(m->shape_rects);
		       m->shape_rects = rects;
		       m->shape_rects_num = num;
		       e_container_shape_rects_set(m->shape, rects, num);
		    }
		  else
		    free(rects);
	       }
	     else
	       {
                  E_FREE(m->shape_rects);
		  m->shape_rects = NULL;
		  m->shape_rects_num = 0;
		  e_container_shape_rects_set(m->shape, NULL, 0);
	       }
	     m->need_shape_export = 0;
	     if (m->cur.visible)
	       e_container_shape_show(m->shape);
	  }
     }
   /* del refcount to all menus we worked with */
   while (tmp)
     {
	e_object_unref(E_OBJECT(tmp->data));
	tmp = eina_list_remove_list(tmp, tmp);
     }
   if (!_e_active_menus)
     {
	if (_e_menu_win)
	  {
	     ecore_x_window_del(_e_menu_win);
	     e_grabinput_release(_e_menu_win, _e_menu_win);
	     _e_menu_win = 0;
	  }
     }
}

EAPI Ecore_X_Window
e_menu_grab_window_get(void)
{
  return _e_menu_win;
}

/* local subsystem functions */
static void
_e_menu_free(E_Menu *m)
{
   Eina_List *l, *tmp;
   E_Menu_Category *cat;
   
   /* the foreign menu items */
   cat = evas_hash_find(_e_menu_categories, m->category);
   if (cat)
     {
	for (l = cat->callbacks; l; l = l->next)
	  {
	     E_Menu_Category_Callback *cb;

	     cb = l->data;
	     if (cb->free) cb->free(cb->data);
	  }
     }
   _e_menu_unrealize(m);
   E_FREE(m->shape_rects);
   m->shape_rects_num = 0;
   for (l = m->items; l;)
     {
	tmp = l;
	l = l->next;
	e_object_del(E_OBJECT(tmp->data));
     }
   if (m->in_active_list)
     {
	_e_active_menus = eina_list_remove(_e_active_menus, m);
	m->in_active_list = 0;
	e_object_unref(E_OBJECT(m));
     }
   if (m->header.title) eina_stringshare_del(m->header.title);
   if (m->header.icon_file) eina_stringshare_del(m->header.icon_file);
   free(m);
}

static void
_e_menu_item_free(E_Menu_Item *mi)
{
   if (mi == _e_active_menu_item) _e_active_menu_item = NULL;
   if (mi->submenu)
     {
	mi->submenu->parent_item = NULL;
	e_object_unref(E_OBJECT(mi->submenu));
     }
   if (mi->menu->realized) _e_menu_item_unrealize(mi);
   mi->menu->items = eina_list_remove(mi->menu->items, mi);
   if (mi->icon) eina_stringshare_del(mi->icon);
   if (mi->icon_key) eina_stringshare_del(mi->icon_key);
   if (mi->label) eina_stringshare_del(mi->label);
   free(mi);
}

static void
_e_menu_cb_intercept_item_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y)
{
   E_Menu_Item *mi;
   
   mi = data;
   mi->x = x;
   mi->y = y;
   evas_object_move(mi->event_object, x, y);
   evas_object_move(o, x, y);
   if ((mi->submenu) && (mi->submenu->parent_item))
     _e_menu_reposition(mi->submenu);
}

static void
_e_menu_cb_intercept_item_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h)
{
   E_Menu_Item *mi;
   
   mi = data;
   mi->w = w;
   mi->h = h;
   evas_object_resize(mi->event_object, w, h);
   evas_object_resize(o, w, h);
   if ((mi->submenu) && (mi->submenu->parent_item))
     _e_menu_reposition(mi->submenu);
}

static void
_e_menu_cb_intercept_container_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y)
{
   E_Menu *m;
   
   m = data;
   m->container_x = x;
   m->container_y = y;
   if (m->parent_item) _e_menu_reposition(m);
   evas_object_move(o, x, y);
}

static void
_e_menu_cb_intercept_container_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h)
{
   E_Menu *m;
   
   m = data;
   m->container_w = w;
   m->container_h = h;
   if (m->parent_item) _e_menu_reposition(m);
   evas_object_resize(o, w, h);
}

static void
_e_menu_item_realize(E_Menu_Item *mi)
{
   Evas_Object *o;
   Evas_Coord ww, hh;

   /* and set up initial item state */   
   if (mi->separator)
     {
	o = edje_object_add(mi->menu->evas);
	mi->separator_object = o;
	e_theme_edje_object_set(o, "base/theme/menus", 
				"e/widgets/menu/default/separator");
	evas_object_show(o);
	edje_object_size_min_calc(mi->separator_object, &ww, &hh);
	mi->separator_w = ww;
	mi->separator_h = hh;
	e_box_pack_end(mi->menu->container_object, mi->separator_object);
     }
   else
     {
	o = edje_object_add(mi->menu->evas);
	mi->bg_object = o;
	evas_object_intercept_move_callback_add  (o, _e_menu_cb_intercept_item_move,   mi);
	evas_object_intercept_resize_callback_add(o, _e_menu_cb_intercept_item_resize, mi);

	if ((mi->submenu) || (mi->submenu_pre_cb.func))
	  {
	     if (!e_theme_edje_object_set(mi->bg_object, "base/theme/menus",
					  "e/widgets/menu/default/submenu_bg"))
	       goto no_submenu_item;
	  }
	else
	  {
	     no_submenu_item:
	     e_theme_edje_object_set(mi->bg_object, "base/theme/menus",
				     "e/widgets/menu/default/item_bg");
	  }
	evas_object_show(o);
	
	o = e_box_add(mi->menu->evas);
	e_box_homogenous_set(o, 0);
	mi->container_object = o;
	e_box_orientation_set(o, 1);
	evas_object_show(o);
	
	e_box_freeze(mi->container_object);

	if (mi->check)
	  {
	     o = edje_object_add(mi->menu->evas);
	     mi->toggle_object = o;
	     e_theme_edje_object_set(o, "base/theme/menus",
				     "e/widgets/menu/default/check");
	     evas_object_pass_events_set(o, 1);
	     evas_object_show(o);
	     e_box_pack_end(mi->container_object, o);
	     edje_object_size_min_calc(mi->toggle_object, &ww, &hh);
	     mi->toggle_w = ww;
	     mi->toggle_h = hh;
	  }
	else if (mi->radio)
	  {
	     o = edje_object_add(mi->menu->evas);
	     mi->toggle_object = o;
	     e_theme_edje_object_set(o, "base/theme/menus",
				     "e/widgets/menu/default/radio");
	     evas_object_pass_events_set(o, 1);
	     evas_object_show(o);
	     e_box_pack_end(mi->container_object, o);
	     edje_object_size_min_calc(mi->toggle_object, &ww, &hh);
	     mi->toggle_w = ww;
	     mi->toggle_h = hh;
	  }
	else
	  {
	     o = evas_object_rectangle_add(mi->menu->evas);
	     mi->toggle_object = o;
	     evas_object_color_set(o, 0, 0, 0, 0);
	     evas_object_pass_events_set(o, 1);
	     e_box_pack_end(mi->container_object, o);
	  }
	if (mi->icon || mi->realize_cb.func)
	  {
	     int icon_w, icon_h;
	     
	     o = edje_object_add(mi->menu->evas);
	     if (e_theme_edje_object_set(o, "base/theme/menus",
					 "e/widgets/menu/default/icon"))
	       {
		  mi->icon_bg_object = o;
		  evas_object_show(o);
	       }
	     else
	       evas_object_del(o);
	     
             /* FIXME: Not sure why there are two different tries to get the icon size, surely only the last one si needed. */
             /* FIXME: Do it this way later, when e_app_icon_add() just registers a request for an icon to be filled in when it's ready.
	     if (mi->app)
	       {
		  o = e_app_icon_add(mi->menu->evas, mi->app);
		  mi->icon_object = o;
		  e_icon_size_get(mi->icon_object, &icon_w, &icon_h);
	       }
	     else
             */
	     if (mi->icon)
	       {
		  /* This is done this way to match up with how e_app_icon_add does it. */
		  if (mi->icon_key)
		    {
		       Evas_Coord iww, ihh;

		       o = edje_object_add(mi->menu->evas);
		       if (edje_object_file_set(o, mi->icon, mi->icon_key))
			 {
			    mi->icon_object = o;
			    edje_object_size_max_get(o, &iww, &ihh);
			    icon_w = iww;
			    icon_h = ihh;
			 }
		    }
		  if (!mi->icon_object)
		    {
		       o = e_icon_add(mi->menu->evas);
		       mi->icon_object = o;
		       e_icon_file_set(o, mi->icon);
		       e_icon_fill_inside_set(o, 1);
		       e_icon_size_get(mi->icon_object, &icon_w, &icon_h);
		    }
	       }
	     if (_e_menu_realize_call(mi))
	       {
		  o = mi->icon_object;
		  e_icon_fill_inside_set(o, 1);
		  e_icon_size_get(o, &icon_w, &icon_h);
	       }

	     evas_object_pass_events_set(o, 1);
	     evas_object_show(o);
	     
	     if (mi->icon_bg_object)
	       {
		  edje_extern_object_min_size_set(mi->icon_object,
						  icon_w, icon_h);
		  edje_object_part_swallow(mi->icon_bg_object, "e.swallow.content", 
					   mi->icon_object);
		  edje_object_size_min_calc(mi->icon_bg_object, &ww, &hh);
		  mi->icon_w = ww;
		  mi->icon_h = hh;
		  
		  edje_extern_object_min_size_set(mi->icon_object, 0, 0);
		  edje_object_part_swallow(mi->icon_bg_object, "e.swallow.content", 
					   mi->icon_object);
		  e_box_pack_end(mi->container_object, mi->icon_bg_object);
	       }
	     else
	       {
		  int icon_w, icon_h;
	     
		  o = edje_object_add(mi->menu->evas);
		  e_icon_size_get(mi->icon_object, &icon_w, &icon_h);
		  mi->icon_w = icon_w;
		  mi->icon_h = icon_h;
		  e_box_pack_end(mi->container_object, o);
	       }
	  }
	else
	  {
	     o = evas_object_rectangle_add(mi->menu->evas);
	     mi->icon_object = o;
	     evas_object_color_set(o, 0, 0, 0, 0);
	     evas_object_pass_events_set(o, 1);
	     e_box_pack_end(mi->container_object, o);
	  }
	
	if (mi->label)
	  {
	     o = edje_object_add(mi->menu->evas);
	     mi->label_object = o;
	     e_theme_edje_object_set(o, "base/theme/menus",
				     "e/widgets/menu/default/label");
	     /* default label */
	     edje_object_part_text_set(o, "e.text.label", mi->label);
	     evas_object_pass_events_set(o, 1);
	     evas_object_show(o);
	     e_box_pack_end(mi->container_object, o);
	     edje_object_size_min_calc(mi->label_object, &ww, &hh);
	     mi->label_w = ww;
	     mi->label_h = hh;
	  }
	else
	  {
	     o = evas_object_rectangle_add(mi->menu->evas);
	     mi->label_object = o;
	     evas_object_color_set(o, 0, 0, 0, 0);
	     evas_object_pass_events_set(o, 1);
	     e_box_pack_end(mi->container_object, o);
	  }
	if ((mi->submenu) || (mi->submenu_pre_cb.func))
	  {
	     o = edje_object_add(mi->menu->evas);
	     mi->submenu_object = o;
	     e_theme_edje_object_set(o, "base/theme/menus",
				     "e/widgets/menu/default/submenu");
	     evas_object_pass_events_set(o, 1);
	     evas_object_show(o);
	     e_box_pack_end(mi->container_object, o);
	     edje_object_size_min_calc(mi->submenu_object, &ww, &hh);
	     mi->submenu_w = ww;
	     mi->submenu_h = hh;
	  }
	else
	  {
	     o = evas_object_rectangle_add(mi->menu->evas);
	     mi->submenu_object = o;
	     evas_object_color_set(o, 0, 0, 0, 0);
	     evas_object_pass_events_set(o, 1);
	     e_box_pack_end(mi->container_object, o);
	  }

	edje_object_part_swallow(mi->bg_object, "e.swallow.content", mi->container_object);
	
	o = evas_object_rectangle_add(mi->menu->evas);
	evas_object_color_set(o, 0, 0, 0, 0);
	evas_object_layer_set(o, 1);
	evas_object_repeat_events_set(o, 1);
	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN,  _e_menu_cb_item_in,  mi);
	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT, _e_menu_cb_item_out, mi);
	evas_object_show(o);
	mi->event_object = o;

	e_box_thaw(mi->container_object);
	
	e_box_pack_end(mi->menu->container_object, mi->bg_object);
     }
   if (mi->active) e_menu_item_active_set(mi, 1);
   if (mi->toggle) e_menu_item_toggle_set(mi, 1);
}

static void
_e_menu_realize(E_Menu *m)
{
   Evas_Object *o;
   Eina_List *l;
 
   int ok;
   
   if (m->realized) return;
   m->realized = 1;
   m->ecore_evas = e_canvas_new(e_config->evas_engine_menus, m->zone->container->win,
				m->cur.x, m->cur.y, m->cur.w, m->cur.h, 1, 1,
				&(m->evas_win), NULL);
   e_canvas_add(m->ecore_evas);
   m->shape = e_container_shape_add(m->zone->container);
   e_container_shape_move(m->shape, m->cur.x, m->cur.y);
   e_container_shape_resize(m->shape, m->cur.w, m->cur.h);
   
   ecore_evas_callback_resize_set(m->ecore_evas, _e_menu_cb_ecore_evas_resize);
   m->evas = ecore_evas_get(m->ecore_evas);
   evas_event_freeze(m->evas);
   /* move cursor out to avoid event cycles during setup */
   evas_event_feed_mouse_in(m->evas, ecore_x_current_time_get(), NULL);
   evas_event_feed_mouse_move(m->evas, -1000000, -1000000, ecore_x_current_time_get(), NULL);
   ecore_x_window_shape_events_select(m->evas_win, 1);
   ecore_evas_name_class_set(m->ecore_evas, "E", "_e_menu_window");
   ecore_evas_title_set(m->ecore_evas, "E Menu");
   
   o = edje_object_add(m->evas);
   m->bg_object = o;
   evas_object_name_set(o, "menu/background");
   evas_object_data_set(o, "e_menu", m);
   evas_object_move(o, 0, 0);
   evas_object_resize(o, m->cur.w, m->cur.h);
   ok = e_theme_edje_object_set(o, "base/theme/menus",
				"e/widgets/menu/default/background");
   if (ok)
     {
	const char *shape_option;
	
	shape_option = edje_object_data_get(o, "shaped");
	if (shape_option)
	  {
	     if (!strcmp(shape_option, "1"))
	       {
		  m->shaped = 1;
	       }
	  }
     }
   if (m->header.title)
     {
	edje_object_part_text_set(o, "e.text.title", m->header.title);
	edje_object_signal_emit(o, "e,action,show,title", "e");
	edje_object_message_signal_process(o);
     }
   evas_object_show(o);

   if (m->shaped)
     {       
	if (e_config->use_composite)
	  {
	     ecore_evas_alpha_set(m->ecore_evas, m->shaped);
	     m->evas_win = ecore_evas_software_x11_window_get(m->ecore_evas);
	  }
	else
	  ecore_evas_shaped_set(m->ecore_evas, m->shaped);
     }
   
   o = e_box_add(m->evas);
   m->container_object = o;
   evas_object_intercept_move_callback_add(o, _e_menu_cb_intercept_container_move, m);
   evas_object_intercept_resize_callback_add(o, _e_menu_cb_intercept_container_resize, m);
   e_box_freeze(o);
   evas_object_show(o);
   e_box_homogenous_set(o, 0);
   edje_object_part_swallow(m->bg_object, "e.swallow.content", m->container_object);
   
   
   for (l = m->items; l; l = l->next)
     {
	E_Menu_Item *mi;
	
	mi = l->data;
	_e_menu_item_realize(mi);
     }

   o = m->container_object;
   _e_menu_items_layout_update(m);
   e_box_thaw(o);
   evas_object_resize(m->bg_object, m->cur.w, m->cur.h);
   evas_event_thaw(m->evas);
}

static void
_e_menu_items_layout_update(E_Menu *m)
{
   Eina_List *l;
   Evas_Coord bw, bh, mw, mh;
   int toggles_on = 0;
   int icons_on = 0;
   int labels_on = 0;
   int submenus_on = 0;
   int min_icon_w = 0, min_icon_h = 0;
   int min_label_w = 0, min_label_h = 0;
   int min_submenu_w = 0, min_submenu_h = 0;
   int min_toggle_w = 0, min_toggle_h = 0;
   int min_w = 0, min_h = 0;
   
   e_box_freeze(m->container_object);
   for (l = m->items; l; l = l->next)
     {
	E_Menu_Item *mi;
	
	mi = l->data;
	
	if (mi->icon) icons_on = 1;
	if (mi->icon_object) icons_on = 1;
	if (mi->label) labels_on = 1;
	if (mi->submenu) submenus_on = 1;
	if (mi->check) toggles_on = 1;
	if (mi->radio) toggles_on = 1;
	
	if (mi->icon_w > min_icon_w) min_icon_w = mi->icon_w;
	if (mi->icon_h > min_icon_h) min_icon_h = mi->icon_h;
	if (mi->label_w > min_label_w) min_label_w = mi->label_w;
	if (mi->label_h > min_label_h) min_label_h = mi->label_h;
	if (mi->submenu_w > min_submenu_w) min_submenu_w = mi->submenu_w;
	if (mi->submenu_h > min_submenu_h) min_submenu_h = mi->submenu_h;
	if (mi->toggle_w > min_toggle_w) min_toggle_w = mi->toggle_w;
	if (mi->toggle_h > min_toggle_h) min_toggle_h = mi->toggle_h;
     }
   if (labels_on)
     {
	if (submenus_on)
	  {
	     if (min_label_h < min_submenu_h)
	       min_label_h = min_submenu_h;
	  }
	if (toggles_on)
	  {
	     if (min_label_h < min_toggle_h)
	       min_label_h = min_toggle_h;
	  }
	if ((icons_on) && (min_icon_h > 0))
	  {
	     min_icon_w = (min_icon_w * min_label_h) / min_icon_h;
	     min_icon_h = min_label_h;
	  }
	min_w = min_label_w + min_icon_w + min_submenu_w + min_toggle_w;
	min_h = min_label_h;
     }
   else if (icons_on)
     {
	if (submenus_on)
	  {
	     if (min_icon_h < min_submenu_h)
	       min_icon_h = min_submenu_h;
	  }
	if (toggles_on)
	  {
	     if (min_icon_h < min_toggle_h)
	       min_icon_h = min_toggle_h;
	  }
	min_w = min_icon_w + min_toggle_w + min_submenu_w;
	min_h = min_icon_h;
     }
   else if (toggles_on)
     {
	if (submenus_on)
	  {
	     if (min_toggle_h < min_submenu_h)
	       min_toggle_h = min_submenu_h;
	  }
	min_w = min_toggle_w + min_submenu_w;
	min_h = min_toggle_h;
     }
   for (l = m->items; l; l = l->next)
     {
	E_Menu_Item *mi;
	
	mi = l->data;
	if (mi->separator)
	  {
	     e_box_pack_options_set(mi->separator_object, 
				    1, 1, /* fill */
				    1, 0, /* expand */
				    0.5, 0.5, /* align */
				    mi->separator_w, mi->separator_h, /* min */
				    -1, mi->separator_h /* max */
				    );
	  }
	else
	  {
	     e_box_freeze(mi->container_object);
	     if (toggles_on)
	       e_box_pack_options_set(mi->toggle_object,
				      1, 1, /* fill */
				      0, 1, /* expand */
				      0.5, 0.5, /* align */
				      min_toggle_w, min_toggle_h, /* min */
				      -1, -1 /* max */
				      );
	     else
	       e_box_pack_options_set(mi->toggle_object,
				      1, 1, /* fill */
				      0, 0, /* expand */
				      0.5, 0.5, /* align */
				      0, 0, /* min */
				      0, 0 /* max */
				      );
	     if (icons_on)
	       {
		  if (mi->icon_bg_object)
		    e_box_pack_options_set(mi->icon_bg_object,
					   1, 1, /* fill */
					   0, 1, /* expand */
					   0.5, 0.5, /* align */
					   min_icon_w, min_icon_h, /* min */
					   -1, -1 /* max */
					   );
		  else
		    e_box_pack_options_set(mi->icon_object,
					   1, 1, /* fill */
					   0, 1, /* expand */
					   0.5, 0.5, /* align */
					   min_icon_w, min_icon_h, /* min */
					   -1, -1 /* max */
					   );
	       }
	     else
	       e_box_pack_options_set(mi->icon_object,
				      1, 1, /* fill */
				      0, 1, /* expand */
				      0.5, 0.5, /* align */
				      0, 0, /* min */
				      0, 0 /* max */
				      );
	     if (labels_on)
	       e_box_pack_options_set(mi->label_object,
				      1, 1, /* fill */
				      0, 1, /* expand */
				      0.5, 0.5, /* align */
				      min_label_w, min_label_h, /* min */
				      -1, -1 /* max */
				      );
	     else
	       e_box_pack_options_set(mi->label_object,
				      1, 1, /* fill */
				      0, 0, /* expand */
				      0.5, 0.5, /* align */
				      0, 0, /* min */
				      0, 0 /* max */
				      );
	     if (submenus_on)
	       e_box_pack_options_set(mi->submenu_object,
				      1, 1, /* fill */
				      0, 1, /* expand */
				      0.5, 0.5, /* align */
				      min_submenu_w, min_submenu_h, /* min */
				      -1, -1 /* max */
				      );
	     else
	       e_box_pack_options_set(mi->submenu_object,
				      1, 1, /* fill */
				      0, 0, /* expand */
				      0.5, 0.5, /* align */
				      0, 0, /* min */
				      0, 0 /* max */
				      );
	     edje_extern_object_min_size_set(mi->container_object, min_w, min_h);
	     edje_object_part_swallow(mi->bg_object, "e.swallow.content", mi->container_object);
	     edje_object_size_min_calc(mi->bg_object, &mw, &mh);
	     e_box_pack_options_set(mi->bg_object,
				    1, 1, /* fill */
				    1, 0, /* expand */
				    0.5, 0.5, /* align */
				    mw, mh, /* min */
				    -1, -1 /* max */
				    );
	     e_box_thaw(mi->container_object);
	  }
     }
   e_box_min_size_get(m->container_object, &bw, &bh);
   edje_extern_object_min_size_set(m->container_object, bw, bh);
   edje_extern_object_max_size_set(m->container_object, bw, bh);
   edje_object_part_swallow(m->bg_object, "e.swallow.content", m->container_object);
   edje_object_size_min_calc(m->bg_object, &mw, &mh);
   e_box_thaw(m->container_object);
   m->cur.w = mw;
   m->cur.h = mh;
}

static void
_e_menu_item_unrealize(E_Menu_Item *mi)
{
   e_box_freeze(mi->container_object);
   if (mi->separator_object) evas_object_del(mi->separator_object);
   mi->separator_object = NULL;
   if (mi->bg_object) evas_object_del(mi->bg_object);
   mi->bg_object = NULL;
   if (mi->container_object) evas_object_del(mi->container_object);
   mi->container_object = NULL;
   if (mi->toggle_object) evas_object_del(mi->toggle_object);
   mi->toggle_object = NULL;
   if (mi->icon_bg_object) evas_object_del(mi->icon_bg_object);
   mi->icon_bg_object = NULL;
   if (mi->icon_object) evas_object_del(mi->icon_object);
   mi->icon_object = NULL;
   if (mi->label_object) evas_object_del(mi->label_object);
   mi->label_object = NULL;
   if (mi->submenu_object) evas_object_del(mi->submenu_object);
   mi->submenu_object = NULL;
   if (mi->event_object) evas_object_del(mi->event_object);
   mi->event_object = NULL;
}

static void
_e_menu_unrealize(E_Menu *m)
{
   Eina_List *l;
 
   if (!m->realized) return;
   evas_event_freeze(m->evas);
   e_container_shape_hide(m->shape);
   e_object_del(E_OBJECT(m->shape));
   m->shape = NULL;
   e_box_freeze(m->container_object);
   
   for (l = m->items; l; l = l->next)
     {
	E_Menu_Item *mi;
	
	mi = l->data;
	_e_menu_item_unrealize(mi);
     }
   if (m->header.icon) evas_object_del(m->header.icon);
   m->header.icon = NULL;
   if (m->bg_object) evas_object_del(m->bg_object);
   m->bg_object = NULL;
   if (m->container_object) evas_object_del(m->container_object);
   m->container_object = NULL;
   m->cur.visible = 0;
   m->prev.visible = 0;
   m->realized = 0;
   m->zone = NULL;
   e_canvas_del(m->ecore_evas);
   ecore_evas_free(m->ecore_evas);
   m->ecore_evas = NULL;
   m->evas = NULL;
   m->evas_win = 0;
}

static void
_e_menu_activate_internal(E_Menu *m, E_Zone *zone)
{
   Eina_List *l;
   E_Menu_Category *cat;

   if (m->pre_activate_cb.func)
     m->pre_activate_cb.func(m->pre_activate_cb.data, m);
   m->fast_mouse = 0;
   m->pending_new_submenu = 0;
   if (!_e_menu_win)
     {
	_e_menu_win = ecore_x_window_input_new(zone->container->win, 
					       zone->x, zone->y,
					       zone->w, zone->h);
	ecore_x_window_show(_e_menu_win);
	if (!e_grabinput_get(_e_menu_win, 1, _e_menu_win))
	  {
	     ecore_x_window_del(_e_menu_win);
	     _e_menu_win = 0;
	     return;
	  }
     }
   if ((m->zone) && (m->zone->container != zone->container))
     {
	printf("FIXME: cannot move menus between containers yet\n");
	return;
     }
   m->zone = zone;
   if (!m->active)
     {
	/* this remove is in case the menu is marked as inactive but hasnt */
	/* been removed from the list yet */
	if (m->in_active_list)
	  {
	     _e_active_menus = eina_list_remove(_e_active_menus, m);
	     m->in_active_list = 0;
	     e_object_unref(E_OBJECT(m));
	  }
	_e_active_menus = eina_list_append(_e_active_menus, m);
	m->in_active_list = 1;
	m->active = 1;
	e_object_ref(E_OBJECT(m));
     }
   /* the foreign menu items */
   cat = evas_hash_find(_e_menu_categories, m->category);
   if (cat)
     {
	for (l = cat->callbacks; l; l = l->next)
	  {
	     E_Menu_Category_Callback *cb;
	     
	     cb = l->data;
	     if (cb->create) cb->create(m, cat->data, cb->data);
	  }
     }
   m->cur.visible = 1;
}

static void
_e_menu_deactivate_all(void)
{
   Eina_List *l, *tmp = NULL;

   for (l = _e_active_menus; l; l = l->next)
     {
	e_object_ref(E_OBJECT(l->data));
	tmp = eina_list_append(tmp, l->data);
     }
   while (tmp)
     {
	E_Menu *m;
	
	m = tmp->data;
	tmp = eina_list_remove_list(tmp, tmp);
	e_menu_deactivate(m);
	m->parent_item = NULL;
	e_object_unref(E_OBJECT(m));
     }
   _e_menu_activate_floating = 0;
   _e_menu_activate_maybe_drag = 0;
   _e_menu_activate_dragging = 0;
}

static void
_e_menu_deactivate_above(E_Menu *ma)
{
   Eina_List *l, *tmp = NULL;
   int above = 0;

   for (l = _e_active_menus; l; l = l->next)
     {
	e_object_ref(E_OBJECT(l->data));
	tmp = eina_list_append(tmp, l->data);
     }
   while (tmp)
     {
	E_Menu *m;
	
	m = tmp->data;
	tmp = eina_list_remove_list(tmp, tmp);
	if (above)
	  {
	     e_menu_deactivate(m);
	     m->parent_item = NULL;
	  }
	if (ma == m) above = 1;
	e_object_unref(E_OBJECT(m));
     }
}

static void
_e_menu_submenu_activate(E_Menu_Item *mi)
{
   if (!mi->menu->active) return;
   if (mi->menu->fast_mouse)
     {
	mi->menu->pending_new_submenu = 1;
	return;
     }
   mi->menu->pending_new_submenu = 0;
   _e_menu_deactivate_above(mi->menu);
   if (mi->submenu_pre_cb.func)
     mi->submenu_pre_cb.func(mi->submenu_pre_cb.data, mi->menu, mi);
   if (mi->submenu)
     {
	E_Menu *m;
	
	m = mi->submenu;
	e_object_ref(E_OBJECT(m));
	m->parent_item = mi;
	_e_menu_activate_internal(m, mi->menu->zone);
	_e_menu_reposition(m);
	e_object_unref(E_OBJECT(m));
     }
}

static void
_e_menu_submenu_deactivate(E_Menu_Item *mi)
{
   if (!mi->menu->active) return;
   if (mi->submenu_post_cb.func)
     mi->submenu_post_cb.func(mi->submenu_post_cb.data, mi->menu, mi);
}

static void
_e_menu_reposition(E_Menu *m)
{
   Eina_List *l, *tmp = NULL;
   int parent_item_bottom;
   
   if (!m->parent_item) return;
   m->cur.x = m->parent_item->menu->cur.x + m->parent_item->menu->cur.w;

   parent_item_bottom = m->parent_item->menu->cur.y + m->parent_item->y;
   if (m->cur.h > m->zone->h)
     {
	/* menu is larger than screen */
	if (parent_item_bottom > (m->zone->h / 2))
	  /* more is shown if menu goes up */
	  m->cur.y = (parent_item_bottom - (m->container_h + 1)); 
	else
	  /* more is shown if menu goes down */
	  m->cur.y = parent_item_bottom - m->container_y;
     }
   else
     {  
	/* menu is smaller than screen */
	if (((parent_item_bottom + m->cur.h - m->container_y) > m->zone->h) && 
	   (parent_item_bottom > (m->zone->h / 2)))
	  /* menu is partially out of screen and more is shown if menu goes up */
	  m->cur.y = (parent_item_bottom - (m->container_h + 1)) + m->parent_item->h; 
	else
	  m->cur.y = parent_item_bottom - m->container_y;
     }
   
   /* FIXME: this will suck for big menus */
   for (l = _e_active_menus; l; l = l->next)
     {
	tmp = eina_list_append(tmp, l->data);
	e_object_ref(E_OBJECT(l->data));
     }
   for (l = m->items; l; l = l->next)
     {
	E_Menu_Item *mi;
	
	mi = l->data;
	if ((mi->active) && (mi->submenu)) _e_menu_reposition(mi->submenu);
     }
   while (tmp)
     {
	e_object_unref(E_OBJECT(tmp->data));
	tmp = eina_list_remove_list(tmp, tmp);
     }
}

static int
_e_menu_active_call(void)
{
   E_Menu_Item *mi;

   mi = _e_menu_item_active_get();
   if (mi)
     {
	if (mi->submenu) return 0;
	if (mi->check)
	  e_menu_item_toggle_set(mi, !mi->toggle);
	if ((mi->radio) && (!e_menu_item_toggle_get(mi)))
	  e_menu_item_toggle_set(mi, 1);
	if (mi->cb.func)
	  mi->cb.func(mi->cb.data, mi->menu, mi);
	return 1;
     }
   return -1;
}

static int
_e_menu_realize_call(E_Menu_Item *mi)
{
   if (mi)
     {
	if (mi->realize_cb.func)
	  {
	     mi->realize_cb.func(mi->realize_cb.data, mi->menu, mi);
	     return 1;
	  }
     }
   return 0;
}

static void
_e_menu_item_activate_next(void)
{
   E_Menu_Item *mi;
   Eina_List *ll;

   ll = _e_menu_list_item_active_get();
   mi = _e_menu_item_active_get();
   if (ll && mi) 
     {
	/* Look at the next item and then cycle until we're not on
	 * a separator. */
	if (!(ll->next)) 
	  ll = mi->menu->items;
	else
	   ll = ll->next;
	mi = ll->data;
	while (mi->separator)
	  { 
	     if (!(ll->next)) 
	       ll = mi->menu->items;
	     else
	       ll = ll->next;
	     mi = ll->data;
	  }

	e_menu_item_active_set(mi, 1);
	_e_menu_item_ensure_onscreen(mi);
	return;
     }

   _e_menu_activate_first();
}

static void
_e_menu_item_activate_previous(void)
{
   E_Menu_Item *mi;
   Eina_List *ll;

   ll = _e_menu_list_item_active_get();
   mi = _e_menu_item_active_get();
   if (ll && mi) 
     {
	/* Look at the prev item and then cycle until we're not on
	 * a separator. */
	if (!(ll->prev)) 
	  ll = eina_list_last(ll);
	else
	   ll = ll->prev;
	mi = ll->data;
	while (mi->separator)
	  { 
	     if (!(ll->prev)) 
	       ll = eina_list_last(ll);
	     else
	       ll = ll->prev;
	     mi = ll->data;
	  }

	e_menu_item_active_set(mi, 1);
	_e_menu_item_ensure_onscreen(mi);
	return;
     }

   _e_menu_activate_first();
}

static void
_e_menu_item_activate_first(void)
{
   E_Menu *m;
   Eina_List *ll;
   E_Menu_Item *mi;

   m = _e_menu_active_get();
   if (m)
     {
	ll = m->items;
	mi = ll->data;
	while ((mi->separator) && (ll->next))
	  {
	     ll = ll->next;
	     mi = ll->data;
	  }
	if (mi->separator) return;
	e_menu_item_active_set(mi, 1);
	_e_menu_item_ensure_onscreen(mi);
	return;
     }
   _e_menu_activate_first();
}

static void
_e_menu_item_activate_last(void)
{
   E_Menu *m;
   Eina_List *ll;
   E_Menu_Item *mi;

   m = _e_menu_active_get();
   if (m)
     {
	ll = eina_list_last(m->items);
	mi = ll->data;
	while ((mi->separator) && (ll->prev))
	  {
	     ll = ll->prev;
	     mi = ll->data;
	  }
	if (mi->separator) return;
	e_menu_item_active_set(mi, 1);
	_e_menu_item_ensure_onscreen(mi);
	return;
     }
   _e_menu_activate_first();
}

static void
_e_menu_item_activate_nth(int n)
{
   E_Menu *m;
   E_Menu_Item *mi;
   Eina_List *ll;
   int i;
   
   mi = _e_menu_item_active_get();
   if (!mi)
     {
	_e_menu_activate_first();
	mi = _e_menu_item_active_get();
	if (!mi) return;
     }
   m = mi->menu;
   for (i = -1, ll = m->items; ll; ll = ll->next)
     {
	mi = ll->data;
	if (!mi->separator) i++;
	if (i == n) break;
     }
   e_menu_item_active_set(mi, 1);
   _e_menu_item_ensure_onscreen(mi);
}

static void
_e_menu_item_activate_char(char * key_compose)
{
   E_Menu *m;
   E_Menu_Item *mi;
   Eina_List *ll, *ll_orig;

   /* Ignore modifiers and such. */
   if (!key_compose) return;

   /* Check we've got a menu and it's active. */
   m = _e_menu_active_get();
   if (!m) 
     {
	if (!_e_active_menus) return;
	m = _e_active_menus->data;
	if (!m) return;
     }

   ll = _e_menu_list_item_active_get();
   /* If we don't have an active item, start from the top of the list. */
   if (!ll)
     {
	ll = m->items;
	mi = ll->data;
	/* Only check the current item if it wasn't active before. */
	if (!mi->separator && mi->label && !strncasecmp(key_compose, mi->label, strlen(key_compose))) 
	  {
	     e_menu_item_active_set(mi, 1);
	     _e_menu_item_ensure_onscreen(mi);
	     return;
	  }
     }

   ll_orig = ll;

   mi = ll->data;
   if (!(ll->next)) 
     ll = mi->menu->items;
   else
     ll = ll->next;
   mi = ll->data;

   /* While we don't have a label OR we don't match  AND we haven't
    * wrapped around */
   while ((!mi->label || strncasecmp(key_compose, mi->label, strlen(key_compose))) 
	 && ll != ll_orig)
     {
	if (!(ll->next)) 
	  ll = mi->menu->items;
	else
	  ll = ll->next;
	mi = ll->data;

	while (mi->separator)
	  { 
	     if (!(ll->next)) 
	       ll = mi->menu->items;
	     else
	       ll = ll->next;
	     mi = ll->data;
	  }
     }

   e_menu_item_active_set(mi, 1);
   _e_menu_item_ensure_onscreen(mi);
   return;
}

static void
_e_menu_activate_next(void)
{
   E_Menu_Item *mi;

   mi = _e_menu_item_active_get();
   if (mi)
     {
	if (mi->submenu)
	  {
	     if (mi->submenu->items)
	       {
		  mi = mi->submenu->items->data;
		  e_menu_item_active_set(mi, 1);
		  _e_menu_item_ensure_onscreen(mi);
	       }
	  }
	return;
     }
   _e_menu_activate_first();
}

static void
_e_menu_activate_previous(void)
{
   E_Menu_Item *mi;

   mi = _e_menu_item_active_get();
   if (mi)
     {
	if (mi->menu->parent_item)
	  {
	     mi = mi->menu->parent_item;
	     e_menu_item_active_set(mi, 1);
	     _e_menu_item_ensure_onscreen(mi);
	  }
	return;
     }
   _e_menu_activate_last();
}

static void
_e_menu_activate_first(void)
{
   E_Menu *m;
   E_Menu_Item *mi;
   Eina_List *ll;
   
   if (!_e_active_menus) return;
   m = _e_active_menus->data;
   if (!m->items) return;
   ll = m->items;
   mi = ll->data;
   while ((mi->separator) && (ll->next))
     {
	ll = ll->next;
	mi = ll->data;
     }
   if (mi->separator) return;
   e_menu_item_active_set(mi, 1);   
   _e_menu_item_ensure_onscreen(mi);
}

static void
_e_menu_activate_last(void)
{
   E_Menu *m;
   E_Menu_Item *mi;
   Eina_List *ll;
   
   if (!_e_active_menus) return;
   m = _e_active_menus->data;
   if (!m->items) return;
   ll = eina_list_last(m->items);
   mi = ll->data;
   while ((mi->separator) && (ll->prev))
     {
	ll = ll->prev;
	mi = ll->data;
     }
   if (mi->separator) return;
   e_menu_item_active_set(mi, 1);   
   _e_menu_item_ensure_onscreen(mi);
}

#if 0
static void
_e_menu_activate_nth(int n)
{
   E_Menu *m;
   E_Menu_Item *mi;
   Eina_List *ll;
   int i;
   
   mi = _e_menu_item_active_get();
   if (!mi)
     {
	_e_menu_activate_first();
	mi = _e_menu_item_active_get();
	if (!mi) return;
     }
   m = mi->menu;
   for (i = -1, ll = m->items; ll; ll = ll->next)
     {
	E_Menu_Item *mi;
	
	mi = ll->data;
	if (!mi->separator) i++;
	if (i == n)
	  {
	     e_menu_item_active_set(mi, 1);
	     _e_menu_item_ensure_onscreen(mi);
	     return;
	  }
     }
}
#endif

static E_Menu *
_e_menu_active_get(void)
{
   if (_e_active_menu_item) return _e_active_menu_item->menu;
   return NULL;
}

static E_Menu_Item *
_e_menu_item_active_get(void)
{
   return _e_active_menu_item;
}

static Eina_List * 
_e_menu_list_item_active_get(void)
{
   if (_e_active_menu_item)
      return _e_active_menu_item->list_position;
   else
     return NULL;
}

static int
_e_menu_outside_bounds_get(int xdir, int ydir)
{
   Eina_List *l;
   int outl = 0;
   int outr = 0;
   int outt = 0;
   int outb = 0;
   int i;
   
   for (l = _e_active_menus; l; l = l->next)
     {
	E_Menu *m;

	m = l->data;
	if (m->cur.x < m->zone->x + e_config->menu_autoscroll_margin)
	  {
	     i = m->zone->x - m->cur.x + e_config->menu_autoscroll_margin;
	     if (i > outl) outl = i;
	  }
	if (m->cur.y < m->zone->y + e_config->menu_autoscroll_margin)
	  {
	     i = m->zone->y - m->cur.y + e_config->menu_autoscroll_margin;
	     if (i > outt) outt = i;
	  }
	if ((m->cur.x + m->cur.w) > (m->zone->w - e_config->menu_autoscroll_margin))
	  {
	     i = m->cur.x + m->cur.w - (m->zone->x + m->zone->w - e_config->menu_autoscroll_margin);
	     if (i > outr) outr = i;
	  }
	if ((m->cur.y + m->cur.h) > (m->zone->h - e_config->menu_autoscroll_margin))
	  {
	     i = m->cur.y + m->cur.h - (m->zone->y + m->zone->h - e_config->menu_autoscroll_margin);
	     if (i > outb) outb = i;
	  }
     }
   if (xdir == -1)
     {
	if (outl) return outl;
     }
   else if (xdir == 1)
     {
	if (outr) return outr;
     }
   else if (ydir == -1)
     {
	if (outt) return outt;
     }
   else if (ydir == 1)
     {
	if (outb) return outb;
     }
   return 0;
}

static void
_e_menu_scroll_by(int dx, int dy)
{
   Eina_List *l;
   
   for (l = _e_active_menus; l; l = l->next)
     {
	E_Menu *m;
	
	m = l->data;
	m->cur.x += dx;
	m->cur.y += dy;
     }
}

static void
_e_menu_mouse_autoscroll_check(void)
{
   int autoscroll_x = 0;
   int autoscroll_y = 0;
   
   if (_e_menu_x - e_config->menu_autoscroll_cursor_margin <= 0)
     {
	if (_e_menu_outside_bounds_get(-1, 0)) autoscroll_x = -1;
     }
   if (_e_menu_y - e_config->menu_autoscroll_cursor_margin <= 0)
     {
	if (_e_menu_outside_bounds_get(0, -1)) autoscroll_y = -1;
     }
   if ((!autoscroll_x) && (!autoscroll_y))
     {
	if (_e_active_menus)
	  {
	     E_Menu *m;
	     
	     m = _e_active_menus->data;
	     if (_e_menu_x + e_config->menu_autoscroll_cursor_margin >= (m->zone->w - 1))
	       {
		  if (_e_menu_outside_bounds_get(1, 0)) autoscroll_x = 1;
	       }
	     if (_e_menu_y + e_config->menu_autoscroll_cursor_margin >= (m->zone->h - 1))
	       {
		  if (_e_menu_outside_bounds_get(0, 1)) autoscroll_y = 1;
	       }
	  }
     }
   _e_menu_autoscroll_x = autoscroll_x;
   _e_menu_autoscroll_y = autoscroll_y;
   if ((!autoscroll_x) && (!autoscroll_y)) return;
   if (_e_menu_scroll_animator) return;
   _e_menu_scroll_animator = ecore_animator_add(_e_menu_cb_scroll_animator,
						NULL);
   _e_menu_scroll_start = ecore_time_get();
}

static void
_e_menu_item_ensure_onscreen(E_Menu_Item *mi)
{
   int x, y, w, h;
   int dx, dy;
   
   if (!mi->menu) return;
   if (!mi->menu->zone) return;
   x = mi->x + mi->menu->cur.x;
   y = mi->y + mi->menu->cur.y;
   w = mi->w;
   h = mi->h;
   dx = 0;
   dy = 0;
   if ((x + w) > (mi->menu->zone->x + mi->menu->zone->w))
     dx = (mi->menu->zone->x + mi->menu->zone->w) - (x + w);
   else if (x < mi->menu->zone->x)
     dx = mi->menu->zone->x - x;
   if ((y + h) > (mi->menu->zone->y + mi->menu->zone->h))
     dy = (mi->menu->zone->y + mi->menu->zone->h) - (y + h);
   else if (y < mi->menu->zone->y)
     dy = mi->menu->zone->y - y;
   if ((dx != 0) || (dy != 0))
     _e_menu_scroll_by(dx, dy);
}

static int
_e_menu_auto_place(E_Menu *m, int x, int y, int w, int h)
{
   double xr, yr;
   
   _e_menu_realize(m);
   /* +-----+
    * |\ T /|
    * | \ / |
    * |L X R|
    * | / \ |
    * |/ B \|
    * +-----+
    * 
    * quadrants... which one
    */
   if (w != m->zone->w)
     xr = (double)(x - m->zone->x) /
     (double)(m->zone->w - w);
   else
     xr = 0.0;
   if (h != m->zone->h)
     yr = (double)(y - m->zone->y) /
     (double)(m->zone->h - h);
   else
     yr = 0.0;
   if ((xr + yr) < 0.99) /* top or left */
     {
	if (((1.0 - yr) + xr) <= 1.0)
	  /* L */
	  {
	     m->cur.x = x + w;
	     if (y < (m->zone->y + ((m->zone->h * 1) / 3)))
	       m->cur.y = y;
	     else if (y < (m->zone->y + ((m->zone->h * 2) / 3)))
	       m->cur.y = y + ((h - m->cur.h) / 2);
	     else
	       m->cur.y = y + h - m->cur.h;
	     return 1;
	  }
	else
	  /* T */
	  {
	     m->cur.y = y + h;
	     if (x < (m->zone->x + ((m->zone->w * 1) / 3)))
	       m->cur.x = x;
	     else if (x < (m->zone->x + ((m->zone->w * 2) / 3)))
	       m->cur.x = x + ((w - m->cur.w) / 2);
	     else
	       m->cur.x = x + w - m->cur.w;
	     return 3;
	  }
     }
   else /* bottom or right */
     {
	if (((1.0 - yr) + xr) <= 1.01)
	  /* B */
	  {
	     m->cur.y = y - m->cur.h;
	     if (x < (m->zone->x + ((m->zone->w * 1) / 3)))
	       m->cur.x = x;
	     else if (x < (m->zone->x + ((m->zone->w * 2) / 3)))
	       m->cur.x = x + ((w - m->cur.w) / 2);
	     else
	       m->cur.x = x + w - m->cur.w;
	     return 4;
	  }
	else
	  /* R */
	  {
	     m->cur.x = x - m->cur.w;
	     if (y < (m->zone->y + ((m->zone->h * 1) / 3)))
	       m->cur.y = y;
	     else if (y < (m->zone->y + ((m->zone->h * 2) / 3)))
	       m->cur.y = y + ((h - m->cur.h) / 2);
	     else
	       m->cur.y = y + h - m->cur.h;
	     return 2;
	  }
     }
   return 0;
}

static void
_e_menu_cb_ecore_evas_resize(Ecore_Evas *ee)
{
   Evas *evas;
   Evas_Object *o;
   E_Menu *m;
   Evas_Coord w, h;
   
   evas = ecore_evas_get(ee);
   evas_output_viewport_get(evas, NULL, NULL, &w, &h);
   o = evas_object_name_find(evas, "menu/background");
   m = evas_object_data_get(o, "e_menu");
   evas_object_resize(o, w, h);
}

static void
_e_menu_cb_item_in(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Menu_Item *mi;
   
   mi = data;
   e_menu_item_active_set(mi, 1);
}

static void
_e_menu_cb_item_out(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Menu_Item *mi;
   Evas_Event_Mouse_In *ev;
   
   mi = data;
   ev = event_info;
   e_menu_item_active_set(mi, 0);
   if (_e_menu_activate_maybe_drag)
     {
	if (mi->drag_cb.func)
	  {
	     /* User is dragging a draggable item elsewhere. */
	     mi->drag.x = ev->output.x - (ev->output.x - mi->x);
	     mi->drag.y = ev->output.y - (ev->output.y - mi->y);
	     _e_menu_deactivate_all();
	     mi->drag_cb.func(mi->drag_cb.data, mi->menu, mi);
	  }
	/* Either way, the maybe drag stops here. */
	_e_menu_activate_maybe_drag = 0;
     }
}

static int
_e_menu_cb_key_down(void *data, int type, void *event)
{
   Ecore_X_Event_Key_Down *ev;
   
   ev = event;
   if (ev->win != _e_menu_win) return 1;
   if      ((!strcmp(ev->keysymbol, "Up")) ||
	    (!strcmp(ev->keysymbol, "KP_Up")))
     _e_menu_item_activate_previous();
   else if ((!strcmp(ev->keysymbol, "Down")) ||
	    (!strcmp(ev->keysymbol, "KP_Down")))
     _e_menu_item_activate_next();
   else if ((!strcmp(ev->keysymbol, "Left")) ||
	    (!strcmp(ev->keysymbol, "KP_Left")))
     _e_menu_activate_previous();
   else if ((!strcmp(ev->keysymbol, "Right")) ||
	    (!strcmp(ev->keysymbol, "KP_Right")))
     _e_menu_activate_next();
   else if ((!strcmp(ev->keysymbol, "Home")) ||
	    (!strcmp(ev->keysymbol, "KP_Home")))
     _e_menu_item_activate_first();
   else if ((!strcmp(ev->keysymbol, "End")) ||
	    (!strcmp(ev->keysymbol, "KP_End")))
     _e_menu_item_activate_last();
   else if (!strcmp(ev->keysymbol, "space"))
     {
	_e_menu_active_call();
     }
   else if ((!strcmp(ev->keysymbol, "Return")) ||
	    (!strcmp(ev->keysymbol, "KP_Enter")))
     {
	_e_menu_active_call();
	_e_menu_deactivate_all();
     }
   else if (!strcmp(ev->keysymbol, "Escape"))
     _e_menu_deactivate_all();
   else if ((!strcmp(ev->keysymbol, "1")) || (!strcmp(ev->keysymbol, "KP_1")))
     _e_menu_item_activate_first();
   else if ((!strcmp(ev->keysymbol, "2")) || (!strcmp(ev->keysymbol, "KP_2")))
     _e_menu_item_activate_nth(1);
   else if ((!strcmp(ev->keysymbol, "3")) || (!strcmp(ev->keysymbol, "KP_3")))
     _e_menu_item_activate_nth(2);
   else if ((!strcmp(ev->keysymbol, "4")) || (!strcmp(ev->keysymbol, "KP_4")))
     _e_menu_item_activate_nth(3);
   else if ((!strcmp(ev->keysymbol, "5")) || (!strcmp(ev->keysymbol, "KP_5")))
     _e_menu_item_activate_nth(4);
   else if ((!strcmp(ev->keysymbol, "6")) || (!strcmp(ev->keysymbol, "KP_6")))
     _e_menu_item_activate_nth(5);
   else if ((!strcmp(ev->keysymbol, "7")) || (!strcmp(ev->keysymbol, "KP_7")))
     _e_menu_item_activate_nth(6);
   else if ((!strcmp(ev->keysymbol, "8")) || (!strcmp(ev->keysymbol, "KP_8")))
     _e_menu_item_activate_nth(7);
   else if ((!strcmp(ev->keysymbol, "9")) || (!strcmp(ev->keysymbol, "KP_9")))
     _e_menu_item_activate_nth(8);
   else if ((!strcmp(ev->keysymbol, "0")) || (!strcmp(ev->keysymbol, "KP_0")))
     _e_menu_item_activate_last();
   else if (ev->key_compose)
     _e_menu_item_activate_char(ev->key_compose);
   return 1;
}

static int
_e_menu_cb_key_up(void *data, int type, void *event)
{
   Ecore_X_Event_Key_Up *ev;
   
   ev = event;
   if (ev->win != _e_menu_win) return 1;
   return 1;
}

/* we need all of these because menus are special and grab the mouse and
 * keyboard and thus the normal event mechanism doesnt work, so we feed
 * events directly to the canvases from our grab window
 */

static int
_e_menu_cb_mouse_down(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Down *ev;
   
   ev = event;
   if (ev->win != _e_menu_win) return 1;

   /* Only allow dragging from floating menus for now.
    * The reason for this is that for non floating menus, 
    * the mouse is already down and dragging, so the decision
    * to start a drag is much more complex.
    */
   if (_e_menu_activate_floating)
     _e_menu_activate_maybe_drag = 1;

   return 1;
}

static int
_e_menu_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Up *ev;
   Ecore_X_Time t;
   int ret = 0;
   
   ev = event;
   if (ev->win != _e_menu_win) return 1;

   t = ev->time - _e_menu_activate_time;
   if ((_e_menu_activate_time != 0) &&
       (t < (e_config->menus_click_drag_timeout * 1000)))
     {
	_e_menu_activate_floating = 1;
	return 1;
     }

   if (_e_menu_activate_dragging)
     {
	/* FIXME: This is a drop, which is not allowed for now.
	 * Once dragging is working, this will be subject to some experimenattion.
	 */
     }
   else
      ret = _e_menu_active_call();
   _e_menu_activate_maybe_drag = 0;
   _e_menu_activate_dragging = 0;
   if (ret == 1)
     {
/* allow mouse to pop down menu if clicked elsewhere */	
/*	if (_e_menu_activate_time != 0) */
	  _e_menu_deactivate_all();
     }
   else if (ret == -1)
     _e_menu_deactivate_all();
   else if (!_e_menu_activate_floating)
     _e_menu_deactivate_all();
   return 1;
}

static int
_e_menu_cb_mouse_move(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Move *ev;
   Eina_List *l, *tmp = NULL;
   int dx, dy, d;
   double dt;
   double fast_move_threshold;
   int is_fast = 0;
   
   ev = event;
   if (ev->win != _e_menu_win) return 1;
   fast_move_threshold = e_config->menus_fast_mouse_move_threshhold;
   dx = ev->x - _e_menu_x;
   dy = ev->y - _e_menu_y;
   d = (dx * dx) + (dy * dy);
   dt = (double)(ev->time - _e_menu_time) / 1000.0;
   dt = dt * dt;
   if ((dt > 0.0) && ((d / dt) >= (fast_move_threshold * fast_move_threshold)))
     is_fast = 1;

   for (l = _e_active_menus; l; l = l->next)
     {
	tmp = eina_list_append(tmp, l->data);
	e_object_ref(E_OBJECT(l->data));
     }
   for (l = _e_active_menus; l; l = l->next)
     {
	E_Menu *m;
	
	m = l->data;
	if ((m->realized) && (m->cur.visible))
	  {
	     if (is_fast)
	       m->fast_mouse = 1;
	     else if (dt > 0.0)
	       {
		  m->fast_mouse = 0;
		  if (m->pending_new_submenu)
		    {
		       E_Menu_Item *mi;
		       
		       mi = _e_menu_item_active_get();
		       if (mi)
			 _e_menu_submenu_activate(mi);
		    }
	       }
	     evas_event_feed_mouse_move(m->evas,
					ev->x - m->cur.x + m->zone->x,
					ev->y - m->cur.y + m->zone->y,
					ev->time,
					NULL);
	  }
     }
   while (tmp)
     {
	e_object_unref(E_OBJECT(tmp->data));
	tmp = eina_list_remove_list(tmp, tmp);
     }
     
   _e_menu_x = ev->x;
   _e_menu_y = ev->y;
   _e_menu_time = ev->time;
   _e_menu_mouse_autoscroll_check();
   return 1;
}

static int
_e_menu_cb_mouse_wheel(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Wheel *ev;
   
   ev = event;
   if (ev->win != _e_menu_win) return 1;
   if (ev->z < 0) /* up */
     {
	int i;
	
	for (i = ev->z; i < 0; i++)
	  _e_menu_item_activate_previous();
     }
   else if (ev->z > 0) /* down */
     {
	int i;
	
	for (i = ev->z; i > 0; i--)
	  _e_menu_item_activate_next();
     }
   return 1;
}

static int
_e_menu_cb_scroll_animator(void *data)
{
   double t, dt;
   double dx, dy;
   int out;
   double spd;
   
   t = ecore_time_get();
   spd = e_config->menus_scroll_speed;
   dt = t - _e_menu_scroll_start;
   _e_menu_scroll_start = t;
   dx = 0;
   dy = 0;
   if (_e_menu_autoscroll_x)
     {
	out = _e_menu_outside_bounds_get(_e_menu_autoscroll_x, 0);
	dx = (-_e_menu_autoscroll_x) * spd * dt;
	if (_e_menu_autoscroll_x == -1)
	  {
	     if (dx > out) dx = out;
	  }
	else
	  {
	     if (dx < -out) dx = -out;
	  }
     }
   if (_e_menu_autoscroll_y)
     {
	out = _e_menu_outside_bounds_get(0, _e_menu_autoscroll_y);
	dy = (-_e_menu_autoscroll_y) * spd * dt;
	if (_e_menu_autoscroll_y == -1)
	  {
	     if (dy > out) dy = out;
	  }
	else
	  {
	     if (dy < -out) dy = -out;
	  }
     }
   _e_menu_scroll_by(dx, dy);
   _e_menu_mouse_autoscroll_check();
   if ((_e_menu_autoscroll_x == 0) && (_e_menu_autoscroll_y == 0))
     {
	_e_menu_scroll_animator = NULL;
	return 0;
     }
   return 1;
}

static int
_e_menu_cb_window_shape(void *data, int ev_type, void *ev)
{
   Eina_List *l;
   Ecore_X_Event_Window_Shape *e;
   
   e = ev;
   for (l = _e_active_menus; l; l = l->next)
     {
	E_Menu *m;
	
	m = l->data;
	if (m->evas_win == e->win)
	  m->need_shape_export = 1;
     }
   return 1;
}

static void
_e_menu_cb_item_submenu_post_default(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Menu *subm;

   if (!mi->submenu) return;

   subm = mi->submenu;
   e_menu_item_submenu_set(mi, NULL);
   e_object_del(E_OBJECT(subm));
}


static Evas_Bool
_e_menu_categories_free_cb(const Evas_Hash *hash, const void *key, void *data, void *fdata)
{
   Eina_List *l;
   E_Menu_Category *cat;
   
   cat = (E_Menu_Category *) data;
   l = (Eina_List *) cat->callbacks;
   while (l)
     {
	free(l->data); /* free the callback struct */
	l = eina_list_remove_list(l, l);
     }
   free(cat);
   return 1;
}
