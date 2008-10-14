/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/*
 * TODO: gadcon client ordering on drop
 */

static void _e_gadcon_free(E_Gadcon *gc);
static void _e_gadcon_client_free(E_Gadcon_Client *gcc);

static void _e_gadcon_moveresize_handle(E_Gadcon_Client *gcc);
static int _e_gadcon_cb_client_scroll_timer(void *data);
static int _e_gadcon_cb_client_scroll_animator(void *data);
static void _e_gadcon_cb_client_frame_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_client_frame_moveresize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_gadcon_client_save(E_Gadcon_Client *gcc);
static void _e_gadcon_client_drag_begin(E_Gadcon_Client *gcc, int x, int y);
static void _e_gadcon_client_inject(E_Gadcon *gc, E_Gadcon_Client *gcc, int x, int y);

static void _e_gadcon_cb_min_size_request(void *data, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_size_request(void *data, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_moveresize(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_client_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_client_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info);
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
static void _e_gadcon_client_cb_menu_resizable(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_gadcon_client_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_gadcon_client_cb_menu_remove(void *data, E_Menu *m, E_Menu_Item *mi);

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

/********************/
#define E_LAYOUT_ITEM_DRAG_RESIST_LEVEL 10

typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Layout_Item_Container E_Layout_Item_Container;

static void _e_gadcon_client_current_position_sync(E_Gadcon_Client *gcc);
static void _e_gadcon_layout_smart_sync_clients(E_Gadcon *gc);
static void _e_gadcon_layout_smart_gadcon_position_shrinked_mode(E_Smart_Data *sd);
static void _e_gadcon_layout_smart_gadcons_asked_position_set(E_Smart_Data *sd);
static Evas_List *_e_gadcon_layout_smart_gadcons_wrap(E_Smart_Data *sd);
static void _e_gadcon_layout_smart_gadcons_position(E_Smart_Data *sd, Evas_List **list);
static void _e_gadcon_layout_smart_gadcons_position_static(E_Smart_Data *sd, Evas_List **list);
static E_Layout_Item_Container * _e_gadcon_layout_smart_containers_position_adjust(E_Smart_Data *sd, E_Layout_Item_Container *lc, E_Layout_Item_Container *lc2);
static void _e_gadcon_layout_smart_position_items_inside_container(E_Smart_Data *sd, E_Layout_Item_Container *lc);
static void _e_gadcon_layout_smart_containers_merge(E_Smart_Data *sd, E_Layout_Item_Container *lc, E_Layout_Item_Container *lc2);
static void _e_gadcon_layout_smart_restore_gadcons_position_before_move(E_Smart_Data *sd, E_Layout_Item_Container **lc_moving, E_Layout_Item_Container *lc_back, Evas_List **con_list);

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

   struct {
      int min_seq, max_seq;
   } state_info;

   E_Smart_Data *sd;
   Evas_List *items;

   E_Layout_Item_Container_State state;
};

#define LC_FREE(__lc) \
   if (__lc->items) \
     evas_list_free(__lc->items); \
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
   if (__lc->sd->horizontal) \
     { \
	if (__increase) \
	  __lc->size += __bi->w; \
	else \
	  __lc->size -= __bi->w; \
     } \
   else \
     { \
	if (__increase) \
	  __lc->size += __bi->h; \
	else \
	  __lc->size -= __bi->h; \
     }
/********************/

static Evas_Hash *providers = NULL;
static Evas_List *providers_list = NULL;
static Evas_List *gadcons = NULL;

/* This is the gadcon client which is currently dragged */
static E_Gadcon_Client *drag_gcc = NULL;
/* This is the gadcon client created on entering a new shelf */
static E_Gadcon_Client *new_gcc = NULL;

/* externally accessible functions */
EAPI int
e_gadcon_init(void)
{
   return 1;
}

EAPI int
e_gadcon_shutdown(void)
{
   return 1;
}

EAPI void
e_gadcon_provider_register(const E_Gadcon_Client_Class *cc)
{
   Evas_List *l;
   E_Gadcon *gc;
   
   providers = evas_hash_direct_add(providers, cc->name, cc);
   providers_list = evas_list_append(providers_list, cc);
   for (l = gadcons; l; l = l->next)
     {
	gc = l->data;
	e_gadcon_populate_class(gc, cc);
     }
}

EAPI void
e_gadcon_provider_unregister(const E_Gadcon_Client_Class *cc)
{
   Evas_List *l, *ll, *dlist = NULL;
   E_Gadcon *gc;
   E_Gadcon_Client *gcc;
   
   for (l = gadcons; l; l = l->next)
     {
	gc = l->data;
	for (ll = gc->clients; ll; ll = ll->next)
	  {
	     gcc = ll->data;
	     if (gcc->client_class == cc)
	       dlist = evas_list_append(dlist, gcc);
	  }
     }
   while (dlist)
     {
	gcc = dlist->data;
	dlist = evas_list_remove_list(dlist, dlist);
	e_object_del(E_OBJECT(gcc));
     }
   providers = evas_hash_del(providers, cc->name, cc);
   providers_list = evas_list_remove(providers_list, cc);
}

EAPI Evas_List *
e_gadcon_provider_list(void)
{
   return providers_list;
}

EAPI E_Gadcon *
e_gadcon_swallowed_new(const char *name, int id, Evas_Object *obj, char *swallow_name)
{
   E_Gadcon    *gc;
   Evas_List   *l;
   Evas_Coord   x, y, w, h;
   const char  *drop_types[] = { "enlightenment/gadcon_client" };
   
   gc = E_OBJECT_ALLOC(E_Gadcon, E_GADCON_TYPE, _e_gadcon_free);
   if (!gc) return NULL;
 
   gc->name = evas_stringshare_add(name);
   gc->id = id;
   gc->layout_policy = E_GADCON_LAYOUT_POLICY_PANEL;
   
   gc->edje.o_parent = obj;
   gc->edje.swallow_name = evas_stringshare_add(swallow_name);

   gc->orient = E_GADCON_ORIENT_HORIZ;
   gc->evas = evas_object_evas_get(obj);
   gc->o_container = e_gadcon_layout_add(gc->evas);
   evas_object_geometry_get(gc->o_container, &x, &y, &w, &h);
   gc->drop_handler = e_drop_handler_add(E_OBJECT(gc), gc,
					 _e_gadcon_cb_dnd_enter, _e_gadcon_cb_dnd_move,
					 _e_gadcon_cb_dnd_leave, _e_gadcon_cb_drop,
					 drop_types, 1,
					 x, y, w, h);
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
   gadcons = evas_list_append(gadcons, gc);

   for (l = e_config->gadcons; l; l = l->next)
     {
	E_Config_Gadcon *cf_gc;

	cf_gc = l->data;
	if ((!strcmp(cf_gc->name, gc->name)) &&
	    (cf_gc->id == gc->id))
	  {
	     gc->cf = cf_gc;
	     break;
	  }
     }
   if (!gc->cf)
     {
	gc->cf = E_NEW(E_Config_Gadcon, 1);
	gc->cf->name = evas_stringshare_add(gc->name);
	gc->cf->id = gc->id;
	e_config->gadcons = evas_list_append(e_config->gadcons, gc->cf);
	e_config_save_queue();
     }
   return gc;
}

EAPI void
e_gadcon_swallowed_min_size_set(E_Gadcon *gc, Evas_Coord w, Evas_Coord h)
{
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
   Evas_List *l;
   
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   e_gadcon_layout_freeze(gc->o_container);
   for (l = gc->cf->clients; l; l = l->next)
     {
	E_Config_Gadcon_Client *cf_gcc;
	E_Gadcon_Client_Class *cc;

	cf_gcc = l->data;
	if (!cf_gcc->name) continue;
	cc = evas_hash_find(providers, cf_gcc->name);
	if (cc)
	  {
	     E_Gadcon_Client *gcc;

	     if ((!cf_gcc->id) &&
		 (_e_gadcon_client_class_feature_check(cc, "id_new", cc->func.id_new)))
	       cf_gcc->id = evas_stringshare_add(cc->func.id_new());

	     if (!cf_gcc->style)
	       {
		  gcc = cc->func.init(gc, cf_gcc->name, cf_gcc->id,
			cc->default_style);
	       }
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
		  
		  e_gadcon_client_autoscroll_set(gcc, cf_gcc->autoscroll);
		  e_gadcon_client_resizable_set(gcc, cf_gcc->resizable);
		  if (gcc->client_class->func.orient)
		    gcc->client_class->func.orient(gcc);

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
   while (gc->clients) 
     {
	gcc = gc->clients->data;
	if (gcc->menu) 
	  {
	     e_menu_post_deactivate_callback_set(gcc->menu, NULL, NULL);
	     e_object_del(E_OBJECT(gcc->menu));
	     gcc->menu = NULL;
	  }
	gc->clients = evas_list_remove_list(gc->clients, gc->clients);
	e_object_del(E_OBJECT(gcc));
     }
}

EAPI void
e_gadcon_populate_class(E_Gadcon *gc, const E_Gadcon_Client_Class *cc)
{
   Evas_List *l;
   
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   e_gadcon_layout_freeze(gc->o_container);
   for (l = gc->cf->clients; l; l = l->next)
     {
	E_Config_Gadcon_Client *cf_gcc;

	cf_gcc = l->data;
	if ((cf_gcc->name) && (cc->name) && 
	    (!strcmp(cf_gcc->name, cc->name)) &&
	    (cf_gcc->id) && (cf_gcc->style))
	  {
	     E_Gadcon_Client *gcc;

	     if ((!cf_gcc->id) &&
		 (_e_gadcon_client_class_feature_check(cc, "id_new", cc->func.id_new)))
	       cf_gcc->id = evas_stringshare_add(cc->func.id_new());

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

		  if (gcc->client_class->func.orient)
		    gcc->client_class->func.orient(gcc); 

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
   Evas_List *l;
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
   for (l = gc->clients; l; l = l->next)
     {
	E_Gadcon_Client *gcc;
	
	gcc = l->data;
	e_box_orientation_set(gcc->o_box, horiz);
	if (gcc->client_class->func.orient)
	  gcc->client_class->func.orient(gcc);
     }
   e_gadcon_layout_thaw(gc->o_container);
}

EAPI void
e_gadcon_edit_begin(E_Gadcon *gc)
{
   Evas_List *l;
   
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   e_gadcon_layout_freeze(gc->o_container);
   if (gc->shelf) e_shelf_locked_set(gc->shelf, 1);
   gc->editing = 1;
   for (l = gc->clients; l; l = l->next)
     {
	E_Gadcon_Client *gcc;
	
	gcc = l->data;
	e_gadcon_client_edit_begin(gcc);
     }
   e_gadcon_layout_thaw(gc->o_container);
}

EAPI void
e_gadcon_edit_end(E_Gadcon *gc)
{
   Evas_List *l;
   
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   e_gadcon_layout_freeze(gc->o_container);
   gc->editing = 0;
   for (l = gc->clients; l; l = l->next)
     {
	E_Gadcon_Client *gcc;
	
	gcc = l->data;
	e_gadcon_client_edit_end(gcc);
     }
   e_gadcon_layout_thaw(gc->o_container);
   if (gc->shelf) e_shelf_locked_set(gc->shelf, 0);
}

EAPI void
e_gadcon_all_edit_begin(void)
{
   Evas_List *l;
   
   for (l = gadcons; l; l = l->next)
     {
	E_Gadcon *gc;
	
	gc = l->data;
	e_gadcon_edit_begin(gc);
     }
}

EAPI void
e_gadcon_all_edit_end(void)
{
   Evas_List *l;
   
   for (l = gadcons; l; l = l->next)
     {
	E_Gadcon *gc;
	
	gc = l->data;
	e_gadcon_edit_end(gc);
     }
}

EAPI void
e_gadcon_zone_set(E_Gadcon *gc, E_Zone *zone)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   gc->zone = zone;
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
   if (gc->zone)
     {
	x -= gc->zone->x;
	y -= gc->zone->y;
     }
   return 1;
}

EAPI void
e_gadcon_util_menu_attach_func_set(E_Gadcon *gc, 
				   void (*func) (void *data, E_Gadcon_Client *gcc, E_Menu *menu),
				   void *data)
{
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   gc->menu_attach.func = func;
   gc->menu_attach.data = data;
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
   E_Gadcon_Client_Class  *cc;
   E_Config_Gadcon_Client *cf_gcc;

   E_OBJECT_CHECK_RETURN(gc, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(gc, E_GADCON_TYPE, NULL);
   if (!name) return NULL;

   cc = evas_hash_find(providers, name);
   if (!cc) return NULL;
   if (!_e_gadcon_client_class_feature_check(cc, "id_new", cc->func.id_new)) return NULL;

   cf_gcc = E_NEW(E_Config_Gadcon_Client, 1);
   if (!cf_gcc) return NULL;
   cf_gcc->name = evas_stringshare_add(name);
   cf_gcc->id = evas_stringshare_add(cc->func.id_new());
   cf_gcc->geom.res = 800;
   cf_gcc->geom.size = 80;
   cf_gcc->geom.pos = cf_gcc->geom.res - cf_gcc->geom.size;
   cf_gcc->style = NULL;
   cf_gcc->autoscroll = 0;
   cf_gcc->resizable = 0;
   gc->cf->clients = evas_list_append(gc->cf->clients, cf_gcc);
   e_config_save_queue();
   return cf_gcc;
}

EAPI void
e_gadcon_client_config_del(E_Config_Gadcon *cf_gc, E_Config_Gadcon_Client *cf_gcc)
{
   if (!cf_gcc) return;

   if (cf_gcc->name) evas_stringshare_del(cf_gcc->name);
   if (cf_gcc->id) evas_stringshare_del(cf_gcc->id);
   if (cf_gcc->style) evas_stringshare_del(cf_gcc->style);
   if (cf_gc) cf_gc->clients = evas_list_remove(cf_gc->clients, cf_gcc);
   free(cf_gcc);
}

EAPI E_Gadcon_Client *
e_gadcon_client_new(E_Gadcon *gc, const char *name, const char *id, const char *style, Evas_Object *base_obj)
{
   E_Gadcon_Client *gcc;
   
   E_OBJECT_CHECK_RETURN(gc, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(gc, E_GADCON_TYPE, NULL);
   gcc = E_OBJECT_ALLOC(E_Gadcon_Client, E_GADCON_CLIENT_TYPE, _e_gadcon_client_free);
   if (!gcc) return NULL;
   gcc->name = evas_stringshare_add(name);
   gcc->gadcon = gc;
   gcc->o_base = base_obj;
   if (gc->clients)
     gcc->id = E_GADCON_CLIENT(evas_list_data(evas_list_last(gc->clients)))->id + 1;
   gc->clients = evas_list_append(gc->clients, gcc);
   /* This must only be unique during runtime */
   if (gcc->o_base)
     evas_object_event_callback_add(gcc->o_base, EVAS_CALLBACK_DEL,
				    _e_gadcon_client_del_hook, gcc);
   if ((gc->frame_request.func) && (style))
     {
	gcc->o_frame = gc->frame_request.func(gc->frame_request.data, gcc, style);
	gcc->style = evas_stringshare_add(style);
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

   if (gcc->gadcon->shelf) e_shelf_locked_set(gcc->gadcon->shelf, 1);
   gcc->gadcon->editing = 1;
   gcc->o_control = edje_object_add(gcc->gadcon->evas);
   evas_object_layer_set(gcc->o_control, 100);
   if (gcc->o_frame)
     evas_object_geometry_get(gcc->o_frame, &x, &y, &w, &h);
   else if (gcc->o_base)
     evas_object_geometry_get(gcc->o_base, &x, &y, &w, &h);
   evas_object_move(gcc->o_control, x, y);
   evas_object_resize(gcc->o_control, w, h);
   e_theme_edje_object_set(gcc->o_control, "base/theme/gadman", "e/gadman/control");

   if ((gcc->autoscroll) || (gcc->resizable))
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
   
   evas_object_event_callback_add(gcc->o_event, EVAS_CALLBACK_MOUSE_DOWN, _e_gadcon_cb_client_mouse_down, gcc);
   evas_object_event_callback_add(gcc->o_event, EVAS_CALLBACK_MOUSE_UP, _e_gadcon_cb_client_mouse_up, gcc);
   evas_object_event_callback_add(gcc->o_event, EVAS_CALLBACK_MOUSE_IN, _e_gadcon_cb_client_mouse_in, gcc);
   evas_object_event_callback_add(gcc->o_event, EVAS_CALLBACK_MOUSE_OUT, _e_gadcon_cb_client_mouse_out, gcc);

   if (gcc->o_frame)
     {
	evas_object_event_callback_add(gcc->o_frame, EVAS_CALLBACK_MOVE, _e_gadcon_cb_client_move, gcc);
	evas_object_event_callback_add(gcc->o_frame, EVAS_CALLBACK_RESIZE, _e_gadcon_cb_client_resize, gcc);
     }
   else if (gcc->o_base)
     {
	evas_object_event_callback_add(gcc->o_base, EVAS_CALLBACK_MOVE, _e_gadcon_cb_client_move, gcc);
	evas_object_event_callback_add(gcc->o_base, EVAS_CALLBACK_RESIZE, _e_gadcon_cb_client_resize, gcc);
     }
   
   evas_object_show(gcc->o_event);
   evas_object_show(gcc->o_control);
}

EAPI void
e_gadcon_client_edit_end(E_Gadcon_Client *gcc)
{
   Evas_List *l = NULL;

   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);
   
   if (gcc->o_frame)
     {
	evas_object_event_callback_del(gcc->o_frame, EVAS_CALLBACK_MOVE, _e_gadcon_cb_client_move);
	evas_object_event_callback_del(gcc->o_frame, EVAS_CALLBACK_RESIZE, _e_gadcon_cb_client_resize);
     }
   else if (gcc->o_base)
     {
	evas_object_event_callback_del(gcc->o_base, EVAS_CALLBACK_MOVE, _e_gadcon_cb_client_move);
	evas_object_event_callback_del(gcc->o_base, EVAS_CALLBACK_RESIZE, _e_gadcon_cb_client_resize);
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
   
   for (l = gcc->gadcon->clients; l; l = l->next) 
     {
	E_Gadcon_Client *client = NULL;
	
	client = l->data;
	if (!client) continue;
	if (client->o_control) return;
     }
   gcc->gadcon->editing = 0;
   if (gcc->gadcon->shelf) e_shelf_locked_set(gcc->gadcon->shelf, 0);
}

EAPI void
e_gadcon_client_show(E_Gadcon_Client *gcc)
{
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
   if ((!gcc->autoscroll) && (!gcc->resizable))
     {
	if (gcc->o_frame)
	  e_gadcon_layout_pack_min_size_set(gcc->o_frame, w + gcc->pad.w, h + gcc->pad.h);
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
   if ((!gcc->autoscroll) && (!gcc->resizable))
     {
	if (gcc->o_frame)
	  {
	     e_gadcon_layout_pack_aspect_pad_set(gcc->o_frame, gcc->pad.w, gcc->pad.h);
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
   if (gcc->autoscroll)
     {
	if (gcc->o_frame)
	  {
	     e_gadcon_layout_pack_aspect_pad_set(gcc->o_frame, gcc->pad.w, gcc->pad.h);
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
     {
	if (gcc->o_frame)
	  {
	     e_gadcon_layout_pack_aspect_pad_set(gcc->o_frame, gcc->pad.w, gcc->pad.h);
	     e_gadcon_layout_pack_aspect_set(gcc->o_frame, gcc->aspect.w, gcc->aspect.h);
	     e_gadcon_layout_pack_min_size_set(gcc->o_frame, gcc->min.w, gcc->min.h);
	  }
	else if (gcc->o_base)
	  {
	     e_gadcon_layout_pack_min_size_set(gcc->o_base, gcc->min.w, gcc->min.h);
	     e_gadcon_layout_pack_aspect_set(gcc->o_base, gcc->aspect.w, gcc->aspect.h);
	  }
     }
}
    
EAPI void
e_gadcon_client_resizable_set(E_Gadcon_Client *gcc, int resizable)
{
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);
   
   gcc->resizable = resizable;
   if (gcc->resizable)
     {
	if (gcc->o_frame)
	  {
	     e_gadcon_layout_pack_aspect_pad_set(gcc->o_frame, gcc->pad.w, gcc->pad.h);
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
	     e_gadcon_layout_pack_aspect_pad_set(gcc->o_frame, gcc->pad.w, gcc->pad.h);
	     e_gadcon_layout_pack_aspect_set(gcc->o_frame, gcc->aspect.w, gcc->aspect.h);
	     e_gadcon_layout_pack_min_size_set(gcc->o_frame, gcc->min.w, gcc->min.h);
	  }
	else if (gcc->o_base)
	  {
	     e_gadcon_layout_pack_min_size_set(gcc->o_base, gcc->min.w, gcc->min.h);
	     e_gadcon_layout_pack_aspect_set(gcc->o_base, gcc->aspect.w, gcc->aspect.h);
	  }
     }
}

EAPI int
e_gadcon_client_geometry_get(E_Gadcon_Client *gcc, int *x, int *y, int *w, int *h)
{
   int gx = 0, gy = 0;

   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);
   if (!e_gadcon_canvas_zone_geometry_get(gcc->gadcon, &gx, &gy, NULL, NULL)) return 0;
   
   if (gcc->o_base)
     evas_object_geometry_get(gcc->o_base, x, y, w, h);
   if (x) *x += gx;
   if (y) *y += gy;

   return 1;
}

EAPI void
e_gadcon_client_util_menu_items_append(E_Gadcon_Client *gcc, E_Menu *menu, int flags)
{
   E_Menu *mn;
   E_Menu_Item *mi;
   
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);

   /*
   if (gcc->gadcon->shelf) 
     e_shelf_locked_set(gcc->gadcon->shelf, 1);
   e_menu_post_deactivate_callback_set(menu, _e_gadcon_client_cb_menu_post, gcc);
   gcc->menu = menu;
   */

   if (gcc->gadcon->shelf) 
     {
	mn = e_menu_new();
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Plain"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/plain");
	e_menu_item_radio_group_set(mi, 1);
	e_menu_item_radio_set(mi, 1);
	if ((gcc->style) && (!strcmp(gcc->style, E_GADCON_CLIENT_STYLE_PLAIN)))
	  e_menu_item_toggle_set(mi, 1);
	e_menu_item_callback_set(mi, _e_gadcon_client_cb_menu_style_plain, gcc);
   
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Inset"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/inset");
	e_menu_item_radio_group_set(mi, 1);
	e_menu_item_radio_set(mi, 1);
	if ((gcc->style) && (!strcmp(gcc->style, E_GADCON_CLIENT_STYLE_INSET)))
	  e_menu_item_toggle_set(mi, 1);
	e_menu_item_callback_set(mi, _e_gadcon_client_cb_menu_style_inset, gcc);

	mi = e_menu_item_new(menu);
	e_menu_item_label_set(mi, _("Appearance"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/appearance");
	e_menu_item_submenu_set(mi, mn);
	e_object_del(E_OBJECT(mn));
     }

   if ((gcc->gadcon->shelf) || (gcc->gadcon->toolbar))
     {
	mi = e_menu_item_new(menu);
	e_menu_item_label_set(mi, _("Automatically scroll contents"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/autoscroll");
	e_menu_item_check_set(mi, 1);
	if (gcc->autoscroll) e_menu_item_toggle_set(mi, 1);
	e_menu_item_callback_set(mi, _e_gadcon_client_cb_menu_autoscroll, gcc);

	mi = e_menu_item_new(menu);
	e_menu_item_label_set(mi, _("Able to be resized"));
	e_util_menu_item_edje_icon_set(mi, "widget/resize");
	e_menu_item_check_set(mi, 1);
	if (gcc->resizable) e_menu_item_toggle_set(mi, 1);
	e_menu_item_callback_set(mi, _e_gadcon_client_cb_menu_resizable, gcc);

	mi = e_menu_item_new(menu);
	e_menu_item_separator_set(mi, 1);
   
	if (!gcc->o_control) 
	  {
		mi = e_menu_item_new(menu);
		e_menu_item_label_set(mi, _("Begin move/resize this gadget"));
		e_util_menu_item_edje_icon_set(mi, "widget/resize");
		e_menu_item_callback_set(mi, _e_gadcon_client_cb_menu_edit, gcc);
	  }

	mi = e_menu_item_new(menu);
	e_menu_item_label_set(mi, _("Remove this gadget"));
	e_util_menu_item_edje_icon_set(mi, "widget/del");
	e_menu_item_callback_set(mi, _e_gadcon_client_cb_menu_remove, gcc);
     }
   if (gcc->gadcon->menu_attach.func)
     {
	mi = e_menu_item_new(menu);
	e_menu_item_separator_set(mi, 1);
	
	gcc->gadcon->menu_attach.func(gcc->gadcon->menu_attach.data, gcc, menu);
     }
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

/*
 * NOTE: x & y are relative to the o_box of the gadcon.
 */
EAPI void
e_gadcon_client_autoscroll_update(E_Gadcon_Client *gcc, Evas_Coord x, Evas_Coord y)
{
   if (gcc->autoscroll)
     {
	Evas_Coord w, h;
	double d;
	
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
   gcc->scroll_cb.func = func;
   gcc->scroll_cb.data = data;
}

/* local subsystem functions */
static void
_e_gadcon_free(E_Gadcon *gc)
{
   e_gadcon_unpopulate(gc);
   gadcons = evas_list_remove(gadcons, gc);
   if (gc->o_container) evas_object_del(gc->o_container);
   evas_stringshare_del(gc->name);
   evas_stringshare_del(gc->edje.swallow_name);
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
     evas_object_event_callback_del(gcc->o_base,
				    EVAS_CALLBACK_DEL,
				    _e_gadcon_client_del_hook);
   if (gcc->menu)
     {
        e_menu_post_deactivate_callback_set(gcc->menu, NULL, NULL);
	e_object_del(E_OBJECT(gcc->menu));
	gcc->menu = NULL;
     }
   e_gadcon_client_edit_end(gcc);
   gcc->client_class->func.shutdown(gcc);
   if (gcc->client_class->func.id_del)
     gcc->client_class->func.id_del(gcc->cf->id);
   gcc->gadcon->clients = evas_list_remove(gcc->gadcon->clients, gcc);
   if (gcc->o_box) evas_object_del(gcc->o_box);
   if (gcc->o_frame) evas_object_del(gcc->o_frame);
   evas_stringshare_del(gcc->name);
   if (gcc->scroll_timer) ecore_timer_del(gcc->scroll_timer);
   if (gcc->scroll_animator) ecore_animator_del(gcc->scroll_animator);
   if (gcc->style) evas_stringshare_del(gcc->style);
   free(gcc);
}

static void
_e_gadcon_moveresize_handle(E_Gadcon_Client *gcc)
{
   Evas_Coord x, y, w, h;
   
   evas_object_geometry_get(gcc->o_box, &x, &y, &w, &h);
   if ((gcc->autoscroll) || (gcc->resizable))
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
   if (gcc->o_base)
     e_box_pack_options_set(gcc->o_base,
			    1, 1, /* fill */
			    1, 1, /* expand */
			    0.5, 0.5, /* align */
			    w, h, /* min */
			    w, h /* max */
			    );
}

static int
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
	return 0;
     }
   v = 0.05;
   gcc->scroll_pos = (gcc->scroll_pos * (1.0 - v)) + (gcc->scroll_wanted * v);
   return 1;
}

static int
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
	return 0;
     }

   if (gcc->scroll_cb.func)
     gcc->scroll_cb.func(gcc->scroll_cb.data);

   return 1;
}

static void
_e_gadcon_cb_client_frame_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
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
_e_gadcon_cb_client_frame_moveresize(void *data, Evas *e, Evas_Object *obj, void *event_info)
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
   if (gcc->cf->style) evas_stringshare_del(gcc->cf->style);
   gcc->cf->style = NULL;
   if (gcc->style)
     gcc->cf->style = evas_stringshare_add(gcc->style);
   gcc->cf->resizable = gcc->resizable;
   e_config_save_queue();
}

static void
_e_gadcon_client_drag_begin(E_Gadcon_Client *gcc, int x, int y)
{
   E_Drag *drag;
   Evas_Object *o = NULL;
   Evas_Coord w, h;
   const char *drag_types[] = { "enlightenment/gadcon_client" };

   drag_gcc = gcc;

   e_object_ref(E_OBJECT(gcc));
   /* Remove this config from the current gadcon */
   gcc->gadcon->cf->clients = 
     evas_list_remove(gcc->gadcon->cf->clients, gcc->cf);
   gcc->state_info.state = E_LAYOUT_ITEM_STATE_NONE;
   gcc->state_info.resist = 0;

   if (!e_drop_inside(gcc->gadcon->drop_handler, x, y))
     e_gadcon_client_hide(gcc);

   if ((gcc->gadcon->zone) && (gcc->gadcon->zone->container))
     {
	drag = e_drag_new(gcc->gadcon->zone->container, gcc->drag.x, gcc->drag.y,
			  drag_types, 1, gcc, -1, NULL, 
			  _e_gadcon_cb_drag_finished);
	if (drag) 
	  {
	     o = gcc->client_class->func.icon(drag->evas);
	     evas_object_geometry_get(o, NULL, NULL, &w, &h);
	     if (!o)
	       {
		  /* FIXME: fallback icon for drag */
		  o = evas_object_rectangle_add(drag->evas);
		  evas_object_color_set(o, 255, 255, 255, 255);
	       }
	     e_drag_object_set(drag, o);
	     e_drag_move(drag, gcc->drag.x - w/2, gcc->drag.y - h/2);
	     e_drag_resize(drag, w, h);
	     e_drag_start(drag, gcc->drag.x, gcc->drag.y);
	  }
     }
}

static void
_e_gadcon_client_inject(E_Gadcon *gc, E_Gadcon_Client *gcc, int x, int y)
{
   Evas_List  *l;
   Evas_Coord  cx, cy, cw, ch;
   int         seq = 1;

   /* Check if the gadcon client is in place */
   if (!gcc->hidden)
     {
	if (gcc->o_frame)
	  evas_object_geometry_get(gcc->o_frame, &cx, &cy, &cw, &ch);
	else if (gcc->o_base)
	  evas_object_geometry_get(gcc->o_base, &cx, &cy, &cw, &ch);
	if (E_INSIDE(x, y, cx, cy, cw, ch)) return;
     }

   /* If x, y is not inside any gadcon client, seq will be 0 and it's position
    * will later be used for placement. */
   gcc->state_info.seq = 0;
   for (l = gc->clients; l; l = l->next)
     {
	E_Gadcon_Client *gcc2;

	gcc2 = l->data;
	if (gcc == gcc2) continue;
	if (gcc2->hidden) continue;
	if (gcc2->o_frame)
	  evas_object_geometry_get(gcc2->o_frame, &cx, &cy, &cw, &ch);
	else if (gcc2->o_base)
	  evas_object_geometry_get(gcc2->o_base, &cx, &cy, &cw, &ch);
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
_e_gadcon_cb_min_size_request(void *data, Evas_Object *obj, void *event_info)
{
   E_Gadcon *gc;
   
   gc = data;
   if (gc->min_size_request.func)
     {
	Evas_Coord w, h;
	
	e_gadcon_layout_min_size_get(gc->o_container, &w, &h);
	gc->min_size_request.func(gc->min_size_request.data, gc, w, h);
     }
}

static void
_e_gadcon_cb_size_request(void *data, Evas_Object *obj, void *event_info)
{
   E_Gadcon   *gc;
 
   gc = data;
   if (gc->resize_request.func)
     {
	Evas_Coord w, h;

	e_gadcon_layout_asked_size_get(gc->o_container, &w, &h);
	gc->resize_request.func(gc->resize_request.data, gc, w, h);
     }
}

static void
_e_gadcon_cb_moveresize(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Gadcon *gc;
   Evas_Coord x, y, w, h;

   gc = data;
   
   evas_object_geometry_get(gc->o_container, &x, &y, &w, &h);
   if (gc->drop_handler) 
     e_drop_handler_geometry_set(gc->drop_handler, x, y, w, h);
}

static void
_e_gadcon_cb_client_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info)
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

	if (gcc->gadcon->shelf)
	  e_shelf_locked_set(gcc->gadcon->shelf, 1);
	mn = e_menu_new();
	e_menu_post_deactivate_callback_set(mn, _e_gadcon_client_cb_menu_post,
					    gcc);
	gcc->menu = mn;

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Stop move/resize this gadget"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/edit");
	e_menu_item_callback_set(mi, _e_gadcon_client_cb_menu_edit, gcc);

	if (gcc->gadcon->menu_attach.func)
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_separator_set(mi, 1);

	     gcc->gadcon->menu_attach.func(gcc->gadcon->menu_attach.data, gcc, mn);
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
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
     }
}

static void
_e_gadcon_cb_client_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   E_Gadcon_Client *gcc;
   
   gcc = data;
   ev = event_info;
}

static void
_e_gadcon_cb_client_mouse_in(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_In *ev;
   E_Gadcon_Client *gcc;
   
   gcc = data;
   ev = event_info;
   edje_object_signal_emit(gcc->o_control, "e,state,focused", "e");
}

static void
_e_gadcon_cb_client_mouse_out(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Out *ev;
   E_Gadcon_Client *gcc;
   
   gcc = data;
   ev = event_info;
   edje_object_signal_emit(gcc->o_control, "e,state,unfocused", "e");
}

static void
_e_gadcon_cb_client_move(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Gadcon_Client *gcc;
   Evas_Coord x, y;
   
   gcc = data;
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   if (gcc->o_control) evas_object_move(gcc->o_control, x, y);
   if (gcc->o_event) evas_object_move(gcc->o_event, x, y);
}

static void
_e_gadcon_cb_client_resize(void *data, Evas *evas, Evas_Object *obj, void *event_info)
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
   evas_object_raise(gcc->o_event);
   evas_object_stack_below(gcc->o_control, gcc->o_event);
   gcc->moving = 1;
   evas_pointer_canvas_xy_get(gcc->gadcon->evas, &gcc->dx, &gcc->dy);
   gcc->drag.x = gcc->dx;
   gcc->drag.y = gcc->dy;
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
   int        cx, cy;
   
   if (!gcc->moving) return;
   evas_pointer_canvas_xy_get(gcc->gadcon->evas, &cx, &cy);
   x = cx - gcc->dx;
   y = cy - gcc->dy; 
   
   gcc->state_info.flags = E_GADCON_LAYOUT_ITEM_LOCK_POSITION | E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;
   _e_gadcon_client_current_position_sync(gcc);
   if (e_gadcon_layout_orientation_get(gcc->gadcon->o_container))
     {
/*	if (abs(cy - gcc->drag.y) > e_config->drag_resist)
	  {
	     _e_gadcon_client_drag_begin(gcc, cx, cy);
	     return;
	  }
	else*/ if (x > 0)
	  {
	     if (gcc->state_info.state != E_LAYOUT_ITEM_STATE_POS_INC)
	       gcc->state_info.resist = 0;
	     gcc->state_info.state = E_LAYOUT_ITEM_STATE_POS_INC;
	  }
	else if (x < 0)
	  {
	     if (gcc->state_info.state != E_LAYOUT_ITEM_STATE_POS_DEC)
	       gcc->state_info.resist = 0; 
	     gcc->state_info.state = E_LAYOUT_ITEM_STATE_POS_DEC;
	  }
     }
   else
     {
/*	if (abs(cx - gcc->drag.x) > e_config->drag_resist)
	  {
	     _e_gadcon_client_drag_begin(gcc, cx, cy);
	     return;
	  }
	else*/ if (y > 0)
	  {
	     if (gcc->state_info.state != E_LAYOUT_ITEM_STATE_POS_INC)
	       gcc->state_info.resist = 0;
	     gcc->state_info.state = E_LAYOUT_ITEM_STATE_POS_INC;
	  }
	else if (y < 0)
	  {
	     if (gcc->state_info.state != E_LAYOUT_ITEM_STATE_POS_DEC)
	       gcc->state_info.resist = 0;
	     gcc->state_info.state = E_LAYOUT_ITEM_STATE_POS_DEC;
	  }
     }


   if (gcc->o_frame)
     evas_object_geometry_get(gcc->o_frame, NULL, NULL, &w, &h);
   else if (gcc->o_base)
     evas_object_geometry_get(gcc->o_base, NULL, NULL, &w, &h);

   if (e_gadcon_layout_orientation_get(gcc->gadcon->o_container))
     {
	if (gcc->o_frame)
	  e_gadcon_layout_pack_request_set(gcc->o_frame, gcc->config.pos + x, w);
	else if (gcc->o_base)
	  e_gadcon_layout_pack_request_set(gcc->o_base, gcc->config.pos + x, w);

	gcc->config.size = w;
	evas_object_geometry_get(gcc->gadcon->o_container, NULL, NULL, &w, &h);
	gcc->config.res = w;
     }
   else
     {
	if (gcc->o_frame)
	  e_gadcon_layout_pack_request_set(gcc->o_frame, gcc->config.pos + y, h);
	else if (gcc->o_base)
	  e_gadcon_layout_pack_request_set(gcc->o_base, gcc->config.pos + y, h);

	gcc->config.size = h;
	evas_object_geometry_get(gcc->gadcon->o_container, NULL, NULL, &w, &h);
	gcc->config.res = h;
     }
   gcc->dx += x;
   gcc->dy += y;
}

static void
_e_gadcon_cb_signal_move_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   _e_gadcon_client_move_start(data);
}

static void
_e_gadcon_cb_signal_move_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   _e_gadcon_client_move_stop(data);
}

static void
_e_gadcon_cb_signal_move_go(void *data, Evas_Object *obj, const char *emission, const char *source)
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
_e_gadcon_cb_signal_resize_left_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   _e_gadcon_client_resize_start(data);
}

static void
_e_gadcon_cb_signal_resize_left_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   _e_gadconclient_resize_stop(data);
}

static void
_e_gadcon_cb_signal_resize_left_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   Evas_Coord x, y, w, h;
   
   gcc = data;
   if (!gcc->resizing) return;
   evas_pointer_canvas_xy_get(gcc->gadcon->evas, &x, &y);
   x = x - gcc->dx;
   y = y - gcc->dy;

   gcc->state_info.flags = E_GADCON_LAYOUT_ITEM_LOCK_POSITION | E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;

   if (gcc->o_frame)
     evas_object_geometry_get(gcc->o_frame, NULL, NULL, &w, &h);
   else if (gcc->o_base)
     evas_object_geometry_get(gcc->o_base, NULL, NULL, &w, &h);

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
_e_gadcon_cb_signal_resize_right_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   _e_gadcon_client_resize_start(data);
}

