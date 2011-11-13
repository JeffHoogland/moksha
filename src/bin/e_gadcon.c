#include "e.h"

/*
 * TODO: gadcon client ordering on drop
 */

#define E_LAYOUT_ITEM_DRAG_RESIST_LEVEL 10

static void _e_gadcon_free(E_Gadcon *gc);
static void _e_gadcon_client_free(E_Gadcon_Client *gcc);

static void _e_gadcon_moveresize_handle(E_Gadcon_Client *gcc);
static Eina_Bool _e_gadcon_cb_client_scroll_timer(void *data);
static Eina_Bool _e_gadcon_cb_client_scroll_animator(void *data);
static void _e_gadcon_cb_client_frame_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_client_frame_moveresize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_gadcon_client_save(E_Gadcon_Client *gcc);
static void _e_gadcon_client_drag_begin(E_Gadcon_Client *gcc, int x, int y);
static void _e_gadcon_client_inject(E_Gadcon *gc, E_Gadcon_Client *gcc, int x, int y);

static void _e_gadcon_cb_min_size_request(void *data, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_size_request(void *data, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_moveresize(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_client_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_client_mouse_in(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_client_mouse_out(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_client_move(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_client_resize(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static void _e_gadcon_client_move_start(E_Gadcon_Client *gcc);
static void _e_gadcon_client_move_stop(E_Gadcon_Client *gcc);
static void _e_gadcon_client_move_go(E_Gadcon_Client *gcc);

static void _e_gadcon_cb_signal_move_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_move_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_move_go(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_left_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_left_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_left_go(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_right_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_right_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_right_go(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_drag_finished(E_Drag *drag, int dropped);
static void _e_gadcon_cb_dnd_enter(void *data, const char *type, void *event);
static void _e_gadcon_cb_dnd_move(void *data, const char *type, void *event);
static void _e_gadcon_cb_dnd_leave(void *data, const char *type, void *event);
static void _e_gadcon_cb_drop(void *data, const char *type, void *event);

static int _e_gadcon_client_class_feature_check(const E_Gadcon_Client_Class *cc, const char *name, void *feature);
static void _e_gadcon_client_cb_menu_post(void *data, E_Menu *m);
static void _e_gadcon_client_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_gadcon_client_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_gadcon_client_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_gadcon_client_cb_menu_style_plain(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_gadcon_client_cb_menu_style_inset(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_gadcon_client_cb_menu_autoscroll(void *data, E_Menu *m, E_Menu_Item *mi);
/*static void _e_gadcon_client_cb_menu_resizable(void *data, E_Menu *m, E_Menu_Item *mi);*/
static void _e_gadcon_client_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_gadcon_client_cb_menu_remove(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_gadcon_client_cb_menu_pre(void *data, E_Menu *m, E_Menu_Item *mi);

static void _e_gadcon_client_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info);

static Evas_Object *e_gadcon_layout_add(Evas *evas);
static void e_gadcon_layout_orientation_set(Evas_Object *obj, int horizontal);
static int e_gadcon_layout_orientation_get(Evas_Object *obj);
static void e_gadcon_layout_freeze(Evas_Object *obj);
static void e_gadcon_layout_thaw(Evas_Object *obj);
static void e_gadcon_layout_min_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
static void e_gadcon_layout_asked_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
static int e_gadcon_layout_pack(Evas_Object *obj, Evas_Object *child);
static void e_gadcon_layout_pack_size_set(Evas_Object *obj, int size);
static void e_gadcon_layout_pack_request_set(Evas_Object *obj, int pos, int size);
static void e_gadcon_layout_pack_options_set(Evas_Object *obj, E_Gadcon_Client *gcc);
static void e_gadcon_layout_pack_min_size_set(Evas_Object *obj, int w, int h);
static void e_gadcon_layout_pack_aspect_set(Evas_Object *obj, int w, int h);
static void e_gadcon_layout_pack_aspect_pad_set(Evas_Object *obj, int w, int h);
static void e_gadcon_layout_unpack(Evas_Object *obj);
static void _e_gadcon_provider_populate_request(const E_Gadcon_Client_Class *cc);
static void _e_gadcon_provider_populate_unrequest(const E_Gadcon_Client_Class *cc);
static Eina_Bool  _e_gadcon_provider_populate_idler(void *data);
static Eina_Bool  _e_gadcon_custom_populate_idler(void *data);

static int _e_gadcon_location_change(E_Gadcon_Client * gcc, E_Gadcon_Location *src, E_Gadcon_Location *dst);

typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Layout_Item_Container E_Layout_Item_Container;

static void _e_gadcon_client_current_position_sync(E_Gadcon_Client *gcc);
static void _e_gadcon_layout_smart_sync_clients(E_Gadcon *gc);
static void _e_gadcon_layout_smart_gadcon_position_shrinked_mode(E_Smart_Data *sd);
static void _e_gadcon_layout_smart_gadcons_asked_position_set(E_Smart_Data *sd);
static Eina_List *_e_gadcon_layout_smart_gadcons_wrap(E_Smart_Data *sd);
static void _e_gadcon_layout_smart_gadcons_position(E_Smart_Data *sd, Eina_List **list);
static void _e_gadcon_layout_smart_gadcons_position_static(E_Smart_Data *sd, Eina_List **list);
static E_Layout_Item_Container * _e_gadcon_layout_smart_containers_position_adjust(E_Smart_Data *sd, E_Layout_Item_Container *lc, E_Layout_Item_Container *lc2);
static void _e_gadcon_layout_smart_position_items_inside_container(E_Smart_Data *sd, E_Layout_Item_Container *lc);
static void _e_gadcon_layout_smart_containers_merge(E_Smart_Data *sd, E_Layout_Item_Container *lc, E_Layout_Item_Container *lc2);
static void _e_gadcon_layout_smart_restore_gadcons_position_before_move(E_Smart_Data *sd, E_Layout_Item_Container **lc_moving, E_Layout_Item_Container *lc_back, Eina_List **con_list);

typedef enum _E_Gadcon_Layout_Item_State
{
   E_LAYOUT_ITEM_STATE_NONE,
   E_LAYOUT_ITEM_STATE_POS_INC,
   E_LAYOUT_ITEM_STATE_POS_DEC,
   E_LAYOUT_ITEM_STATE_SIZE_MIN_END_INC,
   E_LAYOUT_ITEM_STATE_SIZE_MIN_END_DEC,
   E_LAYOUT_ITEM_STATE_SIZE_MAX_END_INC,
   E_LAYOUT_ITEM_STATE_SIZE_MAX_END_DEC,
} E_Gadcon_Layout_Item_State;

typedef enum _E_Gadcon_Layout_Item_Flags
{
   E_GADCON_LAYOUT_ITEM_LOCK_NONE     = 0x00000000,
   E_GADCON_LAYOUT_ITEM_LOCK_POSITION = 0x00000001,
   E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE = 0x00000002
} E_Gadcon_Layout_Item_Flags;

typedef enum _E_Layout_Item_Container_State
{
   E_LAYOUT_ITEM_CONTAINER_STATE_NONE, 
   E_LAYOUT_ITEM_CONTAINER_STATE_POS_INC, 
   E_LAYOUT_ITEM_CONTAINER_STATE_POS_DEC, 
   E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MIN_END_INC, 
   E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MIN_END_DEC, 
   E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MAX_END_INC, 
   E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MAX_END_DEC, 
   E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED
} E_Layout_Item_Container_State;

struct _E_Layout_Item_Container
{
   int pos, size, prev_pos, prev_size;

   struct 
     {
        int min_seq, max_seq;
     } state_info;

   E_Smart_Data *sd;
   Eina_List *items;

   E_Layout_Item_Container_State state;
};

#define LC_FREE(__lc) \
   if (__lc->items) \
     eina_list_free(__lc->items); \
   E_FREE(__lc)

#define E_LAYOUT_ITEM_CONTAINER_STATE_SET(__con_state, __bi_state) \
   if (__bi_state == E_LAYOUT_ITEM_STATE_NONE) \
      __con_state = E_LAYOUT_ITEM_CONTAINER_STATE_NONE; \
   else if (__bi_state == E_LAYOUT_ITEM_STATE_POS_INC) \
      __con_state = E_LAYOUT_ITEM_CONTAINER_STATE_POS_INC; \
   else if (__bi_state == E_LAYOUT_ITEM_STATE_POS_DEC) \
      __con_state = E_LAYOUT_ITEM_CONTAINER_STATE_POS_DEC; \
   else if (__bi_state == E_LAYOUT_ITEM_STATE_SIZE_MIN_END_INC) \
      __con_state = E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MIN_END_INC; \
   else if (__bi_state == E_LAYOUT_ITEM_STATE_SIZE_MIN_END_DEC) \
      __con_state = E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MIN_END_DEC; \
   else if (__bi_state == E_LAYOUT_ITEM_STATE_SIZE_MAX_END_INC) \
      __con_state = E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MAX_END_INC; \
   else if (__bi_state == E_LAYOUT_ITEM_STATE_SIZE_MAX_END_DEC) \
      __con_state = E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MAX_END_DEC

#define LC_OVERLAP(__lc, __lc2) \
   ((((__lc2)->pos >= (__lc)->pos) && ((__lc2)->pos < ((__lc)->pos + (__lc)->size))) || \
    (((__lc)->pos >= (__lc2)->pos) && ((__lc)->pos < ((__lc2)->pos + (__lc2)->size))))

#define E_LAYOUT_ITEM_CONTAINER_SIZE_CHANGE_BY(__lc, __bi, __increase) \
     { \
	if (__increase) \
	  __lc->size += __bi->w; \
	else \
	  __lc->size -= __bi->w; \
     }

/********************/

static Eina_Hash *providers = NULL;
static Eina_List *providers_list = NULL;
static Eina_List *gadcons = NULL;
static Eina_List *populate_requests = NULL;
static Ecore_Idler *populate_idler = NULL;
static Eina_List *custom_populate_requests = NULL;
static Ecore_Idler *custom_populate_idler = NULL;
static Eina_List *gadcon_locations = NULL;

/* This is the gadcon client which is currently dragged */
static E_Gadcon_Client *drag_gcc = NULL;
/* This is the gadcon client created on entering a new shelf */
static E_Gadcon_Client *new_gcc = NULL;

/* externally accessible functions */
EINTERN int
e_gadcon_init(void)
{
   return 1;
}

EINTERN int
e_gadcon_shutdown(void)
{
   populate_requests = eina_list_free(populate_requests);
   if (populate_idler)
     {
	ecore_idler_del(populate_idler);
	populate_idler = NULL;
     }
   custom_populate_requests = eina_list_free(custom_populate_requests);
   if (custom_populate_idler)
     {
	ecore_idler_del(custom_populate_idler);
	custom_populate_idler = NULL;
     }

   return 1;
}

EAPI void
e_gadcon_provider_register(const E_Gadcon_Client_Class *cc)
{
   if (!providers) providers = eina_hash_string_superfast_new(NULL);
   eina_hash_direct_add(providers, cc->name, cc);
   providers_list = eina_list_append(providers_list, cc);
   _e_gadcon_provider_populate_request(cc);
}

EAPI void
e_gadcon_provider_unregister(const E_Gadcon_Client_Class *cc)
{
   Eina_List *l, *ll, *dlist = NULL;
   E_Gadcon *gc;
   E_Gadcon_Client *gcc;

   _e_gadcon_provider_populate_unrequest(cc);

   EINA_LIST_FOREACH(gadcons, l, gc)
     {
	EINA_LIST_FOREACH(gc->clients, ll, gcc)
	  {
	     if (gcc->client_class == cc)
	       dlist = eina_list_append(dlist, gcc);
	  }
     }
   EINA_LIST_FREE(dlist, gcc)
     e_object_del(E_OBJECT(gcc));

   eina_hash_del(providers, cc->name, cc);
   providers_list = eina_list_remove(providers_list, cc);
}

EAPI Eina_List *
e_gadcon_provider_list(void)
{
   return providers_list;
}

EAPI void
e_gadcon_custom_new(E_Gadcon *gc)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);

   gadcons = eina_list_append(gadcons, gc);

   if (!custom_populate_idler) 
     {
        custom_populate_idler = 
          ecore_idler_add(_e_gadcon_custom_populate_idler, NULL);
     }
   if (!eina_list_data_find(custom_populate_requests, gc))
     custom_populate_requests = eina_list_append(custom_populate_requests, gc);
}

EAPI void
e_gadcon_custom_del(E_Gadcon *gc)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   gadcons = eina_list_remove(gadcons, gc);
}

EAPI E_Gadcon *
e_gadcon_swallowed_new(const char *name, int id, Evas_Object *obj, const char *swallow_name)
{
   E_Gadcon *gc;
   E_Config_Gadcon *cf_gc;
   Eina_List *l;
   Evas_Coord x, y, w, h;
   const char *drop_types[] = { "enlightenment/gadcon_client" };

   gc = E_OBJECT_ALLOC(E_Gadcon, E_GADCON_TYPE, _e_gadcon_free);
   if (!gc) return NULL;

   gc->name = eina_stringshare_add(name);
   gc->id = id;
   gc->layout_policy = E_GADCON_LAYOUT_POLICY_PANEL;
   gc->location = NULL;

   gc->edje.o_parent = obj;
   gc->edje.swallow_name = eina_stringshare_add(swallow_name);

   gc->orient = E_GADCON_ORIENT_HORIZ;
   gc->evas = evas_object_evas_get(obj);
   gc->o_container = e_gadcon_layout_add(gc->evas);
   evas_object_geometry_get(gc->o_container, &x, &y, &w, &h);
   gc->drop_handler = 
     e_drop_handler_add(E_OBJECT(gc), gc,
                        _e_gadcon_cb_dnd_enter, _e_gadcon_cb_dnd_move,
                        _e_gadcon_cb_dnd_leave, _e_gadcon_cb_drop,
                        drop_types, 1, x, y, w, h);
   evas_object_event_callback_add(gc->o_container, EVAS_CALLBACK_MOVE,
				  _e_gadcon_cb_moveresize, gc);
   evas_object_event_callback_add(gc->o_container, EVAS_CALLBACK_RESIZE,
				  _e_gadcon_cb_moveresize, gc);
   evas_object_smart_callback_add(gc->o_container, "size_request",
				  _e_gadcon_cb_size_request, gc);
   evas_object_smart_callback_add(gc->o_container, "min_size_request",
				  _e_gadcon_cb_min_size_request, gc);
   evas_object_show(gc->o_container);
   edje_object_part_swallow(gc->edje.o_parent, gc->edje.swallow_name,
			    gc->o_container);
   gadcons = eina_list_append(gadcons, gc);

   EINA_LIST_FOREACH(e_config->gadcons, l, cf_gc)
     {
	if ((!strcmp(cf_gc->name, gc->name)) && (cf_gc->id == gc->id))
	  {
	     gc->cf = cf_gc;
	     break;
	  }
     }
   if (!gc->cf)
     {
	gc->cf = E_NEW(E_Config_Gadcon, 1);
	gc->cf->name = eina_stringshare_add(gc->name);
	gc->cf->id = gc->id;
        if (gc->zone) gc->cf->zone = gc->zone->id;
	e_config->gadcons = eina_list_append(e_config->gadcons, gc->cf);
	e_config_save_queue();
     }
   return gc;
}

EAPI void
e_gadcon_swallowed_min_size_set(E_Gadcon *gc, Evas_Coord w, Evas_Coord h)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   if (gc->edje.o_parent)
     {
	edje_extern_object_min_size_set(gc->o_container, w, h);
	edje_object_part_swallow(gc->edje.o_parent, gc->edje.swallow_name,
				 gc->o_container);
     }
}

EAPI void
e_gadcon_min_size_request_callback_set(E_Gadcon *gc, void (*func) (void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h), void *data)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   gc->min_size_request.func = func;
   gc->min_size_request.data = data;
}

EAPI void
e_gadcon_size_request_callback_set(E_Gadcon *gc, void (*func) (void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h), void *data)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   gc->resize_request.func = func;
   gc->resize_request.data = data;
}

EAPI void
e_gadcon_frame_request_callback_set(E_Gadcon *gc, Evas_Object *(*func) (void *data, E_Gadcon_Client *gcc, const char *style), void *data)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   gc->frame_request.func = func;
   gc->frame_request.data = data;
}

EAPI void
e_gadcon_populate_callback_set(E_Gadcon *gc, void (*func) (void *data, E_Gadcon *gc, const E_Gadcon_Client_Class *cc), void *data)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   gc->populate_class.func = func;
   gc->populate_class.data = data;
}

EAPI void
e_gadcon_layout_policy_set(E_Gadcon *gc, E_Gadcon_Layout_Policy layout_policy)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   if (gc->layout_policy == layout_policy) return;
   gc->layout_policy = layout_policy;
   /* FIXME: delete container obj, re-pack all clients */
}

EAPI void
e_gadcon_populate(E_Gadcon *gc)
{
   Eina_List *l;
   E_Config_Gadcon_Client *cf_gcc;

   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   e_gadcon_layout_freeze(gc->o_container);
   EINA_LIST_FOREACH(gc->cf->clients, l, cf_gcc)
     {
	E_Gadcon_Client_Class *cc;

	if (!cf_gcc->name) continue;
	cc = eina_hash_find(providers, cf_gcc->name);
	if (cc)
	  {
	     E_Gadcon_Client *gcc;

	     if (eina_list_data_find(populate_requests, cc))
	       continue;

	     if ((!cf_gcc->id) &&
		 (_e_gadcon_client_class_feature_check(cc, "id_new", cc->func.id_new)))
	       cf_gcc->id = eina_stringshare_add(cc->func.id_new(cc));

	     if (!cf_gcc->style)
               gcc = cc->func.init(gc, cf_gcc->name, cf_gcc->id,
                                   cc->default_style);
	     else
	       gcc = cc->func.init(gc, cf_gcc->name, cf_gcc->id,
                                   cf_gcc->style);

	     if (gcc)
	       {
		  gcc->cf = cf_gcc;
		  gcc->client_class = cc;
		  gcc->config.pos = cf_gcc->geom.pos;
		  gcc->config.size = cf_gcc->geom.size;
		  gcc->config.res = cf_gcc->geom.res;
		  gcc->state_info.seq = cf_gcc->state_info.seq;
		  gcc->state_info.flags = cf_gcc->state_info.flags;
		  if (gcc->o_frame)
		    e_gadcon_layout_pack_options_set(gcc->o_frame, gcc);
		  else if (gcc->o_base)
		    e_gadcon_layout_pack_options_set(gcc->o_base, gcc);

                  if (!gcc->autoscroll_set)
                    e_gadcon_client_autoscroll_set(gcc, cf_gcc->autoscroll);
//		  e_gadcon_client_resizable_set(gcc, cf_gcc->resizable);
		  if (gcc->client_class->func.orient)
		    gcc->client_class->func.orient(gcc, gc->orient);

		  _e_gadcon_client_save(gcc);
		  if (gc->editing) e_gadcon_client_edit_begin(gcc);
		  if (gc->instant_edit)
		    e_gadcon_client_util_menu_attach(gcc);
	       }
	  }
     }
   e_gadcon_layout_thaw(gc->o_container);
}

EAPI void
e_gadcon_unpopulate(E_Gadcon *gc)
{
   E_Gadcon_Client *gcc;

   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   /* Be careful, e_object_del does remove gcc from gc->clients */
   while (gc->clients)
     {
	gcc = eina_list_data_get(gc->clients);
	if (gcc->menu)
	  {
	     e_menu_post_deactivate_callback_set(gcc->menu, NULL, NULL);
	     e_object_del(E_OBJECT(gcc->menu));
	     gcc->menu = NULL;
	  }
	e_object_del(E_OBJECT(gcc));
     }
}

EAPI void
e_gadcon_populate_class(E_Gadcon *gc, const E_Gadcon_Client_Class *cc)
{
   Eina_List *l;
   E_Config_Gadcon_Client *cf_gcc;

   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   e_gadcon_layout_freeze(gc->o_container);
   EINA_LIST_FOREACH(gc->cf->clients, l, cf_gcc)
     {
	if ((cf_gcc->name) && (cc->name) && 
	    (!strcmp(cf_gcc->name, cc->name)) &&
	    (cf_gcc->id) && (cf_gcc->style))
	  {
	     E_Gadcon_Client *gcc;

	     if ((!cf_gcc->id) &&
		 (_e_gadcon_client_class_feature_check((E_Gadcon_Client_Class *)cc, "id_new", cc->func.id_new)))
	       cf_gcc->id = eina_stringshare_add(cc->func.id_new((E_Gadcon_Client_Class *)cc));

	     gcc = cc->func.init(gc, cf_gcc->name, cf_gcc->id,
				 cf_gcc->style);
	     if (gcc)
	       {
		  gcc->cf = cf_gcc;
		  gcc->client_class = cc;
		  gcc->config.pos = cf_gcc->geom.pos;
		  gcc->config.size = cf_gcc->geom.size;
		  gcc->config.res = cf_gcc->geom.res;
		  gcc->state_info.seq = cf_gcc->state_info.seq;
		  gcc->state_info.flags = cf_gcc->state_info.flags;
		  if (gcc->o_frame)
		    e_gadcon_layout_pack_options_set(gcc->o_frame, gcc);
		  else if (gcc->o_base)
		    e_gadcon_layout_pack_options_set(gcc->o_base, gcc);

                  if (!gcc->autoscroll_set)
                    e_gadcon_client_autoscroll_set(gcc, cf_gcc->autoscroll);
//		  e_gadcon_client_resizable_set(gcc, cf_gcc->resizable);
		  if (gcc->client_class->func.orient)
		    gcc->client_class->func.orient(gcc, gc->orient); 

		  _e_gadcon_client_save(gcc);
		  if (gc->editing) e_gadcon_client_edit_begin(gcc);
		  if (gc->instant_edit)
		    e_gadcon_client_util_menu_attach(gcc);
	       }
	  }
     }
   e_gadcon_layout_thaw(gc->o_container);
}

EAPI void
e_gadcon_orient(E_Gadcon *gc, E_Gadcon_Orient orient)
{
   Eina_List *l;
   E_Gadcon_Client *gcc;
   int horiz = 0;

   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   if (gc->orient == orient) return;
   gc->orient = orient;
   e_gadcon_layout_freeze(gc->o_container);
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
	horiz = 1;
	break;
      case E_GADCON_ORIENT_VERT:
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_CORNER_RT:
      case E_GADCON_ORIENT_CORNER_LB:
      case E_GADCON_ORIENT_CORNER_RB:
	horiz = 0;
	break;
      default:
	break;
     }
   e_gadcon_layout_orientation_set(gc->o_container, horiz);
   EINA_LIST_FOREACH(gc->clients, l, gcc)
     {
	e_box_orientation_set(gcc->o_box, horiz);
	if (gcc->client_class->func.orient)
	  gcc->client_class->func.orient(gcc, gc->orient);
     }
   e_gadcon_layout_thaw(gc->o_container);
}

EAPI void
e_gadcon_edit_begin(E_Gadcon *gc)
{
   Eina_List *l;
   E_Gadcon_Client *gcc;

   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   e_gadcon_layout_freeze(gc->o_container);
   e_gadcon_locked_set(gc, 1);
   gc->editing = 1;
   EINA_LIST_FOREACH(gc->clients, l, gcc)
     e_gadcon_client_edit_begin(gcc);
   e_gadcon_layout_thaw(gc->o_container);
}

EAPI void
e_gadcon_edit_end(E_Gadcon *gc)
{
   Eina_List *l;
   E_Gadcon_Client *gcc;

   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   e_gadcon_layout_freeze(gc->o_container);
   gc->editing = 0;
   EINA_LIST_FOREACH(gc->clients, l, gcc)
     e_gadcon_client_edit_end(gcc);
   e_gadcon_layout_thaw(gc->o_container);
   e_gadcon_locked_set(gc, 0);
}

EAPI void
e_gadcon_all_edit_begin(void)
{
   Eina_List *l;
   E_Gadcon *gc;

   EINA_LIST_FOREACH(gadcons, l, gc)
     e_gadcon_edit_begin(gc);
}

EAPI void
e_gadcon_all_edit_end(void)
{
   Eina_List *l;
   E_Gadcon *gc;

   EINA_LIST_FOREACH(gadcons, l, gc)
     e_gadcon_edit_end(gc);
}

EAPI void
e_gadcon_zone_set(E_Gadcon *gc, E_Zone *zone)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   gc->zone = zone;
   if (gc->cf) gc->cf->zone = zone->id;
}

EAPI E_Zone *
e_gadcon_zone_get(E_Gadcon *gc)
{
   E_OBJECT_CHECK_RETURN(gc, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(gc, E_GADCON_TYPE, NULL);
   return gc->zone;
}

EAPI void
e_gadcon_ecore_evas_set(E_Gadcon *gc, Ecore_Evas *ee)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   gc->ecore_evas = ee;
}

EAPI int
e_gadcon_canvas_zone_geometry_get(E_Gadcon *gc, int *x, int *y, int *w, int *h)
{
   E_OBJECT_CHECK_RETURN(gc, 0);
   E_OBJECT_TYPE_CHECK_RETURN(gc, E_GADCON_TYPE, 0);
   if (!gc->ecore_evas) return 0;
   ecore_evas_geometry_get(gc->ecore_evas, x, y, w, h);
// so much relies on this down here to have been broken... ie return container-relative coords.
//   if (gc->zone)
//     {
//	if (x) *x = *x - gc->zone->x;
//	if (y) *y = *y - gc->zone->y;
//     }
   return 1;
}

EAPI void
e_gadcon_util_menu_attach_func_set(E_Gadcon *gc, void (*func) (void *data, E_Gadcon_Client *gcc, E_Menu *menu), void *data)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   gc->menu_attach.func = func;
   gc->menu_attach.data = data;
}

