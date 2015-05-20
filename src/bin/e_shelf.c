#include "e.h"

static void         _e_shelf_new_dialog_ok(void *data, char *text);
static void         _e_shelf_del_cb(void *d);
static void         _e_shelf_free(E_Shelf *es);
static void         _e_shelf_gadcon_min_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h);
static void         _e_shelf_gadcon_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h);
static Evas_Object *_e_shelf_gadcon_frame_request(void *data, E_Gadcon_Client *gcc, const char *style);
static void         _e_shelf_toggle_border_fix(E_Shelf *es);
static void         _e_shelf_cb_menu_config(void *data, E_Menu *m, E_Menu_Item *mi);
static void         _e_shelf_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);
static void         _e_shelf_cb_menu_contents(void *data, E_Menu *m, E_Menu_Item *mi);
static void         _e_shelf_cb_confirm_dialog_yes(void *data);
static void         _e_shelf_cb_menu_delete(void *data, E_Menu *m, E_Menu_Item *mi);
static void         _e_shelf_menu_append(E_Shelf *es, E_Menu *mn);
static void         _e_shelf_cb_menu_items_append(void *data, E_Gadcon_Client *gcc, E_Menu *mn);
static void         _e_shelf_cb_locked_set(void *data, int lock);
static void         _e_shelf_cb_urgent_show(void *data);
static void         _e_shelf_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static Eina_Bool    _e_shelf_cb_mouse_in(void *data, int type, void *event);
//static Eina_Bool    _e_shelf_cb_mouse_out(void *data, int type, void *event);
//static void          _e_shelf_cb_mouse_out2(E_Shelf *es, Evas *e, Evas_Object *obj, Evas_Event_Mouse_Out *ev);
static int          _e_shelf_cb_id_sort(const void *data1, const void *data2);
static void         _e_shelf_cb_menu_rename(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__);
static Eina_Bool    _e_shelf_cb_hide_animator(void *data);
static Eina_Bool    _e_shelf_cb_hide_animator_timer(void *data);
static Eina_Bool    _e_shelf_cb_hide_urgent_timer(void *data);
static Eina_Bool    _e_shelf_cb_instant_hide_timer(void *data);
static void         _e_shelf_menu_pre_cb(void *data, E_Menu *m);
static void         _e_shelf_gadcon_client_remove(void *data, E_Gadcon_Client *gcc);
static int          _e_shelf_gadcon_client_add(void *data, E_Gadcon_Client *gcc, const E_Gadcon_Client_Class *cc);
static const char  *_e_shelf_orient_icon_name_get(E_Shelf *s);
static void         _e_shelf_bindings_add(E_Shelf *es);
static void         _e_shelf_bindings_del(E_Shelf *es);
static Eina_Bool    _e_shelf_on_current_desk(E_Shelf *es, E_Event_Zone_Edge *ev);
static void          _e_shelf_cb_dummy_del(E_Shelf *, Evas *e, Evas_Object *obj, void *event_info);
static void          _e_shelf_cb_dummy_moveresize(E_Shelf *, Evas *e, Evas_Object *obj, void *event_info);
static Eina_Bool    _e_shelf_gadcon_populate_handler_cb(void *, int, void *);
static Eina_Bool    _e_shelf_module_init_end_handler_cb(void *, int, void *);
static void          _e_shelf_event_rename_end_cb(void *data, E_Event_Shelf *ev);

static Eina_List *shelves = NULL;
static Eina_List *dummies = NULL;
static Eina_Hash *winid_shelves = NULL;

static int orientations[] =
{
   [E_GADCON_ORIENT_FLOAT] = 2,
   [E_GADCON_ORIENT_HORIZ] = 2,
   [E_GADCON_ORIENT_VERT] = 2,
   [E_GADCON_ORIENT_LEFT] = 37,
   [E_GADCON_ORIENT_RIGHT] = 31,
   [E_GADCON_ORIENT_TOP] = 29,
   [E_GADCON_ORIENT_BOTTOM] = 23,
   [E_GADCON_ORIENT_CORNER_TL] = 19,
   [E_GADCON_ORIENT_CORNER_TR] = 17,
   [E_GADCON_ORIENT_CORNER_BL] = 13,
   [E_GADCON_ORIENT_CORNER_BR] = 11,
   [E_GADCON_ORIENT_CORNER_LT] = 7,
   [E_GADCON_ORIENT_CORNER_RT] = 5,
   [E_GADCON_ORIENT_CORNER_LB] = 3,
   [E_GADCON_ORIENT_CORNER_RB] = 2
};

static const char *orient_names[] =
{
   [E_GADCON_ORIENT_FLOAT] = N_("Float"),
   [E_GADCON_ORIENT_HORIZ] = N_("Horizontal"),
   [E_GADCON_ORIENT_VERT] = N_("Vertical"),
   [E_GADCON_ORIENT_LEFT] = N_("Left"),
   [E_GADCON_ORIENT_RIGHT] = N_("Right"),
   [E_GADCON_ORIENT_TOP] = N_("Top"),
   [E_GADCON_ORIENT_BOTTOM] = N_("Bottom"),
   [E_GADCON_ORIENT_CORNER_TL] = N_("Top-left Corner"),
   [E_GADCON_ORIENT_CORNER_TR] = N_("Top-right Corner"),
   [E_GADCON_ORIENT_CORNER_BL] = N_("Bottom-left Corner"),
   [E_GADCON_ORIENT_CORNER_BR] = N_("Bottom-right Corner"),
   [E_GADCON_ORIENT_CORNER_LT] = N_("Left-top Corner"),
   [E_GADCON_ORIENT_CORNER_RT] = N_("Right-top Corner"),
   [E_GADCON_ORIENT_CORNER_LB] = N_("Left-bottom Corner"),
   [E_GADCON_ORIENT_CORNER_RB] = N_("Right-bottom Corner")
};

EAPI int E_EVENT_SHELF_RENAME = -1;
EAPI int E_EVENT_SHELF_ADD = -1;
EAPI int E_EVENT_SHELF_DEL = -1;
static Ecore_Event_Handler *_e_shelf_gadcon_populate_handler = NULL;
static Ecore_Event_Handler *_e_shelf_module_init_end_handler = NULL;
static Ecore_Event_Handler *_e_shelf_zone_moveresize_handler = NULL;

/* externally accessible functions */
EINTERN int
e_shelf_init(void)
{
   E_EVENT_SHELF_RENAME = ecore_event_type_new();
   E_EVENT_SHELF_ADD = ecore_event_type_new();
   E_EVENT_SHELF_DEL = ecore_event_type_new();
   _e_shelf_gadcon_populate_handler = ecore_event_handler_add(E_EVENT_GADCON_POPULATE, _e_shelf_gadcon_populate_handler_cb, NULL);
   _e_shelf_module_init_end_handler = ecore_event_handler_add(E_EVENT_MODULE_INIT_END, _e_shelf_module_init_end_handler_cb, NULL);
   return 1;
}

EINTERN int
e_shelf_shutdown(void)
{
   if (x_fatal) return 1;
   while (shelves)
     {
        E_Shelf *es;

        es = eina_list_data_get(shelves);
        e_object_del(E_OBJECT(es));
     }
   if (_e_shelf_gadcon_populate_handler)
     _e_shelf_gadcon_populate_handler = ecore_event_handler_del(_e_shelf_gadcon_populate_handler);
   if (_e_shelf_module_init_end_handler)
     _e_shelf_module_init_end_handler = ecore_event_handler_del(_e_shelf_module_init_end_handler);
   if (_e_shelf_zone_moveresize_handler)
     _e_shelf_zone_moveresize_handler = ecore_event_handler_del(_e_shelf_zone_moveresize_handler);

   return 1;
}

EAPI void
e_shelf_config_update(void)
{
   Eina_List *l;
   E_Config_Shelf *cf_es;
   int id = 0;

   while (shelves)
     {
        E_Shelf *es;

        es = eina_list_data_get(shelves);
        e_object_del(E_OBJECT(es));
     }

   EINA_LIST_FOREACH(e_config->shelves, l, cf_es)
     {
        E_Zone *zone;

        if (cf_es->id <= 0) cf_es->id = id + 1;
        zone = e_util_container_zone_number_get(cf_es->container, cf_es->zone);
        if (zone)
          e_shelf_config_new(zone, cf_es);
        id = cf_es->id;
     }
}

EAPI Eina_List *
e_shelf_list_all(void)
{
   Eina_List *d = NULL, *s = NULL, *ret = NULL;

   if (shelves)
     s = eina_list_clone(shelves);
   if (dummies)
     d = eina_list_clone(dummies);
   if (s && d)
     ret = eina_list_merge(s, d);
   else
     ret = d ?: s;
   return ret;
}

EAPI Eina_List *
e_shelf_list(void)
{
   shelves = eina_list_sort(shelves, -1, _e_shelf_cb_id_sort);
   return shelves;
}

EAPI E_Shelf *
e_shelf_zone_dummy_new(E_Zone *zone, Evas_Object *obj, int id)
{
   E_Shelf *es;

   es = E_OBJECT_ALLOC(E_Shelf, E_SHELF_DUMMY_TYPE, _e_shelf_free);
   if (!es) return NULL;
   es->id = id;
   evas_object_geometry_get(obj, &es->x, &es->y, &es->w, &es->h);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL, 
                                  (Evas_Object_Event_Cb)_e_shelf_cb_dummy_del, es);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, 
                                  (Evas_Object_Event_Cb)_e_shelf_cb_dummy_moveresize, es);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, 
                                  (Evas_Object_Event_Cb)_e_shelf_cb_dummy_moveresize, es);
   es->zone = zone;
   es->dummy = 1;
   es->o_base = obj;
   es->cfg = E_NEW(E_Config_Shelf, 1);
   
   e_object_del_attach_func_set(E_OBJECT(es), _e_shelf_del_cb);
   dummies = eina_list_append(dummies, es);

   return es;
}