static void
_e_gadcon_cb_signal_resize_right_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   _e_gadconclient_resize_stop(data);
}

static void
_e_gadcon_cb_signal_resize_right_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   Evas_Coord x, y, w, h;
   
   gcc = data;
   if (!gcc->resizing) return;
   evas_pointer_canvas_xy_get(gcc->gadcon->evas, &x, &y);
   x = x - gcc->dx;
   y = y - gcc->dy;

   gcc->state_info.flags = E_GADCON_LAYOUT_ITEM_LOCK_POSITION | E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;

   if (gcc->o_frame)
     evas_object_geometry_get(gcc->o_frame, NULL, NULL, &w, &h);
   else if (gcc->o_base)
     evas_object_geometry_get(gcc->o_base, NULL, NULL, &w, &h);

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
_e_gadcon_cb_dnd_enter(void *data, const char *type, void *event)
{
   E_Event_Dnd_Enter *ev;
   E_Gadcon          *gc;
   E_Gadcon_Client   *gcc;

   ev = event;
   gc = data;
   e_gadcon_layout_freeze(gc->o_container);
   gcc = drag_gcc;

   if (gcc->gadcon == gc)
     {
	/* We have re-entered the gadcon we left, revive gadcon client */
	Evas_Coord   dx, dy;
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
	cc = evas_hash_find(providers, gcc->name);
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
		  e_gadcon_client_resizable_set(new_gcc, gcc->resizable);
		  if (new_gcc->client_class->func.orient)
		    new_gcc->client_class->func.orient(new_gcc);
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
_e_gadcon_cb_dnd_move(void *data, const char *type, void *event)
{
   E_Event_Dnd_Move *ev;
   E_Gadcon         *gc;
   E_Gadcon_Client  *gcc = NULL;

   ev = event;
   gc = data;

   /* If we move in the same gadcon as the client originates */
   if (drag_gcc->gadcon == gc) gcc = drag_gcc;
   /* If we move in the newly entered gadcon */
   else if (new_gcc->gadcon == gc) gcc = new_gcc;
   if (gcc)
     {
	Evas_Coord   dx, dy;
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
	       e_gadcon_layout_pack_request_set(o, gcc->config.pos, gcc->config.size);
	     else
	       e_gadcon_layout_pack_request_set(o, gcc->config.pos, gcc->config.size);
	  }
	e_gadcon_layout_thaw(gc->o_container);
     }
}

static void
_e_gadcon_cb_dnd_leave(void *data, const char *type, void *event)
{
   E_Event_Dnd_Leave *ev;
   E_Gadcon          *gc;

   ev = event;
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
_e_gadcon_cb_drop(void *data, const char *type, void *event)
{
   E_Event_Dnd_Drop *ev;
   E_Gadcon         *gc;
   E_Gadcon_Client  *gcc = NULL;

   ev = event;
   gc = data;
   if (drag_gcc->gadcon == gc) gcc = drag_gcc;
   else if ((new_gcc) && (new_gcc->gadcon == gc)) gcc = new_gcc;

   gc->cf->clients = evas_list_append(gc->cf->clients, gcc->cf);

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
_e_gadcon_client_cb_menu_post(void *data, E_Menu *m)
{
   E_Gadcon_Client *gcc;

   gcc = data;
   if (!gcc) return;
   if ((gcc->gadcon) && (gcc->gadcon->shelf))
     e_shelf_locked_set(gcc->gadcon->shelf, 0);
   if (!gcc->menu) return;
   e_object_del(E_OBJECT(gcc->menu));
   gcc->menu = NULL;
}

static int
_e_gadcon_client_cb_instant_edit_timer(void *data)
{
   E_Gadcon_Client *gcc;
   
   gcc = data;
   e_gadcon_client_edit_begin(gcc);
   _e_gadcon_client_move_start(gcc);
   gcc->instant_edit_timer = NULL;
   return 0;
}

static void
_e_gadcon_client_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Gadcon_Client *gcc;
   
   ev = event_info;
   gcc = data;
   
   if (gcc->menu) return;
   if (ev->button == 3)
     {
	E_Menu *mn;
	int cx, cy, cw, ch;

	if (gcc->gadcon->shelf)
	  e_shelf_locked_set(gcc->gadcon->shelf, 1);
	mn = e_menu_new();
	e_menu_post_deactivate_callback_set(mn, _e_gadcon_client_cb_menu_post,
					    gcc);
	gcc->menu = mn;

	e_gadcon_client_util_menu_items_append(gcc, mn, 0);

	e_gadcon_canvas_zone_geometry_get(gcc->gadcon, &cx, &cy, &cw, &ch);
	e_menu_activate_mouse(mn,
			      e_util_zone_current_get(e_manager_current_get()),
			      cx + ev->output.x, cy + ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
     }
   else if (ev->button == 1)
     {
	if ((!gcc->o_control) && (gcc->gadcon->instant_edit))
	  {
	     if (gcc->instant_edit_timer) ecore_timer_del(gcc->instant_edit_timer);
	     gcc->instant_edit_timer = 
	       ecore_timer_add(1.0, _e_gadcon_client_cb_instant_edit_timer, 
			       gcc);
	  }
     }
}

static void
_e_gadcon_client_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
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
	     printf("EDIT END\n");
	     _e_gadcon_client_move_stop(gcc);
	     e_gadcon_client_edit_end(gcc);
	  }
     }
}