EAPI void
e_gadcon_util_lock_func_set(E_Gadcon *gc, void (*func) (void *data, int lock), void *data)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   gc->locked_set.func = func;
   gc->locked_set.data = data;
}

EAPI void
e_gadcon_util_urgent_show_func_set(E_Gadcon *gc, void (*func) (void *data), void *data)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   gc->urgent_show.func = func;
   gc->urgent_show.data = data;
}

EAPI void
e_gadcon_dnd_window_set(E_Gadcon *gc, Ecore_X_Window win)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   gc->dnd_win = win;
}
							 
EAPI Ecore_X_Window
e_gadcon_dnd_window_get(E_Gadcon *gc)
{
   E_OBJECT_CHECK_RETURN(gc, 0);
   E_OBJECT_TYPE_CHECK_RETURN(gc, E_GADCON_TYPE, 0);
   return gc->dnd_win;
}

EAPI void
e_gadcon_xdnd_window_set(E_Gadcon *gc, Ecore_X_Window win)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   gc->xdnd_win = win;
}
							 
EAPI Ecore_X_Window
e_gadcon_xdnd_window_get(E_Gadcon *gc)
{
   E_OBJECT_CHECK_RETURN(gc, 0);
   E_OBJECT_TYPE_CHECK_RETURN(gc, E_GADCON_TYPE, 0);
   return gc->xdnd_win;
}

EAPI void
e_gadcon_shelf_set(E_Gadcon *gc, E_Shelf *shelf)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   gc->shelf = shelf;
}
							 
EAPI E_Shelf *
e_gadcon_shelf_get(E_Gadcon *gc)
{
   E_OBJECT_CHECK_RETURN(gc, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(gc, E_GADCON_TYPE, NULL);
   return gc->shelf;
}

EAPI void 
e_gadcon_toolbar_set(E_Gadcon *gc, E_Toolbar *toolbar) 
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   gc->toolbar = toolbar;
}

EAPI E_Toolbar *
e_gadcon_toolbar_get(E_Gadcon *gc) 
{
   E_OBJECT_CHECK_RETURN(gc, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(gc, E_GADCON_TYPE, NULL);
   return gc->toolbar;
}

EAPI E_Config_Gadcon_Client *
e_gadcon_client_config_new(E_Gadcon *gc, const char *name)
{
   E_Gadcon_Client_Class *cc;
   E_Config_Gadcon_Client *cf_gcc;

   E_OBJECT_CHECK_RETURN(gc, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(gc, E_GADCON_TYPE, NULL);
   if (!name) return NULL;
   cc = eina_hash_find(providers, name);
   if (!cc) return NULL;
   if (!_e_gadcon_client_class_feature_check(cc, "id_new", cc->func.id_new)) 
     return NULL;

   cf_gcc = E_NEW(E_Config_Gadcon_Client, 1);
   if (!cf_gcc) return NULL;
   cf_gcc->id = eina_stringshare_add(cc->func.id_new(cc));
   if (!cf_gcc->id)
     {
        free(cf_gcc);
        return NULL;
     }
   cf_gcc->name = eina_stringshare_add(name);
   if (gc->zone)
     cf_gcc->geom.res = gc->zone->w;
   else
     cf_gcc->geom.res = 800;
   cf_gcc->geom.size = 80;
   cf_gcc->geom.pos = cf_gcc->geom.res - cf_gcc->geom.size;
   cf_gcc->style = NULL;
   cf_gcc->autoscroll = 0;
   cf_gcc->resizable = 0;
   gc->cf->clients = eina_list_append(gc->cf->clients, cf_gcc);
   e_config_save_queue();
   return cf_gcc;
}

EAPI void
e_gadcon_client_config_del(E_Config_Gadcon *cf_gc, E_Config_Gadcon_Client *cf_gcc)
{
   if (!cf_gcc) return;
   if (cf_gcc->name) eina_stringshare_del(cf_gcc->name);
   if (cf_gcc->id) eina_stringshare_del(cf_gcc->id);
   if (cf_gcc->style) eina_stringshare_del(cf_gcc->style);
   if (cf_gc) cf_gc->clients = eina_list_remove(cf_gc->clients, cf_gcc);
   free(cf_gcc);
}

EAPI E_Gadcon_Client *
e_gadcon_client_new(E_Gadcon *gc, const char *name, const char *id __UNUSED__, const char *style, Evas_Object *base_obj)
{
   E_Gadcon_Client *gcc;

   E_OBJECT_CHECK_RETURN(gc, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(gc, E_GADCON_TYPE, NULL);
   gcc = E_OBJECT_ALLOC(E_Gadcon_Client, E_GADCON_CLIENT_TYPE, 
                        _e_gadcon_client_free);
   if (!gcc) return NULL;
   gcc->name = eina_stringshare_add(name);
   gcc->gadcon = gc;
   gcc->o_base = base_obj;
   if (gc->clients)
     gcc->id = E_GADCON_CLIENT(eina_list_data_get(eina_list_last(gc->clients)))->id + 1;
   gc->clients = eina_list_append(gc->clients, gcc);
   /* This must only be unique during runtime */
   if (gcc->o_base)
     evas_object_event_callback_add(gcc->o_base, EVAS_CALLBACK_DEL,
				    _e_gadcon_client_del_hook, gcc);
   if ((gc->frame_request.func) && (style))
     {
	gcc->o_frame = gc->frame_request.func(gc->frame_request.data, gcc, style);
	gcc->style = eina_stringshare_add(style);
	if (gcc->o_frame)
	  {
	     edje_object_size_min_calc(gcc->o_frame, &(gcc->pad.w), &(gcc->pad.h));
	     gcc->o_box = e_box_add(gcc->gadcon->evas);
	     switch (gcc->gadcon->orient)
	       {
		case E_GADCON_ORIENT_FLOAT:
		case E_GADCON_ORIENT_HORIZ:
		case E_GADCON_ORIENT_TOP:
		case E_GADCON_ORIENT_BOTTOM:
		case E_GADCON_ORIENT_CORNER_TL:
		case E_GADCON_ORIENT_CORNER_TR:
		case E_GADCON_ORIENT_CORNER_BL:
		case E_GADCON_ORIENT_CORNER_BR:
		  e_box_orientation_set(gcc->o_box, 1);
		  break;
		case E_GADCON_ORIENT_VERT:
		case E_GADCON_ORIENT_LEFT:
		case E_GADCON_ORIENT_RIGHT:
		case E_GADCON_ORIENT_CORNER_LT:
		case E_GADCON_ORIENT_CORNER_RT:
		case E_GADCON_ORIENT_CORNER_LB:
		case E_GADCON_ORIENT_CORNER_RB:
		  e_box_orientation_set(gcc->o_box, 0);
		  break;
		default:
		  break;
	       }
	     evas_object_event_callback_add(gcc->o_box, EVAS_CALLBACK_MOVE,
					    _e_gadcon_cb_client_frame_moveresize, gcc);
	     evas_object_event_callback_add(gcc->o_box, EVAS_CALLBACK_RESIZE,
					    _e_gadcon_cb_client_frame_moveresize, gcc);
	     evas_object_event_callback_add(gcc->o_frame, EVAS_CALLBACK_MOUSE_MOVE,
					    _e_gadcon_cb_client_frame_mouse_move, gcc);
	     if (gcc->o_base)
	       {
		  e_box_pack_end(gcc->o_box, gcc->o_base);
		  e_box_pack_options_set(gcc->o_base,
					 1, 1, /* fill */
					 1, 1, /* expand */
					 0.5, 0.5, /* align */
					 0, 0, /* min */
					 -1, -1 /* max */
					 );
	       }
	     edje_object_part_swallow(gcc->o_frame, gc->edje.swallow_name, gcc->o_box);
	     evas_object_show(gcc->o_box);
	     evas_object_show(gcc->o_frame);
	  }
     }
   if (gcc->o_frame)
     e_gadcon_layout_pack(gc->o_container, gcc->o_frame);
   else if (gcc->o_base)
     e_gadcon_layout_pack(gc->o_container, gcc->o_base);
   if (gcc->o_base) evas_object_show(gcc->o_base);
   return gcc;
}

EAPI void
e_gadcon_client_edit_begin(E_Gadcon_Client *gcc)
{
   Evas_Coord x, y, w, h;

   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);
   if (gcc->o_control) return;

   if (gcc->o_frame)
     evas_object_geometry_get(gcc->o_frame, &x, &y, &w, &h);
   else if (gcc->o_base)
     evas_object_geometry_get(gcc->o_base, &x, &y, &w, &h);
   else return; /* make clang happy */

   gcc->o_control = edje_object_add(gcc->gadcon->evas);
   evas_object_layer_set(gcc->o_control, 100);
   e_gadcon_locked_set(gcc->gadcon, 1);
   gcc->gadcon->editing = 1;

   evas_object_move(gcc->o_control, x, y);
   evas_object_resize(gcc->o_control, w, h);
   e_theme_edje_object_set(gcc->o_control, "base/theme/gadman", 
                           "e/gadman/control");

   if ((gcc->autoscroll)/* || (gcc->resizable)*/)
     {
	if (e_box_orientation_get(gcc->o_box))
	  edje_object_signal_emit(gcc->o_control, "e,state,hsize,on", "e");
	else
	  edje_object_signal_emit(gcc->o_control, "e,state,vsize,on", "e");
     }
   else
     {
	edje_object_signal_emit(gcc->o_control, "e,state,hsize,off", "e");
	edje_object_signal_emit(gcc->o_control, "e,state,vsize,off", "e");
     }
   edje_object_signal_emit(gcc->o_control, "e,state,move,on", "e");

   gcc->o_event = evas_object_rectangle_add(gcc->gadcon->evas);
   evas_object_color_set(gcc->o_event, 0, 0, 0, 0);
   evas_object_repeat_events_set(gcc->o_event, 1);
   evas_object_layer_set(gcc->o_event, 100);
   evas_object_move(gcc->o_event, x, y);
   evas_object_resize(gcc->o_event, w, h);

   edje_object_signal_callback_add(gcc->o_control, "e,action,move,start", "",
				   _e_gadcon_cb_signal_move_start, gcc);
   edje_object_signal_callback_add(gcc->o_control, "e,action,move,stop", "",
				   _e_gadcon_cb_signal_move_stop, gcc);
   edje_object_signal_callback_add(gcc->o_control, "e,action,move,go", "",
				   _e_gadcon_cb_signal_move_go, gcc);
   edje_object_signal_callback_add(gcc->o_control, "e,action,resize,left,start", "",
				   _e_gadcon_cb_signal_resize_left_start, gcc);
   edje_object_signal_callback_add(gcc->o_control, "e,action,resize,left,stop", "",
				   _e_gadcon_cb_signal_resize_left_stop, gcc);
   edje_object_signal_callback_add(gcc->o_control, "e,action,resize,left,go", "",
				   _e_gadcon_cb_signal_resize_left_go, gcc);
   edje_object_signal_callback_add(gcc->o_control, "e,action,resize,right,start", "",
				   _e_gadcon_cb_signal_resize_right_start, gcc);
   edje_object_signal_callback_add(gcc->o_control, "e,action,resize,right,stop", "",
				   _e_gadcon_cb_signal_resize_right_stop, gcc);
   edje_object_signal_callback_add(gcc->o_control, "e,action,resize,right,go", "",
				   _e_gadcon_cb_signal_resize_right_go, gcc);
   edje_object_signal_callback_add(gcc->o_control, "e,action,resize,up,start", "",
				   _e_gadcon_cb_signal_resize_left_start, gcc);
   edje_object_signal_callback_add(gcc->o_control, "e,action,resize,up,stop", "",
				   _e_gadcon_cb_signal_resize_left_stop, gcc);
   edje_object_signal_callback_add(gcc->o_control, "e,action,resize,up,go", "",
				   _e_gadcon_cb_signal_resize_left_go, gcc);
   edje_object_signal_callback_add(gcc->o_control, "e,action,resize,down,start", "",
				   _e_gadcon_cb_signal_resize_right_start, gcc);
   edje_object_signal_callback_add(gcc->o_control, "e,action,resize,down,stop", "",
				   _e_gadcon_cb_signal_resize_right_stop, gcc);
   edje_object_signal_callback_add(gcc->o_control, "e,action,resize,down,go", "",
				   _e_gadcon_cb_signal_resize_right_go, gcc);

   evas_object_event_callback_add(gcc->o_event, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _e_gadcon_cb_client_mouse_down, gcc);
   evas_object_event_callback_add(gcc->o_event, EVAS_CALLBACK_MOUSE_IN, 
                                  _e_gadcon_cb_client_mouse_in, gcc);
   evas_object_event_callback_add(gcc->o_event, EVAS_CALLBACK_MOUSE_OUT, 
                                  _e_gadcon_cb_client_mouse_out, gcc);

   if (gcc->o_frame)
     {
	evas_object_event_callback_add(gcc->o_frame, EVAS_CALLBACK_MOVE, 
                                       _e_gadcon_cb_client_move, gcc);
	evas_object_event_callback_add(gcc->o_frame, EVAS_CALLBACK_RESIZE, 
                                       _e_gadcon_cb_client_resize, gcc);
     }
   else if (gcc->o_base)
     {
	evas_object_event_callback_add(gcc->o_base, EVAS_CALLBACK_MOVE, 
                                       _e_gadcon_cb_client_move, gcc);
	evas_object_event_callback_add(gcc->o_base, EVAS_CALLBACK_RESIZE, 
                                       _e_gadcon_cb_client_resize, gcc);
     }

   evas_object_show(gcc->o_event);
   evas_object_show(gcc->o_control);
}

EAPI void
e_gadcon_client_edit_end(E_Gadcon_Client *gcc)
{
   Eina_List *l = NULL;
   E_Gadcon_Client *client = NULL;

   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);

   if (gcc->o_frame)
     {
	evas_object_event_callback_del(gcc->o_frame, EVAS_CALLBACK_MOVE, 
                                       _e_gadcon_cb_client_move);
	evas_object_event_callback_del(gcc->o_frame, EVAS_CALLBACK_RESIZE, 
                                       _e_gadcon_cb_client_resize);
     }
   else if (gcc->o_base)
     {
	evas_object_event_callback_del(gcc->o_base, EVAS_CALLBACK_MOVE, 
                                       _e_gadcon_cb_client_move);
	evas_object_event_callback_del(gcc->o_base, EVAS_CALLBACK_RESIZE, 
                                       _e_gadcon_cb_client_resize);
     }

   if (gcc->moving)
     {
	gcc->moving = 0;
	_e_gadcon_client_save(gcc);
     }
   if (gcc->o_event) evas_object_del(gcc->o_event);
   gcc->o_event = NULL;
   if (gcc->o_control) evas_object_del(gcc->o_control);
   gcc->o_control = NULL;

   EINA_LIST_FOREACH(gcc->gadcon->clients, l, client)
     {
	if (!client) continue;
	if (client->o_control) return;
     }
   gcc->gadcon->editing = 0;
   e_gadcon_locked_set(gcc->gadcon, 0);
}

EAPI void
e_gadcon_client_show(E_Gadcon_Client *gcc)
{
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);

   if (!gcc->hidden) return;
   if (gcc->o_box) evas_object_show(gcc->o_box);
   if (gcc->o_frame) evas_object_show(gcc->o_frame);
   if (gcc->o_base) evas_object_show(gcc->o_base);
   if (gcc->o_control) evas_object_show(gcc->o_control);
   if (gcc->o_event) evas_object_show(gcc->o_event);
   if (gcc->o_frame)
     {
	e_gadcon_layout_pack(gcc->gadcon->o_container, gcc->o_frame);
	e_gadcon_layout_pack_options_set(gcc->o_frame, gcc);
     }
   else if (gcc->o_base)
     {
	e_gadcon_layout_pack(gcc->gadcon->o_container, gcc->o_base);
	e_gadcon_layout_pack_options_set(gcc->o_base, gcc);
     }
   gcc->hidden = 0;
}

EAPI void
e_gadcon_client_hide(E_Gadcon_Client *gcc)
{
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);

   if (gcc->hidden) return;
   if (gcc->o_box) evas_object_hide(gcc->o_box);
   if (gcc->o_frame) evas_object_hide(gcc->o_frame);
   if (gcc->o_base) evas_object_hide(gcc->o_base);
   if (gcc->o_control) evas_object_hide(gcc->o_control);
   if (gcc->o_event) evas_object_hide(gcc->o_event);
   if (gcc->o_frame)
     e_gadcon_layout_unpack(gcc->o_frame);
   else if (gcc->o_base)
     e_gadcon_layout_unpack(gcc->o_base);
   gcc->hidden = 1;
}

EAPI void
e_gadcon_client_size_request(E_Gadcon_Client *gcc, Evas_Coord w, Evas_Coord h)
{
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);
   switch (gcc->gadcon->orient)
     {
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_BOTTOM:
	if (gcc->o_frame)
	  e_gadcon_layout_pack_size_set(gcc->o_frame, w + gcc->pad.w);
	else if (gcc->o_base)
	  e_gadcon_layout_pack_size_set(gcc->o_base, w);
	break;
      case E_GADCON_ORIENT_VERT:
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_RIGHT:
	if (gcc->o_frame)
	  e_gadcon_layout_pack_size_set(gcc->o_frame, h + gcc->pad.h);
	else if (gcc->o_base)
	  e_gadcon_layout_pack_size_set(gcc->o_base, h);
	break;
      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_CORNER_RT:
      case E_GADCON_ORIENT_CORNER_LB:
      case E_GADCON_ORIENT_CORNER_RB:
      default:
	break;
     }
}