EAPI E_Shelf *
e_shelf_zone_new(E_Zone *zone, const char *name, const char *style, int popup, E_Layer layer, int id)
{
   E_Shelf *es;
   char buf[1024];

   es = E_OBJECT_ALLOC(E_Shelf, E_SHELF_TYPE, _e_shelf_free);
   if (!es) return NULL;
   es->id = id;
   es->x = 0;
   es->y = 0;
   es->w = 32;
   es->h = 32;
   es->zone = zone;
   e_object_del_attach_func_set(E_OBJECT(es), _e_shelf_del_cb);
   e_zone_useful_geometry_dirty(zone);
   if (popup)
     {
        es->popup = e_popup_new(zone, es->x, es->y, es->w, es->h);
        e_popup_name_set(es->popup, "shelf");
        e_popup_layer_set(es->popup, layer);
        es->ee = es->popup->ecore_evas;
        es->evas = es->popup->evas;
     }
   else
     {
        es->ee = zone->container->bg_ecore_evas;
        es->evas = zone->container->bg_evas;
     }
   es->fit_along = 1;
   es->layer = layer;

   es->o_event = evas_object_rectangle_add(es->evas);
   evas_object_color_set(es->o_event, 0, 0, 0, 0);
   evas_object_resize(es->o_event, es->w, es->h);
   evas_object_event_callback_add(es->o_event, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _e_shelf_cb_mouse_down, es);

   es->handlers = 
     eina_list_append(es->handlers,
                      ecore_event_handler_add(E_EVENT_ZONE_EDGE_MOVE, 
                                              _e_shelf_cb_mouse_in, es));
   es->o_base = edje_object_add(es->evas);
   es->name = eina_stringshare_add(name);
   evas_object_resize(es->o_base, es->w, es->h);
   
   e_shelf_style_set(es, style);
   
   if (es->popup)
     {
        evas_object_show(es->o_event);
        evas_object_show(es->o_base);
        ecore_x_netwm_window_type_set(es->popup->evas_win, 
                                      ECORE_X_WINDOW_TYPE_DOCK);
     }
   else
     {
        evas_object_move(es->o_event, es->zone->x + es->x, es->zone->y + es->y);
        evas_object_move(es->o_base, es->zone->x + es->x, es->zone->y + es->y);
        evas_object_layer_set(es->o_event, layer);
        evas_object_layer_set(es->o_base, layer);
     }

   es->gadcon = 
     e_gadcon_swallowed_new(es->name, es->id, es->o_base, "e.swallow.content");
   if (es->name)
     snprintf(buf, sizeof(buf), "%s", es->name);
   else
     snprintf(buf, sizeof(buf), _("Shelf #%d"), es->id);
   es->gadcon->location = 
     e_gadcon_location_new(buf, E_GADCON_SITE_SHELF, 
                           _e_shelf_gadcon_client_add, es, 
                           _e_shelf_gadcon_client_remove, es);
   e_gadcon_location_register(es->gadcon->location);
// hmm dnd in ibar and ibox kill this. ok. need to look into this more
//   es->gadcon->instant_edit = 1;
   e_gadcon_min_size_request_callback_set(es->gadcon,
                                          _e_shelf_gadcon_min_size_request, es);

   e_gadcon_size_request_callback_set(es->gadcon,
                                      _e_shelf_gadcon_size_request, es);
   e_gadcon_frame_request_callback_set(es->gadcon,
                                       _e_shelf_gadcon_frame_request, es);
   e_gadcon_orient(es->gadcon, E_GADCON_ORIENT_TOP);
   snprintf(buf, sizeof(buf), "e,state,orientation,%s",
            e_shelf_orient_string_get(es));
   edje_object_signal_emit(es->o_base, buf, "e");
   edje_object_message_signal_process(es->o_base);
   e_gadcon_zone_set(es->gadcon, zone);
   e_gadcon_ecore_evas_set(es->gadcon, es->ee);
   e_gadcon_shelf_set(es->gadcon, es);
   if (popup)
     {
        if (!winid_shelves)
          winid_shelves = eina_hash_string_superfast_new(NULL);
        eina_hash_add(winid_shelves,
                      e_util_winid_str_get(es->popup->evas_win), es);
        e_drop_xdnd_register_set(es->popup->evas_win, 1);
        e_gadcon_xdnd_window_set(es->gadcon, es->popup->evas_win);
        e_gadcon_dnd_window_set(es->gadcon, es->popup->evas_win);
     }
   else
     {
        e_drop_xdnd_register_set(es->zone->container->bg_win, 1);
        e_gadcon_xdnd_window_set(es->gadcon, es->zone->container->bg_win);
        e_gadcon_dnd_window_set(es->gadcon, es->zone->container->event_win);
        evas_object_clip_set(es->o_base, es->zone->bg_clip_object);
     }
   e_gadcon_util_menu_attach_func_set(es->gadcon,
                                      _e_shelf_cb_menu_items_append, es);

   e_gadcon_util_lock_func_set(es->gadcon,
                               _e_shelf_cb_locked_set, es);
   e_gadcon_util_urgent_show_func_set(es->gadcon,
                                      _e_shelf_cb_urgent_show, es);

   shelves = eina_list_append(shelves, es);

   es->hidden = 0;
   es->hide_step = 0;
   es->locked = 0;

   es->hide_origin = -1;

   {
      E_Event_Shelf *ev;

      ev = E_NEW(E_Event_Shelf, 1);
      ev->shelf = es;
      ecore_event_add(E_EVENT_SHELF_ADD, ev, NULL, NULL);
   }

   return es;
}

EAPI void
e_shelf_rename_dialog(E_Shelf *es)
{
   if (!es) return;
   if (es->rename_dialog) return;
   _e_shelf_cb_menu_rename(es, NULL, NULL);
}

EAPI void
e_shelf_zone_move_resize_handle(E_Zone *zone)
{
   Eina_List *l;
   E_Shelf *es;
   Evas_Coord w, h;

   EINA_LIST_FOREACH(shelves, l, es)
     {
        if (es->zone == zone)
          {
             E_Gadcon *gc;

             gc = es->gadcon;
             if (gc->min_size_request.func)
               {
                  /* let gadcon container decrease to any size */
                  evas_object_size_hint_min_set(gc->o_container, 0, 0);
               }
             evas_object_smart_callback_call(gc->o_container, "min_size_request", NULL);
             e_shelf_position_calc(es);
             if (gc->min_size_request.func)
               {
                  evas_object_geometry_get(gc->o_container, NULL, NULL, &w, &h);
                  /* fix gadcon container min size to current geometry */
                  evas_object_size_hint_min_set(gc->o_container, w, h);
               }
          }
     }
}

EAPI void
e_shelf_populate(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);
   e_gadcon_populate(es->gadcon);
}

EAPI void
e_shelf_show(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);
   if (es->popup)
     e_popup_show(es->popup);
   else
     {
        evas_object_show(es->o_event);
        evas_object_show(es->o_base);
     }
}

EAPI void
e_shelf_hide(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);
   if (es->popup)
     e_popup_hide(es->popup);
   else
     {
        evas_object_hide(es->o_event);
        evas_object_hide(es->o_base);
     }
}

EAPI void
e_shelf_locked_set(E_Shelf *es, int lock)
{
   if (lock)
     {
        e_shelf_toggle(es, 1);
        es->locked++;
     }
   else
     {
        if (es->locked > 0)
          es->locked--;
        if (!es->locked)
          e_shelf_toggle(es, es->toggle);
     }
}

EAPI void
e_shelf_name_set(E_Shelf *es, const char *name)
{
   E_Event_Shelf *ev;

   if (!es) return;
   if (!name) return;
   if (es->name == name) return;
   eina_stringshare_replace(&es->name, name);
   eina_stringshare_replace(&es->cfg->name, name);
   ev = E_NEW(E_Event_Shelf, 1);
   ev->shelf = es;
   e_object_ref(E_OBJECT(es));
   ecore_event_add(E_EVENT_SHELF_RENAME, ev, (Ecore_End_Cb)_e_shelf_event_rename_end_cb, NULL);
   if (es->dummy) return;
   e_gadcon_name_set(es->gadcon, name);
}

EAPI void
e_shelf_toggle(E_Shelf *es, int show)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);

   es->toggle = show;
   if (es->autohide_timer) ecore_timer_del(es->autohide_timer);
   es->autohide_timer = NULL;
   if (es->locked) return;
   es->interrupted = -1;
   es->urgent_show = 0;
   if ((show) && (es->hidden))
     {
        es->hidden = 0;
        edje_object_signal_emit(es->o_base, "e,state,visible", "e");
        if (es->instant_delay >= 0.0)
          {
             _e_shelf_cb_instant_hide_timer(es);
             es->hide_timer =
               ecore_timer_add(es->cfg->hide_timeout,
                               _e_shelf_cb_hide_urgent_timer, es);
          }
        else
          {
             if (es->hide_timer)
               {
                  ecore_timer_del(es->hide_timer);
                  es->hide_timer = NULL;
               }
             if (!es->hide_animator)
               es->hide_animator =
                 ecore_animator_add(_e_shelf_cb_hide_animator, es);
          }
     }
   else if ((!show) && (!es->hidden) && ((!es->gadcon) || (!es->gadcon->editing)) &&
            (es->cfg->autohide))
     {
        edje_object_signal_emit(es->o_base, "e,state,hidden", "e");
        if (es->instant_delay >= 0.0)
          {
             if (es->hide_timer)
               {
                  ecore_timer_del(es->hide_timer);
                  es->hide_timer = NULL;
               }
             es->hidden = 1;
             if (!es->instant_timer)
               es->instant_timer =
                 ecore_timer_add(es->instant_delay,
                                 _e_shelf_cb_instant_hide_timer, es);
          }
        else
          {
             if (es->hide_animator)
               {
                  es->interrupted = show;
                  return;
               }
             es->hidden = 1;
             if (es->hide_timer) ecore_timer_del(es->hide_timer);
             es->hide_timer =
               ecore_timer_add(es->cfg->hide_timeout,
                               _e_shelf_cb_hide_animator_timer, es);
          }
     }
}

EAPI void
e_shelf_urgent_show(E_Shelf *es)
{
   e_shelf_toggle(es, 1);
   es->urgent_show = 1;
}

EAPI void
e_shelf_move(E_Shelf *es, int x, int y)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);
   es->x = x;
   es->y = y;
   if (es->popup)
     e_popup_move(es->popup, es->x, es->y);
   else
     {
        evas_object_move(es->o_event, es->zone->x + es->x, es->zone->y + es->y);
        evas_object_move(es->o_base, es->zone->x + es->x, es->zone->y + es->y);
     }
}

EAPI void
e_shelf_resize(E_Shelf *es, int w, int h)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);
   es->w = w;
   es->h = h;
   if (es->popup) e_popup_resize(es->popup, es->w, es->h);
   evas_object_resize(es->o_event, es->w, es->h);
   evas_object_resize(es->o_base, es->w, es->h);
}

EAPI void
e_shelf_move_resize(E_Shelf *es, int x, int y, int w, int h)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);
   es->x = x;
   es->y = y;
   es->w = w;
   es->h = h;
   if (es->popup)
     e_popup_move_resize(es->popup, es->x, es->y, es->w, es->h);
   else
     {
        evas_object_move(es->o_event, es->zone->x + es->x, es->zone->y + es->y);
        evas_object_move(es->o_base, es->zone->x + es->x, es->zone->y + es->y);
     }
   evas_object_resize(es->o_event, es->w, es->h);
   evas_object_resize(es->o_base, es->w, es->h);
}

EAPI void
e_shelf_layer_set(E_Shelf *es, E_Layer layer)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);

   es->layer = layer;
   if (es->popup)
     e_popup_layer_set(es->popup, es->layer);
   else
     {
        evas_object_layer_set(es->o_event, es->layer);
        evas_object_layer_set(es->o_base, es->layer);
     }
}