static void
_e_gadcon_client_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   E_Gadcon_Client *gcc;
   
   ev = event_info;
   gcc = data;
   
   if ((gcc->gadcon->instant_edit))
     {
	if (gcc->o_control)
	  _e_gadcon_client_move_go(gcc);
     }
}

static void
_e_gadcon_client_cb_menu_style_plain(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Gadcon_Client *gcc;
   E_Gadcon *gc;
   
   gcc = data;
   gc = gcc->gadcon;
   if (gcc->style) evas_stringshare_del(gcc->style);
   gcc->style = evas_stringshare_add(E_GADCON_CLIENT_STYLE_PLAIN);
   _e_gadcon_client_save(gcc);
   e_gadcon_unpopulate(gc);
   e_gadcon_populate(gc);
}

static void
_e_gadcon_client_cb_menu_style_inset(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Gadcon_Client *gcc;
   E_Gadcon *gc;
   
   gcc = data;
   gc = gcc->gadcon;
   if (gcc->style) evas_stringshare_del(gcc->style);
   gcc->style = evas_stringshare_add(E_GADCON_CLIENT_STYLE_INSET);
   _e_gadcon_client_save(gcc);
   e_gadcon_unpopulate(gc);
   e_gadcon_populate(gc);
}

static void
_e_gadcon_client_cb_menu_autoscroll(void *data, E_Menu *m, E_Menu_Item *mi)
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