EAPI void
e_gadcon_client_min_size_set(E_Gadcon_Client *gcc, Evas_Coord w, Evas_Coord h)
{
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);
   gcc->min.w = w;
   gcc->min.h = h;
/*   if (!gcc->resizable)*/
     {
	if (gcc->o_frame)
	  e_gadcon_layout_pack_min_size_set(gcc->o_frame, w + gcc->pad.w, 
                                            h + gcc->pad.h);
	else if (gcc->o_base)
	  e_gadcon_layout_pack_min_size_set(gcc->o_base, w, h);
     }
   _e_gadcon_moveresize_handle(gcc);
}

EAPI void
e_gadcon_client_aspect_set(E_Gadcon_Client *gcc, int w, int h)
{
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);
   gcc->aspect.w = w;
   gcc->aspect.h = h;
//   if ((!gcc->autoscroll)/* && (!gcc->resizable)*/)
     {
	if (gcc->o_frame)
	  {
	     e_gadcon_layout_pack_aspect_pad_set(gcc->o_frame, gcc->pad.w, 
                                                 gcc->pad.h);
	     e_gadcon_layout_pack_aspect_set(gcc->o_frame, w, h);
	  }
	else if (gcc->o_base)
	  e_gadcon_layout_pack_aspect_set(gcc->o_base, w, h);
     }
   _e_gadcon_moveresize_handle(gcc);
}

EAPI void
e_gadcon_client_autoscroll_set(E_Gadcon_Client *gcc, int autoscroll)
{
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);

   gcc->autoscroll = autoscroll;
   gcc->autoscroll_set = 1;
/*   
   if (gcc->autoscroll)
     {
	if (gcc->o_frame)
	  {
	     e_gadcon_layout_pack_aspect_pad_set(gcc->o_frame, gcc->pad.w, 
                                                 gcc->pad.h);
	     e_gadcon_layout_pack_aspect_set(gcc->o_frame, 0, 0);
	     e_gadcon_layout_pack_min_size_set(gcc->o_frame, 0, 0);
	  }
	else if (gcc->o_base)
	  {
	     e_gadcon_layout_pack_aspect_set(gcc->o_base, 0, 0);
	     e_gadcon_layout_pack_min_size_set(gcc->o_base, 0, 0);
	  }
     }
   else
 */
     {
	if (gcc->o_frame)
	  {
	     e_gadcon_layout_pack_aspect_pad_set(gcc->o_frame, gcc->pad.w, 
                                                 gcc->pad.h);
	     e_gadcon_layout_pack_aspect_set(gcc->o_frame, gcc->aspect.w, 
                                             gcc->aspect.h);
	     e_gadcon_layout_pack_min_size_set(gcc->o_frame, gcc->min.w, 
                                               gcc->min.h);
	  }
	else if (gcc->o_base)
	  {
	     e_gadcon_layout_pack_min_size_set(gcc->o_base, gcc->min.w, 
                                               gcc->min.h);
	     e_gadcon_layout_pack_aspect_set(gcc->o_base, gcc->aspect.w, 
                                             gcc->aspect.h);
	  }
     }
}
    
EAPI void
e_gadcon_client_resizable_set(E_Gadcon_Client *gcc, int resizable)
{
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);
/*
   gcc->resizable = resizable;
   if (gcc->resizable)
     {
	if (gcc->o_frame)
	  {
	     e_gadcon_layout_pack_aspect_pad_set(gcc->o_frame, gcc->pad.w, 
                                                 gcc->pad.h);
	     e_gadcon_layout_pack_aspect_set(gcc->o_frame, 0, 0);
	     e_gadcon_layout_pack_min_size_set(gcc->o_frame, 0, 0);
	  }
	else if (gcc->o_base)
	  {
	     e_gadcon_layout_pack_min_size_set(gcc->o_base, 0, 0);
	     e_gadcon_layout_pack_aspect_set(gcc->o_base, 0, 0);
	  }
     }
   else
     {
	if (gcc->o_frame)
	  {
	     e_gadcon_layout_pack_aspect_pad_set(gcc->o_frame, gcc->pad.w, 
                                                 gcc->pad.h);
	     e_gadcon_layout_pack_aspect_set(gcc->o_frame, gcc->aspect.w, 
                                             gcc->aspect.h);
	     e_gadcon_layout_pack_min_size_set(gcc->o_frame, gcc->min.w, 
                                               gcc->min.h);
	  }
	else if (gcc->o_base)
	  {
	     e_gadcon_layout_pack_min_size_set(gcc->o_base, gcc->min.w, 
                                               gcc->min.h);
	     e_gadcon_layout_pack_aspect_set(gcc->o_base, gcc->aspect.w, 
                                             gcc->aspect.h);
	  }
     }
 */
   resizable = 0;
   gcc = NULL;
}

EAPI int
e_gadcon_client_geometry_get(E_Gadcon_Client *gcc, int *x, int *y, int *w, int *h)
{
   int gx = 0, gy = 0;

   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);
   if (!e_gadcon_canvas_zone_geometry_get(gcc->gadcon, &gx, &gy, NULL, NULL)) 
     return 0;
   if (gcc->o_base) evas_object_geometry_get(gcc->o_base, x, y, w, h);
   if (x) *x += gx;
   if (y) *y += gy;
   return 1;
}

EAPI int
e_gadcon_client_viewport_geometry_get(E_Gadcon_Client *gcc, int *x, int *y, int *w, int *h)
{
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);

   if (gcc->o_box) evas_object_geometry_get(gcc->o_base, x, y, w, h);
   else if (gcc->o_base) evas_object_geometry_get(gcc->o_base, x, y, w, h);
   else
     {
        if (x) *x = 0;
        if (y) *y = 0;
        if (w) *w = 0;
        if (h) *h = 0;
     }
   return 1;
}

EAPI E_Zone *
e_gadcon_client_zone_get(E_Gadcon_Client *gcc)
{
   E_OBJECT_CHECK_RETURN(gcc, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(gcc, E_GADCON_CLIENT_TYPE, NULL);
   return e_gadcon_zone_get(gcc->gadcon);
}

static void
_e_gadcon_client_change_gadcon(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi)
{
   E_Gadcon_Location *src, *dst;
   E_Gadcon_Client * gcc;

   gcc = data;
   src = gcc->gadcon->location;
   dst = e_object_data_get(E_OBJECT(mi));
   _e_gadcon_location_change(gcc, src, dst);
}

static void
_e_gadcon_add_locations_menu_for_site(E_Menu *m, E_Gadcon_Client *gcc, E_Gadcon_Site site, int * count)
{
   E_Menu_Item *mi;
   const Eina_List *l;
   E_Gadcon_Location * loc;
   int k = *count;

   EINA_LIST_FOREACH(gadcon_locations, l, loc)
     {
	if (loc->site == site)
	  {
             if (k)
               {
                  k = 0;
                  mi = e_menu_item_new(m);
                  e_menu_item_separator_set(mi, 1);
                  (*count) = 0;
               }
             mi = e_menu_item_new(m);
             e_menu_item_label_set(mi, loc->name);
             e_object_data_set(E_OBJECT(mi), loc);
             e_menu_item_callback_set(mi, _e_gadcon_client_change_gadcon, gcc);
             if (loc == gcc->gadcon->location) e_menu_item_disabled_set(mi, 1);
             if (loc->icon_name) 
               e_util_menu_item_theme_icon_set(mi, loc->icon_name);
             (*count)++;
	  }
     }
}

static void 
_e_gadcon_gadget_move_to_pre_cb(void *data, E_Menu *m) 
{
   E_Gadcon_Client *gcc;
   int n = 0;

   e_menu_pre_activate_callback_set(m, NULL, NULL);
   gcc = data;

   if (!gcc->client_class->func.is_site || gcc->client_class->func.is_site(E_GADCON_SITE_SHELF))
     _e_gadcon_add_locations_menu_for_site(m, gcc, E_GADCON_SITE_SHELF, &n);
   if (!gcc->client_class->func.is_site || gcc->client_class->func.is_site(E_GADCON_SITE_DESKTOP))
     _e_gadcon_add_locations_menu_for_site(m, gcc, E_GADCON_SITE_DESKTOP, &n);
   if (!gcc->client_class->func.is_site || gcc->client_class->func.is_site(E_GADCON_SITE_TOOLBAR))
     _e_gadcon_add_locations_menu_for_site(m, gcc, E_GADCON_SITE_TOOLBAR, &n);
   if (!gcc->client_class->func.is_site || gcc->client_class->func.is_site(E_GADCON_SITE_EFM_TOOLBAR))
     _e_gadcon_add_locations_menu_for_site(m, gcc, E_GADCON_SITE_EFM_TOOLBAR, &n);
   _e_gadcon_add_locations_menu_for_site(m, gcc, E_GADCON_SITE_UNKNOWN, &n);
}

EAPI void
e_gadcon_client_add_location_menu(E_Gadcon_Client *gcc, E_Menu *menu)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);

   if (gcc->gadcon->location)
     {
	mn = e_menu_new();
	mi = e_menu_item_new(menu);
	e_menu_item_label_set(mi, _("Move to"));
	e_util_menu_item_theme_icon_set(mi, "preferences-look");
	e_menu_item_submenu_set(mi, mn);
	e_menu_pre_activate_callback_set(mn, _e_gadcon_gadget_move_to_pre_cb, gcc);
     }
}

EAPI E_Menu *
e_gadcon_client_util_menu_items_append(E_Gadcon_Client *gcc, E_Menu *menu_gadget, int flags __UNUSED__)
{
   E_Menu *mo, *menu_main = NULL;
   E_Menu_Item *mi;
   char buf[256];

   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);

   if (e_config->menu_gadcon_client_toplevel)
     menu_main = menu_gadget;
   else
     menu_main = e_menu_new();
   
   if ((gcc->gadcon->shelf) || (gcc->gadcon->toolbar))
     {
        if (e_menu_item_nth(menu_gadget, 0))
          {
             mi = e_menu_item_new(menu_gadget);
             e_menu_item_separator_set(mi, 1);
          }
	if (!gcc->o_control)
	  {
             mi = e_menu_item_new(menu_gadget);
             e_menu_item_label_set(mi, _("Move"));
             e_util_menu_item_theme_icon_set(mi, "transform-scale");
             e_menu_item_callback_set(mi, _e_gadcon_client_cb_menu_edit, gcc);
	  }
/*
	mi = e_menu_item_new(menu_gadget);
	e_menu_item_label_set(mi, _("Resizeable"));
	e_util_menu_item_theme_icon_set(mi, "transform-scale");
	e_menu_item_check_set(mi, 1);
	if (gcc->resizable) e_menu_item_toggle_set(mi, 1);
	e_menu_item_callback_set(mi, _e_gadcon_client_cb_menu_resizable, gcc);
 */
	mi = e_menu_item_new(menu_gadget);
	e_menu_item_label_set(mi, _("Automatically scroll contents"));
	e_util_menu_item_theme_icon_set(mi, "transform-move");
	e_menu_item_check_set(mi, 1);
	if (gcc->autoscroll) e_menu_item_toggle_set(mi, 1);
	e_menu_item_callback_set(mi, _e_gadcon_client_cb_menu_autoscroll, gcc);

        if (gcc->gadcon->shelf) 
          {
             mo = e_menu_new();

             mi = e_menu_item_new(mo);
             e_menu_item_label_set(mi, _("Plain"));
             e_util_menu_item_theme_icon_set(mi, "enlightenment/plain");
             e_menu_item_radio_group_set(mi, 1);
             e_menu_item_radio_set(mi, 1);
             if ((gcc->style) && (!strcmp(gcc->style, E_GADCON_CLIENT_STYLE_PLAIN)))
               e_menu_item_toggle_set(mi, 1);
             e_menu_item_callback_set(mi, _e_gadcon_client_cb_menu_style_plain, gcc);

             mi = e_menu_item_new(mo);
             e_menu_item_label_set(mi, _("Inset"));
             e_util_menu_item_theme_icon_set(mi, "enlightenment/inset");
             e_menu_item_radio_group_set(mi, 1);
             e_menu_item_radio_set(mi, 1);
             if ((gcc->style) && (!strcmp(gcc->style, E_GADCON_CLIENT_STYLE_INSET)))
               e_menu_item_toggle_set(mi, 1);
             e_menu_item_callback_set(mi, _e_gadcon_client_cb_menu_style_inset, gcc);

             mi = e_menu_item_new(menu_gadget);
             e_menu_item_label_set(mi, _("Look"));
             e_util_menu_item_theme_icon_set(mi, "preferences-look");
             e_menu_item_submenu_set(mi, mo);
          }

	mi = e_menu_item_new(menu_gadget);
	e_menu_item_separator_set(mi, 1);

	e_gadcon_client_add_location_menu(gcc, menu_gadget);

	mi = e_menu_item_new(menu_gadget);
	e_menu_item_label_set(mi, _("Remove"));
	e_util_menu_item_theme_icon_set(mi, "list-remove");
	e_menu_item_callback_set(mi, _e_gadcon_client_cb_menu_remove, gcc);
     }

   if (!e_config->menu_gadcon_client_toplevel)
     {
	mi = e_menu_item_new(menu_main);
	if (gcc->client_class->func.label)
	  snprintf(buf, sizeof(buf), "%s", 
		   gcc->client_class->func.label((E_Gadcon_Client_Class *)gcc->client_class));
	else
	  snprintf(buf, sizeof(buf), "%s", gcc->name);

	e_menu_item_label_set(mi, _(buf));
	e_menu_item_realize_callback_set(mi, _e_gadcon_client_cb_menu_pre, gcc);
	e_menu_item_submenu_set(mi, menu_gadget);
     }
   
   if (gcc->gadcon->menu_attach.func)
     {
        if ((gcc->gadcon->shelf) || (gcc->gadcon->toolbar))
	  {
	     if (e_config->menu_gadcon_client_toplevel)
	       {
	          mi = e_menu_item_new(menu_main);
	          e_menu_item_separator_set(mi, 1);	
	       }	     
	     gcc->gadcon->menu_attach.func(gcc->gadcon->menu_attach.data, gcc, menu_main);
	  }	
        else
          gcc->gadcon->menu_attach.func(gcc->gadcon->menu_attach.data, gcc, menu_gadget);
     }
   
   return menu_main;
}
    
EAPI void
e_gadcon_client_util_menu_attach(E_Gadcon_Client *gcc)
{
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);
   if (gcc->o_frame)
     {
	evas_object_event_callback_add(gcc->o_frame, EVAS_CALLBACK_MOUSE_DOWN, 
                                       _e_gadcon_client_cb_mouse_down, gcc);
	evas_object_event_callback_add(gcc->o_frame, EVAS_CALLBACK_MOUSE_UP, 
                                       _e_gadcon_client_cb_mouse_up, gcc);
	evas_object_event_callback_add(gcc->o_frame, EVAS_CALLBACK_MOUSE_MOVE, 
                                       _e_gadcon_client_cb_mouse_move, gcc);
     }
   else if (gcc->o_base)
     {
	evas_object_event_callback_add(gcc->o_base, EVAS_CALLBACK_MOUSE_DOWN, 
                                       _e_gadcon_client_cb_mouse_down, gcc);
	evas_object_event_callback_add(gcc->o_base, EVAS_CALLBACK_MOUSE_UP, 
                                       _e_gadcon_client_cb_mouse_up, gcc);
	evas_object_event_callback_add(gcc->o_base, EVAS_CALLBACK_MOUSE_MOVE, 
                                       _e_gadcon_client_cb_mouse_move, gcc);
     }
}

EAPI void
e_gadcon_locked_set(E_Gadcon *gc, int lock)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   if (gc->locked_set.func)
     gc->locked_set.func(gc->locked_set.data, lock);
}

EAPI void
e_gadcon_urgent_show(E_Gadcon *gc)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   if (gc->urgent_show.func)
     gc->urgent_show.func(gc->urgent_show.data);
}

/*
 * NOTE: x & y are relative to the o_box of the gadcon.
 */
EAPI void
e_gadcon_client_autoscroll_update(E_Gadcon_Client *gcc, Evas_Coord x, Evas_Coord y)
{
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);

   if (gcc->autoscroll)
     {
	Evas_Coord w, h;
	double d;

        /* TODO: When using gadman there is no o_box! */
	evas_object_geometry_get(gcc->o_box, NULL, NULL, &w, &h);
        if (e_box_orientation_get(gcc->o_box))
	  {
 	     if (w > 1) d = (double)x / (double)(w - 1);
	     else d = 0;
	  }
	else
	  {
 	     if (h > 1) d = (double)y / (double)(h - 1);
	     else d = 0;
	  }
	if (d < 0.0) d = 0.0;
	else if (d > 1.0) d = 1.0;
	if (!gcc->scroll_timer)
	  gcc->scroll_timer = 
          ecore_timer_add(0.01, _e_gadcon_cb_client_scroll_timer, gcc);
	if (!gcc->scroll_animator)
	  gcc->scroll_animator = 
          ecore_animator_add(_e_gadcon_cb_client_scroll_animator, gcc);
	gcc->scroll_wanted = d;
     }
}

EAPI void
e_gadcon_client_autoscroll_cb_set(E_Gadcon_Client *gcc, void (*func)(void *data), void *data)
{
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);
   gcc->scroll_cb.func = func;
   gcc->scroll_cb.data = data;
}

EAPI Eina_Bool
e_gadcon_site_is_shelf(E_Gadcon_Site site)
{
   return (site == E_GADCON_SITE_SHELF);
}

EAPI Eina_Bool
e_gadcon_site_is_desktop(E_Gadcon_Site site)
{
   return (site == E_GADCON_SITE_DESKTOP);
}

EAPI Eina_Bool
e_gadcon_site_is_efm_toolbar(E_Gadcon_Site site)
{
   return (site == E_GADCON_SITE_EFM_TOOLBAR);
}

EAPI Eina_Bool
e_gadcon_site_is_any_toolbar(E_Gadcon_Site site)
{
   switch (site)
     {
        // there should be all toolbar sities identifiers
      case E_GADCON_SITE_TOOLBAR:
      case E_GADCON_SITE_EFM_TOOLBAR:
        return EINA_TRUE;
      default:
	 return EINA_FALSE;
     }
   return EINA_FALSE;
}

EAPI Eina_Bool
e_gadcon_site_is_not_toolbar(E_Gadcon_Site site)
{
   switch (site)
     {
        // there should be all toolbar sities identifiers
      case E_GADCON_SITE_TOOLBAR:
      case E_GADCON_SITE_EFM_TOOLBAR:
        return EINA_FALSE;
      default:
	 return EINA_TRUE;
     }
   return EINA_TRUE;
}

/* local subsystem functions */
static void
_e_gadcon_free(E_Gadcon *gc)
{
   e_gadcon_unpopulate(gc);
   gadcons = eina_list_remove(gadcons, gc);
   if (gc->o_container) evas_object_del(gc->o_container);
   eina_stringshare_del(gc->name);
   eina_stringshare_del(gc->edje.swallow_name);
   if (gc->config_dialog) e_object_del(E_OBJECT(gc->config_dialog));
   if (gc->drop_handler) e_drop_handler_del(gc->drop_handler);
   free(gc);
}

static void
_e_gadcon_client_free(E_Gadcon_Client *gcc)
{
   if (gcc->instant_edit_timer)
     {
	ecore_timer_del(gcc->instant_edit_timer);
	gcc->instant_edit_timer = NULL;
     }
   if (gcc->o_base)
     evas_object_event_callback_del(gcc->o_base, EVAS_CALLBACK_DEL,
				    _e_gadcon_client_del_hook);
   if (gcc->menu)
     {
        e_menu_post_deactivate_callback_set(gcc->menu, NULL, NULL);
	e_object_del(E_OBJECT(gcc->menu));
	gcc->menu = NULL;
     }
   e_gadcon_client_edit_end(gcc);
   gcc->client_class->func.shutdown(gcc);
   if ((gcc->client_class->func.id_del) && (gcc->cf))
     gcc->client_class->func.id_del((E_Gadcon_Client_Class *)gcc->client_class,
                                    gcc->cf->id);
   gcc->gadcon->clients = eina_list_remove(gcc->gadcon->clients, gcc);
   if (gcc->o_box) evas_object_del(gcc->o_box);
   if (gcc->o_frame) evas_object_del(gcc->o_frame);
   eina_stringshare_del(gcc->name);
   if (gcc->scroll_timer) ecore_timer_del(gcc->scroll_timer);
   if (gcc->scroll_animator) ecore_animator_del(gcc->scroll_animator);
   if (gcc->style) eina_stringshare_del(gcc->style);
   free(gcc);
}