EAPI void
e_shelf_save(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);
   if (es->cfg)
     {
        es->cfg->orient = es->gadcon->orient;
        if (es->cfg->style) eina_stringshare_del(es->cfg->style);
        es->cfg->style = eina_stringshare_add(es->style);
     }
   else
     {
        E_Config_Shelf *cf_es;

        cf_es = E_NEW(E_Config_Shelf, 1);
        cf_es->name = eina_stringshare_add(es->name);
        cf_es->container = es->zone->container->num;
        cf_es->zone = es->zone->num;
        if (es->popup) cf_es->popup = 1;
        cf_es->layer = es->layer;
        e_config->shelves = eina_list_append(e_config->shelves, cf_es);
        cf_es->orient = es->gadcon->orient;
        cf_es->style = eina_stringshare_add(es->style);
        cf_es->fit_along = es->fit_along;
        cf_es->fit_size = es->fit_size;
        cf_es->overlap = 0;
        cf_es->autohide = 0;
        cf_es->hide_timeout = 1.0;
        cf_es->hide_duration = 1.0;
        es->cfg = cf_es;
     }
   e_config_save_queue();
}

EAPI void
e_shelf_unsave(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);
   es->cfg_delete = 1;
}

EAPI void
e_shelf_orient(E_Shelf *es, E_Gadcon_Orient orient)
{
   char buf[4096];

   E_OBJECT_CHECK(es);
   E_OBJECT_IF_NOT_TYPE(es, E_SHELF_DUMMY_TYPE)
     E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);

   if (!es->dummy)
     {
        e_gadcon_orient(es->gadcon, orient);
        snprintf(buf, sizeof(buf), "e,state,orientation,%s",
                 e_shelf_orient_string_get(es));
        edje_object_signal_emit(es->o_base, buf, "e");
        edje_object_message_signal_process(es->o_base);
        e_gadcon_location_set_icon_name(es->gadcon->location, 
                                        _e_shelf_orient_icon_name_get(es));
     }
   e_zone_useful_geometry_dirty(es->zone);
}

EAPI const char *
e_shelf_orient_string_get(E_Shelf *es)
{
   const char *sig = "";

   switch (es->gadcon->orient)
     {
      case E_GADCON_ORIENT_FLOAT:
        sig = "float";
        break;

      case E_GADCON_ORIENT_HORIZ:
        sig = "horizontal";
        break;

      case E_GADCON_ORIENT_VERT:
        sig = "vertical";
        break;

      case E_GADCON_ORIENT_LEFT:
        sig = "left";
        break;

      case E_GADCON_ORIENT_RIGHT:
        sig = "right";
        break;

      case E_GADCON_ORIENT_TOP:
        sig = "top";
        break;

      case E_GADCON_ORIENT_BOTTOM:
        sig = "bottom";
        break;

      case E_GADCON_ORIENT_CORNER_TL:
        sig = "top_left";
        break;

      case E_GADCON_ORIENT_CORNER_TR:
        sig = "top_right";
        break;

      case E_GADCON_ORIENT_CORNER_BL:
        sig = "bottom_left";
        break;

      case E_GADCON_ORIENT_CORNER_BR:
        sig = "bottom_right";
        break;

      case E_GADCON_ORIENT_CORNER_LT:
        sig = "left_top";
        break;

      case E_GADCON_ORIENT_CORNER_RT:
        sig = "right_top";
        break;

      case E_GADCON_ORIENT_CORNER_LB:
        sig = "left_bottom";
        break;

      case E_GADCON_ORIENT_CORNER_RB:
        sig = "right_bottom";
        break;

      default:
        break;
     }
   return sig;
}

EAPI void
e_shelf_position_calc(E_Shelf *es)
{
   E_Gadcon_Orient orient = E_GADCON_ORIENT_FLOAT;
   int size = (40 * e_scale);
   int x, y, w, h;

   x = es->x, y = es->y, w = es->w, h = es->h;
   if (es->cfg)
     {
        orient = es->cfg->orient;
        size = es->cfg->size * e_scale;
     }
   else
     orient = es->gadcon->orient;
   switch (orient)
     {
      case E_GADCON_ORIENT_FLOAT:
        if (!es->fit_along) es->w = es->zone->w;
        if (!es->fit_size) es->h = size;
        break;

      case E_GADCON_ORIENT_HORIZ:
        if (!es->fit_along) es->w = es->zone->w;
        if (!es->fit_size) es->h = size;
        es->x = (es->zone->w - es->w) / 2;
        break;

      case E_GADCON_ORIENT_VERT:
        if (!es->fit_along) es->h = es->zone->h;
        if (!es->fit_size) es->w = size;
        es->y = (es->zone->h - es->h) / 2;
        break;

      case E_GADCON_ORIENT_LEFT:
        if (!es->fit_along) es->h = es->zone->h;
        if (!es->fit_size) es->w = size;
        es->x = 0;
        es->y = (es->zone->h - es->h) / 2;
        break;

      case E_GADCON_ORIENT_RIGHT:
        if (!es->fit_along) es->h = es->zone->h;
        if (!es->fit_size) es->w = size;
        es->x = es->zone->w - es->w;
        es->y = (es->zone->h - es->h) / 2;
        break;

      case E_GADCON_ORIENT_TOP:
        if (!es->fit_along) es->w = es->zone->w;
        if (!es->fit_size) es->h = size;
        es->x = (es->zone->w - es->w) / 2;
        es->y = 0;
        break;

      case E_GADCON_ORIENT_BOTTOM:
        if (!es->fit_along) es->w = es->zone->w;
        if (!es->fit_size) es->h = size;
        es->x = (es->zone->w - es->w) / 2;
        es->y = es->zone->h - es->h;
        break;

      case E_GADCON_ORIENT_CORNER_TL:
        if (!es->fit_along) es->w = es->zone->w;
        if (!es->fit_size) es->h = size;
        es->x = 0;
        es->y = 0;
        break;

      case E_GADCON_ORIENT_CORNER_TR:
        if (!es->fit_along) es->w = es->zone->w;
        if (!es->fit_size) es->h = size;
        es->x = es->zone->w - es->w;
        es->y = 0;
        break;

      case E_GADCON_ORIENT_CORNER_BL:
        if (!es->fit_along) es->w = es->zone->w;
        if (!es->fit_size) es->h = size;
        es->x = 0;
        es->y = es->zone->h - es->h;
        break;

      case E_GADCON_ORIENT_CORNER_BR:
        if (!es->fit_along) es->w = es->zone->w;
        if (!es->fit_size) es->h = size;
        es->x = es->zone->w - es->w;
        es->y = es->zone->h - es->h;
        break;

      case E_GADCON_ORIENT_CORNER_LT:
        if (!es->fit_along) es->h = es->zone->h;
        if (!es->fit_size) es->w = size;
        es->x = 0;
        es->y = 0;
        break;

      case E_GADCON_ORIENT_CORNER_RT:
        if (!es->fit_along) es->h = es->zone->h;
        if (!es->fit_size) es->w = size;
        es->x = es->zone->w - es->w;
        es->y = 0;
        break;

      case E_GADCON_ORIENT_CORNER_LB:
        if (!es->fit_along) es->h = es->zone->h;
        if (!es->fit_size) es->w = size;
        es->x = 0;
        es->y = es->zone->h - es->h;
        break;

      case E_GADCON_ORIENT_CORNER_RB:
        if (!es->fit_along) es->h = es->zone->h;
        if (!es->fit_size) es->w = size;
        es->x = es->zone->w - es->w;
        es->y = es->zone->h - es->h;
        break;

      default:
        break;
     }
   es->hide_step = 0;
   es->hide_origin = -1;

   if ((es->x == x) && (es->y == y) && (es->w == w) && (es->h == h)) return;

   e_shelf_move_resize(es, es->x, es->y, es->w, es->h);
   if (es->hidden)
     {
        es->hidden = 0;
        e_shelf_toggle(es, 0);
     }
   e_zone_useful_geometry_dirty(es->zone);
   _e_shelf_bindings_add(es);

   do
     {
        Eina_Bool err = EINA_FALSE;

        if (!es->cfg) break;
        if (!es->zone) break;
        if ((!es->cfg->popup) || (!es->cfg->autohide)) break;
        switch (es->cfg->orient)
          {
           case E_GADCON_ORIENT_LEFT:
           case E_GADCON_ORIENT_CORNER_LT:
           case E_GADCON_ORIENT_CORNER_LB:
             if (!e_zone_exists_direction(es->zone, E_ZONE_EDGE_LEFT)) break;
             err = EINA_TRUE;
             break;
           case E_GADCON_ORIENT_RIGHT:
           case E_GADCON_ORIENT_CORNER_RT:
           case E_GADCON_ORIENT_CORNER_RB:
             if (!e_zone_exists_direction(es->zone, E_ZONE_EDGE_RIGHT)) break;
             err = EINA_TRUE;
             break;
           case E_GADCON_ORIENT_TOP:
           case E_GADCON_ORIENT_CORNER_TL:
           case E_GADCON_ORIENT_CORNER_TR:
             if (!e_zone_exists_direction(es->zone, E_ZONE_EDGE_TOP)) break;
             err = EINA_TRUE;
             break;
           case E_GADCON_ORIENT_BOTTOM:
           case E_GADCON_ORIENT_CORNER_BL:
           case E_GADCON_ORIENT_CORNER_BR:
             if (!e_zone_exists_direction(es->zone, E_ZONE_EDGE_BOTTOM)) break;
             err = EINA_TRUE;
             break;
          }
        if (err)
          e_util_dialog_show(_("Shelf Autohide Error"), _("Shelf autohiding will not work properly<br>"
                                                          "with the current configuration; set your shelf to<br>"
                                                          "\"Below Everything\" or disable autohiding."));
        break;
     } while (0);
}

EAPI Eina_Bool
e_shelf_desk_visible(E_Shelf *es, E_Desk *desk)
{
   Eina_List *ll;
   E_Config_Shelf *cf_es;
   E_Zone *zone;
   E_Config_Shelf_Desk *sd;

   EINA_SAFETY_ON_NULL_RETURN_VAL(es, EINA_FALSE);
   if (!desk)
     {
        EINA_LIST_FOREACH(e_util_container_current_get()->zones, ll, zone)
          {
             desk = e_desk_current_get(zone);
             if (e_shelf_desk_visible(es, desk)) return EINA_TRUE;
          }
        return EINA_FALSE;
     }
   if (!es->cfg->desk_show_mode) return EINA_TRUE;
   cf_es = es->cfg;
   if (!cf_es) return EINA_FALSE;

   zone = desk->zone;
   if (cf_es->zone != (int)zone->num) return EINA_FALSE;

   EINA_LIST_FOREACH(es->cfg->desk_list, ll, sd)
     {
        if (!sd) continue;
        if ((desk->x == sd->x) && (desk->y == sd->y))
          return EINA_TRUE;
     }
   return EINA_FALSE;
}