static void
_e_gadcon_client_cb_menu_resizable(void *data, E_Menu *m, E_Menu_Item *mi)
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

static void
_e_gadcon_client_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Gadcon_Client *gcc;
   
   gcc = data;
   if (gcc->o_control)
     e_gadcon_client_edit_end(gcc);
   else
     e_gadcon_client_edit_begin(gcc);
}

static void
_e_gadcon_client_cb_menu_remove(void *data, E_Menu *m, E_Menu_Item *mi)
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
_e_gadcon_client_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info)
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
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   Evas_Object     *clip;
   unsigned char    horizontal : 1;
   unsigned char    doing_config : 1;
   unsigned char    redo_config : 1;
   Evas_List       *items;
   int              frozen;
   Evas_Coord       minw, minh, req;
}; 

struct _E_Gadcon_Layout_Item
{
   E_Smart_Data    *sd;
   struct {
      int           pos, size, size2, res, prev_pos, prev_size;
   } ask;
   int              hookp;
   struct {
      int           w, h;
   } min, aspect, aspect_pad;

   E_Gadcon_Client *gcc;

   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   unsigned char    can_move : 1;
};

/* local subsystem functions */
static E_Gadcon_Layout_Item *_e_gadcon_layout_smart_adopt(E_Smart_Data *sd, Evas_Object *obj);
static void        _e_gadcon_layout_smart_disown(Evas_Object *obj);
static void        _e_gadcon_layout_smart_item_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _e_gadcon_layout_smart_reconfigure(E_Smart_Data *sd);

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
void _e_gadcon_layout_smart_gadcons_width_adjust(E_Smart_Data *sd, int min, int cur);

static int _e_gadcon_layout_smart_sort_by_sequence_number_cb(void *d1, void *d2);
static int _e_gadcon_layout_smart_sort_by_position_cb(void *d1, void *d2);

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
   Evas_List *l;
   Evas_Coord tw = 0, th = 0;
 */
   if (!obj) return;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   if (w) *w = sd->minw;
   if (h) *h = sd->minh;