static void
_e_gadcon_moveresize_handle(E_Gadcon_Client *gcc)
{
   Evas_Coord w, h;

   evas_object_geometry_get(gcc->o_box, NULL, NULL, &w, &h);
/*   
   if (gcc->resizable)
     {
	if (e_box_orientation_get(gcc->o_box))
	  {
	     if ((gcc->aspect.w > 0) && (gcc->aspect.h > 0))
	       w = (h * gcc->aspect.w) / gcc->aspect.h;
	  }
	else
	  {
	     if ((gcc->aspect.w > 0) && (gcc->aspect.h > 0))
	       h = (w * gcc->aspect.h) / gcc->aspect.w;
	  }
     }
 */
   if (gcc->autoscroll)
     {
	if (e_box_orientation_get(gcc->o_box))
	  {
             w = gcc->min.w;
	  }
	else
	  {
             h = gcc->min.h;
	  }
     }
   if (gcc->o_base)
     e_box_pack_options_set(gcc->o_base,
                            1, 1, /* fill */
                            1, 1, /* expand */
                            0.5, 0.5, /* align */
                            w, h, /* min */
                            w, h /* max */
                           );
}

static Eina_Bool
_e_gadcon_cb_client_scroll_timer(void *data)
{
   E_Gadcon_Client *gcc;
   double d, v;

   gcc = data;
   d = gcc->scroll_wanted - gcc->scroll_pos;
   if (d < 0) d = -d;
   if (d < 0.001)
     {
	gcc->scroll_pos =  gcc->scroll_wanted;
	gcc->scroll_timer = NULL;
	return ECORE_CALLBACK_CANCEL;
     }
   v = 0.05;
   gcc->scroll_pos = (gcc->scroll_pos * (1.0 - v)) + (gcc->scroll_wanted * v);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_gadcon_cb_client_scroll_animator(void *data)
{
   E_Gadcon_Client *gcc;

   gcc = data;
   if (e_box_orientation_get(gcc->o_box))
     e_box_align_set(gcc->o_box, 1.0 - gcc->scroll_pos, 0.5);
   else
     e_box_align_set(gcc->o_box, 0.5, 1.0 - gcc->scroll_pos);
   if (!gcc->scroll_timer)
     {
	gcc->scroll_animator = NULL;
	return ECORE_CALLBACK_CANCEL;
     }

   if (gcc->scroll_cb.func)
     gcc->scroll_cb.func(gcc->scroll_cb.data);

   return ECORE_CALLBACK_RENEW;
}

static void
_e_gadcon_cb_client_frame_mouse_move(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   E_Gadcon_Client *gcc;
   Evas_Coord x, y;

   ev = event_info;
   gcc = data;
   evas_object_geometry_get(gcc->o_box, &x, &y, NULL, NULL);
   e_gadcon_client_autoscroll_update(gcc, ev->cur.output.x - x, 
                                     ev->cur.output.y - y);
}

static void
_e_gadcon_cb_client_frame_moveresize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Gadcon_Client *gcc;

   gcc = data;
   _e_gadcon_moveresize_handle(gcc);
}
    
static void
_e_gadcon_client_save(E_Gadcon_Client *gcc)
{
   gcc->cf->geom.pos = gcc->config.pos;
   gcc->cf->geom.size = gcc->config.size;
   gcc->cf->geom.res = gcc->config.res;
   gcc->cf->geom.pos_x = gcc->config.pos_x;
   gcc->cf->geom.pos_y = gcc->config.pos_y;
   gcc->cf->geom.size_w = gcc->config.size_w;
   gcc->cf->geom.size_h = gcc->config.size_h;
   gcc->cf->state_info.seq = gcc->state_info.seq;
   gcc->cf->state_info.flags = gcc->state_info.flags;
   gcc->cf->autoscroll = gcc->autoscroll;
   if (gcc->cf->style) eina_stringshare_del(gcc->cf->style);
   gcc->cf->style = NULL;
   if (gcc->style)
     gcc->cf->style = eina_stringshare_add(gcc->style);
/*   gcc->cf->resizable = gcc->resizable;*/
   gcc->cf->resizable = 0;
   e_config_save_queue();
}

static void
_e_gadcon_client_drag_begin(E_Gadcon_Client *gcc, int x, int y)
{
   E_Drag *drag;
   Evas_Object *o = NULL;
   Evas_Coord w = 0, h = 0;
   const char *drag_types[] = { "enlightenment/gadcon_client" };

   if ((drag_gcc) || (!gcc->gadcon->zone) || (!gcc->gadcon->zone->container))
     return;

   drag_gcc = gcc;

   e_object_ref(E_OBJECT(gcc));
   /* Remove this config from the current gadcon */
   gcc->gadcon->cf->clients = 
     eina_list_remove(gcc->gadcon->cf->clients, gcc->cf);
   gcc->state_info.state = E_LAYOUT_ITEM_STATE_NONE;
   gcc->state_info.resist = 0;

   if (!e_drop_inside(gcc->gadcon->drop_handler, x, y))
     e_gadcon_client_hide(gcc);

   ecore_x_pointer_xy_get(gcc->gadcon->zone->container->win, &x, &y);
   
   drag = e_drag_new(gcc->gadcon->zone->container, x, y,
		     drag_types, 1, gcc, -1, NULL, 
		     _e_gadcon_cb_drag_finished);
   if (drag) 
     {
	o = gcc->client_class->func.icon((E_Gadcon_Client_Class *)gcc->client_class, 
                                         e_drag_evas_get(drag));
	evas_object_geometry_get(o, NULL, NULL, &w, &h);
	if (!o)
	  {
	     /* FIXME: fallback icon for drag */
	     o = evas_object_rectangle_add(e_drag_evas_get(drag));
	     evas_object_color_set(o, 255, 255, 255, 100);
	  }
	if (w < 10)
          w = h = 50;
	e_drag_object_set(drag, o);
	e_drag_resize(drag, w, h);
	e_drag_start(drag, x + w/2, y + h/2);
     }
}

static void
_e_gadcon_client_inject(E_Gadcon *gc, E_Gadcon_Client *gcc, int x, int y)
{
   Eina_List *l;
   E_Gadcon_Client *gcc2;
   Evas_Coord cx, cy, cw, ch;
   int seq = 1;

   /* Check if the gadcon client is in place */
   if (!gcc->hidden)
     {
	if (gcc->o_frame)
	  evas_object_geometry_get(gcc->o_frame, &cx, &cy, &cw, &ch);
	else if (gcc->o_base)
	  evas_object_geometry_get(gcc->o_base, &cx, &cy, &cw, &ch);
	else return; /* make clang happy */

	if (E_INSIDE(x, y, cx, cy, cw, ch)) return;
     }

   /* If x, y is not inside any gadcon client, seq will be 0 and it's position
    * will later be used for placement. */
   gcc->state_info.seq = 0;
   EINA_LIST_FOREACH(gc->clients, l, gcc2)
     {
	if (gcc == gcc2) continue;
	if (gcc2->hidden) continue;
	if (gcc2->o_frame)
	  evas_object_geometry_get(gcc2->o_frame, &cx, &cy, &cw, &ch);
	else if (gcc2->o_base)
	  evas_object_geometry_get(gcc2->o_base, &cx, &cy, &cw, &ch);
	else return; /* make clang happy */
	if (e_gadcon_layout_orientation_get(gc->o_container))
	  {
	     if (E_INSIDE(x, y, cx, cy, cw / 2, ch))
	       {
		  gcc->state_info.seq = seq++;
		  gcc2->state_info.seq = seq++;
	       }
	     else if (E_INSIDE(x, y, cx + cw / 2, cy, cw / 2, ch))
	       {
		  gcc2->state_info.seq = seq++;
		  gcc->state_info.seq = seq++;
	       }
	     else
               gcc2->state_info.seq = seq++;
	  }
	else
	  {
	     if (E_INSIDE(x, y, cx, cy, cw, ch / 2))
	       {
		  gcc->state_info.seq = seq++;
		  gcc2->state_info.seq = seq++;
	       }
	     else if (E_INSIDE(x, y, cx, cy + ch / 2, cw, ch / 2))
	       {
		  gcc2->state_info.seq = seq++;
		  gcc->state_info.seq = seq++;
	       }
	     else
               gcc2->state_info.seq = seq++;
	  }
     }
}

static void
_e_gadcon_cb_min_size_request(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Gadcon *gc;

   gc = data;
   if (gc->min_size_request.func)
     {
	Evas_Coord w = 0, h = 0;

	e_gadcon_layout_min_size_get(gc->o_container, &w, &h);
	gc->min_size_request.func(gc->min_size_request.data, gc, w, h);
     }
}

static void
_e_gadcon_cb_size_request(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Gadcon *gc;

   gc = data;
   if (gc->resize_request.func)
     {
	Evas_Coord w = 0, h = 0;

	e_gadcon_layout_asked_size_get(gc->o_container, &w, &h);
	gc->resize_request.func(gc->resize_request.data, gc, w, h);
     }
}

static void
_e_gadcon_cb_moveresize(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Gadcon *gc;
   Evas_Coord x, y, w, h;

   gc = data;
   evas_object_geometry_get(gc->o_container, &x, &y, &w, &h);
   if (gc->drop_handler) 
     e_drop_handler_geometry_set(gc->drop_handler, x, y, w, h);
}

static void
_e_gadcon_cb_client_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Gadcon_Client *gcc;

   gcc = data;
   ev = event_info;
   if (ev->button == 3)
     {
	E_Zone *zone;
	E_Menu *mn;
	E_Menu_Item *mi;
	int cx, cy;

	zone = e_util_zone_current_get(e_manager_current_get());

	e_gadcon_locked_set(gcc->gadcon, 1);
	mn = e_menu_new();
	e_menu_post_deactivate_callback_set(mn, _e_gadcon_client_cb_menu_post,
					    gcc);
	gcc->menu = mn;

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Stop moving"));
	e_util_menu_item_theme_icon_set(mi, "enlightenment/edit");
	e_menu_item_callback_set(mi, _e_gadcon_client_cb_menu_edit, gcc);

	if (gcc->gadcon->menu_attach.func)
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_separator_set(mi, 1);

	     gcc->gadcon->menu_attach.func(gcc->gadcon->menu_attach.data, 
                                           gcc, mn);
	  }

	if (gcc->gadcon->toolbar)
	  ecore_x_pointer_xy_get(zone->container->win, &cx, &cy);
	else 
	  {
	     e_gadcon_canvas_zone_geometry_get(gcc->gadcon, &cx, &cy, NULL, NULL);
	     cx = cx + ev->output.x;
	     cy = cy + ev->output.y;
	  }
	e_menu_activate_mouse(mn, zone, cx, cy, 1, 1, 
			      E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
     }
}

static void
_e_gadcon_cb_client_mouse_in(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Gadcon_Client *gcc;

   gcc = data;
   edje_object_signal_emit(gcc->o_control, "e,state,focused", "e");
}

static void
_e_gadcon_cb_client_mouse_out(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Gadcon_Client *gcc;

   gcc = data;
   edje_object_signal_emit(gcc->o_control, "e,state,unfocused", "e");
}

static void
_e_gadcon_cb_client_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   E_Gadcon_Client *gcc;
   Evas_Coord x, y;

   gcc = data;
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   if (gcc->o_control) evas_object_move(gcc->o_control, x, y);
   if (gcc->o_event) evas_object_move(gcc->o_event, x, y);
}

static void
_e_gadcon_cb_client_resize(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   E_Gadcon_Client *gcc;
   Evas_Coord w, h;

   gcc = data;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   if (gcc->o_control) evas_object_resize(gcc->o_control, w, h);
   if (gcc->o_event) evas_object_resize(gcc->o_event, w, h);
}

static void
_e_gadcon_client_move_start(E_Gadcon_Client *gcc)
{
   int x, y, gcx, gcy, gy ,gx;
   
   evas_object_raise(gcc->o_event);
   evas_object_stack_below(gcc->o_control, gcc->o_event);
   gcc->moving = 1;
   if (gcc->gadcon->toolbar)
     evas_pointer_canvas_xy_get(gcc->gadcon->evas, &gcc->dx, &gcc->dy);
   else
     {
	ecore_x_pointer_xy_get(gcc->gadcon->zone->container->win, &gcc->dx, &gcc->dy);
	e_gadcon_canvas_zone_geometry_get(gcc->gadcon, &gcx, &gcy, NULL, NULL);
	evas_object_geometry_get(gcc->gadcon->o_container, &gx, &gy, NULL, NULL);
	gcc->dx -= (gcx + gx);
	gcc->dy -= (gcy + gy);
     }
   
   if (gcc->o_frame)
     evas_object_geometry_get(gcc->o_frame, &x, &y, NULL, NULL);
   else if (gcc->o_base)
     evas_object_geometry_get(gcc->o_base, &x, &y, NULL, NULL);
   else
     return;

   /* using drag pos to calc offset between pointer and gcc pos */
   gcc->drag.x = (x - gcc->dx);
   gcc->drag.y = (y - gcc->dy);

   gcc->state_info.resist = 0;
}

static void
_e_gadcon_client_move_stop(E_Gadcon_Client *gcc)
{
   gcc->moving = 0;
   gcc->state_info.state = E_LAYOUT_ITEM_STATE_NONE;
   gcc->state_info.resist = 0;
   _e_gadcon_layout_smart_sync_clients(gcc->gadcon);
}

static void
_e_gadcon_client_move_go(E_Gadcon_Client *gcc)
{
   Evas_Coord x, y, w, h;
   int cx, cy;
   int gx, gy, gw, gh;
   int gcx = 0, gcy = 0;
   int changes = 0;
   
   if (!gcc->moving) return;
   /* we need to get output not canvas because things like systray
      can reparent another window so we get no position here */
   /* maybe we should better grab mouse while move resize is active...*/
   //evas_pointer_canvas_xy_get(gcc->gadcon->evas, &cx, &cy);
   if (gcc->gadcon->toolbar)
     evas_pointer_canvas_xy_get(gcc->gadcon->evas, &cx, &cy);
   else
     {
	ecore_x_pointer_xy_get(gcc->gadcon->zone->container->win, &cx, &cy);
	e_gadcon_canvas_zone_geometry_get(gcc->gadcon, &gcx, &gcy, NULL, NULL);
     }
   
   evas_object_geometry_get(gcc->gadcon->o_container, &gx, &gy, &gw, &gh);

   cx -= (gx + gcx);
   cy -= (gy + gcy);
   
   x = cx - gcc->dx;
   y = cy - gcc->dy; 
   
   gcc->state_info.flags = E_GADCON_LAYOUT_ITEM_LOCK_POSITION | E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;
   _e_gadcon_client_current_position_sync(gcc);

   if (gcc->o_frame)
     evas_object_geometry_get(gcc->o_frame, NULL, NULL, &w, &h);
   else if (gcc->o_base)
     evas_object_geometry_get(gcc->o_base, NULL, NULL, &w, &h);
   else
     return; /* make clang happy */

   if (e_gadcon_layout_orientation_get(gcc->gadcon->o_container))
     { 
	if (cy + e_config->drag_resist < 0 || cy - e_config->drag_resist > gh)
	  {
	     _e_gadcon_client_drag_begin(gcc, cx, cy);
	     return;
	  }

	if (x > 0 && (cx + gcc->drag.x > gcc->config.pos))
	  {
	     if (gcc->state_info.state != E_LAYOUT_ITEM_STATE_POS_INC)
	       gcc->state_info.resist = 0;
	     gcc->state_info.state = E_LAYOUT_ITEM_STATE_POS_INC;
	     changes = 1;
	  }
	else if (x < 0 && (cx + gcc->drag.x < gcc->config.pos))
	  {
	     if (gcc->state_info.state != E_LAYOUT_ITEM_STATE_POS_DEC)
	       gcc->state_info.resist = 0; 
	     gcc->state_info.state = E_LAYOUT_ITEM_STATE_POS_DEC;
	     changes = 1;
	  }

	if (changes)
	  {
	     if (gcc->o_frame)
	       e_gadcon_layout_pack_request_set(gcc->o_frame, cx + gcc->drag.x, w);
	     else if (gcc->o_base)
	       e_gadcon_layout_pack_request_set(gcc->o_base, cx + gcc->drag.x, w);

	     gcc->config.size = w;
	     evas_object_geometry_get(gcc->gadcon->o_container, NULL, NULL, &w, &h);
	     gcc->config.res = w;
	  }
     }
   else
     {
	if (cx + e_config->drag_resist < 0 || cx - e_config->drag_resist > gw)
	  {
	     _e_gadcon_client_drag_begin(gcc, cx, cy);
	     return;
	  }

	if (y > 0 && (cy + gcc->drag.y > gcc->config.pos))
	  {
	     if (gcc->state_info.state != E_LAYOUT_ITEM_STATE_POS_INC)
	       gcc->state_info.resist = 0;
	     gcc->state_info.state = E_LAYOUT_ITEM_STATE_POS_INC;
	     changes = 1;
	  }
	else if (y < 0 && (cy + gcc->drag.y < gcc->config.pos))
	  {
	     if (gcc->state_info.state != E_LAYOUT_ITEM_STATE_POS_DEC)
	       gcc->state_info.resist = 0;
	     gcc->state_info.state = E_LAYOUT_ITEM_STATE_POS_DEC;
	     changes = 1;
	  }

	if (changes)
	  {
	     if (gcc->o_frame)
	       e_gadcon_layout_pack_request_set(gcc->o_frame, cy + gcc->drag.y, h);
	     else if (gcc->o_base)
	       e_gadcon_layout_pack_request_set(gcc->o_base, cy + gcc->drag.y, h);

	     gcc->config.size = h;
	     evas_object_geometry_get(gcc->gadcon->o_container, NULL, NULL, &w, &h);
	     gcc->config.res = h;
	  }

     }
   
   gcc->dx += x;
   gcc->dy += y;
}

static void
_e_gadcon_cb_signal_move_start(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _e_gadcon_client_move_start(data);
}

static void
_e_gadcon_cb_signal_move_stop(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _e_gadcon_client_move_stop(data);
}

static void
_e_gadcon_cb_signal_move_go(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _e_gadcon_client_move_go(data);
}

static void
_e_gadcon_client_resize_start(E_Gadcon_Client *gcc)
{
   evas_object_raise(gcc->o_event);
   evas_object_stack_below(gcc->o_control, gcc->o_event);
   gcc->resizing = 1;
   evas_pointer_canvas_xy_get(gcc->gadcon->evas, &gcc->dx, &gcc->dy);
}

static void
_e_gadconclient_resize_stop(E_Gadcon_Client *gcc)
{
   gcc->resizing = 0;
   gcc->state_info.state = E_LAYOUT_ITEM_STATE_NONE;
   _e_gadcon_layout_smart_sync_clients(gcc->gadcon);
   _e_gadcon_client_save(gcc);
}

static void
_e_gadcon_cb_signal_resize_left_start(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _e_gadcon_client_resize_start(data);
}

static void
_e_gadcon_cb_signal_resize_left_stop(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _e_gadconclient_resize_stop(data);
}

static void
_e_gadcon_cb_signal_resize_left_go(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Gadcon_Client *gcc;
   Evas_Coord x, y, w, h;

   gcc = data;
   if (!gcc->resizing) return;
   evas_pointer_canvas_xy_get(gcc->gadcon->evas, &x, &y);
   x = x - gcc->dx;
   y = y - gcc->dy;

   gcc->state_info.flags = E_GADCON_LAYOUT_ITEM_LOCK_POSITION | 
     E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;

   if (gcc->o_frame)
     evas_object_geometry_get(gcc->o_frame, NULL, NULL, &w, &h);
   else if (gcc->o_base)
     evas_object_geometry_get(gcc->o_base, NULL, NULL, &w, &h);
   else return; /* make clang happy */

   _e_gadcon_client_current_position_sync(gcc);

   if (e_gadcon_layout_orientation_get(gcc->gadcon->o_container))
     {
	if (x > 0)
	  gcc->state_info.state = E_LAYOUT_ITEM_STATE_SIZE_MIN_END_INC;
	else if (x < 0)
	  gcc->state_info.state = E_LAYOUT_ITEM_STATE_SIZE_MIN_END_DEC;
     }
   else
     {
	if (y > 0)
	  gcc->state_info.state = E_LAYOUT_ITEM_STATE_SIZE_MIN_END_INC;
	else if (y < 0)
	  gcc->state_info.state = E_LAYOUT_ITEM_STATE_SIZE_MIN_END_DEC;
     }

   if (e_gadcon_layout_orientation_get(gcc->gadcon->o_container))
     {
	if (gcc->o_frame) 
	  e_gadcon_layout_pack_request_set(gcc->o_frame, gcc->config.pos + x, w - x);
	else if (gcc->o_base)
	  e_gadcon_layout_pack_request_set(gcc->o_base, gcc->config.pos + x, w - x);
	evas_object_geometry_get(gcc->gadcon->o_container, NULL, NULL, &w, &h);
	gcc->config.res = w;
     }
   else
     {
	if (gcc->o_frame)
	  e_gadcon_layout_pack_request_set(gcc->o_frame, gcc->config.pos + y, h - y);
	else if (gcc->o_base)
	  e_gadcon_layout_pack_request_set(gcc->o_base, gcc->config.pos + y, h - y);
	evas_object_geometry_get(gcc->gadcon->o_container, NULL, NULL, &w, &h);
	gcc->config.res = h;
     }
   gcc->dx += x;
   gcc->dy += y;
}

static void
_e_gadcon_cb_signal_resize_right_start(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _e_gadcon_client_resize_start(data);
}

static void
_e_gadcon_cb_signal_resize_right_stop(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _e_gadconclient_resize_stop(data);
}