EAPI void
e_shelf_style_set(E_Shelf *es, const char *style)
{
   const char *option;
   char buf[1024];

   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);

   if (!es->o_base) return;
   if (style != es->style)
     eina_stringshare_replace(&es->style, style);

   if (style)
     snprintf(buf, sizeof(buf), "e/shelf/%s/base", style);
   else
     snprintf(buf, sizeof(buf), "e/shelf/%s/base", "default");

   if (!e_theme_edje_object_set(es->o_base, "base/theme/shelf", buf))
     e_theme_edje_object_set(es->o_base, "base/theme/shelf",
                             "e/shelf/default/base");

   option = edje_object_data_get(es->o_base, "hidden_state_size");
   if (option)
     es->hidden_state_size = atoi(option);
   else
     es->hidden_state_size = 4;
   option = edje_object_data_get(es->o_base, "instant_delay");
   if (option)
     es->instant_delay = atof(option);
   else
     es->instant_delay = -1.0;

   if (es->popup) e_popup_edje_bg_object_set(es->popup, es->o_base);
}

EAPI void
e_shelf_popup_set(E_Shelf *es, int popup)
{
   /* FIXME: Needs to recreate the evas objects. */
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);

   if (!es->cfg) return;
   if (((popup) && (es->popup)) || ((!popup) && (!es->popup))) return;

   if (popup)
     {
        evas_object_clip_unset(es->o_base);
        es->popup = e_popup_new(es->zone, es->x, es->y, es->w, es->h);
        e_popup_name_set(es->popup, "shelf");
        e_popup_layer_set(es->popup, es->cfg->layer);

        es->ee = es->popup->ecore_evas;
        es->evas = es->popup->evas;
        evas_object_show(es->o_event);
        evas_object_show(es->o_base);
        e_popup_edje_bg_object_set(es->popup, es->o_base);
        ecore_x_netwm_window_type_set(es->popup->evas_win, 
                                      ECORE_X_WINDOW_TYPE_DOCK);

        e_drop_xdnd_register_set(es->popup->evas_win, 1);
        e_gadcon_xdnd_window_set(es->gadcon, es->popup->evas_win);
        e_gadcon_dnd_window_set(es->gadcon, es->popup->evas_win);
     }
   else
     {
        e_drop_xdnd_register_set(es->popup->evas_win, 0);
        e_object_del(E_OBJECT(es->popup));
        es->popup = NULL;

        es->ee = es->zone->container->bg_ecore_evas;
        es->evas = es->zone->container->bg_evas;

        evas_object_move(es->o_event, es->zone->x + es->x, es->zone->y + es->y);
        evas_object_move(es->o_base, es->zone->x + es->x, es->zone->y + es->y);
        evas_object_layer_set(es->o_event, es->cfg->layer);
        evas_object_layer_set(es->o_base, es->cfg->layer);

        e_drop_xdnd_register_set(es->zone->container->bg_win, 1);
        e_gadcon_xdnd_window_set(es->gadcon, es->zone->container->bg_win);
        e_gadcon_dnd_window_set(es->gadcon, es->zone->container->event_win);
        evas_object_clip_set(es->o_base, es->zone->bg_clip_object);
     }
}

EAPI void
e_shelf_autohide_set(E_Shelf *es, int autohide_type)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);

   if ((es->cfg->autohide == !!autohide_type) && ((!!es->autohide) == !!autohide_type))
     {
        if ((!es->autohide) || (es->cfg->autohide_show_action == autohide_type - 1))
          return;
     }
   es->cfg->autohide = !!autohide_type;
   if (!es->cfg->autohide)
     {
        if (!es->autohide) return;
        ecore_event_handler_del(es->autohide);
        es->autohide = NULL;
        return;
     }
   es->cfg->autohide_show_action = autohide_type - 1;
   if (es->autohide) ecore_event_handler_del(es->autohide);
   es->autohide = NULL;
/*
 * see FIXME in _e_shelf_cb_mouse_in() for why these are commented out
   es->handlers = 
     eina_list_append(es->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_IN, 
                                              _e_shelf_cb_mouse_in, es));
*/
   if (!es->cfg->autohide_show_action)
     es->autohide = ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, 
                                            _e_shelf_cb_mouse_in, es);
/*
   es->handlers = 
     eina_list_append(es->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_OUT, 
                                              _e_shelf_cb_mouse_out, es));
   if (!popup)
     evas_object_event_callback_add(es->o_event, EVAS_CALLBACK_MOUSE_OUT, 
                                    (Evas_Object_Event_Cb)_e_shelf_cb_mouse_out2, es);
*/
}

EAPI E_Shelf *
e_shelf_config_new(E_Zone *zone, E_Config_Shelf *cf_es)
{
   E_Shelf *es;
   Eina_Bool can_show = EINA_FALSE;

   es = e_shelf_zone_new(zone, cf_es->name, cf_es->style,
                         cf_es->popup, cf_es->layer, cf_es->id);
   if (!es) return NULL;

   if (!cf_es->hide_timeout) cf_es->hide_timeout = 1.0;
   if (!cf_es->hide_duration) cf_es->hide_duration = 1.0;
   es->cfg = cf_es;
   es->fit_along = cf_es->fit_along;
   es->fit_size = cf_es->fit_size;

   e_shelf_autohide_set(es, cf_es->autohide + (cf_es->autohide * cf_es->autohide_show_action));
   e_shelf_orient(es, cf_es->orient);
   e_shelf_position_calc(es);
   e_shelf_populate(es);

   if (cf_es->desk_show_mode)
     {
        E_Desk *desk;
        Eina_List *ll;
        E_Config_Shelf_Desk *sd;

        desk = e_desk_current_get(zone);
        EINA_LIST_FOREACH(cf_es->desk_list, ll, sd)
          {
             if ((desk->x == sd->x) && (desk->y == sd->y))
               {
                  can_show = EINA_TRUE;
                  break;
               }
          }
     }
   else
     can_show = EINA_TRUE;

   if (can_show)
     {
        /* at this point, we cleverly avoid showing the shelf
         * if its gadcon has not populated; instead we show it in
         * the E_EVENT_GADCON_POPULATE handler
         */
        if ((es->gadcon->populated_classes && es->gadcon->clients) || (!es->gadcon->cf->clients))
          if (e_shelf_desk_visible(es, NULL)) e_shelf_show(es);
     }

   e_shelf_toggle(es, 0);
   return es;
}

EAPI E_Entry_Dialog *
e_shelf_new_dialog(E_Zone *zone)
{
   char buf[256];

   snprintf(buf, sizeof(buf), _("Shelf #%d"), eina_list_count(e_config->shelves));
   return e_entry_dialog_show(_("Add New Shelf"), "preferences-desktop-shelf",
                               _("Name:"), buf, NULL, NULL,
                               _e_shelf_new_dialog_ok, NULL, zone);
}

/* local subsystem functions */

static void
_e_shelf_new_dialog_ok(void *data, char *text)
{
   E_Config_Shelf *cfg, *es_cf;
   E_Zone *zone = data;
   Eina_List *l;
   unsigned int x;
   unsigned long orient = 1;
   E_Shelf *es;

   if ((!text) || (!text[0])) return;
   EINA_LIST_FOREACH(e_config->shelves, l, es_cf)
     {
        if (strcmp(es_cf->name, text)) continue;
        e_util_dialog_internal(_("Shelf Error"), _("A shelf with that name already exists!"));
        return;
     }

   cfg = E_NEW(E_Config_Shelf, 1);
   cfg->name = eina_stringshare_add(text);
   cfg->container = zone->container->num;
   cfg->zone = zone->num;
   cfg->popup = 1;
   cfg->layer = 200;
   EINA_LIST_FOREACH(e_config->shelves, l, es_cf)
     orient *= orientations[es_cf->orient];
   for (x = 3; x < (sizeof(orientations) / sizeof(orientations[0])); x++)
     if (orient % orientations[x])
       {
          cfg->orient = x;
          break;
       }
   cfg->fit_along = 1;
   cfg->fit_size = 0;
   cfg->style = eina_stringshare_add("default");
   cfg->size = 40;
   cfg->overlap = 0;
   cfg->autohide = 0;
   e_config->shelves = eina_list_append(e_config->shelves, cfg);
   e_config_save_queue();

   es_cf = eina_list_last_data_get(e_config->shelves);
   cfg->id = es_cf->id + 1;
   es = e_shelf_config_new(zone, cfg);
   if (es && es->gadcon)
     e_int_gadcon_config_shelf(es->gadcon);
}

static void
_e_shelf_del_cb(void *d)
{
   E_Shelf *es;

   es = d;
   if (es->dummy)
     {
        dummies = eina_list_remove(dummies, es);
        E_FREE(es->cfg);
     }
   else
     shelves = eina_list_remove(shelves, es);
}

static void
_e_shelf_event_rename_end_cb(void *data __UNUSED__, E_Event_Shelf *ev)
{
   e_object_unref(E_OBJECT(ev->shelf));
   free(ev);
}

static void
_e_shelf_free_cb(void *data __UNUSED__, void *event)
{
   E_Event_Shelf *ev = event;
   E_Shelf *es = ev->shelf;

   eina_stringshare_del(es->name);
   eina_stringshare_del(es->style);

   if (es->cfg_delete)
     {
        if (es->cfg)
          {
             e_config->shelves = eina_list_remove(e_config->shelves, es->cfg);
             eina_stringshare_del(es->cfg->name);
             eina_stringshare_del(es->cfg->style);
             free(es->cfg);
          }
        e_config_save_queue();
     }
   free(es);
   free(ev);
}

static void
_e_shelf_free(E_Shelf *es)
{
   E_Event_Shelf *ev;

   if (!es->dummy)
     _e_shelf_bindings_del(es);

   e_zone_useful_geometry_dirty(es->zone);
   E_FREE_LIST(es->handlers, ecore_event_handler_del);

   if (es->autohide)
     {
        ecore_event_handler_del(es->autohide);
        es->autohide = NULL;
     }

   if (es->hide_timer)
     {
        ecore_timer_del(es->hide_timer);
        es->hide_timer = NULL;
     }
   if (es->hide_animator)
     {
        ecore_animator_del(es->hide_animator);
        es->hide_animator = NULL;
     }
   if (es->instant_timer)
     {
        ecore_timer_del(es->instant_timer);
        es->instant_timer = NULL;
     }

   if (es->menu)
     {
        e_menu_post_deactivate_callback_set(es->menu, NULL, NULL);
        e_object_del(E_OBJECT(es->menu));
        es->menu = NULL;
     }
   if (es->module_init_end_timer)
     {
        ecore_timer_del(es->module_init_end_timer);
        es->module_init_end_timer = NULL;
     }
   if (es->dummy)
     {
        evas_object_event_callback_del_full(es->o_base, EVAS_CALLBACK_DEL, 
                                            (Evas_Object_Event_Cb)_e_shelf_cb_dummy_del, es);
        evas_object_event_callback_del_full(es->o_base, EVAS_CALLBACK_MOVE, 
                                            (Evas_Object_Event_Cb)_e_shelf_cb_dummy_moveresize, es);
        evas_object_event_callback_del_full(es->o_base, EVAS_CALLBACK_RESIZE, 
                                            (Evas_Object_Event_Cb)_e_shelf_cb_dummy_moveresize, es);
     }
   else
     {
        e_gadcon_location_unregister(es->gadcon->location);
        e_gadcon_location_free(es->gadcon->location);
        if (es->cfg_delete) e_gadcon_config_del(es->gadcon);
        e_object_del(E_OBJECT(es->gadcon));
        es->gadcon = NULL;
     }
   if (es->config_dialog) e_object_del(E_OBJECT(es->config_dialog));
   es->config_dialog = NULL;
   evas_object_del(es->o_event);
   evas_object_del(es->o_base);
   es->o_base = es->o_event = NULL;
   if (es->popup)
     {
        e_drop_xdnd_register_set(es->popup->evas_win, 0);
        eina_hash_del(winid_shelves,
                      e_util_winid_str_get(es->popup->evas_win), es);
        if (!eina_hash_population(winid_shelves))
          {
             eina_hash_free(winid_shelves);
             winid_shelves = NULL;
          }
        e_object_del(E_OBJECT(es->popup));
     }
   if (es->autohide_timer) ecore_timer_del(es->autohide_timer);
   es->autohide_timer = NULL;
   es->popup = NULL;

   ev = E_NEW(E_Event_Shelf, 1);
   ev->shelf = es;
   ecore_event_add(E_EVENT_SHELF_DEL, ev, _e_shelf_free_cb, NULL);
}