/*   
   for (l = sd->items; l; l = l->next)
     {
	E_Gadcon_Layout_Item *bi;
	Evas_Object *obj;
	
	obj = l->data;
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
   for (l = sd->items; l; l = l->next)
     {
	E_Gadcon_Layout_Item *bi;
	Evas_Object *obj;
	
	obj = l->data;
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
   sd->items = evas_list_prepend(sd->items, child);
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
   int xx;

   if (!obj) return;
   
   bi = evas_object_data_get(obj, "e_gadcon_layout_data");
   if (!bi) return;
   xx = bi->ask.pos + (bi->ask.size / 2);
   if (xx < (bi->ask.res / 3))
     { /* hooked to start */
	bi->ask.size = size;
     }
   else if (xx > ((2 * bi->ask.res) / 3))
     { /* hooked to end */
	bi->ask.pos = (bi->ask.pos + bi->ask.size) - size;
	bi->ask.size = size;
     }
   else
     { /* hooked to middle */
	if ((bi->ask.pos <= (bi->ask.res / 2)) &&
	    ((bi->ask.pos + bi->ask.size) > (bi->ask.res / 2)))
	  { /* straddles middle */
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
	     if (xx < (bi->ask.res / 2))
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
   if (bi->sd->horizontal)
     bi->ask.res = bi->sd->w;
   else
     bi->ask.res = bi->sd->h;
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
   Evas_List *l;
   E_Gadcon_Layout_Item *bi, *bi2;

   if (!obj) return;
   
   bi = evas_object_data_get(obj, "e_gadcon_layout_data");
   if (!bi) return;
   bi->ask.res = gcc->config.res;
   bi->ask.size = gcc->config.size;
   bi->ask.pos = gcc->config.pos;
   bi->gcc = gcc;

   ok = 0;
   if (!gcc->state_info.seq) 
     ok = 1;

   seq = 1;
   for (l = bi->sd->items; l; l = l->next)
     {
	bi2 = evas_object_data_get(l->data, "e_gadcon_layout_data");
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
   bi->min.w = w;
   bi->min.h = h;
   _e_gadcon_layout_smart_reconfigure(bi->sd);
}

static void
e_gadcon_layout_pack_aspect_set(Evas_Object *obj, int w, int h)
{
   E_Gadcon_Layout_Item *bi;

   if (!obj) return;
   
   bi = evas_object_data_get(obj, "e_gadcon_layout_data");
   if (!bi) return;
   bi->aspect.w = w;
   bi->aspect.h = h;
   _e_gadcon_layout_smart_reconfigure(bi->sd);
}

static void
e_gadcon_layout_pack_aspect_pad_set(Evas_Object *obj, int w, int h)
{
   E_Gadcon_Layout_Item *bi;

   if (!obj) return;
   
   bi = evas_object_data_get(obj, "e_gadcon_layout_data");
   if (!bi) return;
   bi->aspect_pad.w = w;
   bi->aspect_pad.h = h;
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
   sd->items = evas_list_remove(sd->items, obj);
   _e_gadcon_layout_smart_disown(obj);
   _e_gadcon_layout_smart_reconfigure(sd);
}

/* local subsystem functions */
static E_Gadcon_Layout_Item *
_e_gadcon_layout_smart_adopt(E_Smart_Data *sd, Evas_Object *obj)
{
   E_Gadcon_Layout_Item *bi;

   if (!obj) return NULL;
   
   bi = calloc(1, sizeof(E_Gadcon_Layout_Item));
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
   evas_object_event_callback_del(obj,
				  EVAS_CALLBACK_DEL,
				  _e_gadcon_layout_smart_item_del_hook);
   evas_object_smart_member_del(obj);
   evas_object_clip_unset(obj);
   evas_object_data_del(obj, "e_gadcon_layout_data");
   free(bi);
}

static void
_e_gadcon_layout_smart_item_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   if (!obj) return;

   e_gadcon_layout_unpack(obj);
}

static void
_e_gadcon_layout_smart_reconfigure(E_Smart_Data *sd)
{
   Evas_Coord xx, yy;
   Evas_List *l;
   int min, mino, cur;
   Evas_List *list = NULL;
   E_Gadcon_Layout_Item *bi;
   E_Layout_Item_Container *lc;
   int i;
   int set_prev_pos = 0;

   if (sd->frozen) return;
   if (sd->doing_config)
     {
	sd->redo_config = 1;
	return;
     }

   min = mino = cur = 0;

   _e_gadcon_layout_smart_min_cur_size_calc(sd, &min, &mino, &cur); 

   if (sd->horizontal)
     {
	if ((sd->minw != min) || (sd->minh != mino))
	  {
	     sd->minw = min;
	     sd->minh = mino;
	     evas_object_smart_callback_call(sd->obj, "min_size_request", NULL);
	  }
     }
   else
     {
	if ((sd->minh != min) || (sd->minw != mino))
	  {
	     sd->minh = min;
	     sd->minw = mino;
	     evas_object_smart_callback_call(sd->obj, "min_size_request", NULL);
	  }
     }

   if (sd->req != cur)
     {
	if (((sd->horizontal) && (cur >= sd->minw)) ||
	    ((!sd->horizontal) && (cur >= sd->minh)))
	  {
	     sd->req = cur;
	     evas_object_smart_callback_call(sd->obj, "size_request", NULL);
	  }
	else
	  {
	     if (sd->horizontal)
	       sd->req = sd->minw;
	     else
	       sd->req = sd->minh;
	  }
     }
   _e_gadcon_layout_smart_gadcons_width_adjust(sd, min, cur);

   if (((sd->horizontal) && (sd->w <= sd->req)) || ((!sd->horizontal) && (sd->h <= sd->req)))
     { 
	_e_gadcon_layout_smart_gadcon_position_shrinked_mode(sd);
	set_prev_pos = 0;
     }
   else
     { 
	_e_gadcon_layout_smart_gadcons_asked_position_set(sd);

	list = _e_gadcon_layout_smart_gadcons_wrap(sd);

	_e_gadcon_layout_smart_gadcons_position(sd, &list);

	for (l = list; l; l = l->next)
	  {
	     lc = l->data;
	     LC_FREE(lc);
	  }
	list = evas_list_free(list);
	set_prev_pos = 1;
     }


   sd->items = evas_list_sort(sd->items, evas_list_count(sd->items),
			      _e_gadcon_layout_smart_sort_by_position_cb);
   for (l = sd->items, i = 1; l; l = l->next, i++)
     {
	bi = evas_object_data_get(l->data, "e_gadcon_layout_data"); 
	if (bi->gcc->gadcon->editing)
	  bi->gcc->state_info.seq = i; 

	if (set_prev_pos)
	  {
	     if (sd->horizontal)
	       { 
		  bi->ask.prev_pos = bi->x; 
		  bi->ask.prev_size = bi->w;
	       }
	     else
	       {
		  bi->ask.prev_pos = bi->y;
		  bi->ask.prev_size = bi->h;
	       }
	  }
	if (sd->horizontal)
	  {
	     if ((bi->x == bi->ask.pos) &&
		 (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_POSITION))
	       bi->gcc->state_info.flags |= E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;
	  }
	else
	  {
	     if ((bi->y == bi->ask.pos) &&
		 (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_POSITION))
	       bi->gcc->state_info.flags |= E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;
	  }

	if ((bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_POSITION) &&
	    (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE))
	  {
	     if (((sd->horizontal) && (bi->x != bi->ask.pos)) ||
	         ((!sd->horizontal) && (bi->y != bi->ask.pos)))
	       bi->gcc->state_info.flags &= ~E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;
	  }
     } 
   
   for (l = sd->items; l; l = l->next)
     {
	E_Gadcon_Layout_Item *bi;
	Evas_Object *obj;
	
	obj = l->data;
	bi = evas_object_data_get(obj, "e_gadcon_layout_data");
	if (!bi) continue;
	if (sd->horizontal)
	  {
	     bi->h = sd->h;
	     xx = sd->x + bi->x;
	     yy = sd->y + ((sd->h - bi->h) / 2);
	  }
	else
	  {
	     bi->w = sd->w;
	     xx = sd->x + ((sd->w - bi->w) / 2);
	     yy = sd->y + bi->y;
	  }
	evas_object_move(obj, xx, yy);
	evas_object_resize(obj, bi->w, bi->h);
     }
   sd->doing_config = 0;
   if (sd->redo_config)
     {
	_e_gadcon_layout_smart_reconfigure(sd);
	sd->redo_config = 0;
     }
   if (sd->horizontal)
     {
	if ((sd->minw != min) || (sd->minh != mino))
	  {
	     sd->minw = min;
	     sd->minh = mino;
	     evas_object_smart_callback_call(sd->obj, "min_size_request", NULL);
	  }
     }
   else
     {
	if ((sd->minh != min) || (sd->minw != mino))
	  {
	     sd->minw = mino;
	     sd->minh = min;
	     evas_object_smart_callback_call(sd->obj, "min_size_request", NULL);
	  }
     }
   if (sd->req != cur)
     {
	if (((sd->horizontal) && (cur >= sd->minw)) ||
	    ((!sd->horizontal) && (cur >= sd->minh)))
	  {
	     sd->req = cur;
	     evas_object_smart_callback_call(sd->obj, "size_request", NULL);
	  }
     }
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
	       NULL,
	       NULL
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
	
	child = sd->items->data;
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
	Evas_List *l;
	Evas_Coord dx, dy;
	
	dx = x - sd->x;
	dy = y - sd->y;
	for (l = sd->items; l; l = l->next)
	  {
	     Evas_Coord ox, oy;
	     
	     evas_object_geometry_get(l->data, &ox, &oy, NULL, NULL);
	     evas_object_move(l->data, ox + dx, oy + dy);
	  }
     }
   sd->x = x;
   sd->y = y;
}

static void
_e_gadcon_layout_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   if (!obj) return;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((w == sd->w) && (h == sd->h)) return;
   sd->w = w;
   sd->h = h;
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
   Evas_List *l;

   for (l = sd->items; l; l = l->next)
     {
	bi = evas_object_data_get(l->data, "e_gadcon_layout_data");
	bi->ask.size2 = bi->ask.size;

	if ((bi->aspect.w > 0) && (bi->aspect.h > 0))
	  {
	     if (sd->horizontal)
	       {
		  bi->ask.size2 = (((sd->h - bi->aspect_pad.h) * bi->aspect.w) / bi->aspect.h)
				   + bi->aspect_pad.w;
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
		  bi->ask.size2 = (((sd->w - bi->aspect_pad.w) * bi->aspect.h) / bi->aspect.w)
				   + bi->aspect_pad.h;
		  if (bi->ask.size2 > bi->min.h)
		    {
		       *min += bi->ask.size2;
		       *cur += bi->ask.size2;
		    }
		  else
		    {
		       *cur += bi->min.h;
		       *min += bi->min.h;
		    }
	       }
	  }
	else
	  {
	     if (sd->horizontal)
	       {
		  *min += bi->min.w;
		  if (bi->min.h > *mino) *mino = bi->min.h;
		  if (bi->ask.size < bi->min.w)
		    *cur += bi->min.w;
		  else
		    *cur += bi->ask.size;
	       }
	     else
	       {
		  *min += bi->min.h;
		  if (bi->min.w > *mino) *mino = bi->min.w;
		  if (bi->ask.size < bi->min.h)
		    *cur += bi->min.h;
		  else
		    *cur += bi->ask.size;
	       }
	  }
     }
}