static void
_e_gadcon_cb_signal_resize_right_go(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Gadcon_Client *gcc;
   Evas_Coord x, y, w, h;

   gcc = data;
   if (!gcc->resizing) return;
   evas_pointer_canvas_xy_get(gcc->gadcon->evas, &x, &y);
   x = x - gcc->dx;
   y = y - gcc->dy;

   gcc->state_info.flags = E_GADCON_LAYOUT_ITEM_LOCK_POSITION | 
     E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;

   if (gcc->o_frame)
     evas_object_geometry_get(gcc->o_frame, NULL, NULL, &w, &h);
   else if (gcc->o_base)
     evas_object_geometry_get(gcc->o_base, NULL, NULL, &w, &h);
   else return; /* make clang happy */

   _e_gadcon_client_current_position_sync(gcc);

   if (e_gadcon_layout_orientation_get(gcc->gadcon->o_container))
     {
	if (x > 0)
	  gcc->state_info.state = E_LAYOUT_ITEM_STATE_SIZE_MAX_END_INC;
	else if (x < 0)
	  gcc->state_info.state = E_LAYOUT_ITEM_STATE_SIZE_MAX_END_DEC;
     }
   else
     {
	if (y > 0)
	  gcc->state_info.state = E_LAYOUT_ITEM_STATE_SIZE_MAX_END_INC;
	else if (y < 0)
	  gcc->state_info.state = E_LAYOUT_ITEM_STATE_SIZE_MAX_END_INC;
     }

   if (e_gadcon_layout_orientation_get(gcc->gadcon->o_container))
     {
	if (gcc->o_frame)
	  e_gadcon_layout_pack_request_set(gcc->o_frame, gcc->config.pos, w + x);
	else if (gcc->o_base)
	  e_gadcon_layout_pack_request_set(gcc->o_base, gcc->config.pos, w + x);
	evas_object_geometry_get(gcc->gadcon->o_container, NULL, NULL, &w, &h);
	gcc->config.res = w;
     }
   else
     {
	if (gcc->o_frame)
	  e_gadcon_layout_pack_request_set(gcc->o_frame, gcc->config.pos, h + y);
	else if (gcc->o_base)
	  e_gadcon_layout_pack_request_set(gcc->o_base, gcc->config.pos, h + y);
	evas_object_geometry_get(gcc->gadcon->o_container, NULL, NULL, &w, &h);
	gcc->config.res = h;
     }
   gcc->dx += x;
   gcc->dy += y;
}

static void
_e_gadcon_cb_drag_finished(E_Drag *drag, int dropped)
{
   E_Gadcon_Client *gcc;

   gcc = drag->data;
   if (!dropped)
     {
	/* free client config */
	e_gadcon_client_config_del(NULL, gcc->cf);
	/* delete the gadcon client */
	/* TODO: Clean up module config too? */
	e_object_del(E_OBJECT(gcc));
     }
   else if (new_gcc)
     {
	/* dropped on new gadcon, delete this one as it is no longer in use */
	e_object_del(E_OBJECT(gcc));
     }
   e_object_unref(E_OBJECT(gcc));
   new_gcc = NULL;
   drag_gcc = NULL;
}

static void
_e_gadcon_cb_dnd_enter(void *data, const char *type __UNUSED__, void *event)
{
   E_Event_Dnd_Enter *ev;
   E_Gadcon *gc;
   E_Gadcon_Client *gcc;

   ev = event;
   gc = data;
   e_gadcon_layout_freeze(gc->o_container);
   gcc = drag_gcc;

   if (gcc->gadcon == gc)
     {
	/* We have re-entered the gadcon we left, revive gadcon client */
	Evas_Coord dx, dy;
	Evas_Object *o;

	if (e_gadcon_layout_orientation_get(gc->o_container))
	  gcc->config.pos = ev->x - gcc->config.size / 2;
	else
	  gcc->config.pos = ev->y - gcc->config.size / 2;
	gcc->state_info.prev_pos = gcc->config.pos;

	evas_object_geometry_get(gc->o_container, &dx, &dy, NULL, NULL);
	_e_gadcon_client_inject(gc, gcc, ev->x + dx, ev->y + dy);
	e_gadcon_client_show(gcc);

	o = gcc->o_frame ? gcc->o_frame : gcc->o_base;
	if (o)
	  {
	     if (e_gadcon_layout_orientation_get(gc->o_container))
	       e_gadcon_layout_pack_request_set(o, gcc->config.pos, gcc->config.size);
	     else
	       e_gadcon_layout_pack_request_set(o, gcc->config.pos, gcc->config.size);
	  }
	gcc->state_info.resist = 1;
     }
   else if (ev->data)
     {
	/* Create a new gadcon to show where the gadcon will end up */
	E_Gadcon_Client_Class *cc;

	gcc = ev->data;
	cc = eina_hash_find(providers, gcc->name);
	if (cc)
	  {
	     if (!gcc->style)
	       {
		  new_gcc = cc->func.init(gc, gcc->name, gcc->cf->id,
                                          cc->default_style);
	       }
	     else
	       new_gcc = cc->func.init(gc, gcc->name, gcc->cf->id,
                                       gcc->style);

	     if (new_gcc)
	       {
		  new_gcc->cf = gcc->cf;
		  new_gcc->client_class = cc;
		  new_gcc->config.pos = gcc->config.pos;
		  new_gcc->config.size = gcc->config.size;
		  new_gcc->config.res = gcc->config.res;
		  new_gcc->state_info.seq = gcc->state_info.seq;
		  new_gcc->state_info.flags = gcc->state_info.flags;
		  if (new_gcc->o_frame)
		    e_gadcon_layout_pack_options_set(new_gcc->o_frame, new_gcc);
		  else if (new_gcc->o_base)
		    e_gadcon_layout_pack_options_set(new_gcc->o_base, new_gcc);

		  e_gadcon_client_autoscroll_set(new_gcc, gcc->autoscroll);
/*		  e_gadcon_client_resizable_set(new_gcc, gcc->resizable);*/
		  if (new_gcc->client_class->func.orient)
		    new_gcc->client_class->func.orient(new_gcc, gc->orient);
		  new_gcc->state_info.resist = 1;
		  if (gc->instant_edit)
		    e_gadcon_client_util_menu_attach(new_gcc);
	       }
	  }
     }
   else
     {
	/* TODO: Create a placeholder to show where the gadcon will end up */
     }
   e_gadcon_layout_thaw(gc->o_container);
}

static void
_e_gadcon_cb_dnd_move(void *data, const char *type __UNUSED__, void *event)
{
   E_Event_Dnd_Move *ev;
   E_Gadcon *gc;
   E_Gadcon_Client *gcc = NULL;

   ev = event;
   gc = data;

   /* If we move in the same gadcon as the client originates */
   if (drag_gcc->gadcon == gc) gcc = drag_gcc;
   /* If we move in the newly entered gadcon */
   else if (new_gcc->gadcon == gc) gcc = new_gcc;
   if (gcc)
     {
	Evas_Coord dx, dy;
	Evas_Object *o;

	if (gcc->state_info.resist > 0)
	  {
	     gcc->state_info.resist--;
	     return;
	  }
	e_gadcon_layout_freeze(gc->o_container);

	if (e_gadcon_layout_orientation_get(gc->o_container))
	  gcc->config.pos = ev->x - gcc->config.size / 2;
	else
	  gcc->config.pos = ev->y - gcc->config.size / 2;
	gcc->state_info.prev_pos = gcc->config.pos;

	evas_object_geometry_get(gc->o_container, &dx, &dy, NULL, NULL);
	_e_gadcon_client_inject(gc, gcc, ev->x + dx, ev->y + dy);

	o = gcc->o_frame ? gcc->o_frame : gcc->o_base;
	if (o)
	  {
	     if (e_gadcon_layout_orientation_get(gc->o_container))
	       e_gadcon_layout_pack_request_set(o, gcc->config.pos, 
                                                gcc->config.size);
	     else
	       e_gadcon_layout_pack_request_set(o, gcc->config.pos, 
                                                gcc->config.size);
	  }
	e_gadcon_layout_thaw(gc->o_container);
     }
}

static void
_e_gadcon_cb_dnd_leave(void *data, const char *type __UNUSED__, void *event __UNUSED__)
{
   E_Gadcon *gc;

   gc = data;
   /* If we exit the starting container hide the gadcon visual */
   if (drag_gcc->gadcon == gc) e_gadcon_client_hide(drag_gcc);

   /* Delete temporary object */
   if (new_gcc)
     {
       	e_object_del(E_OBJECT(new_gcc));
	new_gcc = NULL;
     }
}

static void
_e_gadcon_cb_drop(void *data, const char *type __UNUSED__, void *event __UNUSED__)
{
   E_Gadcon *gc;
   E_Gadcon_Client *gcc = NULL;

   gc = data;
   if (drag_gcc->gadcon == gc) gcc = drag_gcc;
   else if ((new_gcc) && (new_gcc->gadcon == gc)) gcc = new_gcc;
   else return;  /* make clang happy */

   gc->cf->clients = eina_list_append(gc->cf->clients, gcc->cf);

   if (gc->editing) e_gadcon_client_edit_begin(gcc);
   e_config_save_queue();
}

static int
_e_gadcon_client_class_feature_check(const E_Gadcon_Client_Class *cc, const char *name, void *feature)
{
   if (!feature)
     {
	e_util_dialog_show("Insufficent gadcon support",
	                   "Module %s needs to support %s",
			   cc->name, name);
	return 0;
     }
   return 1;
}

static void 
_e_gadcon_client_cb_menu_post(void *data, E_Menu *m __UNUSED__)
{
   E_Gadcon_Client *gcc;

   if (!(gcc = data)) return;
   if (gcc->gadcon) e_gadcon_locked_set(gcc->gadcon, 0);
   if (!gcc->menu) return;
   e_object_del(E_OBJECT(gcc->menu));
   gcc->menu = NULL;
}

static Eina_Bool
_e_gadcon_client_cb_instant_edit_timer(void *data)
{
   E_Gadcon_Client *gcc;

   gcc = data;
   e_gadcon_client_edit_begin(gcc);
   _e_gadcon_client_move_start(gcc);
   gcc->instant_edit_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_gadcon_client_cb_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Gadcon_Client *gcc;

   ev = event_info;
   gcc = data;
   if (gcc->menu) return;
   if (ev->button == 3)
     {
	E_Menu *m;
        E_Zone *zone;
	int cx, cy, cw, ch;

	e_gadcon_locked_set(gcc->gadcon, 1);
	m = e_menu_new();

	m = e_gadcon_client_util_menu_items_append(gcc, m, 0);
	e_menu_post_deactivate_callback_set(m, _e_gadcon_client_cb_menu_post,
					    gcc);
	gcc->menu = m;

	e_gadcon_canvas_zone_geometry_get(gcc->gadcon, &cx, &cy, &cw, &ch);
        zone = gcc->gadcon->zone;
        if (!zone) zone = e_util_zone_current_get(e_manager_current_get());
	e_menu_activate_mouse(m, zone, 
			      cx + ev->output.x, 
                              cy + ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
     }
   else if (ev->button == 1)
     {
	if ((!gcc->o_control) && (gcc->gadcon->instant_edit))
	  {
	     if (gcc->instant_edit_timer) 
               ecore_timer_del(gcc->instant_edit_timer);
	     gcc->instant_edit_timer = 
	       ecore_timer_add(1.0, _e_gadcon_client_cb_instant_edit_timer, 
			       gcc);
	  }
     }
}

static void
_e_gadcon_client_cb_mouse_up(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   E_Gadcon_Client *gcc;

   ev = event_info;
   gcc = data;
   if ((ev->button == 1) && (gcc->gadcon->instant_edit))
     {
	if (gcc->instant_edit_timer)
	  {
	     ecore_timer_del(gcc->instant_edit_timer);
	     gcc->instant_edit_timer = NULL;
	  }
	if (gcc->o_control)
	  {
	     _e_gadcon_client_move_stop(gcc);
	     e_gadcon_client_edit_end(gcc);
	  }
     }
}

static void
_e_gadcon_client_cb_mouse_move(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Gadcon_Client *gcc;

   gcc = data;
   if ((gcc->gadcon->instant_edit))
     {
	if (gcc->o_control) _e_gadcon_client_move_go(gcc);
     }
}

static void
_e_gadcon_client_cb_menu_style_plain(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Gadcon_Client *gcc;
   E_Gadcon *gc;

   gcc = data;
   gc = gcc->gadcon;
   if (gcc->style) eina_stringshare_del(gcc->style);
   gcc->style = eina_stringshare_add(E_GADCON_CLIENT_STYLE_PLAIN);
   _e_gadcon_client_save(gcc);
   e_gadcon_unpopulate(gc);
   e_gadcon_populate(gc);
}

static void
_e_gadcon_client_cb_menu_style_inset(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Gadcon_Client *gcc;
   E_Gadcon *gc;

   gcc = data;
   gc = gcc->gadcon;
   if (gcc->style) eina_stringshare_del(gcc->style);
   gcc->style = eina_stringshare_add(E_GADCON_CLIENT_STYLE_INSET);
   _e_gadcon_client_save(gcc);
   e_gadcon_unpopulate(gc);
   e_gadcon_populate(gc);
}

static void
_e_gadcon_client_cb_menu_autoscroll(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Gadcon_Client *gcc;

   gcc = data;
   e_gadcon_layout_freeze(gcc->gadcon->o_container);
   if (gcc->autoscroll) gcc->autoscroll = 0;
   else gcc->autoscroll = 1; 
   e_gadcon_client_autoscroll_set(gcc, gcc->autoscroll);
   _e_gadcon_client_save(gcc);
   e_gadcon_layout_thaw(gcc->gadcon->o_container);
}
/*
static void
_e_gadcon_client_cb_menu_resizable(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Gadcon_Client *gcc;

   gcc = data;
   e_gadcon_layout_freeze(gcc->gadcon->o_container);
   if (gcc->resizable) gcc->resizable = 0;
   else gcc->resizable = 1;
   e_gadcon_client_resizable_set(gcc, gcc->resizable);
   _e_gadcon_client_save(gcc);
   e_gadcon_layout_thaw(gcc->gadcon->o_container);
}
*/
static void
_e_gadcon_client_cb_menu_edit(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Gadcon_Client *gcc;

   gcc = data;
   if (gcc->o_control)
     e_gadcon_client_edit_end(gcc);
   else
     e_gadcon_client_edit_begin(gcc);
}

static void
_e_gadcon_client_cb_menu_remove(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Gadcon *gc;
   E_Gadcon_Client *gcc;

   gcc = data;
   gc = gcc->gadcon;

   e_gadcon_client_config_del(gc->cf, gcc->cf);
   e_gadcon_unpopulate(gc);
   e_gadcon_populate(gc);
   e_config_save_queue();
}

static void 
_e_gadcon_client_cb_menu_pre(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi) 
{
   E_Gadcon_Client *gcc;

   if (!(gcc = data)) return;
   if (gcc->client_class->func.icon) 
     {
        Evas_Object *ic;
        
        // e menu ASSUMES... EXPECTS the icon to be an.... e_icon! make it so.
        ic = gcc->client_class->func.icon
           ((E_Gadcon_Client_Class *)gcc->client_class, 
               mi->menu->evas);
        mi->icon_object = e_icon_add(mi->menu->evas);
        e_icon_object_set(mi->icon_object, ic);
     }
   else
      e_util_menu_item_theme_icon_set(mi, "preferences-gadget"); // FIXME: Needs icon in theme
}

static void
_e_gadcon_client_del_hook(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Gadcon_Client *gcc;

   gcc = data;
   gcc->o_base = NULL;
   if (gcc->o_frame)
     {
	evas_object_del(gcc->o_frame);
	gcc->o_frame = NULL;
     }
   e_object_del(E_OBJECT(gcc));
}

/* a smart object JUST for gadcon */

typedef struct _E_Gadcon_Layout_Item  E_Gadcon_Layout_Item;

struct _E_Smart_Data
{ 
   Evas_Coord x, y, w, h;
   Evas_Object *obj, *clip;
   unsigned char horizontal : 1;
   unsigned char doing_config : 1;
   unsigned char redo_config : 1;
   Eina_List *items;
   int frozen;
   Evas_Coord minw, minh, req;
}; 

struct _E_Gadcon_Layout_Item
{
   E_Smart_Data *sd;
   struct 
     {
        int pos, size, size2, res, prev_pos, prev_size;
     } ask;
   int hookp;
   struct 
     {
        int w, h;
     } min, aspect, aspect_pad;

   E_Gadcon_Client *gcc;

   Evas_Coord x, y, w, h;
   Evas_Object *obj;
   unsigned char can_move : 1;
};

/* local subsystem functions */
static E_Gadcon_Layout_Item *_e_gadcon_layout_smart_adopt(E_Smart_Data *sd, Evas_Object *obj);
static void _e_gadcon_layout_smart_disown(Evas_Object *obj);
static void _e_gadcon_layout_smart_item_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_gadcon_layout_smart_reconfigure(E_Smart_Data *sd);
static void _e_gadcon_layout_smart_init(void);
static void _e_gadcon_layout_smart_add(Evas_Object *obj);
static void _e_gadcon_layout_smart_del(Evas_Object *obj);
static void _e_gadcon_layout_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_gadcon_layout_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_gadcon_layout_smart_show(Evas_Object *obj);
static void _e_gadcon_layout_smart_hide(Evas_Object *obj);
static void _e_gadcon_layout_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_gadcon_layout_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_gadcon_layout_smart_clip_unset(Evas_Object *obj);
static void _e_gadcon_layout_smart_min_cur_size_calc(E_Smart_Data *sd, int *min, int *mino, int *cur);
static void _e_gadcon_layout_smart_gadcons_width_adjust(E_Smart_Data *sd, int min, int cur);
static int _e_gadcon_layout_smart_sort_by_sequence_number_cb(const void *d1, const void *d2);
static int _e_gadcon_layout_smart_sort_by_position_cb(const void *d1, const void *d2);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

/* externally accessible functions */
static Evas_Object *
e_gadcon_layout_add(Evas *evas)
{   
   _e_gadcon_layout_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

static void
e_gadcon_layout_orientation_set(Evas_Object *obj, int horizontal)
{
   E_Smart_Data *sd;

   if (!obj) return;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (((sd->horizontal) && (horizontal)) || 
       ((!sd->horizontal) && (!horizontal))) return;
   sd->horizontal = horizontal;
   _e_gadcon_layout_smart_reconfigure(sd);
}

static int
e_gadcon_layout_orientation_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!obj) return 0;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return 0;
   return sd->horizontal;
}

static void
e_gadcon_layout_freeze(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!obj) return;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->frozen++;
}

static void
e_gadcon_layout_thaw(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!obj) return;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->frozen--;
   _e_gadcon_layout_smart_reconfigure(sd);
}

static void
e_gadcon_layout_min_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   E_Smart_Data *sd;
/*   
   Eina_List *l;
   Evas_Object *obj;
   Evas_Coord tw = 0, th = 0;
 */
   if (!obj) return;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->horizontal)
     {
	if (w) *w = sd->minw;
	if (h) *h = sd->minh;
     }
   else
     {
	if (w) *w = sd->minh;
	if (h) *h = sd->minw;
     }
   
/*   
   EINA_LIST_FOREACH(sd->items, l, obj)
     {
	E_Gadcon_Layout_Item *bi;
	
	bi = evas_object_data_get(obj, "e_gadcon_layout_data");
	if (sd->horizontal)
	  {
	     tw += bi->min.w;
	     if (bi->min.h > th) th = bi->min.h;
	  }
	else
	  {
	     th += bi->min.h;
	     if (bi->min.w > tw) tw = bi->min.w;
	  }
     }
   if (w) *w = tw;
   if (h) *h = th;
 */
}

static void
e_gadcon_layout_asked_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   E_Smart_Data *sd;
   Evas_Coord tw = 0, th = 0;

   if (!obj) return;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->horizontal)
     tw = sd->req;
   else
     th = sd->req;
/*   
   Evas_Object *obj;
   EINA_LIST_FOREACH(sd->items, l, obj)
     {
	E_Gadcon_Layout_Item *bi;
	
	bi = evas_object_data_get(obj, "e_gadcon_layout_data");
	if (sd->horizontal)
	  {
	     tw += bi->ask.size;
	  }
	else
	  {
	     th += bi->ask.size;
	  }
     }
 */
   if (w) *w = tw;
   if (h) *h = th;
}

static int
e_gadcon_layout_pack(Evas_Object *obj, Evas_Object *child)
{
   E_Smart_Data *sd;

   if (!obj) return 0;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return 0;
   _e_gadcon_layout_smart_adopt(sd, child);
   sd->items = eina_list_prepend(sd->items, child);
   _e_gadcon_layout_smart_reconfigure(sd);
   return 0;
}

static void
e_gadcon_layout_pack_size_set(Evas_Object *obj, int size)
{
   /*
    * FIXME:
    * simplify this function until the is redone
    * _e_gadcon_layout_smart_gadcons_asked_position_set(E_Smart_Data *sd)
    */
   E_Gadcon_Layout_Item *bi;
   int pos;

   if (!obj) return;
   bi = evas_object_data_get(obj, "e_gadcon_layout_data");
   if (!bi) return;
   pos = bi->ask.pos + (bi->ask.size / 2);
   if (pos < (bi->ask.res / 3))
     { 
        /* hooked to start */
	bi->ask.size = size;
     }
   else if (pos > ((2 * bi->ask.res) / 3))
     { 
        /* hooked to end */
	bi->ask.pos = (bi->ask.pos + bi->ask.size) - size;
	bi->ask.size = size;
     }
   else
     { 
        /* hooked to middle */
	if ((bi->ask.pos <= (bi->ask.res / 2)) &&
	    ((bi->ask.pos + bi->ask.size) > (bi->ask.res / 2)))
	  { 
             /* straddles middle */
	     if (bi->ask.res > 2)
	       bi->ask.pos = (bi->ask.res / 2) +
	       (((bi->ask.pos + (bi->ask.size / 2) -
		  (bi->ask.res / 2)) *
		 (bi->ask.res / 2)) /
		(bi->ask.res / 2)) - (bi->ask.size / 2);
	     else
	       bi->x = bi->ask.res / 2;
	     bi->ask.size = size;
	  }
	else
	  { 
	     if (pos < (bi->ask.res / 2))
	       {
		  bi->ask.pos = (bi->ask.pos + bi->ask.size) - size;
		  bi->ask.size = size;
	       }
	     else
	       bi->ask.size = size;
	  }
        bi->ask.size = size;
     }
   _e_gadcon_layout_smart_reconfigure(bi->sd);
}