static void
_e_shelf_gadcon_min_size_request(void *data __UNUSED__, E_Gadcon *gc __UNUSED__, Evas_Coord w __UNUSED__, Evas_Coord h __UNUSED__)
{
   return;
}

static void
_e_shelf_gadcon_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h)
{
   E_Shelf *es;
   Evas_Coord nx, ny, nw, nh, ww, hh, wantw, wanth;

   es = data;
   nx = es->x;
   ny = es->y;
   nw = es->w;
   nh = es->h;
   ww = hh = 0;
   evas_object_geometry_get(gc->o_container, NULL, NULL, &ww, &hh);
   switch (gc->orient)
     {
      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
        if (!es->fit_along) w = ww;
        if (!es->fit_size) h = hh;
        break;

      case E_GADCON_ORIENT_VERT:
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_CORNER_RT:
      case E_GADCON_ORIENT_CORNER_LB:
      case E_GADCON_ORIENT_CORNER_RB:
        if (!es->fit_along) h = hh;
        if (!es->fit_size) w = ww;
        break;

      default:
        break;
     }
   e_gadcon_swallowed_min_size_set(gc, w, h);
   edje_object_size_min_calc(es->o_base, &nw, &nh);
   wantw = nw;
   wanth = nh;
   switch (gc->orient)
     {
      case E_GADCON_ORIENT_FLOAT:
        if (!es->fit_along) nw = es->w;
        if (!es->fit_size) nh = es->h;
        if (nw > es->zone->w) nw = es->zone->w;
        if (nh > es->zone->h) nh = es->zone->h;
        /*if (nw != es->w)*/ nx = es->x + ((es->w - nw) / 2);
        /*if (nh != es->h)*/ ny = es->y + ((es->h - nh) / 2);
        break;

      case E_GADCON_ORIENT_HORIZ:
        if (!es->fit_along) nw = es->w;
        if (!es->fit_size) nh = es->h;
        if (nw > es->zone->w) nw = es->zone->w;
        if (nh > es->zone->h) nh = es->zone->h;
        /*if (nw != es->w)*/ nx = es->x + ((es->w - nw) / 2);
        break;

      case E_GADCON_ORIENT_VERT:
        if (!es->fit_along) nh = es->h;
        if (!es->fit_size) nw = es->w;
        if (nw > es->zone->w) nw = es->zone->w;
        if (nh > es->zone->h) nh = es->zone->h;
        /*if (nh != es->h)*/ ny = es->y + ((es->h - nh) / 2);
        break;

      case E_GADCON_ORIENT_LEFT:
        if (!es->fit_along) nh = es->h;
        if (!es->fit_size) nw = es->w;
        if (nw > es->zone->w) nw = es->zone->w;
        if (nh > es->zone->h) nh = es->zone->h;
        /*if (nh != es->h)*/ ny = (es->zone->h - nh) / 2;
        //nx = 0;
        break;

      case E_GADCON_ORIENT_RIGHT:
        if (!es->fit_along) nh = es->h;
        if (!es->fit_size) nw = es->w;
        if (nw > es->zone->w) nw = es->zone->w;
        if (nh > es->zone->h) nh = es->zone->h;
        /*if (nh != es->h)*/ ny = (es->zone->h - nh) / 2;
        //nx = es->zone->w - nw;
        break;

      case E_GADCON_ORIENT_TOP:
        if (!es->fit_along) nw = es->w;
        if (!es->fit_size) nh = es->h;
        if (nw > es->zone->w) nw = es->zone->w;
        if (nh > es->zone->h) nh = es->zone->h;
        /*if (nw != es->w)*/ nx = (es->zone->w - nw) / 2;
        //ny = 0;
        break;

      case E_GADCON_ORIENT_BOTTOM:
        if (!es->fit_along) nw = es->w;
        if (!es->fit_size) nh = es->h;
        if (nw > es->zone->w) nw = es->zone->w;
        if (nh > es->zone->h) nh = es->zone->h;
        /*if (nw != es->w)*/ nx = (es->zone->w - nw) / 2;
        //ny = es->zone->h - nh;
        break;

      case E_GADCON_ORIENT_CORNER_TL:
        if (!es->fit_along) nw = es->w;
        if (!es->fit_size) nh = es->h;
        if (nw > es->zone->w) nw = es->zone->w;
        if (nh > es->zone->h) nh = es->zone->h;
        /*if (nw != es->w)*/ nx = 0;
        //ny = 0;
        break;

      case E_GADCON_ORIENT_CORNER_TR:
        if (!es->fit_along) nw = es->w;
        if (!es->fit_size) nh = es->h;
        if (nw > es->zone->w) nw = es->zone->w;
        if (nh > es->zone->h) nh = es->zone->h;
        /*if (nw != es->w)*/ nx = es->zone->w - nw;
        //ny = 0;
        break;

      case E_GADCON_ORIENT_CORNER_BL:
        if (!es->fit_along) nw = es->w;
        if (!es->fit_size) nh = es->h;
        if (nw > es->zone->w) nw = es->zone->w;
        if (nh > es->zone->h) nh = es->zone->h;
        /*if (nw != es->w)*/ nx = 0;
        //ny = es->zone->h - nh;
        break;

      case E_GADCON_ORIENT_CORNER_BR:
        if (!es->fit_along) nw = es->w;
        if (!es->fit_size) nh = es->h;
        if (nw > es->zone->w) nw = es->zone->w;
        if (nh > es->zone->h) nh = es->zone->h;
        /*if (nw != es->w)*/ nx = es->zone->w - nw;
        //ny = es->zone->h - nh;
        break;

      case E_GADCON_ORIENT_CORNER_LT:
        if (!es->fit_along) nh = es->h;
        if (!es->fit_size) nw = es->w;
        if (nw > es->zone->w) nw = es->zone->w;
        if (nh > es->zone->h) nh = es->zone->h;
        /*if (nh != es->h)*/ ny = 0;
        //nx = 0;
        break;

      case E_GADCON_ORIENT_CORNER_RT:
        if (!es->fit_along) nh = es->h;
        if (!es->fit_size) nw = es->w;
        if (nw > es->zone->w) nw = es->zone->w;
        if (nh > es->zone->h) nh = es->zone->h;
        /*if (nh != es->h)*/ ny = 0;
        //nx = es->zone->w - nw;
        break;

      case E_GADCON_ORIENT_CORNER_LB:
        if (!es->fit_along) nh = es->h;
        if (!es->fit_size) nw = es->w;
        if (nw > es->zone->w) nw = es->zone->w;
        if (nh > es->zone->h) nh = es->zone->h;
        /*if (nh != es->h)*/ ny = es->zone->h - nh;
        //nx = 0;
        break;

      case E_GADCON_ORIENT_CORNER_RB:
        if (!es->fit_along) nh = es->h;
        if (!es->fit_size) nw = es->w;
        if (nw > es->zone->w) nw = es->zone->w;
        if (nh > es->zone->h) nh = es->zone->h;
        /*if (nh != es->h)*/ ny = es->zone->h - nh;
        //nx = es->zone->w - nw;
        break;

      default:
        break;
     }
   w -= (wantw - nw);
   h -= (wanth - nh);
   e_gadcon_swallowed_min_size_set(gc, w, h);
   e_shelf_move_resize(es, nx, ny, nw, nh);
   e_zone_useful_geometry_dirty(es->zone);
}

static Evas_Object *
_e_shelf_gadcon_frame_request(void *data, E_Gadcon_Client *gcc, const char *style)
{
   E_Shelf *es;
   Evas_Object *o;
   char buf[4096];

   es = data;
   o = edje_object_add(gcc->gadcon->evas);

   snprintf(buf, sizeof(buf), "e/shelf/%s/%s", es->style, style);
   if (!e_theme_edje_object_set(o, "base/theme/shelf", buf))
     {
        /* if an inset style (e.g. plain) isn't implemented for a given
         * shelf style, fall back to the default one. no need for every
         * theme to implement the plain style */
        snprintf(buf, sizeof(buf), "e/shelf/default/%s", style);
        if (!e_theme_edje_object_set(o, "base/theme/shelf", buf))
          {
             evas_object_del(o);
             return NULL;
          }
     }
   snprintf(buf, sizeof(buf), "e,state,orientation,%s",
            e_shelf_orient_string_get(es));
   edje_object_signal_emit(o, buf, "e");
   edje_object_message_signal_process(o);
   return o;
}

static void
_e_shelf_toggle_border_fix(E_Shelf *es)
{
   Eina_List *l;
   E_Border *bd;

   if ((es->cfg->overlap) || (!e_config->border_fix_on_shelf_toggle))
     return;

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if ((bd->maximized & E_MAXIMIZE_TYPE) == E_MAXIMIZE_NONE)
          {
             if (bd->lock_client_location) continue;
             if (es->hidden)
               {
                  if (!bd->shelf_fix.modified) continue;
                  if (!--bd->shelf_fix.modified)
                    {
                       e_border_move(bd, bd->shelf_fix.x, bd->shelf_fix.y);
                       continue;
                    }
               }

             if (!E_INTERSECTS(bd->x, bd->y, bd->w, bd->h,
                               es->x, es->y, es->w, es->h))
               continue;

             if (!es->hidden)
               {
                  if (!bd->shelf_fix.modified++)
                    bd->shelf_fix.x = bd->x;
                  bd->shelf_fix.y = bd->y;
               }

             switch (es->gadcon->orient)
               {
                case E_GADCON_ORIENT_TOP:
                case E_GADCON_ORIENT_CORNER_TL:
                case E_GADCON_ORIENT_CORNER_TR:
                  if (!es->hidden)
                    e_border_move(bd, bd->x, bd->y + es->h);
                  break;

                case E_GADCON_ORIENT_BOTTOM:
                case E_GADCON_ORIENT_CORNER_BL:
                case E_GADCON_ORIENT_CORNER_BR:
                  if (!es->hidden)
                    e_border_move(bd, bd->x, bd->y - es->h);
                  break;

                case E_GADCON_ORIENT_LEFT:
                case E_GADCON_ORIENT_CORNER_LB:
                case E_GADCON_ORIENT_CORNER_LT:
                  if (!es->hidden)
                    e_border_move(bd, bd->x + es->w, bd->y);
                  break;

                case E_GADCON_ORIENT_RIGHT:
                case E_GADCON_ORIENT_CORNER_RB:
                case E_GADCON_ORIENT_CORNER_RT:
                  if (!es->hidden)
                    e_border_move(bd, bd->x - es->w, bd->y);
                  break;

                default:
                  break;
               }
          }
        else
          {
             E_Maximize max;

             max = bd->maximized;
             e_border_unmaximize(bd, E_MAXIMIZE_BOTH);
             e_border_maximize(bd, max);
          }
     }
}