static int 
_e_gadcon_layout_smart_width_smart_sort_reverse_cb(void *d1, void *d2)
{
   E_Gadcon_Layout_Item *bi, *bi2;

   bi = evas_object_data_get(d1, "e_gadcon_layout_data");
   bi2 = evas_object_data_get(d2, "e_gadcon_layout_data"); 
   
   if (bi->sd->horizontal)
     { 
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
     }
   else
     {
	if (bi->ask.size2 > bi->min.h)
	  {
	     if (bi2->ask.size2 > bi2->min.h) 
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
	     if (bi2->ask.size2 > bi2->min.h)
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
     }
   return 0;
}

void 
_e_gadcon_layout_smart_gadcons_width_adjust(E_Smart_Data *sd, int min, int cur)
{ 
   int need, limit, reduce_total, reduce;
   int max_size;
   int c;
   Evas_List *l, *l2;
   E_Gadcon_Layout_Item *bi, *bi2;

   if (sd->horizontal)
     {
	if (sd->w < cur)
	  {
	     if (sd->w < min)
	       max_size = min;
	     else
	       max_size = cur;

	     need = max_size - sd->w;
	  }
	else
	  return;
     }
   else
     {
	if (sd->h < cur)
	  {
	     if (sd->h < min)
	       max_size = min;
	     else
	       max_size = cur; 
	     need = max_size - sd->h;
	  }
	else
	  return;
     } 

   sd->items = evas_list_sort(sd->items, evas_list_count(sd->items), 
			      _e_gadcon_layout_smart_width_smart_sort_reverse_cb); 

   __adjust_size_again: 
   for (l = sd->items, c = 0; l; l = l->next) 
     { 
	if (l->next) 
	  { 
	     bi = evas_object_data_get(l->data, "e_gadcon_layout_data");
	     bi2 = evas_object_data_get(l->next->data, "e_gadcon_layout_data"); 
	     
	     if (bi->ask.size2 > bi2->ask.size2)
	       { 
		  limit = bi2->ask.size2;
		  c++;
		  break;
	       } 
	     c++;
	  }
     } 

   if (evas_list_count(sd->items) == 1)
     c = 1;
   
   if (l)
     { 
	reduce = bi->ask.size2 - limit; 
	reduce_total = reduce * c; 
	
	if (reduce_total <= need) 
	  { 
	     for (l2 = l; l2; l2 = l2->prev) 
	       { 
		  bi2 = evas_object_data_get(l2->data, "e_gadcon_layout_data"); 
		  bi2->ask.size2 -= reduce; 
	       } 
	     need -= reduce * c;
	     if (need) 
	       goto __adjust_size_again;
	  } 
	else
	  { 
	     int reduce_by, c2; 
	     
	     while (need)
	       { 
		  reduce_by = 1;
		  while (1) 
		    { 
		       if (((reduce_by + 1) * c) < need) 
			 reduce_by++; 
		       else 
			 break; 
		    } 
		  c2 = c;
		  for (l2 = sd->items; l2 && c2 && need; l2 = l2->next, c2--) 
		    { 
		       bi2 = evas_object_data_get(l2->data, "e_gadcon_layout_data");
		       bi2->ask.size2 -= reduce_by; 
		       need -= reduce_by; 
		    } 
	       }
	  } 
     }
   else
     { 
	int reduce_by, c2; 
	
	while (need)
	  { 
	     reduce_by = 1;
	     while (1) 
	       { 
		  if (((reduce_by + 1) * c) < need) 
		    reduce_by++; 
		  else 
		    break; 
	       } 
	     c2 = c;
	     for (l2 = sd->items; l2 && c2 && need; l2 = l2->next, c2--) 
	       { 
		  bi2 = evas_object_data_get(l2->data, "e_gadcon_layout_data");
		  bi2->ask.size2 -= reduce_by; 
		  need -= reduce_by; 
	       } 
	  }
     }
}

static int 
_e_gadcon_layout_smart_sort_by_sequence_number_cb(void *d1, void *d2)
{
   E_Gadcon_Layout_Item *bi, *bi2;

   bi = evas_object_data_get(d1, "e_gadcon_layout_data");
   bi2 = evas_object_data_get(d2, "e_gadcon_layout_data");

   if ((!bi->gcc->state_info.seq) && (!bi2->gcc->state_info.seq)) return 0;
   else if (!bi->gcc->state_info.seq) return 1;
   else if (!bi2->gcc->state_info.seq) return -1;

   return bi->gcc->state_info.seq - bi2->gcc->state_info.seq;
} 

static int
_e_gadcon_layout_smart_sort_by_position_cb(void *d1, void *d2)
{
   E_Gadcon_Layout_Item *bi, *bi2;

   bi = evas_object_data_get(d1, "e_gadcon_layout_data");
   bi2 = evas_object_data_get(d2, "e_gadcon_layout_data");

   if (bi->sd->horizontal) 
     return (bi->x - bi2->x);
   return (bi->y - bi2->y);
}

static int
_e_gadcon_layout_smart_containers_sort_cb(void *d1, void *d2)
{
   E_Layout_Item_Container *lc, *lc2;

   lc = d1;
   lc2 = d2;

   if (lc->pos < lc2->pos) return -1;
   else if (lc->pos > lc2->pos) return 1;

   return 0;
}

static int
_e_gadcon_layout_smart_seq_sort_cb(void *d1, void *d2)
{
   E_Gadcon_Layout_Item *bi, *bi2;

   bi = d1;
   bi2 = d2;

   return (bi->gcc->state_info.seq - bi2->gcc->state_info.seq);
}

static void
_e_gadcon_layout_smart_sync_clients(E_Gadcon *gc)
{
   E_Gadcon_Client *gcc;
   Evas_List *l;

   for (l = gc->clients; l; l = l->next)
     {
	gcc = l->data;
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
   
   gcc->state_info.prev_pos = gcc->config.pos;
   gcc->state_info.prev_size = gcc->config.size;
   if (e_gadcon_layout_orientation_get(gcc->gadcon->o_container)) 
     gcc->config.pos = bi->x;
   else
     gcc->config.pos = bi->y;
}

static void
_e_gadcon_layout_smart_gadcon_position_shrinked_mode(E_Smart_Data *sd)
{ 
   Evas_List *l;
   E_Gadcon_Layout_Item *bi, *bi2; 
   void *tp; 
   int pos = 0; 

   sd->items = evas_list_sort(sd->items, evas_list_count(sd->items),
			      _e_gadcon_layout_smart_sort_by_sequence_number_cb); 
   for (l = sd->items; l; l = l->next) 
     { 
	bi = evas_object_data_get(l->data, "e_gadcon_layout_data"); 
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
		  if (l->next) 
		    { 
		       tp = l->next->data; 
		       l->next->data = l->data; 
		       l->data = tp; 
		       
		       bi2 = evas_object_data_get(tp, "e_gadcon_layout_data");
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
		  if (l->prev)
		    { 
		       E_Gadcon_Layout_Item *bi2;
		       void *tp;
		       tp = l->prev->data;
		       l->prev->data = l->data;
		       l->data = tp; 
		       
		       bi2 = evas_object_data_get(tp, "e_gadcon_layout_data");
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
	     if (sd->horizontal)
	       { 
		  if (bi->w < bi->min.w) 
		    bi->gcc->config.size = bi->w = bi->min.w;
		  else
		    bi->gcc->config.size = bi->w;
	       }
	     else
	       {
		  if (bi->h < bi->min.h)
		    bi->gcc->config.size = bi->h = bi->min.h;
		  else
		    bi->gcc->config.size = bi->h;
	       }
	     bi->gcc->config.pos = bi->gcc->state_info.prev_pos;
	  } 
     } 
   
   for (l = sd->items; l; l = l->next)
     { 
	bi = evas_object_data_get(l->data, "e_gadcon_layout_data");
	if (sd->horizontal) 
	  { 
	     bi->x = pos; 
	     bi->gcc->config.size = bi->w = bi->ask.size2; 
	     pos = bi->x + bi->w;
	  }
	else
	  {
	     bi->y = pos; 
	     bi->gcc->config.size = bi->h = bi->ask.size2; 
	     pos = bi->y + bi->h;
	  }
     }
}

/* 
 * The function returns a list of E_Gadcon_Layout_Item_Container
 */
static void
_e_gadcon_layout_smart_gadcons_asked_position_set(E_Smart_Data *sd)
{
   E_Gadcon_Layout_Item *bi;
   Evas_List *l;

   for (l = sd->items; l; l = l->next)
     {
	bi = evas_object_data_get(l->data, "e_gadcon_layout_data");
	if (!bi) continue;

	if (sd->horizontal)
	  {
	     bi->x = bi->ask.pos;
	     bi->w = bi->ask.size2;
	  }
	else
	  {
	     bi->y = bi->ask.pos;
	     bi->h = bi->ask.size2;
	  }
     }

#if 0
   for (l = sd->items; l; l = l->next)
     {
	bi = evas_object_data_get(l->data, "e_gadcon_layout_data");
	if (!bi) continue;
	if (sd->horizontal)
	  {
	     xx = bi->ask.pos + (bi->ask.size / 2);
	     if (xx < (bi->ask.res / 3))
	       { /* hooked to start */
		  bi->x = bi->ask.pos;
		  bi->w = bi->ask.size2;
		  bi->hookp = 0;
	       }
	     else if (xx > ((2 * bi->ask.res) / 3))
	       { /* hooked to end */
		  bi->x = (bi->ask.pos - bi->ask.res) + sd->w;
		  bi->w = bi->ask.size2;
		  bi->hookp = bi->ask.res;
	       }
	     else
	       { /* hooked to middle */
		  if ((bi->ask.pos <= (bi->ask.res / 2)) &&
		      ((bi->ask.pos + bi->ask.size2) > (bi->ask.res / 2)))
		    { /* straddles middle */
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
		    { /* either side of middle */
		       bi->x = (bi->ask.pos - (bi->ask.res / 2)) + (sd->w / 2);
		       bi->w = bi->ask.size2;
		    }
		  bi->hookp = bi->ask.res / 2;
	       }
	  }
	else
	  {
	     yy = bi->ask.pos + (bi->ask.size2 / 2);
	     if (yy < (bi->ask.res / 3))
	       { /* hooked to start */
		  bi->y = bi->ask.pos;
		  bi->h = bi->ask.size2;
		  bi->hookp = 0;
	       }
	     else if (yy > ((2 * bi->ask.res) / 3))
	       { /* hooked to end */
		  bi->y = (bi->ask.pos - bi->ask.res) + sd->h;
		  bi->h = bi->ask.size2;
		  bi->hookp = bi->ask.res;
	       }
	     else
	       { /* hooked to middle */
		  if ((bi->ask.pos <= (bi->ask.res / 2)) &&
		      ((bi->ask.pos + bi->ask.size2) > (bi->ask.res / 2)))
		    { /* straddles middle */
		       if (bi->ask.res > 2)
			 bi->y = (sd->h / 2) + 
			 (((bi->ask.pos + (bi->ask.size2 / 2) - 
			    (bi->ask.res / 2)) * 
			   (bi->ask.res / 2)) /
			  (bi->ask.res / 2)) - (bi->ask.size2 / 2);
		       else
			 bi->y = sd->h / 2;
		       bi->h = bi->ask.size2;
		    }
		  else
		    { /* either side of middle */
		       bi->y = (bi->ask.pos - (bi->ask.res / 2)) + (sd->h / 2);
		       bi->h = bi->ask.size2;
		    }
		  bi->hookp = bi->ask.res / 2;
	       }
	  }
     }
#endif
}

static Evas_List *
_e_gadcon_layout_smart_gadcons_wrap(E_Smart_Data *sd)
{
   Evas_List *l, *list = NULL;
   E_Layout_Item_Container *lc;
   E_Gadcon_Layout_Item *bi;

   for (l = sd->items; l; l = l->next)
     {
	bi = evas_object_data_get(l->data, "e_gadcon_layout_data");
	lc = E_NEW(E_Layout_Item_Container, 1);
	lc->state_info.min_seq = lc->state_info.max_seq = bi->gcc->state_info.seq;
	lc->sd = sd;
	if (sd->horizontal)
	  {
	     lc->pos = bi->x;
	     lc->size = bi->w;
	  }
	else
	  {
	     lc->pos = bi->y;
	     lc->size = bi->h;
	  }
	lc->prev_pos = bi->ask.prev_pos;
	lc->prev_size = bi->ask.prev_size;

	E_LAYOUT_ITEM_CONTAINER_STATE_SET(lc->state, bi->gcc->state_info.state);

	if ((bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_POSITION) &&
	    (lc->state == E_LAYOUT_ITEM_CONTAINER_STATE_NONE))
	  lc->state = E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED;

	lc->items = evas_list_append(lc->items, bi);
	list = evas_list_append(list, lc);
     }
   return list;
}

static void
_e_gadcon_layout_smart_gadcons_position(E_Smart_Data *sd, Evas_List **list)
{
   int ok, lc_moving_prev_pos;
   Evas_List *l, *l2, *l3;
   E_Layout_Item_Container *lc_moving = NULL, *lc_back, *lc, *lc3;
   E_Gadcon_Layout_Item *bi, *bi_moving;

   if (!list || !*list) return;

   for (l = *list; l; l = l->next)
     {
	lc_moving = l->data;

	if ((lc_moving->state == E_LAYOUT_ITEM_CONTAINER_STATE_POS_INC) ||
	    (lc_moving->state == E_LAYOUT_ITEM_CONTAINER_STATE_POS_DEC) ||
	    (lc_moving->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MIN_END_INC) ||
	    (lc_moving->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MIN_END_DEC) ||
	    (lc_moving->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MAX_END_INC) ||
	    (lc_moving->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MAX_END_DEC))
	  {
	     lc_back = E_NEW(E_Layout_Item_Container, 1);
	     lc_back->pos = lc_moving->pos;
	     lc_back->prev_pos = lc_moving->prev_pos;
	     lc_back->size = lc_moving->size;
	     lc_back->prev_size = lc_moving->prev_size;
	     lc_back->state_info.min_seq = lc_moving->state_info.min_seq;
	     lc_back->state_info.max_seq = lc_moving->state_info.max_seq;
	     lc_back->sd = lc_moving->sd;
	     for (l2 = lc_moving->items; l2; l2 = l2->next)
	       lc_back->items = evas_list_append(lc_back->items, l2->data);
	     lc_back->state = lc_moving->state;
	     bi_moving = lc_back->items->data;
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
	for (l = *list; (l) && (l->data != lc_moving); l = l->next); 

	ok = 0;
	if ((l) && (l->prev))
	  {
	     lc = l->prev->data;

	     if (lc_moving->pos < (lc->pos + lc->size))
	       {
		  bi = lc_moving->items->data;
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
			    *list = evas_list_remove_list(*list, l->prev);
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
	     for (l = *list; (l) && (l->data != lc_moving); l = l->next); 

	     pos = lc_moving->pos + lc_moving->size;
	     prev_pos = lc_moving_prev_pos;

	     if ((l) && (l->next)) 
	       { 
		  stop = 0;
		  for (l2 = l->next; l2 && !stop; l2 = l2->next)
		    {
		       lc = l2->data;

		       if (lc->pos != prev_pos) break;
		       prev_pos = lc->pos + lc->size;

		       for (l3 = lc->items; l3; l3 = l3->next)
			 {
			    bi = l3->data;
			    if (bi->ask.pos <= pos)
			      {
				 if (sd->horizontal) 
				   { 
				      bi->x = pos;
				      pos = bi->x + bi->w;
				   }
				 else
				   { 
				      bi->y = pos;
				      pos = bi->y + bi->h;
				   }
			      }
			    else if (((sd->horizontal) && (bi->ask.pos < bi->x)) ||
				     ((!sd->horizontal) && (bi->ask.pos < bi->y)))
			      {
				 if (sd->horizontal)
				   {
				      bi->x = bi->ask.pos;
				      pos = bi->x + bi->w;
				   }
				 else
				   {
				      bi->y = bi->ask.pos;
				      pos = bi->y + bi->h;
				   }
			      }
			    else if (((sd->horizontal) && (bi->ask.pos == bi->x)) ||
				     ((!sd->horizontal) && (bi->ask.pos == bi->y)))
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
	for (l = *list; (l) && (l->data != lc_moving); l = l->next); 

	ok = 0;
	if ((l) && (l->next))
	  {
	     lc = l->next->data;

	     if ((lc_moving->pos + lc_moving->size) > lc->pos)
	       {
		  bi = lc_moving->items->data;
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
			    *list = evas_list_remove_list(*list, l->next);
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

	     for (l = *list; (l) && (l->data != lc_moving); l = l->next);

	     pos = lc_moving->pos;
	     prev_pos = lc_moving_prev_pos;

	     if ((l) && (l->prev))
	       {
		  stop = 0;
		  for (l2 = l->prev; l2 && !stop; l2 = l2->prev)
		    {
		       lc = l2->data;
		       if ((lc->pos + lc->size) == prev_pos) break;
		       prev_pos = lc->pos;

		       for (l3 = evas_list_last(lc->items); l3; l3 = l3->prev)
			 {
			    bi = l3->data;

			    if (((sd->horizontal) && ((bi->ask.pos + bi->w) >= pos)) ||
			        ((!sd->horizontal) && ((bi->ask.pos + bi->h) >= pos)))
			      {
				 if (sd->horizontal)
				   {
				      bi->x = pos - bi->w;
				      pos = bi->x;
				   }
				 else
				   {
				      bi->y = pos - bi->h;
				      pos = bi->y;
				   }
			      }
			    else if (((sd->horizontal) && (bi->ask.pos > bi->x)) ||
				     ((!sd->horizontal) && (bi->ask.pos > bi->w)))
			      {
				 if (sd->horizontal)
				   {
				      bi->x = bi->ask.pos;
				      pos = bi->x;
				   }
				 else
				   {
				      bi->y = bi->ask.pos;
				      pos = bi->y;
				   }
			      }
			    else if (((sd->horizontal) && (bi->ask.pos == bi->x)) ||
				     ((!sd->horizontal) && (bi->ask.pos == bi->y)))
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
	for (l = *list; (l) && (l->data != lc_moving); l = l->next);

	if ((l) && (l->prev))
	  {
	     int new_pos = 0;

	     ok = 0;
	     new_pos = lc_moving->pos;
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

		  bi = lc_moving->items->data;
		  if (sd->horizontal)
		    bi->w = lc_moving->size;
		  else
		    bi->h = lc_moving->size;
		  _e_gadcon_layout_smart_position_items_inside_container(sd, lc_moving);

		  new_pos = 0;
	       }

	     if (ok)
	       { 
		  if (!l2) l2 = *list; 
		  else l2 = l2->next;
		  
		  for (; l2 != l; l2 = l2->next) 
		    { 
		       lc = l2->data; 
		       lc->pos = new_pos; 
		       _e_gadcon_layout_smart_position_items_inside_container(sd, lc); 
		       for (l3 = lc->items; l3; l3 = l3->next)
			 {
			    bi = l3->data;
			    bi->gcc->state_info.flags &= ~E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;
			 }
		       new_pos += lc->size; 
		    }
	       }
	  }
	else if ((l) && (!l->prev))
	  {
	     if (lc_moving->pos <= 0)
	       {
		  lc_moving->size = lc_moving->prev_size;
		  lc_moving->pos = 0;

		  bi = lc_moving->items->data;
		  if (sd->horizontal)
		    bi->w = lc_moving->size;
		  else
		    bi->h = lc_moving->size;
		  _e_gadcon_layout_smart_position_items_inside_container(sd, lc_moving);
	       }
	  }
     }
   else if (lc_moving->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MIN_END_INC)
     {
	lc_moving->state = E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED;
	_e_gadcon_layout_smart_gadcons_position_static(sd, list);
	LC_FREE(lc_back);
     }
   else if (lc_moving->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MAX_END_INC)
     {
	_e_gadcon_layout_smart_restore_gadcons_position_before_move(sd, &lc_moving, lc_back, list); 
	for (l = *list; (l) && (l->data != lc_moving); l = l->next);

	if ((l) && (l->next))
	  {
	     Evas_List *stop;
	     int new_pos = 0;

	     ok = 0;
	     new_pos = lc_moving->pos + lc_moving->size;
	     for (l2 = l->next; l2; l2 = l2->next)
	       {
		  lc = l2->data;

		  if (new_pos <= lc->pos)
		    {
		       stop = l2;
		       break;
		    }

		  ok = 1;
		  new_pos += lc->size;
	       }

	     if (sd->horizontal)
	       {
		  if (new_pos > sd->w)
		    {
		       lc_moving->size -= (new_pos - sd->w);
		       bi = lc_moving->items->data;
		       bi->w = lc_moving->size;

		       new_pos = lc_moving->pos + lc_moving->size;
		    }
	       }
	     else
	       {
		  if (new_pos > sd->h)
		    {
		       lc_moving->size -= (new_pos - sd->h);
		       bi = lc_moving->items->data;
		       bi->h = lc_moving->size;

		       new_pos = lc_moving->pos + lc_moving->size;
		    }
	       }

	     if (ok)
	       {
		  new_pos = lc_moving->pos + lc_moving->size;
		  for (l2 = l->next; l2 && l2 != stop; l2 = l2->next)
		    {
		       lc = l2->data;
		       lc->pos = new_pos;
		       _e_gadcon_layout_smart_position_items_inside_container(sd, lc);
		       for (l3 = lc->items; l3; l3 = l3->next)
			 {
			    bi = l3->data;
			    bi->gcc->state_info.flags &= ~E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE;
			 }
		       new_pos += lc->size;
		    }
	       }
	  }
	else if ((l) && (!l->next))
	  {
	     if (sd->horizontal) 
	       {
		  if ((lc_moving->pos + lc_moving->size) >= sd->w)
		    {
		       lc_moving->size = lc_moving->prev_size;
		       bi = lc_moving->items->data;
		       bi->w = lc_moving->size;
		    }
	       }
	     else
	       {
		  if ((lc_moving->pos + lc_moving->size) > sd->h)
		    {
		       lc_moving->size = lc_moving->prev_size;
		       bi = lc_moving->items->data;
		       bi->h = lc_moving->size;
		    }
	       }
	  }
     }
   else if (lc_moving->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MAX_END_DEC)
     {
	lc_moving->state = E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED;
	_e_gadcon_layout_smart_gadcons_position_static(sd, list);
	LC_FREE(lc_back);
     }

   if (sd->horizontal) 
     { 
	bi_moving->gcc->config.pos = bi_moving->ask.pos = bi_moving->x; 
	bi_moving->gcc->config.size = bi_moving->w;
     }
   else
     {
	bi_moving->gcc->config.pos = bi_moving->ask.pos = bi_moving->y;
	bi_moving->gcc->config.size = bi_moving->h;
     }
}

static void
_e_gadcon_layout_smart_gadcons_position_static(E_Smart_Data *sd, Evas_List **list)
{
   int ok;
   Evas_List *l;
   E_Layout_Item_Container *lc, *lc2, *lc3;

   *list = evas_list_sort(*list, evas_list_count(*list), _e_gadcon_layout_smart_containers_sort_cb);

   __reposition_again:
   for (l = *list; l; l = l->next)
     {
	if (!l->next) continue;

	lc = l->data;
	lc2 = l->next->data;

	if (LC_OVERLAP(lc, lc2))
	  {
	     lc3 = _e_gadcon_layout_smart_containers_position_adjust(sd, lc, lc2);
	     if (lc3)
	       { 
		  l->data = lc3; 
		  *list = evas_list_remove_list(*list, l->next); 
		  LC_FREE(lc);
		  LC_FREE(lc2);
		  goto __reposition_again;
	       }
	  }
     }

   ok = 1;
   for (l = *list; l; l = l->next)
     {
	lc = l->data;
	if (lc->pos < 0)
	  {
	     ok = 0;
	     lc->pos = 0;
	     _e_gadcon_layout_smart_position_items_inside_container(sd, lc);
	     continue;
	  }

	if (sd->horizontal)
	  {
	     if (((lc->pos + lc->size) > sd->w) && (lc->size <= sd->w))
	       {
		  ok = 0;
		  lc->pos = sd->w - lc->size;
		  _e_gadcon_layout_smart_position_items_inside_container(sd, lc);
	       }
	  }
	else
	  {
	     if (((lc->pos + lc->size) > sd->h) && (lc->size <= sd->h))
	       {
		  ok = 0;
		  lc->pos = sd->h - lc->size;
		  _e_gadcon_layout_smart_position_items_inside_container(sd, lc);
	       }
	  }
     }
   if (!ok)
     _e_gadcon_layout_smart_gadcons_position_static(sd, list);
}

static E_Layout_Item_Container *
_e_gadcon_layout_smart_containers_position_adjust(E_Smart_Data *sd, E_Layout_Item_Container *lc, E_Layout_Item_Container *lc2)
{
   int create_new;
   Evas_List *l;
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

	     bi = lc->items->data;
	     bi2  = lc2->items->data;

	     if (sd->horizontal)
	       {
		  bi->gcc->config.pos = bi->ask.pos = bi->x = (bi2->x + bi2->w) - bi->w;
		  bi->gcc->config.size = bi->w;
		  bi2->x = bi->x - bi2->w;
	       }
	     else
	       {
		  bi->gcc->config.pos = bi->ask.pos = bi->y = (bi2->y + bi2->h) - bi->h;
		  bi->gcc->config.size = bi->h;
		  bi2->y = bi->y - bi2->h;
	       }
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

		  for (l = lc->items; l; l = l->next)
		    {
		       bi = l->data;
		       if (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_POSITION)
			 {
			    if (sd->horizontal) shift = bi->ask.pos - bi->x;
			    else shift = bi->ask.pos - bi->y;
			 }

		       if (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE)
			 break;
		    }

		  for (l = lc->items; l && shift; l = l->next)
		    {
		       bi = l->data;

		       if (sd->horizontal) bi->x += shift;
		       else bi->y += shift;

		       if (l == lc->items)
			 {
			    if (sd->horizontal) lc->pos = bi->x;
			    else lc->pos = bi->y;
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

		  for (l = lc->items; l; l = l->next)
		    {
		       bi = l->data;
		       if (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_POSITION)
			 {
			    if (sd->horizontal) shift = bi->ask.pos - bi->x;
			    else shift = bi->ask.pos - bi->y;
			 }

		       if (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE)
			 break;
		    }

		  for (l = lc->items; l && shift; l = l->next)
		    {
		       bi = l->data;

		       if (sd->horizontal) bi->x += shift;
		       else bi->y += shift;

		       if (l == lc->items)
			 {
			    if (sd->horizontal) lc->pos = bi->x;
			    else lc->pos = bi->y;
			 }
		    }
	       }
	  }
	else if (lc2->state == E_LAYOUT_ITEM_CONTAINER_STATE_POS_DEC)
	  {
	     int t;

	     bi = (evas_list_last(lc->items))->data;
	     bi2 = lc2->items->data;

	     if (sd->horizontal)
	       {
		  bi2->gcc->config.pos = bi2->ask.pos = bi2->x = bi->x;
		  bi2->gcc->config.size = bi2->w;
		  bi->x = bi2->x + bi2->w;
	       }
	     else
	       {
		  bi2->gcc->config.pos = bi2->ask.pos = bi2->y = bi->y;
		  bi2->gcc->config.size = bi2->h;
		  bi->y = bi2->y + bi2->h;
	       }

	     t = bi->gcc->state_info.seq;
	     bi->gcc->state_info.seq = bi2->gcc->state_info.seq;
	     bi2->gcc->state_info.seq = t;

	     lc->items = evas_list_remove_list(lc->items, evas_list_last(lc->items));
	     lc->items = evas_list_append(lc->items, bi2);
	     lc->items = evas_list_append(lc->items, bi);
	     lc2->items = evas_list_free(lc2->items);
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
		  for (l = lc->items; l; l = l->next)
		    {
		       bi = l->data;
		       if (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE)
			 {
			    move_lc1 = 0;
			    break;
			 }
		    }
		  for (l = lc2->items; l; l = l->next)
		    {
		       bi = l->data;
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

		  for (l = lc->items; l; l = l->next)
		    {
		       bi = l->data;
		       if (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE)
			 {
			    move_lc1 = 0;
			    break;
			 }
		    }
		  for (l = lc2->items; l; l = l->next)
		    {
		       bi = l->data;
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

		  for (l = lc->items; l; l = l->next)
		    {
		       bi = l->data;
		       if ((bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_POSITION) &&
			   (bi->gcc->state_info.flags & E_GADCON_LAYOUT_ITEM_LOCK_ABSOLUTE))
			 {
			    if (sd->horizontal)
			      shift = bi->ask.pos - bi->x;
			    else
			      shift = bi->ask.pos - bi->y;
			    break;
			 }
		    }

		  for (l = lc->items; l && shift; l = l->next)
		    {
		       bi = l->data;

		       if (sd->horizontal)
			 bi->x += shift;
		       else
			 bi->y += shift;

		       if (l == lc->items)
			 {
			    if (sd->horizontal)
			      lc->pos = bi->x;
			    else
			      lc->pos = bi->y;
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
	     for (l = lc->items; l; l = l->next)
	       lc3->items = evas_list_append(lc3->items, l->data);
	     for (l = lc2->items; l; l = l->next)
	       lc3->items = evas_list_append(lc3->items, l->data);

	     lc3->state_info.min_seq = lc->state_info.min_seq;
	     if (lc2->items)
	       lc3->state_info.max_seq = lc2->state_info.max_seq;
	     else
	       lc3->state_info.max_seq = lc->state_info.max_seq;
	  }
	else
	  {
	     lc3->pos = lc2->pos;
	     for (l = lc2->items; l; l = l->next)
	       lc3->items = evas_list_append(lc3->items, l->data);
	     for (l = lc->items; l; l = l->next)
	       lc3->items = evas_list_append(lc3->items, l->data);

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
_e_gadcon_layout_smart_position_items_inside_container(E_Smart_Data *sd, E_Layout_Item_Container *lc)
{
   int shift;
   Evas_List *l;
   E_Gadcon_Layout_Item *bi;

   if (!lc->items) return;

   bi = lc->items->data;
   if (sd->horizontal)
     shift = lc->pos - bi->x;
   else
     shift = lc->pos - bi->y;
   if (!shift) return;

   for (l = lc->items; l; l = l->next)
     {
	bi = l->data;
	if (sd->horizontal)
	  bi->x += shift;
	else
	  bi->y += shift;

	if ((bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_POS_DEC) ||
	    (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_POS_INC))
	  {
	     if (sd->horizontal)
	       bi->gcc->config.pos = bi->ask.pos = bi->x;
	     else
	       bi->gcc->config.pos = bi->ask.pos = bi->y;
	  }
     }
} 

static void
_e_gadcon_layout_smart_containers_merge(E_Smart_Data *sd, E_Layout_Item_Container *lc, E_Layout_Item_Container *lc2)
{
   int start = 0, size = 0, next = 0, min_seq = 0, max_seq = 0;
   Evas_List *l, *nl = NULL;
   E_Gadcon_Layout_Item *bi;

   for (l = lc->items; l; l = l->next)
     nl = evas_list_append(nl, l->data);
   for (l = lc2->items; l; l = l->next)
     nl = evas_list_append(nl, l->data);

   nl = evas_list_sort(nl, evas_list_count(nl), _e_gadcon_layout_smart_seq_sort_cb);

   for (l = nl; l; l = l->next)
     {
	bi = l->data;
	if (l == nl)
	  {
	     min_seq = max_seq = bi->gcc->state_info.seq;
	     if (sd->horizontal)
	       {
		  start = bi->x;
		  size = bi->w;
		  next = bi->x + bi->w;
	       }
	     else
	       {
		  start = bi->y;
		  size = bi->h;
		  next = bi->y + bi->h;
	       }
	     continue;
	  }

	max_seq = bi->gcc->state_info.seq;
	if (sd->horizontal)
	  {
	     bi->x = next;
	     size += bi->w;
	     next = bi->x + bi->w;
	  }
	else
	  {
	     bi->y = next;
	     size += bi->h;
	     next = bi->y + bi->h;
	  }
     }

   lc->items = evas_list_free(lc->items);
   lc2->items = evas_list_free(lc->items);
   lc->items = nl;
   lc->pos = start;
   lc->size = size;
   lc->state_info.min_seq = min_seq;
   lc->state_info.max_seq = max_seq;
   lc2->pos = lc->pos + lc->size;
   lc2->size = 0;
} 

static void
_e_gadcon_layout_smart_restore_gadcons_position_before_move(E_Smart_Data *sd, E_Layout_Item_Container **lc_moving, E_Layout_Item_Container *lc_back, Evas_List **con_list)
{
   int ok;
   Evas_List *l, *l2, *l3;
   E_Gadcon_Layout_Item *bi, *bi2;
   E_Layout_Item_Container *lc, *lc2, *lc3;

   (*lc_moving)->pos = (*lc_moving)->prev_pos;
   if (((*lc_moving)->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MIN_END_INC) || 
       ((*lc_moving)->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MIN_END_DEC) || 
       ((*lc_moving)->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MAX_END_INC) || 
       ((*lc_moving)->state == E_LAYOUT_ITEM_CONTAINER_STATE_SIZE_MAX_END_DEC))
     {
	(*lc_moving)->size = (*lc_moving)->prev_size;
	bi = (*lc_moving)->items->data;
	if (sd->horizontal)
	  bi->w = (*lc_moving)->prev_size;
	else
	  bi->h = (*lc_moving)->prev_size;

     }
   _e_gadcon_layout_smart_position_items_inside_container(sd, (*lc_moving));
   (*lc_moving)->state = E_LAYOUT_ITEM_CONTAINER_STATE_POS_LOCKED;
   _e_gadcon_layout_smart_gadcons_position_static(sd, con_list);

   lc2 = NULL;
   lc3 = NULL;
   ok = 0;
   for (l = *con_list; l; l = l->next)
     {
	lc = l->data;
	if (lc->state == E_LAYOUT_ITEM_CONTAINER_STATE_NONE) continue;

	if (evas_list_count(lc->items) == 1)
	  {
	     bi = lc->items->data;
	     if ((bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_POS_INC) || 
		 (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_POS_DEC) || 
		 (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_SIZE_MIN_END_INC) || 
		 (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_SIZE_MIN_END_DEC) || 
		 (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_SIZE_MAX_END_INC) || 
		 (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_SIZE_MAX_END_DEC))
	       {
		  LC_FREE(lc);
		  l->data = *lc_moving = lc_back;
		  _e_gadcon_layout_smart_position_items_inside_container(sd, (*lc_moving));

		  if (((*lc_moving)->state != E_LAYOUT_ITEM_CONTAINER_STATE_POS_INC) &&
		      ((*lc_moving)->state != E_LAYOUT_ITEM_CONTAINER_STATE_POS_DEC))
		    {
		       bi = (*lc_moving)->items->data;
		       if (sd->horizontal)
			 bi->w = (*lc_moving)->size;
		       else
			 bi->h = (*lc_moving)->size;
		    }
	       }
	  }
	else
	  {
	     for (l2 = lc->items; l2; l2 = l2->next)
	       {
		  bi = l2->data; 
		  if ((bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_POS_INC) || 
		      (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_POS_DEC) || 
		      (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_SIZE_MIN_END_INC) || 
		      (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_SIZE_MIN_END_DEC) || 
		      (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_SIZE_MAX_END_INC) || 
		      (bi->gcc->state_info.state == E_LAYOUT_ITEM_STATE_SIZE_MAX_END_DEC))
		    {
		       ok = 1;
		       if (l2 != lc->items)
			 {
			    lc2 = E_NEW(E_Layout_Item_Container, 1);
			    lc2->sd = sd;
			    lc2->state = E_LAYOUT_ITEM_CONTAINER_STATE_NONE;
			    for (l3 = lc->items; l3 != l2; l3 = l3->next)
			      {
				 bi2 = l3->data;
				 lc2->items = evas_list_append(lc2->items, bi2);
				 if (l3 == lc->items)
				   {
				      lc2->state_info.min_seq = bi2->gcc->state_info.seq;
				      if (sd->horizontal)
					lc2->pos = lc2->prev_pos = bi2->x;
				      else
					lc2->pos = lc2->prev_pos = bi2->y;
				   }
				 lc2->state_info.max_seq = bi2->gcc->state_info.seq;
				 E_LAYOUT_ITEM_CONTAINER_SIZE_CHANGE_BY(lc2, bi2, 1);
			      }
			 }

		       if (l2->next)
			 {
			    lc3 = E_NEW(E_Layout_Item_Container, 1);
			    lc3->sd = sd;
			    lc3->state = E_LAYOUT_ITEM_CONTAINER_STATE_NONE;
			    for (l3 = l2->next; l3; l3 = l3->next)
			      {
				 bi2 = l3->data;
				 lc3->items = evas_list_append(lc3->items, bi2);
				 if (l3 == l2->next)
				   {
				      lc3->state_info.min_seq = bi2->gcc->state_info.seq;
				      if (sd->horizontal)
					lc3->pos = lc3->prev_pos = bi2->x;
				      else
					lc3->pos = lc3->prev_pos = bi2->y;
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
			    bi = (*lc_moving)->items->data; 
			    if (sd->horizontal) 
			      bi->w = (*lc_moving)->size; 
			    else 
			      bi->h = (*lc_moving)->size; 
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
		       *con_list = evas_list_append(*con_list, *lc_moving);
		       if (lc3)
			 *con_list = evas_list_append(*con_list, lc3);
		       *con_list = evas_list_sort(*con_list, evas_list_count(*con_list),
						  _e_gadcon_layout_smart_containers_sort_cb);
		    }
		  else
		    {
		       l->data = *lc_moving;
		       if (lc3)
			 {
			    *con_list = evas_list_append(*con_list, lc3);
			    *con_list = evas_list_sort(*con_list, evas_list_count(*con_list),
						       _e_gadcon_layout_smart_containers_sort_cb);
			 }
		    }
		  break;
	       }
	  }
     }

   for (l = *con_list; l; l = l->next)
     {
	lc = l->data;
	if (lc == *lc_moving) continue;
	lc->state = E_LAYOUT_ITEM_CONTAINER_STATE_NONE;
     }
}