/* called when a users moves/resizes the gadcon client explicitly */
static void
e_gadcon_layout_pack_request_set(Evas_Object *obj, int pos, int size)
{
   E_Gadcon_Layout_Item *bi;

   if (!obj) return;
   bi = evas_object_data_get(obj, "e_gadcon_layout_data");
   if (!bi) return;

   bi->ask.res = bi->sd->w;
   if (pos < 0) pos = 0;
   if ((bi->ask.res - pos) < size) pos = bi->ask.res - size;
   bi->ask.size = size;
   bi->ask.pos = pos;
   _e_gadcon_layout_smart_reconfigure(bi->sd);
}

/* called when restoring config from saved config */
static void
e_gadcon_layout_pack_options_set(Evas_Object *obj, E_Gadcon_Client *gcc)
{
   int ok, seq;
   Eina_List *l;
   Evas_Object *item;
   E_Gadcon_Layout_Item *bi, *bi2;

   if (!obj) return;
   bi = evas_object_data_get(obj, "e_gadcon_layout_data");
   if (!bi) return;
   bi->ask.res = gcc->config.res;
   bi->ask.size = gcc->config.size;
   bi->ask.pos = gcc->config.pos;
   bi->gcc = gcc;

   ok = 0;
   if (!gcc->state_info.seq) ok = 1;

   seq = 1;
   EINA_LIST_FOREACH(bi->sd->items, l, item)
     {
	bi2 = evas_object_data_get(item, "e_gadcon_layout_data");
	if (bi == bi2) continue;
	if (bi->gcc->state_info.seq == bi2->gcc->state_info.seq)
	  ok = 1;

	if (bi2->gcc->state_info.seq > seq)
	  seq = bi2->gcc->state_info.seq;
     }

   if (ok)
     { 
	gcc->state_info.seq = seq + 1; 
	gcc->state_info.want_save = 1;
	gcc->state_info.flags = E_GADCON_LAYOUT_ITEM_LOCK_NONE;
     }
   _e_gadcon_layout_smart_reconfigure(bi->sd); 
}

static void
e_gadcon_layout_pack_min_size_set(Evas_Object *obj, int w, int h)
{
   E_Gadcon_Layout_Item *bi;

   if (!obj) return;
   bi = evas_object_data_get(obj, "e_gadcon_layout_data");
   if (!bi) return;
   if (bi->sd->horizontal)
     {
	bi->min.w = w;
	bi->min.h = h;
     }
   else
     {
	bi->min.w = h;
	bi->min.h = w;
     }
   
   _e_gadcon_layout_smart_reconfigure(bi->sd);
}

static void
e_gadcon_layout_pack_aspect_set(Evas_Object *obj, int w, int h)
{
   E_Gadcon_Layout_Item *bi;

   if (!obj) return;
   bi = evas_object_data_get(obj, "e_gadcon_layout_data");
   if (!bi) return;
   if (bi->sd->horizontal)
     {
	bi->aspect.w = w;
	bi->aspect.h = h;
     }
   else
     {
	bi->aspect.w = h;
	bi->aspect.h = w;
     }
     
   _e_gadcon_layout_smart_reconfigure(bi->sd);
}

static void
e_gadcon_layout_pack_aspect_pad_set(Evas_Object *obj, int w, int h)
{
   E_Gadcon_Layout_Item *bi;

   if (!obj) return;
   bi = evas_object_data_get(obj, "e_gadcon_layout_data");
   if (!bi) return;
   if (bi->sd->horizontal)
     {
	bi->aspect_pad.w = w;
	bi->aspect_pad.h = h;
     }
   else
     {
	bi->aspect_pad.w = h;
	bi->aspect_pad.h = w;
     }   
}

static void
e_gadcon_layout_unpack(Evas_Object *obj)
{
   E_Gadcon_Layout_Item *bi;
   E_Smart_Data *sd;

   if (!obj) return;
   bi = evas_object_data_get(obj, "e_gadcon_layout_data");
   if (!bi) return;
   sd = bi->sd;
   if (!sd) return;
   sd->items = eina_list_remove(sd->items, obj);
   _e_gadcon_layout_smart_disown(obj);
   _e_gadcon_layout_smart_reconfigure(sd);
}

/* local subsystem functions */
static E_Gadcon_Layout_Item *
_e_gadcon_layout_smart_adopt(E_Smart_Data *sd, Evas_Object *obj)
{
   E_Gadcon_Layout_Item *bi;

   if (!obj) return NULL;
   bi = E_NEW(E_Gadcon_Layout_Item, 1);
   if (!bi) return NULL;
   bi->sd = sd;
   bi->obj = obj;
   /* defaults */
   evas_object_clip_set(obj, sd->clip);
   evas_object_smart_member_add(obj, bi->sd->obj);
   evas_object_data_set(obj, "e_gadcon_layout_data", bi);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL,
				  _e_gadcon_layout_smart_item_del_hook, NULL);
   if ((!evas_object_visible_get(sd->clip)) &&
       (evas_object_visible_get(sd->obj)))
     evas_object_show(sd->clip);
   return bi;
}

static void
_e_gadcon_layout_smart_disown(Evas_Object *obj)
{
   E_Gadcon_Layout_Item *bi;

   if (!obj) return;
   bi = evas_object_data_get(obj, "e_gadcon_layout_data");
   if (!bi) return;
   if (!bi->sd->items)
     {
	if (evas_object_visible_get(bi->sd->clip))
	  evas_object_hide(bi->sd->clip);
     }
   evas_object_event_callback_del(obj, EVAS_CALLBACK_DEL,
				  _e_gadcon_layout_smart_item_del_hook);
   evas_object_smart_member_del(obj);
   evas_object_clip_unset(obj);
   evas_object_data_del(obj, "e_gadcon_layout_data");
   E_FREE(bi);
}

static void
_e_gadcon_layout_smart_item_del_hook(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   if (!obj) return;
   e_gadcon_layout_unpack(obj);
}

static void
_e_gadcon_layout_smart_reconfigure(E_Smart_Data *sd)
{
   Evas_Coord xx, yy;
   Eina_List *l;
   Evas_Object *obj;
   int min, mino, cur;
   Eina_List *list = NULL;
   E_Gadcon_Layout_Item *bi;
   E_Layout_Item_Container *lc;
   int i, set_prev_pos = 0;
   static int recurse = 0;

   if (sd->frozen) return;
   if (sd->doing_config)
     {
	sd->redo_config = 1;
	return;
     }

   recurse++;
   min = mino = cur = 0;

   _e_gadcon_layout_smart_min_cur_size_calc(sd, &min, &mino, &cur); 

   if ((sd->minw != min) || (sd->minh != mino))
     {
	sd->minw = min;
	sd->minh = mino;
	evas_object_smart_callback_call(sd->obj, "min_size_request", NULL);
     }

   if (sd->req != cur)
     {
	if (cur >= sd->minw)
	  {
	     sd->req = cur;
	     evas_object_smart_callback_call(sd->obj, "size_request", NULL);
	  }
	else
	  {
	     sd->req = sd->minw;     
	  }
     }
   if (recurse == 1) _e_gadcon_layout_smart_gadcons_width_adjust(sd, min, cur);

   if (sd->w <= sd->req)
     { 
	_e_gadcon_layout_smart_gadcon_position_shrinked_mode(sd);
	set_prev_pos = 0;
     }
   else
     { 
	_e_gadcon_layout_smart_gadcons_asked_position_set(sd);

	list = _e_gadcon_layout_smart_gadcons_wrap(sd);

	_e_gadcon_layout_smart_gadcons_position(sd, &list);

	EINA_LIST_FREE(list, lc)
	  LC_FREE(lc);

	set_prev_pos = 1;
     }

   sd->items = eina_list_sort(sd->items, eina_list_count(sd->items),
			      _e_gadcon_layout_smart_sort_by_position_cb);
   i = 1;
   EINA_LIST_FOREACH(sd->items, l, obj)
     {
	bi = evas_object_data_get(obj, "e_gadcon_layout_data"); 
	if (bi->gcc->gadcon->editing) bi->gcc->state_info.seq = i; 

	if (set_prev_pos)
	  {
	    bi->ask.prev_pos = bi->x; 
	    bi->ask.prev_size = bi->w;
	  }

	if ((bi->x == bi->ask.pos) &&
	    (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_POSITION))
	  bi->gcc->state_info.flags |= E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;

	if ((bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_POSITION) &&
	    (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE))
	  {
	    if (bi->x != bi->ask.pos)
	      bi->gcc->state_info.flags &= ~E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;
	  }
	i++;
     } 

   EINA_LIST_FOREACH(sd->items, l, obj)
     {
	E_Gadcon_Layout_Item *bi;

	bi = evas_object_data_get(obj, "e_gadcon_layout_data");
	if (!bi) continue;

	bi->h = sd->h;
	xx = sd->x + bi->x;
	yy = sd->y; // + ((sd->h - bi->h) / 2);
        
	if (sd->horizontal)
	  {
	     evas_object_move(obj, xx, yy);
	     evas_object_resize(obj, bi->w, bi->h);
	  }
	else
	  {
	     evas_object_move(obj, yy, xx);
	     evas_object_resize(obj, bi->h, bi->w);
	  }
     }
   sd->doing_config = 0;
   if (sd->redo_config)
     {
	_e_gadcon_layout_smart_reconfigure(sd);
	sd->redo_config = 0;
     }

   if ((sd->minw != min) || (sd->minh != mino))
     {
	sd->minw = min;
	sd->minh = mino;
	evas_object_smart_callback_call(sd->obj, "min_size_request", NULL);
     }

   if (sd->req != cur)
     {
	  if (cur >= sd->minw)
	  {
	     sd->req = cur;
	     evas_object_smart_callback_call(sd->obj, "size_request", NULL);
	  }
     }
   recurse--;
}

static void
_e_gadcon_layout_smart_init(void)
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     "e_gadcon_layout",
	       EVAS_SMART_CLASS_VERSION,
	       _e_gadcon_layout_smart_add,
	       _e_gadcon_layout_smart_del,
	       _e_gadcon_layout_smart_move,
	       _e_gadcon_layout_smart_resize,
	       _e_gadcon_layout_smart_show,
	       _e_gadcon_layout_smart_hide,
	       _e_gadcon_layout_smart_color_set,
	       _e_gadcon_layout_smart_clip_set,
	       _e_gadcon_layout_smart_clip_unset,
	       NULL, NULL, NULL, NULL, NULL, NULL, NULL
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_e_gadcon_layout_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!obj) return;
   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
   sd->obj = obj;
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   sd->horizontal = 1;
   evas_object_smart_member_add(sd->clip, obj);
   evas_object_move(sd->clip, -100005, -100005);
   evas_object_resize(sd->clip, 200010, 200010);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);
   evas_object_smart_data_set(obj, sd);
}
   
static void
_e_gadcon_layout_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!obj) return;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   while (sd->items)
     {
	Evas_Object *child;

	child = eina_list_data_get(sd->items);
	e_gadcon_layout_unpack(child);
     }
   evas_object_del(sd->clip);
   free(sd);
}

static void
_e_gadcon_layout_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   if (!obj) return;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((x == sd->x) && (y == sd->y)) return;
     {
	Eina_List *l;
	Evas_Object *item;
	Evas_Coord dx, dy;

	if (sd->horizontal)
	  {
	     dx = x - sd->x;
	     dy = y - sd->y;
	  }
	else
	  {
	     dx = x - sd->y;
	     dy = y - sd->x;
	  }
	
	EINA_LIST_FOREACH(sd->items, l, item)
	  {
	     Evas_Coord ox, oy;

	     evas_object_geometry_get(item, &ox, &oy, NULL, NULL);
	     evas_object_move(item, ox + dx, oy + dy);
	  }
     }

     if (sd->horizontal)
       {
	  sd->x = x;
	  sd->y = y;
       }
     else
       {
	  sd->x = y;
	  sd->y = x;
       }
}

static void
_e_gadcon_layout_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   if (!obj) return;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((w == sd->w) && (h == sd->h)) return;
   if (sd->horizontal)
     {	
	sd->w = w;
	sd->h = h;
     }
   else
     {
	sd->w = h;
	sd->h = w;

     }
   
   _e_gadcon_layout_smart_reconfigure(sd);
}

static void
_e_gadcon_layout_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!obj) return;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->items) evas_object_show(sd->clip);
}

static void
_e_gadcon_layout_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!obj) return;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_gadcon_layout_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;

   if (!obj) return;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;   
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_gadcon_layout_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;

   if (!obj) return;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_gadcon_layout_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!obj) return;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
}  

/*
 * @min - the minimum width required by all the gadcons
 * @cur - the current width required by all the gadcons
 * @mino - the smalest width/height among all the objects
 */
static void
_e_gadcon_layout_smart_min_cur_size_calc(E_Smart_Data *sd, int *min, int *mino, int *cur)
{
   E_Gadcon_Layout_Item	*bi;
   Eina_List *l;
   Evas_Object *item;

   EINA_LIST_FOREACH(sd->items, l, item)
     {
	bi = evas_object_data_get(item, "e_gadcon_layout_data");
	bi->ask.size2 = bi->ask.size;

	if ((bi->aspect.w > 0) && (bi->aspect.h > 0))
	  {
	     bi->ask.size2 =
	       (((sd->h - bi->aspect_pad.h) * bi->aspect.w) / bi->aspect.h) + bi->aspect_pad.w;
	     
	     if (bi->ask.size2 > bi->min.w)
	       {
		  *min += bi->ask.size2;
		  *cur += bi->ask.size2;
	       }
	     else
	       {
		  *min += bi->min.w;
		  *cur += bi->min.w;
	       }
	  }
	else
	  {
             bi->ask.size2 = bi->ask.size = bi->min.w;
	     *min += bi->min.w;
	     if (bi->min.h > *mino) *mino = bi->min.h;
	     if (bi->ask.size < bi->min.w)
	       *cur += bi->min.w;
	     else
	       *cur += bi->ask.size;
	  }
     }
}

static int 
_e_gadcon_layout_smart_width_smart_sort_reverse_cb(const void *d1, const void *d2)
{
   const E_Gadcon_Layout_Item *bi, *bi2;

   bi = evas_object_data_get(d1, "e_gadcon_layout_data");
   bi2 = evas_object_data_get(d2, "e_gadcon_layout_data"); 

   if (bi->ask.size2 > bi->min.w)
     {
	if (bi2->ask.size2 > bi2->min.w) 
	  { 
	     if (bi->ask.size2 < bi2->ask.size2)
	       return 1;
	     else
	       return -1;
	  }
	else
	  {
	     if (bi->ask.size2 == bi2->ask.size2)
	       return -1;
	     else
	       { 
		  if (bi->ask.size2 < bi2->ask.size2)
		    return 1;
		  else
		    return -1;
	       }
	  }
     }
   else
     {
	if (bi2->ask.size2 > bi2->min.w)
	  {
	     if (bi->ask.size2 == bi2->ask.size2)
	       return 1;
	     else
	       { 
		  if (bi->ask.size2 < bi2->ask.size2)
		    return 1;
		  else
		    return -1;
	       }
	  }
	else
	  { 
	     if (bi->ask.size2 < bi2->ask.size2)
	       return 1;
	     else if (bi->ask.size2 > bi2->ask.size2)
	       return -1;
	  }
     }

   return 0;
}

static void 
_e_gadcon_layout_smart_gadcons_width_adjust(E_Smart_Data *sd, int min, int cur)
{ 
   E_Gadcon_Layout_Item *bi = NULL;
   Eina_List *l, *l2;
   Evas_Object *item;
   int needed = 0;
   int need = 0;
   int max_size, autosize = 0;

   if (sd->w < cur)
     {
	if (sd->w < min) max_size = min;
	else max_size = cur;
	need = max_size - sd->w;
     }
   else
     return;
   
   sd->items = eina_list_sort(sd->items, eina_list_count(sd->items), 
			      _e_gadcon_layout_smart_width_smart_sort_reverse_cb);
   EINA_LIST_FOREACH(sd->items, l, item)
     { 
        bi = evas_object_data_get(item, "e_gadcon_layout_data");
        if (bi->gcc->autoscroll) autosize += bi->ask.size2;
     } 

   if (autosize < 1) autosize = 1;
   while (need > 0)
     { 
        needed = need;
        EINA_LIST_REVERSE_FOREACH(sd->items, l2, item)
          { 
             if (need <= 0) break;
             bi = evas_object_data_get(item, "e_gadcon_layout_data");
             if (bi->gcc->autoscroll)
               {
                  int reduce_by; 
                  
                  reduce_by = (need * bi->ask.size2) / autosize;
                  if (reduce_by < 1) reduce_by = 1;
                  if (bi->ask.size2 - reduce_by > 8)
                    {
                       bi->ask.size2 -= reduce_by;
                       need -= reduce_by ; 
                    }
                  else
                    {
                       need -= bi->ask.size2 - 8;
                       bi->ask.size2 = 8;
                    }
	       }
	  } 
        /* If the 'needed' size change didn't get modified (no gadget has autoscroll)
           then we must break or we end up in an infinite loop */
        if (need == needed) break;
     }
}

static int 
_e_gadcon_layout_smart_sort_by_sequence_number_cb(const void *d1, const void *d2)
{
   const E_Gadcon_Layout_Item *bi, *bi2;

   bi = evas_object_data_get(d1, "e_gadcon_layout_data");
   bi2 = evas_object_data_get(d2, "e_gadcon_layout_data");

   if ((!bi->gcc->state_info.seq) && (!bi2->gcc->state_info.seq)) return 0;
   else if (!bi->gcc->state_info.seq) return 1;
   else if (!bi2->gcc->state_info.seq) return -1;
   return bi->gcc->state_info.seq - bi2->gcc->state_info.seq;
} 

static int
_e_gadcon_layout_smart_sort_by_position_cb(const void *d1, const void *d2)
{
   const E_Gadcon_Layout_Item *bi, *bi2;

   bi = evas_object_data_get(d1, "e_gadcon_layout_data");
   bi2 = evas_object_data_get(d2, "e_gadcon_layout_data");

   return (bi->x - bi2->x);
}

static int
_e_gadcon_layout_smart_containers_sort_cb(const void *d1, const void *d2)
{
   const E_Layout_Item_Container *lc, *lc2;

   lc = d1;
   lc2 = d2;
   if (lc->pos < lc2->pos) return -1;
   else if (lc->pos > lc2->pos) return 1;
   return 0;
}

static int
_e_gadcon_layout_smart_seq_sort_cb(const void *d1, const void *d2)
{
   const E_Gadcon_Layout_Item *bi, *bi2;

   bi = d1;
   bi2 = d2;
   return (bi->gcc->state_info.seq - bi2->gcc->state_info.seq);
}

static void
_e_gadcon_layout_smart_sync_clients(E_Gadcon *gc)
{
   E_Gadcon_Client *gcc;
   Eina_List *l;

   EINA_LIST_FOREACH(gc->clients, l, gcc)
     {
	_e_gadcon_client_save(gcc);
     }
}

static void
_e_gadcon_client_current_position_sync(E_Gadcon_Client *gcc)
{
   E_Gadcon_Layout_Item *bi;
   Evas_Object *o;

   o = gcc->o_frame ? gcc->o_frame : gcc->o_base;
   if (o)
     {
	bi = evas_object_data_get(o, "e_gadcon_layout_data");
	if (!bi) return;
     }
   else return; /* make clang happy */

   gcc->state_info.prev_pos = gcc->config.pos;
   gcc->state_info.prev_size = gcc->config.size;
   gcc->config.pos = bi->x;
}