static void
_e_shelf_menu_item_free(void *data)
{
   E_Shelf *es;

   es = e_object_data_get(data);
   e_shelf_locked_set(es, 0);
   e_object_unref(E_OBJECT(es));
}

static void
_e_shelf_menu_append(E_Shelf *es, E_Menu *mn)
{
   E_Menu_Item *mi;
   E_Menu *subm;
   char buf[256];

   if (es->name)
     snprintf(buf, sizeof(buf), "%s", es->name);
   else
     snprintf(buf, sizeof(buf), _("Shelf %s"),
              e_shelf_orient_string_get(es));

   e_shelf_locked_set(es, 1);

   subm = e_menu_new();
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, buf);
   e_util_menu_item_theme_icon_set(mi, "preferences-desktop-shelf");
   e_menu_pre_activate_callback_set(subm, _e_shelf_menu_pre_cb, es);
   e_object_ref(E_OBJECT(es));
   e_object_free_attach_func_set(E_OBJECT(mi), _e_shelf_menu_item_free);
   e_object_data_set(E_OBJECT(mi), es);
   e_menu_item_submenu_set(mi, subm);
   e_object_unref(E_OBJECT(subm));

   mi = e_menu_item_new(mn);
   e_menu_item_separator_set(mi, 1);

   mi = e_menu_item_new(mn);
   if (es->gadcon->editing)
     e_menu_item_label_set(mi, _("Stop Moving Gadgets"));
   else
     e_menu_item_label_set(mi, _("Begin Moving Gadgets"));
   e_util_menu_item_theme_icon_set(mi, "transform-scale");
   e_menu_item_callback_set(mi, _e_shelf_cb_menu_edit, es);
}

static void
_e_shelf_cb_menu_items_append(void *data, E_Gadcon_Client *gcc __UNUSED__, E_Menu *mn)
{
   E_Shelf *es;

   es = data;
   _e_shelf_menu_append(es, mn);
}

static void
_e_shelf_cb_locked_set(void *data, int lock)
{
   E_Shelf *es;

   es = data;
   e_shelf_locked_set(es, lock);
}

static void
_e_shelf_cb_urgent_show(void *data)
{
   E_Shelf *es;

   es = data;
   e_shelf_urgent_show(es);
}

static void
_e_shelf_cb_menu_autohide(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Shelf *es = data;

   e_shelf_autohide_set(es, (!es->cfg->autohide) + (!es->cfg->autohide * es->cfg->autohide_show_action));
   if ((es->cfg->autohide) && (!es->hidden))
     e_shelf_toggle(es, 0);
   else if ((!es->cfg->autohide) && (es->hidden))
     e_shelf_toggle(es, 1);
   e_zone_useful_geometry_dirty(es->zone);
   e_config_save_queue();
}

static void
_e_shelf_cb_menu_refresh(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Shelf *es = data;

   e_gadcon_unpopulate(es->gadcon);
   e_gadcon_populate(es->gadcon);
}

static void
_e_shelf_cb_menu_config(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Shelf *es;

   es = data;
   if (!es->config_dialog) e_int_shelf_config(es);
}

static void
_e_shelf_cb_menu_edit(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Shelf *es;

   es = data;
   if (es->gadcon->editing)
     {
        e_gadcon_edit_end(es->gadcon);
        e_shelf_toggle(es, 0);
     }
   else
     {
        e_shelf_toggle(es, 1);
        e_gadcon_edit_begin(es->gadcon);
     }
}

static void
_e_shelf_cb_menu_contents(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Shelf *es;

   es = data;
   if (!es->gadcon->config_dialog) e_int_gadcon_config_shelf(es->gadcon);
}

static void
_e_shelf_cb_confirm_dialog_destroy(void *data)
{
   E_Shelf *es;

   es = data;
   e_object_unref(E_OBJECT(es));
}

static void
_e_shelf_cb_confirm_dialog_yes(void *data)
{
   E_Shelf *es;

   es = data;
   if (e_object_is_del(E_OBJECT(es))) return;
   es->cfg_delete = 1;
   e_object_del(E_OBJECT(es));
}

static void
_e_shelf_cb_menu_delete(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Shelf *es;

   es = data;
   if (e_config->cnfmdlg_disabled)
     {
        if (e_object_is_del(E_OBJECT(es))) return;
        es->cfg_delete = 1;
        e_object_del(E_OBJECT(es));

        e_config_save_queue();
        return;
     }

   e_object_ref(E_OBJECT(es));
   e_confirm_dialog_show(_("Are you sure you want to delete this shelf?"), 
                         "enlightenment",
                         _("You requested to delete this shelf.<br>"
                           "<br>"
                           "Are you sure you want to delete it?"), 
                         _("Delete"), _("Keep"),
                         _e_shelf_cb_confirm_dialog_yes, NULL, data, NULL,
                         _e_shelf_cb_confirm_dialog_destroy, data);
}

static void
_e_shelf_cb_menu_post(void *data, E_Menu *m __UNUSED__)
{
   E_Shelf *es;

   es = data;
   if (!es->menu) return;
   e_object_del(E_OBJECT(es->menu));
   es->menu = NULL;
}

static void
_e_shelf_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Shelf *es;
   E_Menu *mn;
   int cx, cy;

   es = data;
   ev = event_info;
   switch (ev->button)
     {
      case 1:
        if (es->cfg->autohide_show_action) e_shelf_toggle(es, 1);
        break;

      case 3:
        mn = e_menu_new();
        e_menu_post_deactivate_callback_set(mn, _e_shelf_cb_menu_post, es);
        es->menu = mn;

        _e_shelf_menu_pre_cb(es, mn);

        e_gadcon_canvas_zone_geometry_get(es->gadcon, &cx, &cy, NULL, NULL);
        e_menu_activate_mouse(mn,
                              e_util_zone_current_get(e_manager_current_get()),
                              cx + ev->output.x,
                              cy + ev->output.y, 1, 1,
                              E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
        break;
     }
}

static Eina_Bool
_e_shelf_cb_mouse_move_autohide_fuck_systray(E_Shelf *es)
{
   int x, y;
   Ecore_Event_Mouse_Move ev;

   memset(&ev, 0, sizeof(Ecore_Event_Mouse_Move));
   ecore_x_pointer_xy_get(es->zone->container->manager->root, &x, &y);
   ev.root.x = x, ev.root.y = y;
   _e_shelf_cb_mouse_in(es, ECORE_EVENT_MOUSE_MOVE, &ev);
   return EINA_TRUE;
}