static void
_e_gadcon_layout_smart_gadcon_position_shrinked_mode(E_Smart_Data *sd)
{ 
   Eina_List *l;
   Evas_Object *item;
   E_Gadcon_Layout_Item *bi, *bi2; 
   void *tp; 
   int pos = 0; 

   sd->items = eina_list_sort(sd->items, eina_list_count(sd->items),
			      _e_gadcon_layout_smart_sort_by_sequence_number_cb); 
   EINA_LIST_FOREACH(sd->items, l, item)
     { 
	bi = evas_object_data_get(item, "e_gadcon_layout_data"); 
	if (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_POS_INC) 
	  { 
	     if (bi->gcc->state_info.resist <= E_LAYOUT_ITEM_DRAG_RESIST_LEVEL) 
	       { 
	     	  bi->gcc->state_info.resist++; 
	     	  bi->gcc->config.pos = bi->ask.pos = bi->gcc->state_info.prev_pos;
	       } 
	     else
	       { 
		  bi->gcc->state_info.resist = 0; 
		  if (eina_list_next(l)) 
		    { 
		       tp = eina_list_data_get(eina_list_next(l));
		       l->next->data = eina_list_data_get(l); 
		       l->data = tp; 

		       bi2 = evas_object_data_get(tp, "e_gadcon_layout_data");
		       
		       if (bi2->x + bi2->w/2 > bi->ask.pos + bi->w)
			 {
			    bi->gcc->config.pos = bi->ask.pos = bi->gcc->state_info.prev_pos;
			    return;
			 }
		       
		       bi->gcc->config.pos = bi->ask.pos = bi2->ask.pos;
		       bi->gcc->state_info.flags &= ~E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;
		       bi->gcc->state_info.want_save = 1;
		       bi2->gcc->state_info.want_save = 1;
		       break;
		    }
		  else
		    bi->gcc->config.pos = bi->ask.pos = bi->gcc->state_info.prev_pos;
	       } 
	  } 
	else if (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_POS_DEC)
	  { 
	     if (bi->gcc->state_info.resist <= E_LAYOUT_ITEM_DRAG_RESIST_LEVEL)
	       {
		  bi->gcc->state_info.resist++;
		  bi->gcc->config.pos = bi->ask.pos = bi->gcc->state_info.prev_pos;
	       }
	     else
	       {
		  bi->gcc->state_info.resist = 0; 
		  if (eina_list_prev(l))
		    { 
		       E_Gadcon_Layout_Item *bi2;
		       void *tp;

		       tp = eina_list_data_get(eina_list_prev(l));
		       l->prev->data = eina_list_data_get(l);

		       l->data = tp; 
		       bi2 = evas_object_data_get(tp, "e_gadcon_layout_data");

		       if (bi->ask.pos > bi2->x + bi2->w/2)
			 {
			    bi->gcc->config.pos = bi->ask.pos = bi->gcc->state_info.prev_pos;
			    return;
			 }

		       bi->gcc->config.pos = bi->ask.pos = bi2->ask.pos;
		       bi->gcc->state_info.flags &= ~E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;
		       bi->gcc->state_info.want_save = 1;
		       bi2->gcc->state_info.want_save = 1;
		       break;
		    }
		  else
		    bi->gcc->config.pos = bi->ask.pos = bi->gcc->state_info.prev_pos;
	       }
	  }
	else if ((bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_SIZE_MIN_END_INC) ||
	         (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_SIZE_MAX_END_DEC) ||
		 (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_SIZE_MIN_END_DEC) ||
	         (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_SIZE_MAX_END_INC))
	  { 
	     if (bi->w < bi->min.w) 
	       bi->gcc->config.size = bi->w = bi->min.w;
	     else
	       bi->gcc->config.size = bi->w;

	     bi->gcc->config.pos = bi->gcc->state_info.prev_pos;
	  }	
     } 

   EINA_LIST_FOREACH(sd->items, l, item)
     { 
	bi = evas_object_data_get(item, "e_gadcon_layout_data");

	bi->x = pos;
	bi->w = bi->ask.size2; 
	bi->gcc->config.size = bi->w;
	pos = bi->x + bi->w;
     }
}

static void
_e_gadcon_layout_smart_gadcons_asked_position_set(E_Smart_Data *sd)
{
   E_Gadcon_Layout_Item *bi;
   Eina_List *l;
   Evas_Object *item;

#if 0
   EINA_LIST_FOREACH(sd->items, l, item)
     {
	bi = evas_object_data_get(item, "e_gadcon_layout_data");
	if (!bi) continue;
	
	bi->x = bi->ask.pos;
	bi->w = bi->ask.size2;
     }
#else
   int pos;
   EINA_LIST_FOREACH(sd->items, l, item)
     {
	bi = evas_object_data_get(item, "e_gadcon_layout_data");
	if (!bi) continue;

	pos = bi->ask.pos + (bi->ask.size / 2);
	if (pos < (bi->ask.res / 3))
	  { 
	     /* hooked to start */
	     bi->x = bi->ask.pos;
	     bi->w = bi->ask.size2;
	     bi->hookp = 0;
	  }
	else if (pos > ((2 * bi->ask.res) / 3))
	  { 
	     /* hooked to end */
	     bi->x = (bi->ask.pos - bi->ask.res) + sd->w;
	     bi->w = bi->ask.size2;
	     bi->hookp = bi->ask.res;
	  }
	else
	  { 
	     /* hooked to middle */
	     if ((bi->ask.pos <= (bi->ask.res / 2)) &&
		 ((bi->ask.pos + bi->ask.size2) > (bi->ask.res / 2)))
	       { 
		  /* straddles middle */
		  if (bi->ask.res > 2)
		    bi->x = (sd->w / 2) + 
		      (((bi->ask.pos + (bi->ask.size2 / 2) - 
			 (bi->ask.res / 2)) * 
			(bi->ask.res / 2)) /
		       (bi->ask.res / 2)) - (bi->ask.size2 / 2);
		  else
		    bi->x = sd->w / 2;
		  bi->w = bi->ask.size2;
	       }
	     else
	       { 
		  /* either side of middle */
		  bi->x = (bi->ask.pos - (bi->ask.res / 2)) + (sd->w / 2);
		  bi->w = bi->ask.size2;
	       }
	     bi->hookp = bi->ask.res / 2;
	  }
     }
#endif
}

/* 
 * The function returns a list of E_Gadcon_Layout_Item_Container
 */
static Eina_List *
_e_gadcon_layout_smart_gadcons_wrap(E_Smart_Data *sd)
{
   Eina_List *l, *list = NULL;
   Evas_Object *item;
   E_Layout_Item_Container *lc;
   E_Gadcon_Layout_Item *bi;

   
   EINA_LIST_FOREACH(sd->items, l, item)
     {
	bi = evas_object_data_get(item, "e_gadcon_layout_data");
	lc = E_NEW(E_Layout_Item_Container, 1);
	lc->state_info.min_seq = lc->state_info.max_seq = bi->gcc->state_info.seq;
	lc->sd = sd;

	lc->pos = bi->x;
	lc->size = bi->w;

	lc->prev_pos = bi->ask.prev_pos;
	lc->prev_size = bi->ask.prev_size;

	E_LAYOUT_ITEM_CONTAINER_STATE_SET(lc->state, bi->gcc->state_info.state);

	if ((bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_POSITION) &&
	    (lc->state == E_LAYOUT_ITEM_CONTAINER_STATE_NONE))
	  lc->state = E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED;

	lc->items = eina_list_append(lc->items, bi);
	list = eina_list_append(list, lc);
     }
   return list;
}

static void
_e_gadcon_layout_smart_gadcons_position(E_Smart_Data *sd, Eina_List **list)
{
   int ok, lc_moving_prev_pos;
   Eina_List *l, *l2, *l3;
   E_Layout_Item_Container *lc_moving = NULL, *lc_back = NULL, *lc, *lc3;
   E_Gadcon_Layout_Item *bi, *bi_moving = NULL;

   if ((!list) || (!*list)) return;

   EINA_LIST_FOREACH(*list, l, lc_moving)
     {
	if ((lc_moving->state != E_LAYOUT_ITEM_CONTAINER_STATE_NONE) &&
	    (lc_moving->state != E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED))
	  {
	     lc_back = E_NEW(E_Layout_Item_Container, 1);
	     lc_back->pos = lc_moving->pos;
	     lc_back->prev_pos = lc_moving->prev_pos;
	     lc_back->size = lc_moving->size;
	     lc_back->prev_size = lc_moving->prev_size;
	     lc_back->state_info.min_seq = lc_moving->state_info.min_seq;
	     lc_back->state_info.max_seq = lc_moving->state_info.max_seq;
	     lc_back->sd = lc_moving->sd;
	     EINA_LIST_FOREACH(lc_moving->items, l2, lc)
	       lc_back->items = eina_list_append(lc_back->items, lc);
	     lc_back->state = lc_moving->state;
	     bi_moving = eina_list_data_get(lc_back->items);
	     
	     break;
	  }
	lc_moving = NULL;
     }

   if (!lc_moving)
     {
	_e_gadcon_layout_smart_gadcons_position_static(sd, list);
	return;
     } 

   lc_moving_prev_pos = lc_moving->prev_pos;
   if (lc_moving->state == E_LAYOUT_ITEM_CONTAINER_STATE_POS_DEC) 
     { 
	_e_gadcon_layout_smart_restore_gadcons_position_before_move(sd, &lc_moving, lc_back, list); 
	EINA_LIST_FOREACH(*list, l, lc)
	   if (lc == lc_moving) break;

	ok = 0;
	if ((l) && eina_list_prev(l))
	  {
	     lc = eina_list_data_get(eina_list_prev(l));

	     if (lc_moving->pos < (lc->pos + lc->size))
	       {
		  bi = eina_list_data_get(lc_moving->items);
		  if (bi->gcc->state_info.resist <= E_LAYOUT_ITEM_DRAG_RESIST_LEVEL)
		    {
		       if (lc_moving->prev_pos == (lc->pos + lc->size))
			 ok = 1;
		       bi->gcc->state_info.resist++;
		       lc_moving->pos = lc->pos + lc->size;
		       _e_gadcon_layout_smart_position_items_inside_container(sd, lc_moving);
		    }
		  else
		    {
		       bi->gcc->state_info.resist = 0;
		       if (lc_moving->pos < lc->pos) 
			 { 
			    lc_moving->pos = (lc->pos + lc->size) - 1;
			    _e_gadcon_layout_smart_position_items_inside_container(sd, lc_moving);
			 }
		       lc3 = _e_gadcon_layout_smart_containers_position_adjust(sd, lc, lc_moving);
		       if (lc3)
			 {
			    if (lc_moving->prev_pos == (lc->pos + lc->size))
			      ok = 1;

			    l->data = lc3;
			    *list = eina_list_remove_list(*list, eina_list_prev(l));
			    LC_FREE(lc_moving);
			    LC_FREE(lc);
			    lc_moving = lc3;
			 }
		    }
	       }
	  }
	if (!ok) 
	  { 
	     int pos, prev_pos, stop;

	     EINA_LIST_FOREACH(*list, l, lc)
		if (lc == lc_moving) break;

	     pos = lc_moving->pos + lc_moving->size;
	     prev_pos = lc_moving_prev_pos;
	     if ((l) && (eina_list_next(l)))
	       { 
		  stop = 0;
		  EINA_LIST_FOREACH(eina_list_next(l), l2, lc)
		    {
		       if (stop) break;
		       if (lc->pos != prev_pos) break;
		       prev_pos = lc->pos + lc->size;

		       EINA_LIST_FOREACH(lc->items, l3, bi)
			 {
			    if (bi->ask.pos <= pos)
			      {
				bi->x = pos;
				pos = (bi->x) + (bi->w);
			      }
			    else if (bi->ask.pos < bi->x)
			      {
				bi->x = bi->ask.pos;
				pos = (bi->x) + (bi->w);				
			      }
			    else if (bi->ask.pos == bi->x)
			      {
				 stop = 1;
				 break;
			      }
			 }
		    }
	       }
	  }
     }
   else if (lc_moving->state == E_LAYOUT_ITEM_CONTAINER_STATE_POS_INC)
     {
	_e_gadcon_layout_smart_restore_gadcons_position_before_move(sd, &lc_moving, lc_back, list); 
	EINA_LIST_FOREACH(*list, l, lc)
	   if (lc == lc_moving) break;

	ok = 0;
	if ((l) && eina_list_next(l))
	  {
	     lc = eina_list_data_get(eina_list_next(l));

	     if ((lc_moving->pos + lc_moving->size) > lc->pos)
	       {
		  bi = eina_list_data_get(lc_moving->items);
		  if (bi->gcc->state_info.resist <= E_LAYOUT_ITEM_DRAG_RESIST_LEVEL)
		    {
		       if ((lc_moving->prev_pos + lc_moving->size) == lc->pos)
			 ok = 1;
		       bi->gcc->state_info.resist++;
		       lc_moving->pos = lc->pos - lc_moving->size;
		       _e_gadcon_layout_smart_position_items_inside_container(sd, lc_moving);
		    } 
		  else
		    {
		       bi->gcc->state_info.resist = 0;
		       if ((lc_moving->pos + lc_moving->size) > lc->pos)
			 {
			    lc_moving->pos = (lc->pos - lc_moving->size) + 1;
			    _e_gadcon_layout_smart_position_items_inside_container(sd, lc_moving);
			 }
		       lc3 = _e_gadcon_layout_smart_containers_position_adjust(sd, lc_moving, lc);
		       if (lc3)
			 {
			    if ((lc_moving->prev_pos + lc_moving->size) == lc->pos)
			      ok = 1;

			    l->data = lc3;
			    *list = eina_list_remove_list(*list, eina_list_next(l));
			    LC_FREE(lc_moving);
			    LC_FREE(lc);
			    lc_moving = lc3;
			 }
		    }
	       }
	  }

	if (!ok)
	  {
	     int pos, prev_pos, stop;
	     
	     EINA_LIST_FOREACH(*list, l, lc)
		if (lc == lc_moving) break;

	     pos = lc_moving->pos;
	     prev_pos = lc_moving_prev_pos;

	     if ((l) && eina_list_prev(l))
	       {
		  stop = 0;
		  /* EINA_FUCK_REVERSE_FOREACH(eina_list_prev(l), l2, lc) */
		  for (l2 = l->prev; l2; l2 = l2->prev)
		    {
		       lc = l2->data;
		       
		       if (stop) break;
		       if ((lc->pos + lc->size) == prev_pos) break;
		       prev_pos = lc->pos;

		       EINA_LIST_REVERSE_FOREACH(lc->items, l3, bi)
			 {
			    if ((bi->ask.pos + bi->w) >= pos)
			      {
				 bi->x = pos - bi->w;
				 pos = bi->x;
			      }
			    else if (bi->ask.pos > bi->x)
			      {
				 bi->x = bi->ask.pos;
				 pos = bi->x;
			      }
			    else if (bi->ask.pos == bi->x)
			      {
				 stop = 1;
				 break;
			      }
			 }
		    }
	       }
	  }
     } 
   else if (lc_moving->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MIN_END_DEC)
     { 
	_e_gadcon_layout_smart_restore_gadcons_position_before_move(sd, &lc_moving, lc_back, list); 
	EINA_LIST_FOREACH(*list, l, lc)
	   if (lc == lc_moving) break;

	if ((l) && eina_list_prev(l))
	  {
	     int new_pos = 0;

	     ok = 0;
	     new_pos = lc_moving->pos;
	     /* EINA_FUCK_REVERSE_FOREACH(eina_list_prev(l), l2, lc) */
	     for (l2 = l->prev; l2; l2 = l2->prev)
	       {
		  lc = l2->data;		  
		  if (new_pos >= (lc->pos + lc->size)) break;

		  ok = 1;
		  new_pos -= lc->size;
	       }

	     if (new_pos < 0)
	       {
		  lc_moving->size += new_pos;
		  lc_moving->pos -= new_pos;

		  bi = eina_list_data_get(lc_moving->items);
		  bi->w = lc_moving->size;
		  _e_gadcon_layout_smart_position_items_inside_container(sd, lc_moving);

		  new_pos = 0;
	       }

	     if (ok)
	       { 
		  if (!l2) l2 = *list; 
		  else l2 = eina_list_next(l2);

		  EINA_LIST_FOREACH(l2, l2, lc)
		    { 
		       if (l2 == l) break;
		       lc->pos = new_pos; 
		       _e_gadcon_layout_smart_position_items_inside_container(sd, lc); 
		       EINA_LIST_FOREACH(lc->items, l3, bi)
			 {
			    bi->gcc->state_info.flags &= ~E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;
			 }
		       new_pos += lc->size; 
		    }
	       }
	  }
	else if ((l) && (!eina_list_prev(l)))
	  {
	     if (lc_moving->pos <= 0)
	       {
		  lc_moving->size = lc_moving->prev_size;
		  lc_moving->pos = 0;

		  bi = eina_list_data_get(lc_moving->items);
		  bi->w = lc_moving->size;

		  _e_gadcon_layout_smart_position_items_inside_container(sd, lc_moving);
	       }
	  }
     }
   else if (lc_moving->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MIN_END_INC)
     {
	lc_moving->state = E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED;
	_e_gadcon_layout_smart_gadcons_position_static(sd, list);
	if (lc_back) LC_FREE(lc_back);
     }
   else if (lc_moving->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MAX_END_INC)
     {
	_e_gadcon_layout_smart_restore_gadcons_position_before_move(sd, &lc_moving, lc_back, list); 
	EINA_LIST_FOREACH(*list, l, lc)
	   if (lc == lc_moving) break;

	if ((l) && eina_list_next(l))
	  {
	     Eina_List *stop = NULL;
	     int new_pos = 0;

	     ok = 0;
	     new_pos = lc_moving->pos + lc_moving->size;
	     EINA_LIST_FOREACH(eina_list_next(l), l2, lc)
	       {
		  if (new_pos <= lc->pos)
		    {
		       stop = l2;
		       break;
		    }

		  ok = 1;
		  /* new_pos += lc->size; */
	       }

	     if (new_pos > sd->w)
	       {
		  lc_moving->size -= (new_pos - sd->w);
		  bi = eina_list_data_get(lc_moving->items);
		  bi->w = lc_moving->size;

		  new_pos = lc_moving->pos + lc_moving->size;
	       }

	     if (ok)
	       {
		  EINA_LIST_FOREACH(eina_list_next(l), l2, lc)
		    {
		       if (l2 == stop) break;
		       lc->pos = new_pos;
		       _e_gadcon_layout_smart_position_items_inside_container(sd, lc);
		       EINA_LIST_FOREACH(lc->items, l3, bi)
			 {
			    bi->gcc->state_info.flags &= ~E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;
			 }
		       new_pos += lc->size;
		    }
	       }
	  }
	else if ((l) && (!eina_list_next(l)))
	  {
	     if ((lc_moving->pos + lc_moving->size) >= sd->w)
	       {
		  lc_moving->size = lc_moving->prev_size;
		  bi = eina_list_data_get(lc_moving->items);
		  bi->w = lc_moving->size;
	       }
	  }
     }
   else if (lc_moving->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MAX_END_DEC)
     {
	lc_moving->state = E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED;
	_e_gadcon_layout_smart_gadcons_position_static(sd, list);
	if (lc_back) LC_FREE(lc_back);
     }

   if (bi_moving)
     {
	bi_moving->gcc->config.pos = bi_moving->ask.pos = bi_moving->x; 
	bi_moving->gcc->config.size = bi_moving->w;
     }
}

static void
_e_gadcon_layout_smart_gadcons_position_static(E_Smart_Data *sd, Eina_List **list)
{
   int ok;
   Eina_List *l;
   E_Layout_Item_Container *lc, *lc2, *lc3;

   *list = eina_list_sort(*list, eina_list_count(*list), _e_gadcon_layout_smart_containers_sort_cb);

   __reposition_again:
   EINA_LIST_FOREACH(*list, l, lc)
     {
	if (!eina_list_next(l)) continue;

	lc2 = eina_list_data_get(eina_list_next(l));

	if (LC_OVERLAP(lc, lc2))
	  {
	     lc3 = _e_gadcon_layout_smart_containers_position_adjust(sd, lc, lc2);
	     if (lc3)
	       { 
		  l->data = lc3; 
		  *list = eina_list_remove_list(*list, eina_list_next(l)); 
		  LC_FREE(lc);
		  LC_FREE(lc2);
		  goto __reposition_again;
	       }
	  }
     }

   ok = 1;
   EINA_LIST_FOREACH(*list, l, lc)
     {
	if (lc->pos < 0)
	  {
	     ok = 0;
	     lc->pos = 0;
	     _e_gadcon_layout_smart_position_items_inside_container(sd, lc);
	     continue;
	  }

	if (((lc->pos + lc->size) > sd->w) && (lc->size <= sd->w))
	  {
	     ok = 0;
	     lc->pos = sd->w - lc->size;
	     _e_gadcon_layout_smart_position_items_inside_container(sd, lc);
	  }
     }
   if (!ok)
     _e_gadcon_layout_smart_gadcons_position_static(sd, list);
}