static Eina_Bool
_e_shelf_cb_mouse_in(void *data, int type, void *event)
{
   E_Shelf *es;

   es = data;
   if (es->cfg->autohide_show_action) return ECORE_CALLBACK_PASS_ON;

   if (type == E_EVENT_ZONE_EDGE_MOVE)
     {
        E_Event_Zone_Edge *ev;
        int show = 0;

        ev = event;
        if (es->zone != ev->zone) return ECORE_CALLBACK_PASS_ON;
        if (!_e_shelf_on_current_desk(es, ev)) return ECORE_CALLBACK_PASS_ON;

        ev->x -= es->zone->x, ev->y -= es->zone->y;
        switch (es->gadcon->orient)
          {
           case E_GADCON_ORIENT_LEFT:
           case E_GADCON_ORIENT_CORNER_LT:
           case E_GADCON_ORIENT_CORNER_LB:
             if (((ev->edge == E_ZONE_EDGE_LEFT) ||
                  (ev->edge == E_ZONE_EDGE_TOP_LEFT) ||
                  (ev->edge == E_ZONE_EDGE_BOTTOM_LEFT)) &&
                 (ev->y >= es->y) && (ev->y <= (es->y + es->h)))
               show = 1;
             break;

           case E_GADCON_ORIENT_RIGHT:
           case E_GADCON_ORIENT_CORNER_RT:
           case E_GADCON_ORIENT_CORNER_RB:
             if (((ev->edge == E_ZONE_EDGE_RIGHT) ||
                  (ev->edge == E_ZONE_EDGE_TOP_RIGHT) ||
                  (ev->edge == E_ZONE_EDGE_BOTTOM_RIGHT)) &&
                 (ev->y >= es->y) && (ev->y <= (es->y + es->h)))
               show = 1;
             break;

           case E_GADCON_ORIENT_TOP:
           case E_GADCON_ORIENT_CORNER_TL:
           case E_GADCON_ORIENT_CORNER_TR:
             if (((ev->edge == E_ZONE_EDGE_TOP) ||
                  (ev->edge == E_ZONE_EDGE_TOP_LEFT) ||
                  (ev->edge == E_ZONE_EDGE_TOP_RIGHT)) &&
                 (ev->x >= es->x) && (ev->x <= (es->x + es->w)))
               show = 1;
             break;

           case E_GADCON_ORIENT_BOTTOM:
           case E_GADCON_ORIENT_CORNER_BL:
           case E_GADCON_ORIENT_CORNER_BR:
             if (((ev->edge == E_ZONE_EDGE_BOTTOM) ||
                  (ev->edge == E_ZONE_EDGE_BOTTOM_LEFT) ||
                  (ev->edge == E_ZONE_EDGE_BOTTOM_RIGHT)) &&
                 (ev->x >= es->x) && (ev->x <= (es->x + es->w)))
               show = 1;
             break;

           case E_GADCON_ORIENT_FLOAT:
           case E_GADCON_ORIENT_HORIZ:
           case E_GADCON_ORIENT_VERT:
           default:
             /* noop */
             break;
          }

        if (show)
          {
             edje_object_signal_emit(es->o_base, "e,state,focused", "e");
             e_shelf_toggle(es, 1);
          }
        else
          e_shelf_toggle(es, 0);
     }
     /*
   else if (type == ECORE_X_EVENT_MOUSE_IN)
     {
        Ecore_X_Event_Mouse_In *ev;

        ev = event;
        if ((!es->hidden) && (!es->menu))
          {
             int x, y, w, h;
             evas_object_geometry_get(es->o_event, &x, &y, &w, &h);
             if (!E_INSIDE(ev->x, ev->y, x, y, w, h))
               e_shelf_toggle(es, 0);
          }
        if (!es->popup) return ECORE_CALLBACK_PASS_ON;
        if (ev->win == es->popup->evas_win)
          {
             if (es->hidden || (!es->toggle))
               {
                  edje_object_signal_emit(es->o_base, "e,state,focused", "e");
                  e_shelf_toggle(es, 1);
               }
          }
     }
     */
   else if (type == ECORE_EVENT_MOUSE_MOVE)
     {
        Ecore_Event_Mouse_Move *ev;
        int x, y;
        Eina_Bool inside = EINA_FALSE;

        ev = event;
        /* FIXME: checking every mouse movement here is only necessary because of
         * shitty systray embedding xwindows into itself which generates unreliable
         * mouse in/out events. in the future, when we remove systray, we should go
         * back to mouse in/out events
         */
        inside = (es->popup && ((ev->event_window == es->popup->evas_win)));
        if (!inside)
          {
             inside = E_INSIDE(ev->root.x, ev->root.y, es->zone->x, es->zone->y, es->zone->w + 4, es->zone->h + 4);
             x = ev->root.x - es->zone->x, y = ev->root.y - es->zone->y;
             if (inside)
               inside = (
                         ((E_INSIDE(x, y, es->x, es->y, es->w, es->h)) ||
                         (E_INSIDE(x, y, es->x - 2, es->y - 2, es->w + 4, es->h + 4)) ||
                         (E_INSIDE(x, y, es->x + 2, es->y + 2, es->w + 4, es->h + 4)))
                        );
             if (inside && es->popup)
               {
                  if (es->autohide_timer)
                    ecore_timer_reset(es->autohide_timer);
                  else
                    es->autohide_timer = ecore_timer_add(0.5, (Ecore_Task_Cb)_e_shelf_cb_mouse_move_autohide_fuck_systray, es);
               }
          }
        if (inside)
          {
             if (es->hidden || (!es->toggle))
               {
                  edje_object_signal_emit(es->o_base, "e,state,focused", "e");
                  e_shelf_toggle(es, 1);
               }
          }
        else
          {
             if ((!es->hidden) && (es->toggle))
               e_shelf_toggle(es, 0);
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}
#if 0
static void
_e_shelf_cb_mouse_out2(E_Shelf *es, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, Evas_Event_Mouse_Out *ev)
{
   int x, y, w, h;

   evas_object_geometry_get(es->o_event, &x, &y, &w, &h);
   if (!E_INSIDE(ev->output.x, ev->output.y, x, y, w, h))
     e_shelf_toggle(es, 0);
}

static Eina_Bool
_e_shelf_cb_mouse_out(void *data, int type, void *event)
{
   E_Shelf *es;
   Ecore_X_Window win;

   es = data;

   if (type == ECORE_X_EVENT_MOUSE_OUT)
     {
        Ecore_X_Event_Mouse_Out *ev;
        int x, y, w, h;

        ev = event;

        if (es->popup) win = es->popup->evas_win;
        else win = es->zone->container->event_win;
        if (ev->win != win) return ECORE_CALLBACK_PASS_ON;

        /*
         * ECORE_X_EVENT_DETAIL_INFERIOR means focus went to children windows
         * so do not hide shelf on this case (ie: systray base window, or
         * embedded icons).
         *
         * Problem: when child window get mouse out, shelf window will
         * not get mouse out itself, so it will stay visible and
         * autohide will fail.
         */
        if (ev->detail == ECORE_X_EVENT_DETAIL_INFERIOR)
          {
             //fprintf(stderr, "SYSTRAYED\n");
             return ECORE_CALLBACK_PASS_ON;
          }

        evas_object_geometry_get(es->o_event, &x, &y, &w, &h);
        if (!E_INSIDE(ev->x, ev->y, x, y, w, h))
          {
             //fprintf(stderr, "EVENT: %d,%d %dx%d || MOUSE: %d,%d\n", x, y, w, h, ev->x, ev->y);
             e_shelf_toggle(es, 0);
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}
#endif
static void
_e_shelf_cb_dummy_moveresize(E_Shelf *es, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   int x, y, w, h;

   evas_object_geometry_get(obj, &x, &y, &w, &h);
   if ((x != es->x) || (y != es->y) || (w != es->w) || (h != es->h))
     {
        es->x = x, es->y = y, es->w = w, es->h = h;
        e_zone_useful_geometry_dirty(es->zone);
     }
}

static void
_e_shelf_cb_dummy_del(E_Shelf *es, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   e_object_free(E_OBJECT(es));
}

static int
_e_shelf_cb_id_sort(const void *data1, const void *data2)
{
   const E_Shelf *es1, *es2;

   es1 = data1;
   es2 = data2;
   if ((es1->id) < (es2->id)) return -1;
   else if (es1->id > es2->id)
     return 1;
   return 0;
}

static Eina_Bool
_e_shelf_cb_hide_animator(void *data)
{
   E_Shelf *es;
   int step, hide_max = 0;

   es = data;
   if (!es->gadcon)
     {
        es->hide_animator = NULL;
        return EINA_FALSE;
     }
   switch (es->gadcon->orient)
     {
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
        hide_max = es->h - es->hidden_state_size - 1;
        if (es->hide_origin == -1) es->hide_origin = es->y;
        break;

      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_CORNER_LB:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_RB:
      case E_GADCON_ORIENT_CORNER_RT:
        hide_max = es->w - es->hidden_state_size;
        if (es->hide_origin == -1) es->hide_origin = es->x;
        break;

      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_VERT:
      default:
        break;
     }

   step = (hide_max / e_config->framerate) / es->cfg->hide_duration;
   if (!step) step = 1;

   if (es->hidden)
     {
        if (es->hide_step < hide_max)
          {
             if (es->hide_step + step > hide_max)
               es->hide_step = hide_max;
             else
               es->hide_step += step;
          }
        else goto end;
     }
   else
     {
        if (es->hide_step > 0)
          {
             if (es->hide_step < step)
               es->hide_step = 0;
             else
               es->hide_step -= step;
          }
        else goto end;
     }

   switch (es->gadcon->orient)
     {
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
        e_shelf_move(es, es->x, es->hide_origin - es->hide_step);
        break;

      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
        e_shelf_move(es, es->x, es->hide_origin + es->hide_step);
        break;

      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_CORNER_LB:
      case E_GADCON_ORIENT_CORNER_LT:
        e_shelf_move(es, es->hide_origin - es->hide_step, es->y);
        break;

      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_RB:
      case E_GADCON_ORIENT_CORNER_RT:
        e_shelf_move(es, es->hide_origin + es->hide_step, es->y);
        break;

      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_VERT:
      default:
        break;
     }

   return ECORE_CALLBACK_RENEW;

end:
   es->hide_animator = NULL;
   if (es->interrupted > -1)
     e_shelf_toggle(es, es->interrupted);
   else if (es->urgent_show)
     e_shelf_toggle(es, 0);
   else
     _e_shelf_toggle_border_fix(es);
   if ((!es->hidden) && es->cfg->autohide_show_action)
     {
        edje_object_signal_emit(es->o_base, "e,state,hidden", "e");
        es->hidden = 1;
        if (!es->hide_timer)
          es->hide_timer = ecore_timer_add(es->cfg->hide_timeout, _e_shelf_cb_hide_animator_timer, es);
     }
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_shelf_cb_hide_animator_timer(void *data)
{
   E_Shelf *es;

   es = data;
   if (!es->hide_animator)
     es->hide_animator = ecore_animator_add(_e_shelf_cb_hide_animator, es);
   es->hide_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_shelf_cb_hide_urgent_timer(void *data)
{
   E_Shelf *es;

   es = data;
   es->hide_timer = NULL;
   if (es->urgent_show) e_shelf_toggle(es, 0);
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_shelf_cb_instant_hide_timer(void *data)
{
   E_Shelf *es;

   es = data;
   switch (es->gadcon->orient)
     {
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
        if (es->hidden)
          e_shelf_move(es, es->x, es->y - es->h + es->hidden_state_size);
        else
          e_shelf_move(es, es->x, es->y + es->h - es->hidden_state_size);
        break;

      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
        if (es->hidden)
          e_shelf_move(es, es->x, es->y + es->h - es->hidden_state_size);
        else
          e_shelf_move(es, es->x, es->y - es->h + es->hidden_state_size);
        break;

      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_CORNER_LB:
      case E_GADCON_ORIENT_CORNER_LT:
        if (es->hidden)
          e_shelf_move(es, es->x - es->w + es->hidden_state_size, es->y);
        else
          e_shelf_move(es, es->x + es->w - es->hidden_state_size, es->y);
        break;

      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_RB:
      case E_GADCON_ORIENT_CORNER_RT:
        if (es->hidden)
          e_shelf_move(es, es->x + es->w - es->hidden_state_size, es->y);
        else
          e_shelf_move(es, es->x - es->w + es->hidden_state_size, es->y);
        break;

      default:
        break;
     }
   es->instant_timer = NULL;
   _e_shelf_toggle_border_fix(es);
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_shelf_module_init_end_timer_cb(void *data)
{
   E_Shelf *es = data;
   if (e_shelf_desk_visible(es, NULL)) e_shelf_show(es);
   es->module_init_end_timer = NULL;
   return EINA_FALSE;
}

static Eina_Bool
_e_shelf_module_init_end_handler_cb(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   Eina_List *l;
   E_Shelf *es;

   EINA_LIST_FOREACH(shelves, l, es)
     {
        if ((!es->gadcon->populate_requests) || (!es->gadcon->cf->clients))
          {
             if (e_shelf_desk_visible(es, NULL))
               e_shelf_show(es);
          }
        else if (!es->module_init_end_timer)
          es->module_init_end_timer = ecore_timer_add(3.0, _e_shelf_module_init_end_timer_cb, es);
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_shelf_gadcon_populate_handler_cb(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Gadcon_Populate *ev = event;
   Eina_List *l;
   E_Shelf *es;

   EINA_LIST_FOREACH(shelves, l, es)
     if (es->gadcon == ev->gc)
       {
          /* any shelf that's not already shown at this point will be
           * waiting for this event to show it so that we don't ever resize the shelf
           * object
           */
          if (e_shelf_desk_visible(es, NULL)) e_shelf_show(es);
          break;
       }
   return ECORE_CALLBACK_RENEW;
}

static void
_e_shelf_cb_menu_rename_yes_cb(void *data, char *text)
{
   E_Shelf *es = data;
   Eina_List *l;
   E_Config_Shelf *cf_es;

   if ((!text) || (!text[0])) return;
   if (!strcmp(text, es->cfg->name)) return;
   EINA_LIST_FOREACH(e_config->shelves, l, cf_es)
     if (!strcmp(cf_es->name, text))
       {
          e_util_dialog_internal(_("Error"), _("A shelf with that name and id already exists!"));
          return;
       }
   e_shelf_name_set(es, text);
   e_config_save_queue();
}

static void
_e_shelf_cb_menu_rename_cb(void *data)
{
   E_Shelf *es = e_object_data_get(data);
   es->rename_dialog = NULL;
}

static void
_e_shelf_cb_menu_rename(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Shelf *es = data;
   if (es->rename_dialog) return;
   es->rename_dialog = e_entry_dialog_show(_("Rename Shelf"), "edit-rename",
                                          _("Name:"), es->name, NULL, NULL,
                                          _e_shelf_cb_menu_rename_yes_cb,
                                          NULL, es);
   e_object_data_set(E_OBJECT(es->rename_dialog), es);
   e_object_del_attach_func_set(E_OBJECT(es->rename_dialog),
                                _e_shelf_cb_menu_rename_cb);
}

static void
_e_shelf_cb_menu_orient(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Shelf *es = data;
   E_Menu_Item *mii;
   int orient = E_GADCON_ORIENT_LEFT;
   Eina_List *l;

   EINA_LIST_FOREACH(m->items, l, mii)
     {
        if (mi == mii)
          {
             E_Zone *zone;
             E_Config_Shelf *cf_es;
             void *cfd;
             if (es->cfg->orient == orient) return;
             es->cfg->orient = orient;
             zone = es->zone;
             cf_es = es->cfg;
             cfd = es->config_dialog;
             es->config_dialog = NULL;
             e_gadcon_unpopulate(es->gadcon);
             e_object_del(E_OBJECT(es));
             es = e_shelf_config_new(zone, cf_es);
             es->config_dialog = cfd;
             return;
          }
        orient++;
     }
}

static void
_e_shelf_menu_orientation_pre_cb(void *data, E_Menu *m)
{
   E_Menu_Item *mi;
   E_Shelf *es = data;
   int orient;

   if (m->items) return;

   for (orient = E_GADCON_ORIENT_LEFT; orient < E_GADCON_ORIENT_LAST; orient++)
     {
        mi = e_menu_item_new(m);
        e_util_gadcon_orient_menu_item_icon_set(orient, mi);
        e_menu_item_radio_set(mi, 1);
        e_menu_item_radio_group_set(mi, 1);
        e_menu_item_label_set(mi, _(orient_names[orient]));
        e_menu_item_callback_set(mi, _e_shelf_cb_menu_orient, es);
        if (es->cfg->orient == orient)
          e_menu_item_toggle_set(mi, 1);
        e_menu_item_disabled_set(mi, es->cfg->orient == orient);
     }
}

static void
_e_shelf_menu_pre_cb(void *data, E_Menu *m)
{
   E_Shelf *es;
   E_Menu_Item *mi;
   E_Menu *subm;

   es = data;
   e_menu_pre_activate_callback_set(m, NULL, NULL);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Contents"));
   e_util_menu_item_theme_icon_set(mi, "preferences-desktop-shelf");
   e_menu_item_callback_set(mi, _e_shelf_cb_menu_contents, es);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Settings"));
   e_util_menu_item_theme_icon_set(mi, "configure");
   e_menu_item_callback_set(mi, _e_shelf_cb_menu_config, es);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Orientation"));
   e_menu_item_callback_set(mi, _e_shelf_cb_menu_config, es);
   subm = e_menu_new();
   e_menu_pre_activate_callback_set(subm, _e_shelf_menu_orientation_pre_cb, es);
   e_object_data_set(E_OBJECT(subm), es);
   e_menu_item_submenu_set(mi, subm);
   e_object_unref(E_OBJECT(subm));

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Autohide"));
   e_menu_item_check_set(mi, 1);
   e_menu_item_toggle_set(mi, es->cfg->autohide);
   //e_util_menu_item_theme_icon_set(mi, ""); FIXME
   e_menu_item_callback_set(mi, _e_shelf_cb_menu_autohide, es);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Refresh"));
   e_util_menu_item_theme_icon_set(mi, "view-refresh");
   e_menu_item_callback_set(mi, _e_shelf_cb_menu_refresh, es);

   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Rename"));
   e_util_menu_item_theme_icon_set(mi, "edit-rename");
   e_menu_item_callback_set(mi, _e_shelf_cb_menu_rename, es);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Delete"));
   e_util_menu_item_theme_icon_set(mi, "list-remove");
   e_menu_item_callback_set(mi, _e_shelf_cb_menu_delete, es);

   if (m->parent_item) return;

   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);

   mi = e_menu_item_new(m);
   if (es->gadcon->editing)
     e_menu_item_label_set(mi, _("Stop Moving Gadgets"));
   else
     e_menu_item_label_set(mi, _("Begin Moving Gadgets"));
   e_util_menu_item_theme_icon_set(mi, "transform-scale");
   e_menu_item_callback_set(mi, _e_shelf_cb_menu_edit, es);
}

static void
_e_shelf_gadcon_client_remove(void *data, E_Gadcon_Client *gcc)
{
   E_Shelf *s;
   E_Gadcon *gc;

   s = data;
   gc = s->gadcon;
   if (gcc->cf) e_gadcon_client_config_del(gc->cf, gcc->cf);
   e_gadcon_unpopulate(gc);
   e_gadcon_populate(gc);
   e_config_save_queue();
}

static int
_e_shelf_gadcon_client_add(void *data, E_Gadcon_Client *gcc, const E_Gadcon_Client_Class *cc)
{
   E_Shelf *s;
   E_Gadcon *gc;

   s = data;
   gc = s->gadcon;
   if (gcc)
     {
        E_Config_Gadcon_Client *cf_gcc = gcc->cf;
        gcc->gadcon->cf->clients = eina_list_remove(gcc->gadcon->cf->clients, cf_gcc);
        if (gc->zone)
          cf_gcc->geom.res = gc->zone->w;
        else if (gc->o_container)
          {
             int w, h;
             evas_object_geometry_get(gc->o_container, NULL, NULL, &w, &h);
             switch (gc->orient)
               {
                  case E_GADCON_ORIENT_VERT:
                  case E_GADCON_ORIENT_LEFT:
                  case E_GADCON_ORIENT_RIGHT:
                  case E_GADCON_ORIENT_CORNER_LT:
                  case E_GADCON_ORIENT_CORNER_RT:
                  case E_GADCON_ORIENT_CORNER_LB:
                  case E_GADCON_ORIENT_CORNER_RB:
                  cf_gcc->geom.res = h;
                  break;
                  default:
                  cf_gcc->geom.res = w;
               }
          }
        else
          cf_gcc->geom.res = 800;
        cf_gcc->geom.size = 80;
        cf_gcc->geom.pos = cf_gcc->geom.res - cf_gcc->geom.size;
        gc->cf->clients = eina_list_append(gc->cf->clients, cf_gcc);
     }
   else if (!e_gadcon_client_config_new(gc, cc->name)) return 0;
   e_gadcon_unpopulate(gc);
   e_gadcon_populate(gc);
   e_config_save_queue();
   return 1;
}

static const char *
_e_shelf_orient_icon_name_get(E_Shelf *s)
{
   const char *name;

   name = NULL;
   switch (s->cfg->orient)
     {
      case E_GADCON_ORIENT_LEFT:
        name = "preferences-position-left";
        break;

      case E_GADCON_ORIENT_RIGHT:
        name = "preferences-position-right";
        break;

      case E_GADCON_ORIENT_TOP:
        name = "preferences-position-top";
        break;

      case E_GADCON_ORIENT_BOTTOM:
        name = "preferences-position-bottom";
        break;

      case E_GADCON_ORIENT_CORNER_TL:
        name = "preferences-position-top-left";
        break;

      case E_GADCON_ORIENT_CORNER_TR:
        name = "preferences-position-top-right";
        break;

      case E_GADCON_ORIENT_CORNER_BL:
        name = "preferences-position-bottom-left";
        break;

      case E_GADCON_ORIENT_CORNER_BR:
        name = "preferences-position-bottom-right";
        break;

      case E_GADCON_ORIENT_CORNER_LT:
        name = "preferences-position-left-top";
        break;

      case E_GADCON_ORIENT_CORNER_RT:
        name = "preferences-position-right-top";
        break;

      case E_GADCON_ORIENT_CORNER_LB:
        name = "preferences-position-left-bottom";
        break;

      case E_GADCON_ORIENT_CORNER_RB:
        name = "preferences-position-right-bottom";
        break;

      default:
        name = "preferences-desktop-shelf";
        break;
     }
   return name;
}

static void
_e_shelf_bindings_add(E_Shelf *es)
{
   char buf[1024];

   /* TODO: This might delete edge windows, and then add them again further down. Should prevent this. */
   _e_shelf_bindings_del(es);

   /* Don't need edge binding if we don't hide shelf */
   if ((es->cfg) && (!es->cfg->autohide) &&
       (!es->cfg->autohide_show_action))
     return;

   snprintf(buf, sizeof(buf), "shelf.%d", es->id);
   switch (es->gadcon->orient)
     {
      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_VERT:
      default:
        /* noop */
        break;

      case E_GADCON_ORIENT_LEFT:
        e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_LEFT, 
                            E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
        break;

      case E_GADCON_ORIENT_RIGHT:
        e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_RIGHT, 
                            E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
        break;

      case E_GADCON_ORIENT_TOP:
        e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_TOP, 
                            E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
        break;

      case E_GADCON_ORIENT_BOTTOM:
        e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_BOTTOM, 
                            E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
        break;

      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_LT:
        e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_TOP, 
                            E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
        e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_LEFT, 
                            E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
        e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_TOP_LEFT, 
                            E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
        break;

      case E_GADCON_ORIENT_CORNER_TR:
      case E_GADCON_ORIENT_CORNER_RT:
        e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_TOP, 
                            E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
        e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_RIGHT, 
                            E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
        e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_TOP_RIGHT, 
                            E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
        break;

      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_LB:
        e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_BOTTOM, 
                            E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
        e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_LEFT, 
                            E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
        e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_BOTTOM_LEFT, 
                            E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
        break;

      case E_GADCON_ORIENT_CORNER_BR:
      case E_GADCON_ORIENT_CORNER_RB:
        e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_BOTTOM, 
                            E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
        e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_RIGHT, 
                            E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
        e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_BOTTOM_RIGHT, 
                            E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
        break;
     }
}

static void
_e_shelf_bindings_del(E_Shelf *es)
{
   char buf[1024];
   E_Zone_Edge edge;

   snprintf(buf, sizeof(buf), "shelf.%d", es->id);
   for (edge = E_ZONE_EDGE_LEFT; edge <= E_ZONE_EDGE_BOTTOM_LEFT; edge++)
     e_bindings_edge_del(E_BINDING_CONTEXT_ZONE, edge, 
                         E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
}

static Eina_Bool
_e_shelf_on_current_desk(E_Shelf *es, E_Event_Zone_Edge *ev)
{
   E_Config_Shelf_Desk *sd;
   Eina_List *ll;
   int on_current_desk = 0;
   int on_all_desks = 1;


   if (ev->zone != es->zone) return EINA_FALSE;
   EINA_LIST_FOREACH(es->cfg->desk_list, ll, sd)
     {
        if (!sd) continue;
        on_all_desks = 0;
        if ((sd->x == ev->zone->desk_x_current) && 
            (sd->y == ev->zone->desk_y_current))
          {
             on_current_desk = 1;
             break;
          }
     }
   if ((!on_all_desks) && (!on_current_desk))
     return EINA_FALSE;

   return EINA_TRUE;
}