static E_Layout_Item_Container *
_e_gadcon_layout_smart_containers_position_adjust(E_Smart_Data *sd, E_Layout_Item_Container *lc, E_Layout_Item_Container *lc2)
{
   int create_new = 0;
   Eina_List *l;
   E_Layout_Item_Container *lc3 = NULL;
   E_Layout_Item_Container_State new_state;
   E_Gadcon_Layout_Item *bi, *bi2;

   if ((lc->state == E_LAYOUT_ITEM_CONTAINER_STATE_NONE) &&
       (lc2->state == E_LAYOUT_ITEM_CONTAINER_STATE_NONE))
     {
	if (lc->state_info.max_seq <= lc2->state_info.min_seq)
	  {
	     lc2->pos = lc->pos + lc->size;
	     _e_gadcon_layout_smart_position_items_inside_container(sd, lc2);
	  }
	else if (lc->state_info.min_seq > lc2->state_info.max_seq)
	  {
	     lc->pos = lc2->pos + lc2->size;
	     _e_gadcon_layout_smart_position_items_inside_container(sd, lc);
	  }
	else if (((lc->state_info.min_seq > lc2->state_info.min_seq) &&
		  (lc->state_info.min_seq < lc2->state_info.max_seq)) ||
	         ((lc2->state_info.min_seq > lc->state_info.min_seq) &&
		  (lc2->state_info.min_seq < lc->state_info.max_seq)))
	  {
	     _e_gadcon_layout_smart_containers_merge(sd, lc, lc2);
	  } 
	create_new = 1; 
	new_state = E_LAYOUT_ITEM_CONTAINER_STATE_NONE;
     }
   else if ((lc->state != E_LAYOUT_ITEM_CONTAINER_STATE_NONE) &&
	    (lc2->state == E_LAYOUT_ITEM_CONTAINER_STATE_NONE))
     {
	if (lc->state == E_LAYOUT_ITEM_CONTAINER_STATE_POS_INC)
	  {
	     int t;

	     bi = eina_list_data_get(lc->items);
	     bi2 = eina_list_data_get(lc2->items);

	     bi->x = ((bi2->x) + (bi2->w)) - (bi->w);
	     bi->gcc->config.pos = bi->ask.pos = bi->x;
	     bi->gcc->config.size = bi->w;
	     bi2->x = (bi->x) - (bi2->w);

	     bi2->gcc->state_info.flags &= ~E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;

	     t = bi->gcc->state_info.seq;
	     bi->gcc->state_info.seq = bi2->gcc->state_info.seq;
	     bi2->gcc->state_info.seq = t;

	     _e_gadcon_layout_smart_containers_merge(sd, lc, lc2);
	  }
	else if (lc->state == E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED)
	  {
	     if (lc->state_info.max_seq < lc2->state_info.min_seq)
	       {
		  lc2->pos = lc->pos + lc->size;
		  _e_gadcon_layout_smart_position_items_inside_container(sd, lc2);
	       }
	     else if (lc->state_info.min_seq > lc2->state_info.max_seq)
	       {
		  lc2->pos = lc->pos - lc2->size;
		  _e_gadcon_layout_smart_position_items_inside_container(sd, lc2);
	       } 
	     else if (((lc->state_info.min_seq > lc2->state_info.min_seq) && 
		      (lc->state_info.min_seq < lc2->state_info.max_seq)) || 
		     ((lc2->state_info.min_seq > lc->state_info.min_seq) && 
		      (lc2->state_info.min_seq < lc->state_info.max_seq)))
	       {
		  int shift = 0;

		  _e_gadcon_layout_smart_containers_merge(sd, lc, lc2);

		  EINA_LIST_FOREACH(lc->items, l, bi)
		    {
		       if (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_POSITION)
			 {
			    shift = bi->ask.pos - bi->x;
			 }

		       if (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE)
			 break;
		    }

		  if (shift)
		    {
     		       EINA_LIST_FOREACH(lc->items, l, bi)
     			 {
			    bi->x += shift;
			    
     			    if (l == lc->items)
			      lc->pos = bi->x;
     			 }
     		    } 
     	       } 
	  }
	create_new = 1; 
	new_state = E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED;
     }
   else if ((lc->state == E_LAYOUT_ITEM_CONTAINER_STATE_NONE) &&
	    (lc2->state != E_LAYOUT_ITEM_CONTAINER_STATE_NONE))
     {
	if (lc2->state == E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED)
	  {
	     if (lc->state_info.max_seq < lc2->state_info.min_seq)
	       {
		  lc->pos = lc2->pos - lc->size;
		  _e_gadcon_layout_smart_position_items_inside_container(sd, lc);
	       }
	     else if (lc->state_info.min_seq > lc2->state_info.max_seq)
	       {
		  lc->pos = lc2->pos + lc2->size;
		  _e_gadcon_layout_smart_position_items_inside_container(sd, lc);
	       }
	     else if (((lc->state_info.min_seq > lc2->state_info.min_seq) && 
		      (lc->state_info.min_seq < lc2->state_info.max_seq)) || 
		     ((lc2->state_info.min_seq > lc->state_info.min_seq) && 
		      (lc2->state_info.min_seq < lc->state_info.max_seq)))
	       {
		  int shift = 0;

		  EINA_LIST_FOREACH(lc->items, l, bi)
		    {
		       if (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_POSITION)
			 {
			    shift = bi->ask.pos - bi->x;
			 }

		       if (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE)
			 break;
		    }

		  if (shift)
		    {
  		       EINA_LIST_FOREACH(lc->items, l, bi)
  			 {
			    bi->x += shift;

     			    if (l == lc->items)
			      lc->pos = bi->x;
     			 }
     		    }
	       }
	  }
	else if (lc2->state == E_LAYOUT_ITEM_CONTAINER_STATE_POS_DEC)
	  {
	     int t;

	     bi = eina_list_data_get(eina_list_last(lc->items));
	     bi2 = eina_list_data_get(lc2->items);

	     bi2->gcc->config.pos = bi2->ask.pos = (bi2->x) = (bi->x);
	     bi2->gcc->config.size = bi2->w;
	     bi->x = bi2->x + bi2->w;

	     t = bi->gcc->state_info.seq;
	     bi->gcc->state_info.seq = bi2->gcc->state_info.seq;
	     bi2->gcc->state_info.seq = t;

	     lc->items = eina_list_remove_list(lc->items, eina_list_last(lc->items));
	     lc->items = eina_list_append(lc->items, bi2);
	     lc->items = eina_list_append(lc->items, bi);
	     lc2->items = eina_list_free(lc2->items);
	     E_LAYOUT_ITEM_CONTAINER_SIZE_CHANGE_BY(lc, bi2, 1);
	     lc2->pos = lc->pos + lc->size;
	     lc2->size = 0;
	  }
	create_new = 1;
	new_state = E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED;
     }
   else if ((lc->state != E_LAYOUT_ITEM_CONTAINER_STATE_NONE) &&
	    (lc2->state != E_LAYOUT_ITEM_CONTAINER_STATE_NONE))
     {
	if ((lc->state == E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED) &&
	    (lc2->state == E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED))
	  {
	     if (lc->state_info.max_seq < lc2->state_info.min_seq)
	       {
		  int move_lc1 = 1;
		  int move_lc2 = 1;

		  EINA_LIST_FOREACH(lc->items, l, bi)
		    {
		       if (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE)
			 {
			    move_lc1 = 0;
			    break;
			 }
		    }
		  EINA_LIST_FOREACH(lc2->items, l, bi)
		    {
		       if (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE)
			 {
			    move_lc2 = 0;
			    break;
			 }
		    }

		  if ((move_lc1) && (!move_lc2))
		    {
		       lc->pos = lc2->pos - lc->size;
		       _e_gadcon_layout_smart_position_items_inside_container(sd, lc);
		    }
		  else
		    {
		       lc2->pos = lc->pos + lc->size;
		       _e_gadcon_layout_smart_position_items_inside_container(sd, lc2);
		    }
	       }
	     else if (lc->state_info.min_seq > lc2->state_info.max_seq)
	       {
		  int move_lc1 = 1;
		  int move_lc2 = 1;

		  EINA_LIST_FOREACH(lc->items, l, bi)
		    {
		       if (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE)
			 {
			    move_lc1 = 0;
			    break;
			 }
		    }
		  EINA_LIST_FOREACH(lc2->items, l, bi)
		    {
		       if (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE)
			 {
			    move_lc2 = 0;
			    break;
			 }
		    }

		  if ((!move_lc1) && (move_lc2))
		    {
		       lc2->pos = lc->pos - lc2->size;
		       _e_gadcon_layout_smart_position_items_inside_container(sd, lc2);
		    }
		  else
		    {
		       lc->pos = lc2->pos + lc2->size;
		       _e_gadcon_layout_smart_position_items_inside_container(sd, lc);
		    }
	       }
	     else if (((lc->state_info.min_seq > lc2->state_info.min_seq) && 
		      (lc->state_info.min_seq < lc2->state_info.max_seq)) || 
		     ((lc2->state_info.min_seq > lc->state_info.min_seq) && 
		      (lc2->state_info.min_seq < lc->state_info.max_seq)))
	       {
		  int shift = 0;

		  _e_gadcon_layout_smart_containers_merge(sd, lc, lc2);

		  EINA_LIST_FOREACH(lc->items, l, bi)
		    {
		       if ((bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_POSITION) &&
			   (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE))
			 {
			    shift = bi->ask.pos - bi->x;
			    break;
			 }
		    }

		  if (shift)
		    {
		       EINA_LIST_FOREACH(lc->items, l, bi)
     			 {
			    bi->x += shift;
			    
     			    if (l == lc->items)
			      lc->pos = bi->x;
     			 }
		    }
	       } 
	     create_new = 1; 
	     new_state = E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED;
	  }
     }

   if (create_new)
     {
	lc3 = E_NEW(E_Layout_Item_Container, 1);
	lc3->sd = sd;
	if (lc->pos < lc2->pos)
	  {
	     lc3->pos = lc->pos;
	     EINA_LIST_FOREACH(lc->items, l, bi)
		lc3->items = eina_list_append(lc3->items, bi);
	     EINA_LIST_FOREACH(lc2->items, l, bi)
		lc3->items = eina_list_append(lc3->items, bi);

	     lc3->state_info.min_seq = lc->state_info.min_seq;
	     if (lc2->items)
	       lc3->state_info.max_seq = lc2->state_info.max_seq;
	     else
	       lc3->state_info.max_seq = lc->state_info.max_seq;
	  }
	else
	  {
	     lc3->pos = lc2->pos;
	     EINA_LIST_FOREACH(lc2->items, l, bi)
	       lc3->items = eina_list_append(lc3->items, bi);
	     EINA_LIST_FOREACH(lc->items, l, bi)
	       lc3->items = eina_list_append(lc3->items, bi);

	     lc3->state_info.min_seq = lc2->state_info.min_seq;
	     if (lc->items)
	       lc3->state_info.max_seq = lc->state_info.max_seq;
	     else
	       lc3->state_info.max_seq = lc2->state_info.max_seq;
	  }
	lc3->size = lc->size + lc2->size;
	lc3->state = new_state;
     }

   return lc3;
} 

static void
_e_gadcon_layout_smart_position_items_inside_container(E_Smart_Data *sd __UNUSED__, E_Layout_Item_Container *lc)
{
   int shift;
   Eina_List *l;
   E_Gadcon_Layout_Item *bi;

   if (!lc->items) return;

   bi = eina_list_data_get(lc->items);
   shift = lc->pos - bi->x;
   
   if (!shift) return;

   EINA_LIST_FOREACH(lc->items, l, bi)
     {
	bi->x += shift;
	
	if ((bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_POS_DEC) ||
	    (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_POS_INC))
	  {
	     bi->gcc->config.pos = bi->ask.pos = bi->x;
	  }
     }
} 

static void
_e_gadcon_layout_smart_containers_merge(E_Smart_Data *sd __UNUSED__, E_Layout_Item_Container *lc, E_Layout_Item_Container *lc2)
{
   int start = 0, size = 0, next = 0, min_seq = 0, max_seq = 0;
   Eina_List *l, *nl = NULL;
   E_Gadcon_Layout_Item *bi;

   EINA_LIST_FOREACH(lc->items, l, bi)
     nl = eina_list_append(nl, bi);
   EINA_LIST_FOREACH(lc2->items, l, bi)
     nl = eina_list_append(nl, bi);

   nl = eina_list_sort(nl, eina_list_count(nl), _e_gadcon_layout_smart_seq_sort_cb);

   EINA_LIST_FOREACH(nl, l, bi)
     {
	if (l == nl)
	  {
	     min_seq = max_seq = bi->gcc->state_info.seq;
	     start = bi->x;
	     size = bi->w;
	     next = bi->x + bi->w;

	     continue;
	  }

	max_seq = bi->gcc->state_info.seq;

	bi->x = next;
	size += bi->w;
	next = bi->x + bi->w;
     }

   lc->items = eina_list_free(lc->items);
   lc2->items = eina_list_free(lc->items);
   lc->items = nl;
   lc->pos = start;
   lc->size = size;
   lc->state_info.min_seq = min_seq;
   lc->state_info.max_seq = max_seq;
   lc2->pos = lc->pos + lc->size;
   lc2->size = 0;
} 

static void
_e_gadcon_layout_smart_restore_gadcons_position_before_move(E_Smart_Data *sd, E_Layout_Item_Container **lc_moving, E_Layout_Item_Container *lc_back, Eina_List **con_list)
{
   int ok;
   Eina_List *l, *l2, *l3;
   E_Gadcon_Layout_Item *bi, *bi2;
   E_Layout_Item_Container *lc, *lc2, *lc3;

   (*lc_moving)->pos = (*lc_moving)->prev_pos;
   if (((*lc_moving)->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MIN_END_INC) || 
       ((*lc_moving)->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MIN_END_DEC) || 
       ((*lc_moving)->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MAX_END_INC) || 
       ((*lc_moving)->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MAX_END_DEC))
     {
	(*lc_moving)->size = (*lc_moving)->prev_size;
	bi = eina_list_data_get((*lc_moving)->items);

	bi->w = (*lc_moving)->prev_size;
     }
   _e_gadcon_layout_smart_position_items_inside_container(sd, (*lc_moving));
   (*lc_moving)->state = E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED;
   _e_gadcon_layout_smart_gadcons_position_static(sd, con_list);

   lc2 = NULL;
   lc3 = NULL;
   ok = 0;
   EINA_LIST_FOREACH(*con_list, l, lc)
     {
	if (lc->state == E_LAYOUT_ITEM_CONTAINER_STATE_NONE) continue;

	if (eina_list_count(lc->items) == 1)
	  {
	     bi = eina_list_data_get(lc->items);
	     if (bi->gcc->state_info.state != E_LAYOUT_ITEM_STATE_NONE)
	       {
		  LC_FREE(lc);
		  l->data = *lc_moving = lc_back;
		  _e_gadcon_layout_smart_position_items_inside_container(sd, (*lc_moving));

		  if (((*lc_moving)->state != E_LAYOUT_ITEM_CONTAINER_STATE_POS_INC) &&
		      ((*lc_moving)->state != E_LAYOUT_ITEM_CONTAINER_STATE_POS_DEC))
		    {
		       bi = eina_list_data_get((*lc_moving)->items);
		       bi->w = (*lc_moving)->size;
		    }
	       }
	  }
	else
	  {
	     EINA_LIST_FOREACH(lc->items, l2, bi)
	       {
		  if (bi->gcc->state_info.state != E_LAYOUT_ITEM_STATE_NONE)
		    {
		       ok = 1;
		       if (l2 != lc->items)
			 {
			    lc2 = E_NEW(E_Layout_Item_Container, 1);
			    lc2->sd = sd;
			    lc2->state = E_LAYOUT_ITEM_CONTAINER_STATE_NONE;
			    EINA_LIST_FOREACH(lc->items, l3, bi2)
			      {
				 if(l2 == l3) break;
				 lc2->items = eina_list_append(lc2->items, bi2);
				 if (l3 == lc->items)
				   {
				      lc2->state_info.min_seq = bi2->gcc->state_info.seq;
				      lc2->pos = lc2->prev_pos = bi2->x;
				   }
				 lc2->state_info.max_seq = bi2->gcc->state_info.seq;
				 E_LAYOUT_ITEM_CONTAINER_SIZE_CHANGE_BY(lc2, bi2, 1);
			      }
			 }

		       if (eina_list_next(l2))
			 {
			    lc3 = E_NEW(E_Layout_Item_Container, 1);
			    lc3->sd = sd;
			    lc3->state = E_LAYOUT_ITEM_CONTAINER_STATE_NONE;
			    EINA_LIST_FOREACH(eina_list_next(l2), l3, bi2)
			      {
				 lc3->items = eina_list_append(lc3->items, bi2);
				 if (l3 == eina_list_next(l2))
				   {
				      lc3->state_info.min_seq = bi2->gcc->state_info.seq;
				      lc3->pos = lc3->prev_pos = bi2->x;
				   }
				 lc3->state_info.max_seq = bi2->gcc->state_info.seq;
				 E_LAYOUT_ITEM_CONTAINER_SIZE_CHANGE_BY(lc3, bi2, 1);
			      }
			 }
		       *lc_moving = lc_back;
		       _e_gadcon_layout_smart_position_items_inside_container(sd, *lc_moving); 

		       if (((*lc_moving)->state != E_LAYOUT_ITEM_CONTAINER_STATE_POS_INC) && 
			   ((*lc_moving)->state != E_LAYOUT_ITEM_CONTAINER_STATE_POS_DEC)) 
			 { 
			    bi = eina_list_data_get((*lc_moving)->items); 
			    bi->w = (*lc_moving)->size; 
			 }
		       break;
		    }
	       }
	     if (ok)
	       {
		  LC_FREE(lc);
		  if (lc2)
		    {
		       l->data = lc2;
		       *con_list = eina_list_append(*con_list, *lc_moving);
		       if (lc3)
			 *con_list = eina_list_append(*con_list, lc3);
		       *con_list = eina_list_sort(*con_list, eina_list_count(*con_list),
						  _e_gadcon_layout_smart_containers_sort_cb);
		    }
		  else
		    {
		       l->data = *lc_moving;
		       if (lc3)
			 {
			    *con_list = eina_list_append(*con_list, lc3);
			    *con_list = eina_list_sort(*con_list, eina_list_count(*con_list),
						       _e_gadcon_layout_smart_containers_sort_cb);
			 }
		    }
		  break;
	       }
	  }
     }

   EINA_LIST_FOREACH(*con_list, l, lc)
     {
	if (lc == *lc_moving) continue;
	lc->state = E_LAYOUT_ITEM_CONTAINER_STATE_NONE;
     }
}

static Eina_Bool
_e_gadcon_custom_populate_idler(void *data __UNUSED__)
{
   const E_Gadcon_Client_Class *cc;
   const Eina_List *l;
   E_Gadcon *gc;

   EINA_LIST_FREE(custom_populate_requests, gc)
     {
	e_gadcon_layout_freeze(gc->o_container);
	EINA_LIST_FOREACH(providers_list, l, cc)
	  {
	     if (gc->populate_class.func)
	       gc->populate_class.func(gc->populate_class.data, gc, cc);
	     else
	       e_gadcon_populate_class(gc, cc);
	  }
	e_gadcon_layout_thaw(gc->o_container);
     }

   custom_populate_idler = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_gadcon_provider_populate_idler(void *data __UNUSED__)
{
   const E_Gadcon_Client_Class *cc;
   const Eina_List *l;
   E_Gadcon *gc;

   EINA_LIST_FOREACH(gadcons, l, gc)
     e_gadcon_layout_freeze(gc->o_container);

   EINA_LIST_FREE(populate_requests, cc)
     {
	EINA_LIST_FOREACH(gadcons, l, gc)
	  {
	     if (gc->populate_class.func)
	       gc->populate_class.func(gc->populate_class.data, gc, cc);
	     else
	       e_gadcon_populate_class(gc, cc);
	  }
     }

   EINA_LIST_FOREACH(gadcons, l, gc)
     e_gadcon_layout_thaw(gc->o_container);

   populate_idler = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_gadcon_provider_populate_request(const E_Gadcon_Client_Class *cc)
{
   if (!populate_idler)
     populate_idler = ecore_idler_add(_e_gadcon_provider_populate_idler, NULL);
   if (!eina_list_data_find(populate_requests, cc))
     populate_requests = eina_list_append(populate_requests, cc);
}

static void
_e_gadcon_provider_populate_unrequest(const E_Gadcon_Client_Class *cc)
{
   populate_requests = eina_list_remove(populate_requests, cc);
   if ((!populate_requests) && (populate_idler))
     {
	ecore_idler_del(populate_idler);
	populate_idler = NULL;
     }
}

/* gadgets movement between different gadcons */

EAPI E_Gadcon_Location *
e_gadcon_location_new(const char * name, 
		      E_Gadcon_Site site,
		      int (*add_func) (void *data, const E_Gadcon_Client_Class *cc),
		      void * add_data,
		      void (*remove_func) (void *data, E_Gadcon_Client *cc),
		      void * remove_data)
{
   E_Gadcon_Location *loc;

   loc  = E_NEW(E_Gadcon_Location, 1);
   loc->name = eina_stringshare_add(name);
   loc->site = site;
   loc->gadget_add.func = add_func; 
   loc->gadget_add.data = add_data; 
   loc->gadget_remove.func = remove_func; 
   loc->gadget_remove.data = remove_data; 
   loc->icon_name = NULL;
   return loc;
}

EAPI void
e_gadcon_location_set_icon_name(E_Gadcon_Location *loc, const char *name)
{
   if (loc->icon_name) eina_stringshare_del(loc->icon_name);
   if (name)
     loc->icon_name = eina_stringshare_add(name);
   else
     loc->icon_name = NULL;
}

EAPI void
e_gadcon_location_free(E_Gadcon_Location *loc)
{
   eina_stringshare_del(loc->name);
   if (loc->icon_name) eina_stringshare_del(loc->icon_name);
   free(loc);
}

EAPI void
e_gadcon_location_register(E_Gadcon_Location * loc)
{
   gadcon_locations = eina_list_append(gadcon_locations, loc);
}

EAPI void
e_gadcon_location_unregister(E_Gadcon_Location * loc)
{
   gadcon_locations = eina_list_remove(gadcon_locations, loc);
}

static int
_e_gadcon_location_change(E_Gadcon_Client * gcc, E_Gadcon_Location *src, E_Gadcon_Location *dst)
{
   E_Gadcon_Client_Class *cc;

   cc = eina_hash_find(providers, gcc->cf->name);
   if (!cc) return 0;
   if (!dst->gadget_add.func(dst->gadget_add.data, cc)) return 0;
   src->gadget_remove.func(src->gadget_remove.data, gcc);
   return 1;
}

