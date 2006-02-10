/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _e_gadcon_free(E_Gadcon *gc);
static void _e_gadcon_client_free(E_Gadcon_Client *gcc);

static void _e_gadcon_client_save(E_Gadcon_Client *gcc);

static void _e_gadcon_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_mouse_in(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_mouse_out(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_move(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_gadcon_cb_resize(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static void _e_gadcon_cb_signal_move_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_move_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_move_go(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_left_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_left_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_left_go(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_right_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_right_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_right_go(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_up_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_up_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_up_go(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_down_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_down_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadcon_cb_signal_resize_down_go(void *data, Evas_Object *obj, const char *emission, const char *source);

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
static void e_gadcon_layout_pack_options_set(Evas_Object *obj, int pos, int size, int res);
static void e_gadcon_layout_pack_min_size_set(Evas_Object *obj, int w, int h);
static void e_gadcon_layout_pack_aspect_set(Evas_Object *obj, int w, int h);
static void e_gadcon_layout_unpack(Evas_Object *obj);

static Evas_Hash *providers = NULL;
static Evas_List *gadcons = NULL;
   
static E_Gadcon_Client *
__test(E_Gadcon *gc, char *name, char *id)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   
   printf("create gadcon client \"%s\" \"%s\" for \"%s\" \"%s\"\n",
	  name, id, 
	  gc->name, gc->id);
   o = evas_object_rectangle_add(gc->evas);
   evas_object_color_set(o, rand() & 0xff, rand() & 0xff, rand() & 0xff, 150);
   gcc = e_gadcon_client_new(gc, name, id, o);
   gcc->data = NULL; // this is where a module would hook private data
   return gcc;
}

static void
__test2(E_Gadcon_Client *gcc)
{
   evas_object_del(gcc->o_base);
}

/* externally accessible functions */
EAPI int
e_gadcon_init(void)
{
   /* FIXME: these would be provided by modules registering gadget creation
    * classes */
     {
	static E_Gadcon_Client_Class cc = 
	  {
	     GADCON_CLIENT_CLASS_VERSION,
	       "ibar",
	       {
		  __test, __test2, NULL
	       }
	  };
	e_gadcon_provider_register(&cc);
     }
     {
	static E_Gadcon_Client_Class cc = 
	  {
	     GADCON_CLIENT_CLASS_VERSION,
	       "clock",
	       {
		  __test, __test2, NULL
	       }
	  };
	e_gadcon_provider_register(&cc);
     }
   return 1;
}

EAPI int
e_gadcon_shutdown(void)
{
   return 1;
}

EAPI void
e_gadcon_provider_register(E_Gadcon_Client_Class *cc)
{
   providers = evas_hash_direct_add(providers, cc->name, cc);
}

EAPI void
e_gadcon_provider_unregister(E_Gadcon_Client_Class *cc)
{
   providers = evas_hash_del(providers, cc->name, cc);
}

EAPI E_Gadcon *
e_gadcon_swallowed_new(char *name, char *id, Evas_Object *obj, char *swallow_name)
{
   E_Gadcon    *gc;
   
   gc = E_OBJECT_ALLOC(E_Gadcon, E_GADCON_TYPE, _e_gadcon_free);
   if (!gc) return NULL;
   
   gc->name = evas_stringshare_add(name);
   gc->id = evas_stringshare_add(id);
   gc->layout_policy = E_GADCON_LAYOUT_POLICY_PANEL;
   
   gc->edje.o_parent = obj;
   gc->edje.swallow_name = evas_stringshare_add(swallow_name);

   gc->orient = E_GADCON_ORIENT_HORIZ;
   gc->evas = evas_object_evas_get(obj);
   gc->o_container = e_gadcon_layout_add(gc->evas);
   evas_object_show(gc->o_container);
   edje_object_part_swallow(gc->edje.o_parent, gc->edje.swallow_name, gc->o_container);
   gadcons = evas_list_append(gadcons, gc);
   return gc;
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
   int ok;
   E_Config_Gadcon *cf_gc;
   E_Config_Gadcon_Client *cf_gcc;
   
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   ok = 0;
   e_gadcon_layout_freeze(gc->o_container);
   printf("e_config->gadcons = %p\n", e_config->gadcons);
   for (l = e_config->gadcons; l; l = l->next)
     {
	cf_gc = l->data;
	printf("%s == %s, %s == %s\n", cf_gc->name, gc->name, cf_gc->id, gc->id);
	if ((!strcmp(cf_gc->name, gc->name)) &&
	    (!strcmp(cf_gc->id, gc->id)))
	  {
	     ok = 1;
	     break;
	  }
     }
   if (ok)
     {
	for (l = cf_gc->clients; l; l = l->next)
	  {
	     E_Gadcon_Client_Class *cc;
	     
	     cf_gcc = l->data;
	     cc = evas_hash_find(providers, cf_gcc->name);
	     if (cc)
	       {
		  E_Gadcon_Client *gcc;
		  
		  gcc = cc->func.init(gc, cf_gcc->name, cf_gcc->id);
		  if (gcc)
		    {
		       gcc->client_class = *cc;
		       gcc->config.pos = cf_gcc->geom.pos;
		       gcc->config.size = cf_gcc->geom.size;
		       gcc->config.res = cf_gcc->geom.res;
		       e_gadcon_layout_pack_options_set(gcc->o_base,
							gcc->config.pos, 
							gcc->config.size,
							gcc->config.res);
		       if (gcc->client_class.func.orient)
			 gcc->client_class.func.orient(gcc);
		    }
	       }
	  }
     }
   e_gadcon_layout_thaw(gc->o_container);
}

EAPI void
e_gadcon_orient(E_Gadcon *gc, E_Gadcon_Orient orient)
{
   Evas_List *l;
   
   E_OBJECT_CHECK(gc);
   E_OBJECT_TYPE_CHECK(gc, E_GADCON_TYPE);
   if (gc->orient == orient) return;
   gc->orient = orient;
   e_gadcon_layout_freeze(gc->o_container);
   for (l = gc->clients; l; l = l->next)
     {
	E_Gadcon_Client *gcc;
	
	gcc = l->data;
	if (gcc->client_class.func.orient)
	  gcc->client_class.func.orient(gcc);
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
   for (l = gc->clients; l; l = l->next)
     {
	E_Gadcon_Client *gcc;
	
	gcc = l->data;
	e_gadcon_client_edit_end(gcc);
     }
   e_gadcon_layout_thaw(gc->o_container);
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

EAPI E_Gadcon_Client *
e_gadcon_client_new(E_Gadcon *gc, char *name, char *id, Evas_Object *base_obj)
{
   E_Gadcon_Client *gcc;
   
   E_OBJECT_CHECK_RETURN(gc, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(gc, E_GADCON_TYPE, NULL);
   gcc = E_OBJECT_ALLOC(E_Gadcon_Client, E_GADCON_CLIENT_TYPE, _e_gadcon_client_free);
   if (!gcc) return NULL;
   gcc->gadcon = gc;
   gcc->o_base = base_obj;
   gcc->name = evas_stringshare_add(name);
   gcc->id = evas_stringshare_add(id);
   gc->clients = evas_list_append(gc->clients, gcc);
   e_gadcon_layout_pack(gc->o_container, gcc->o_base);
   evas_object_show(gcc->o_base);
   return gcc;
}

EAPI void
e_gadcon_client_edit_begin(E_Gadcon_Client *gcc)
{
   Evas_Coord x, y, w, h;
   
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);

   if (gcc->o_control) return;
   
   gcc->o_control = edje_object_add(gcc->gadcon->evas);
   evas_object_layer_set(gcc->o_control, 100);
   evas_object_geometry_get(gcc->o_base, &x, &y, &w, &h);
   evas_object_move(gcc->o_control, x, y);
   evas_object_resize(gcc->o_control, w, h);
   e_theme_edje_object_set(gcc->o_control, "base/theme/gadman",
			   "gadman/control");

   edje_object_signal_emit(gcc->o_control, "hsize", "off");
   edje_object_signal_emit(gcc->o_control, "vsize", "off");
   edje_object_signal_emit(gcc->o_control, "move", "on");
   
   gcc->o_event = evas_object_rectangle_add(gcc->gadcon->evas);
   evas_object_color_set(gcc->o_event, 0, 0, 0, 0);
   evas_object_repeat_events_set(gcc->o_event, 1);
   evas_object_layer_set(gcc->o_event, 100);
   evas_object_move(gcc->o_event, x, y);
   evas_object_resize(gcc->o_event, w, h);

   edje_object_signal_callback_add(gcc->o_control, "move_start", "",
				   _e_gadcon_cb_signal_move_start, gcc);
   edje_object_signal_callback_add(gcc->o_control, "move_stop", "",
				   _e_gadcon_cb_signal_move_stop, gcc);
   edje_object_signal_callback_add(gcc->o_control, "move_go", "",
				   _e_gadcon_cb_signal_move_go, gcc);
   edje_object_signal_callback_add(gcc->o_control, "resize_start", "left",
				   _e_gadcon_cb_signal_resize_left_start, gcc);
   edje_object_signal_callback_add(gcc->o_control, "resize_stop", "left",
				   _e_gadcon_cb_signal_resize_left_stop, gcc);
   edje_object_signal_callback_add(gcc->o_control, "resize_go", "left",
				   _e_gadcon_cb_signal_resize_left_go, gcc);
   edje_object_signal_callback_add(gcc->o_control, "resize_start", "right",
				   _e_gadcon_cb_signal_resize_right_start, gcc);
   edje_object_signal_callback_add(gcc->o_control, "resize_stop", "right",
				   _e_gadcon_cb_signal_resize_right_stop, gcc);
   edje_object_signal_callback_add(gcc->o_control, "resize_go", "right",
				   _e_gadcon_cb_signal_resize_right_go, gcc);
   edje_object_signal_callback_add(gcc->o_control, "resize_start", "up",
				   _e_gadcon_cb_signal_resize_up_start, gcc);
   edje_object_signal_callback_add(gcc->o_control, "resize_stop", "up",
				   _e_gadcon_cb_signal_resize_up_stop, gcc);
   edje_object_signal_callback_add(gcc->o_control, "resize_go", "up",
				   _e_gadcon_cb_signal_resize_up_go, gcc);
   edje_object_signal_callback_add(gcc->o_control, "resize_start", "down",
				   _e_gadcon_cb_signal_resize_down_start, gcc);
   edje_object_signal_callback_add(gcc->o_control, "resize_stop", "down",
				   _e_gadcon_cb_signal_resize_down_stop, gcc);
   edje_object_signal_callback_add(gcc->o_control, "resize_go", "down",
				   _e_gadcon_cb_signal_resize_down_go, gcc);
   
   evas_object_event_callback_add(gcc->o_event, EVAS_CALLBACK_MOUSE_DOWN, _e_gadcon_cb_mouse_down, gcc);
   evas_object_event_callback_add(gcc->o_event, EVAS_CALLBACK_MOUSE_UP, _e_gadcon_cb_mouse_up, gcc);
   evas_object_event_callback_add(gcc->o_event, EVAS_CALLBACK_MOUSE_IN, _e_gadcon_cb_mouse_in, gcc);
   evas_object_event_callback_add(gcc->o_event, EVAS_CALLBACK_MOUSE_OUT, _e_gadcon_cb_mouse_out, gcc);

   evas_object_event_callback_add(gcc->o_base, EVAS_CALLBACK_MOVE, _e_gadcon_cb_move, gcc);
   evas_object_event_callback_add(gcc->o_base, EVAS_CALLBACK_RESIZE, _e_gadcon_cb_resize, gcc);
   
   evas_object_show(gcc->o_event);
   evas_object_show(gcc->o_control);
}

EAPI void
e_gadcon_client_edit_end(E_Gadcon_Client *gcc)
{
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);
   if (gcc->moving)
     {
	gcc->moving = 0;
	_e_gadcon_client_save(gcc);
     }
   if (gcc->o_event) evas_object_del(gcc->o_event);
   gcc->o_event = NULL;
   if (gcc->o_control) evas_object_del(gcc->o_control);
   gcc->o_control = NULL;
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
	e_gadcon_layout_pack_size_set(gcc->o_base, w);
	break;
      case E_GADCON_ORIENT_VERT:
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_RIGHT:
	e_gadcon_layout_pack_size_set(gcc->o_base, h);
	break;
      default:
	break;
     }
}

EAPI void
e_gadcon_client_min_size_set(E_Gadcon_Client *gcc, Evas_Coord w, Evas_Coord h)
{
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);
   e_gadcon_layout_pack_min_size_set(gcc->o_base, w, h);
}

EAPI void
e_gadcon_client_aspect_set(E_Gadcon_Client *gcc, int w, int h)
{
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);
   e_gadcon_layout_pack_aspect_set(gcc->o_base, w, h);
}

/* local subsystem functions */
static void
_e_gadcon_free(E_Gadcon *gc)
{
   gadcons = evas_list_remove(gadcons, gc);
   if (gc->o_container) evas_object_del(gc->o_container);
   evas_stringshare_del(gc->name);
   evas_stringshare_del(gc->id);
   evas_stringshare_del(gc->edje.swallow_name);
   free(gc);
}

static void
_e_gadcon_client_free(E_Gadcon_Client *gcc)
{
   gcc->gadcon->clients = evas_list_remove(gcc->gadcon->clients, gcc);
   evas_stringshare_del(gcc->name);
   evas_stringshare_del(gcc->id);
   free(gcc);
}

static void
_e_gadcon_client_save(E_Gadcon_Client *gcc)
{
   Evas_List *l, *l2;
   E_Config_Gadcon *cf_gc;
   E_Config_Gadcon_Client *cf_gcc;
   int ok;
   
   ok = 0;
   for (l = e_config->gadcons; l; l = l->next)
     {
	cf_gc = l->data;
	if ((!strcmp(cf_gc->name, gcc->gadcon->name)) &&
	    (!strcmp(cf_gc->id, gcc->gadcon->id)))
	  {
	     ok++;
	     for (l2 = cf_gc->clients; l2; l2 = l2->next)
	       {
		  cf_gcc = l2->data;
		  
		  if ((!strcmp(cf_gcc->name, gcc->name)) &&
		      (!strcmp(cf_gcc->id, gcc->id)))
		    {
		       cf_gcc->geom.pos = gcc->config.pos;
		       cf_gcc->geom.size = gcc->config.size;
		       cf_gcc->geom.res = gcc->config.res;
		       ok++;
		       break;
		    }
	       }
	     break;
	  }
     }
   if (ok == 0)
     {
	cf_gc = E_NEW(E_Config_Gadcon, 1);
	cf_gc->name = evas_stringshare_add(gcc->gadcon->name);
	cf_gc->id = evas_stringshare_add(gcc->gadcon->id);
	e_config->gadcons = evas_list_append(e_config->gadcons, cf_gc);
	ok++;
     }
   if (ok == 1)
     {
	cf_gcc = E_NEW(E_Config_Gadcon_Client, 1);
	cf_gcc->name = evas_stringshare_add(gcc->name);
	cf_gcc->id = evas_stringshare_add(gcc->id);
	cf_gcc->geom.pos = gcc->config.pos;
	cf_gcc->geom.size = gcc->config.size;
	cf_gcc->geom.res = gcc->config.res;
	cf_gc->clients = evas_list_append(cf_gc->clients, cf_gcc);
	ok++;
     }
   e_config_save_queue();
}

static void
_e_gadcon_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Gadcon_Client *gcc;
      
   gcc = data;
   ev = event_info;
   if (ev->button == 3)
     {
     }
}

static void
_e_gadcon_cb_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   E_Gadcon_Client *gcc;
   
   gcc = data;
   ev = event_info;
}

static void
_e_gadcon_cb_mouse_in(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_In *ev;
   E_Gadcon_Client *gcc;
   
   gcc = data;
   ev = event_info;
   edje_object_signal_emit(gcc->o_control, "active", "");
}

static void
_e_gadcon_cb_mouse_out(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Out *ev;
   E_Gadcon_Client *gcc;
   
   gcc = data;
   ev = event_info;
   edje_object_signal_emit(gcc->o_control, "inactive", "");
}

static void
_e_gadcon_cb_move(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Gadcon_Client *gcc;
   Evas_Coord x, y;
   
   gcc = data;
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   if (gcc->o_control) evas_object_move(gcc->o_control, x, y);
   if (gcc->o_event) evas_object_move(gcc->o_event, x, y);
}

static void
_e_gadcon_cb_resize(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Gadcon_Client *gcc;
   Evas_Coord w, h;
   
   gcc = data;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   if (gcc->o_control) evas_object_resize(gcc->o_control, w, h);
   if (gcc->o_event) evas_object_resize(gcc->o_event, w, h);
}

static void
_e_gadcon_cb_signal_move_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   Evas_Coord x, y;
   
   gcc = data;
   evas_object_raise(gcc->o_event);
   evas_object_stack_below(gcc->o_control, gcc->o_event);
   gcc->moving = 1;
   evas_pointer_canvas_xy_get(gcc->gadcon->evas, &gcc->dx, &gcc->dy);
   evas_object_geometry_get(gcc->gadcon->o_container, &x, &y, NULL, NULL);
   evas_object_geometry_get(gcc->o_base, &gcc->sx, &gcc->sy, NULL, NULL);
   gcc->sx -= x;
   gcc->sy -= y;
}

static void
_e_gadcon_cb_signal_move_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   
   gcc = data;
   gcc->moving = 0;
   _e_gadcon_client_save(gcc);
}

static void
_e_gadcon_cb_signal_move_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   Evas_Coord x, y, w, h;
   
   gcc = data;
   if (!gcc->moving) return;
   evas_pointer_canvas_xy_get(gcc->gadcon->evas, &x, &y);
   x = x - gcc->dx;
   y = y - gcc->dy;
   evas_object_geometry_get(gcc->o_base, NULL, NULL, &w, &h);
   if (e_gadcon_layout_orientation_get(gcc->gadcon->o_container))
     {
	e_gadcon_layout_pack_request_set(gcc->o_base, gcc->sx + x, w);
	gcc->config.pos = gcc->sx + x;
	gcc->config.size = w;
	evas_object_geometry_get(gcc->gadcon->o_container, NULL, NULL, &w, &h);
	gcc->config.res = w;
     }
   else
     {
	e_gadcon_layout_pack_request_set(gcc->o_base, gcc->sy + y, h);
	gcc->config.pos = gcc->sy + y;
	gcc->config.size = h;
	evas_object_geometry_get(gcc->gadcon->o_container, NULL, NULL, &w, &h);
	gcc->config.res = h;
     }
}

static void
_e_gadcon_cb_signal_resize_left_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   
   gcc = data;
}

static void
_e_gadcon_cb_signal_resize_left_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   
   gcc = data;
}

static void
_e_gadcon_cb_signal_resize_left_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   
   gcc = data;
}

static void
_e_gadcon_cb_signal_resize_right_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   
   gcc = data;
}

static void
_e_gadcon_cb_signal_resize_right_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   
   gcc = data;
}

static void
_e_gadcon_cb_signal_resize_right_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   
   gcc = data;
}

static void
_e_gadcon_cb_signal_resize_up_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   
   gcc = data;
}

static void
_e_gadcon_cb_signal_resize_up_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   
   gcc = data;
}

static void
_e_gadcon_cb_signal_resize_up_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   
   gcc = data;
}

static void
_e_gadcon_cb_signal_resize_down_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   
   gcc = data;
}

static void
_e_gadcon_cb_signal_resize_down_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   
   gcc = data;
}

static void
_e_gadcon_cb_signal_resize_down_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadcon_Client *gcc;
   
   gcc = data;
}





/* a smart object JUST for gadcon */

typedef struct _E_Smart_Data E_Smart_Data;
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
}; 

struct _E_Gadcon_Layout_Item
{
   E_Smart_Data    *sd;
   struct {
      int           pos, size, size2, res;
   } ask;
   int              hookp;
   struct {
      int           w, h;
   } min, aspect;
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
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return 0;
   return sd->horizontal;
}

static void
e_gadcon_layout_freeze(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->frozen++;
}

static void
e_gadcon_layout_thaw(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->frozen--;
   _e_gadcon_layout_smart_reconfigure(sd);
}

static void
e_gadcon_layout_min_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   E_Smart_Data *sd;
   Evas_List *l;
   Evas_Coord tw = 0, th = 0;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   
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
}

static void
e_gadcon_layout_asked_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   E_Smart_Data *sd;
   Evas_List *l;
   Evas_Coord tw = 0, th = 0;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
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
   if (w) *w = tw;
   if (h) *h = th;
}

static int
e_gadcon_layout_pack(Evas_Object *obj, Evas_Object *child)
{
   E_Smart_Data *sd;
   
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
   E_Gadcon_Layout_Item *bi;
   int xx;
   
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
	       {
		  bi->ask.size = size;
	       }
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
e_gadcon_layout_pack_options_set(Evas_Object *obj, int pos, int size, int res)
{
   E_Gadcon_Layout_Item *bi;
   
   bi = evas_object_data_get(obj, "e_gadcon_layout_data");
   if (!bi) return;
   bi->ask.res = res;
   bi->ask.size = size;
   bi->ask.pos = pos;
   _e_gadcon_layout_smart_reconfigure(bi->sd);
}

static void
e_gadcon_layout_pack_min_size_set(Evas_Object *obj, int w, int h)
{
   E_Gadcon_Layout_Item *bi;
   
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
   
   bi = evas_object_data_get(obj, "e_gadcon_layout_data");
   if (!bi) return;
   bi->aspect.w = w;
   bi->aspect.h = h;
   _e_gadcon_layout_smart_reconfigure(bi->sd);
}

static void
e_gadcon_layout_unpack(Evas_Object *obj)
{
   E_Gadcon_Layout_Item *bi;
   E_Smart_Data *sd;
   
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
   
   bi = calloc(1, sizeof(E_Gadcon_Layout_Item));
   if (!bi) return NULL;
   bi->sd = sd;
   bi->obj = obj;
   /* defaults */
   evas_object_clip_set(obj, sd->clip);
   evas_object_smart_member_add(obj, bi->sd->obj);
   evas_object_data_set(obj, "e_gadcon_layout_data", bi);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_FREE,
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
   
   bi = evas_object_data_get(obj, "e_gadcon_layout_data");
   if (!bi) return;
   if (!bi->sd->items)
     {
	if (evas_object_visible_get(bi->sd->clip))
	  evas_object_hide(bi->sd->clip);
     }
   evas_object_event_callback_del(obj,
				  EVAS_CALLBACK_FREE,
				  _e_gadcon_layout_smart_item_del_hook);
   evas_object_smart_member_del(obj);
   evas_object_clip_unset(obj);
   evas_object_data_del(obj, "e_gadcon_layout_data");
   free(bi);
}

static void
_e_gadcon_layout_smart_item_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_gadcon_layout_unpack(obj);
}

static int
_e_gadcon_sort_cb(void *d1, void *d2)
{
   E_Gadcon_Layout_Item *bi1, *bi2;
   int v1, v2;
   
   bi1 = evas_object_data_get(d1, "e_gadcon_layout_data");
   bi2 = evas_object_data_get(d2, "e_gadcon_layout_data");
   v1 = (bi1->ask.pos + (bi1->ask.size / 2)) - bi1->hookp;
   if (v1 < 0) v1 = -v1;
   v2 = (bi2->ask.pos + (bi2->ask.size / 2)) - bi2->hookp;
   if (v2 < 0) v2 = -v2;
   return v1 - v2;
}    

static int
_e_gadcon_sort_all_cb(void *d1, void *d2)
{
   E_Gadcon_Layout_Item *bi1, *bi2;
   int v1, v2;
   
   bi1 = evas_object_data_get(d1, "e_gadcon_layout_data");
   bi2 = evas_object_data_get(d2, "e_gadcon_layout_data");
   v1 = (bi1->ask.pos + (bi1->ask.size / 2));
   if (v1 < 0) v1 = -v1;
   v2 = (bi2->ask.pos + (bi2->ask.size / 2));
   if (v2 < 0) v2 = -v2;
   return v1 - v2;
}    

static void
_e_gadcon_layout_smart_reconfigure(E_Smart_Data *sd)
{
   Evas_Coord x, y, w, h, xx, yy;
   Evas_List *l, *l2;
   int min, cur;
   int count, expand;
   Evas_List *list_s = NULL, *list_m = NULL, *list_e = NULL, *list = NULL;

   if (sd->frozen) return;
   if (sd->doing_config)
     {
	sd->redo_config = 1;
	return;
     }
   
   x = sd->x;
   y = sd->y;
   w = sd->w;
   h = sd->h;

   min = 0;
   cur = 0;
   for (l = sd->items; l; l = l->next)
     {
	E_Gadcon_Layout_Item *bi;
	Evas_Object *obj;
	
	obj = l->data;
	bi = evas_object_data_get(obj, "e_gadcon_layout_data");
	cur += bi->ask.size;
        if (sd->horizontal)
	  min += bi->min.w;
	else
	  min += bi->min.h;
	bi->ask.size2 = bi->ask.size;
	if ((bi->aspect.w > 0) && (bi->aspect.h > 0))
	  {
	     if (sd->horizontal)
	       {
		  bi->ask.size2 = (h * bi->aspect.w) / bi->aspect.h;
	       }
	     else 
	       {
		  bi->ask.size2 = (w * bi->aspect.h) / bi->aspect.w;
	       }
	  }
     }
   if (sd->horizontal)
     {
	if (cur < w)
	  {
	     /* all is fine - it should all fit */
	  }
	else
	  {
	     int sub, give, num, given, i;
	     
	     sub = cur - w; /* we need to find "sub" extra pixels */
	     if (min <= w)
	       {
		  for (l = sd->items; l; l = l->next)
		    {
		       E_Gadcon_Layout_Item *bi;
		       Evas_Object *obj;
		       
		       obj = l->data;
		       bi = evas_object_data_get(obj, "e_gadcon_layout_data");
		       give = bi->ask.size - bi->min.w; // how much give does this have?
		       if (give < sub) give = sub;
		       bi->ask.size2 = bi->ask.size - give;
		       sub -= give;
		       if (sub <= 0) break;
		    }
	       }
	     else
	       { /* EEK - all items just cant fit at their minimum! what do we do? */
		  num = 0;
		  num = evas_list_count(sd->items);
		  give = min - w; // how much give total below minw we need
		  given = 0;
		  for (l = sd->items; l; l = l->next)
		    {
		       E_Gadcon_Layout_Item *bi;
		       Evas_Object *obj;
		       
		       obj = l->data;
		       bi = evas_object_data_get(obj, "e_gadcon_layout_data");
		       bi->ask.size2 = bi->min.w;
		       if (!l->next)
			 {
			    bi->ask.size2 -= (give - given);
			 }
		       else
			 {
			    i = (give + (num / 2)) / num;
			    given -= i;
			    bi->ask.size2 -= i;
			 }
		    }
	       }
	  }
     }
   else
     {
     }
   
   for (l = sd->items; l; l = l->next)
     {
	E_Gadcon_Layout_Item *bi;
	Evas_Object *obj;
	
	obj = l->data;
	bi = evas_object_data_get(obj, "e_gadcon_layout_data");
	list = evas_list_append(list, obj);
	if (sd->horizontal)
	  {
	     xx = bi->ask.pos + (bi->ask.size / 2);
	     if (xx < (bi->ask.res / 3))
	       { /* hooked to start */
		  bi->x = bi->ask.pos;
		  bi->w = bi->ask.size2;
		  list_s = evas_list_append(list_s, obj);
		  bi->hookp = 0;
	       }
	     else if (xx > ((2 * bi->ask.res) / 3))
	       { /* hooked to end */
		  bi->x = (bi->ask.pos - bi->ask.res) + w;
		  bi->w = bi->ask.size2;
		  list_e = evas_list_append(list_e, obj);
		  bi->hookp = bi->ask.res;
	       }
	     else
	       { /* hooked to middle */
		  if ((bi->ask.pos <= (bi->ask.res / 2)) &&
		      ((bi->ask.pos + bi->ask.size2) > (bi->ask.res / 2)))
		    { /* straddles middle */
		       if (bi->ask.res > 2)
			 bi->x = (w / 2) + 
			 (((bi->ask.pos + (bi->ask.size2 / 2) - 
			    (bi->ask.res / 2)) * 
			   (bi->ask.res / 2)) /
			  (bi->ask.res / 2)) - (bi->ask.size2 / 2);
		       else
			 bi->x = w / 2;
		       bi->w = bi->ask.size2;
		    }
		  else
		    { /* either side of middle */
		       bi->x = (bi->ask.pos - (bi->ask.res / 2)) + (w / 2);
		       bi->w = bi->ask.size2;
		    }
		  list_m = evas_list_append(list_m, obj);
		  bi->hookp = bi->ask.res / 2;
	       }
	     if (bi->x < 0) bi->x = 0;
	     else if ((bi->x + bi->w) > w) bi->x = w - bi->w;
	  }
	else
	  {
	     yy = bi->ask.pos + (bi->ask.size2 / 2);
	     if (yy < (bi->ask.res / 3))
	       { /* hooked to start */
		  bi->y = bi->ask.pos;
		  bi->h = bi->ask.size2;
		  list_s = evas_list_append(list_s, obj);
		  bi->hookp = 0;
	       }
	     else if (yy > ((2 * bi->ask.res) / 3))
	       { /* hooked to end */
		  bi->y = (bi->ask.pos - bi->ask.res) + h;
		  bi->h = bi->ask.size2;
		  list_e = evas_list_append(list_e, obj);
		  bi->hookp = bi->ask.res;
	       }
	     else
	       { /* hooked to middle */
		  if ((bi->ask.pos <= (bi->ask.res / 2)) &&
		      ((bi->ask.pos + bi->ask.size2) > (bi->ask.res / 2)))
		    { /* straddles middle */
		       if (bi->ask.res > 2)
			 bi->y = (h / 2) + 
			 (((bi->ask.pos + (bi->ask.size2 / 2) - 
			    (bi->ask.res / 2)) * 
			   (bi->ask.res / 2)) /
			  (bi->ask.res / 2)) - (bi->ask.size2 / 2);
		       else
			 bi->y = h / 2;
		       bi->h = bi->ask.size2;
		    }
		  else
		    { /* either side of middle */
		       bi->y = (bi->ask.pos - (bi->ask.res / 2)) + (h / 2);
		       bi->h = bi->ask.size2;
		    }
		  list_s = evas_list_append(list_s, obj);
		  bi->hookp = bi->ask.res / 2;
	       }
	     if (bi->y < 0) bi->y = 0;
	     else if ((bi->y + bi->h) > h) bi->y = h - bi->y;
	  }
     }
   list_s = evas_list_sort(list_s, evas_list_count(list_s), _e_gadcon_sort_cb);
   list_m = evas_list_sort(list_m, evas_list_count(list_m), _e_gadcon_sort_cb);
   list_e = evas_list_sort(list_e, evas_list_count(list_e), _e_gadcon_sort_cb);
   list = evas_list_sort(list, evas_list_count(list), _e_gadcon_sort_all_cb);
   for (l = list_s; l; l = l->next)
     {
	E_Gadcon_Layout_Item *bi;
	Evas_Object *obj;
	
	obj = l->data;
	bi = evas_object_data_get(obj, "e_gadcon_layout_data");
	again1:
	for (l2 = l->prev; l2; l2 = l2->prev)
	  {
	     E_Gadcon_Layout_Item *bi2;
	     
	     obj = l2->data;
	     bi2 = evas_object_data_get(obj, "e_gadcon_layout_data");
	     if (E_SPANS_COMMON(bi->x, bi->w, bi2->x, bi2->w))
	       {
		  bi->x = bi2->x + bi2->w;
		  goto again1;
	       }
	  }
     }
   for (l = list_m; l; l = l->next)
     {
	E_Gadcon_Layout_Item *bi;
	Evas_Object *obj;
	
	obj = l->data;
	bi = evas_object_data_get(obj, "e_gadcon_layout_data");
	again2:
	for (l2 = l->prev; l2; l2 = l2->prev)
	  {
	     E_Gadcon_Layout_Item *bi2;
	     
	     obj = l2->data;
	     bi2 = evas_object_data_get(obj, "e_gadcon_layout_data");
	     if (E_SPANS_COMMON(bi->x, bi->w, bi2->x, bi2->w))
	       {
		  if ((bi2->x + (bi2->w / 2)) < (w / 2))
		    bi->x = bi2->x - bi->w;
		  else
		    bi->x = bi2->x + bi2->w;
		  goto again2;
	       }
	  }
     }
   for (l = list_e; l; l = l->next)
     {
	E_Gadcon_Layout_Item *bi;
	Evas_Object *obj;
	
	obj = l->data;
	bi = evas_object_data_get(obj, "e_gadcon_layout_data");
	again3:
	for (l2 = l->prev; l2; l2 = l2->prev)
	  {
	     E_Gadcon_Layout_Item *bi2;
	     
	     obj = l2->data;
	     bi2 = evas_object_data_get(obj, "e_gadcon_layout_data");
	     if (E_SPANS_COMMON(bi->x, bi->w, bi2->x, bi2->w))
	       {
		  bi->x = bi2->x - bi->w;
		  goto again3;
	       }
	  }
     }
   for (l = list; l; l = l->next)
     {
	E_Gadcon_Layout_Item *bi;
	Evas_Object *obj;
	
	obj = l->data;
	bi = evas_object_data_get(obj, "e_gadcon_layout_data");
	bi->can_move = 1;
	if (!l->prev)
	  {
	     if (bi->x <= 0)
	       {
		  bi->x = 0;
		  bi->can_move = 0;
	       }
	  }
	if (!l->next)
	  {
	     if ((bi->x + bi->w) >= w)
	       {
		  bi->x = w - bi->w;
		  bi->can_move = 0;
	       }
	  }
     }
   if (sd->horizontal)
     {
	int overlap;
	int count;
	
	overlap = 1;
	count = 0;
	while (overlap)
	  {
	     overlap = 0;
	     for (l = list; l; l = l->next)
	       {
		  E_Gadcon_Layout_Item *bi;
		  Evas_Object *obj;
		  
		  obj = l->data;
		  bi = evas_object_data_get(obj, "e_gadcon_layout_data");
		  if (bi->can_move)
		    {
		       for (l2 = l->next; l2; l2 = l2->next)
			 {
			    E_Gadcon_Layout_Item *bi2;
			    
			    obj = l2->data;
			    bi2 = evas_object_data_get(obj, "e_gadcon_layout_data");
			    if (E_SPANS_COMMON(bi->x, bi->w, bi2->x, bi2->w))
			      {
				 bi->x = bi2->x - bi->w;
				 if (!bi2->can_move) bi->can_move = 0;
				 if ((bi->x + bi->w) >= w) bi->x = w - bi->w;
				 if (bi->x <= 0) bi->x = 0;
				 overlap = 1;
			      }
			 }
		       for (l2 = l->prev; l2; l2 = l2->prev)
			 {
			    E_Gadcon_Layout_Item *bi2;
			    
			    obj = l2->data;
			    bi2 = evas_object_data_get(obj, "e_gadcon_layout_data");
			    if (E_SPANS_COMMON(bi->x, bi->w, bi2->x, bi2->w))
			      {
				 bi->x = bi2->x + bi2->w;
				 if (!bi2->can_move) bi->can_move = 0;
				 if ((bi->x + bi->w) >= w) bi->x = w - bi->w;
				 if (bi->x <= 0) bi->x = 0;
				 overlap = 1;
			      }
			 }
		    }
	       }
	     count++;
	     if (count > 200) break; // quick infinite loop fix
	  }
     }
   else
     {
	/* FIXME: for how this is just a copy of the above but in the vertical
	 *        so when the above is "fixeD" the below needs to mirror it
	 */
	int overlap;
	int count;
	
	overlap = 1;
	count = 0;
	while (overlap)
	  {
	     overlap = 0;
	     for (l = list; l; l = l->next)
	       {
		  E_Gadcon_Layout_Item *bi;
		  Evas_Object *obj;
		  
		  obj = l->data;
		  bi = evas_object_data_get(obj, "e_gadcon_layout_data");
		  if (bi->can_move)
		    {
		       for (l2 = l->next; l2; l2 = l2->next)
			 {
			    E_Gadcon_Layout_Item *bi2;
			    
			    obj = l2->data;
			    bi2 = evas_object_data_get(obj, "e_gadcon_layout_data");
			    if (E_SPANS_COMMON(bi->y, bi->h, bi2->y, bi2->h))
			      {
				 bi->y = bi2->y - bi->h;
				 if (!bi2->can_move) bi->can_move = 0;
				 if ((bi->y + bi->h) >= h) bi->y = h - bi->h;
				 if (bi->y <= 0) bi->y = 0;
				 overlap = 1;
			      }
			 }
		       for (l2 = l->prev; l2; l2 = l2->prev)
			 {
			    E_Gadcon_Layout_Item *bi2;
			    
			    obj = l2->data;
			    bi2 = evas_object_data_get(obj, "e_gadcon_layout_data");
			    if (E_SPANS_COMMON(bi->y, bi->h, bi2->y, bi2->h))
			      {
				 bi->y = bi2->y + bi2->h;
				 if (!bi2->can_move) bi->can_move = 0;
				 if ((bi->y + bi->h) >= h) bi->y = h - bi->h;
				 if (bi->y <= 0) bi->y = 0;
				 overlap = 1;
			      }
			 }
		    }
	       }
	     count++;
	     if (count > 200) break; // quick infinite loop fix
	  }
     }
   
   evas_list_free(list_s);
   evas_list_free(list_m);
   evas_list_free(list_e);
   evas_list_free(list);
   
   for (l = sd->items; l; l = l->next)
     {
	E_Gadcon_Layout_Item *bi;
	Evas_Object *obj;
	
	obj = l->data;
	bi = evas_object_data_get(obj, "e_gadcon_layout_data");
	if (sd->horizontal)
	  {
	     bi->h = h;
	     xx = x + bi->x;
	     yy = y + ((h - bi->h) / 2);
	  }
	else
	  {
	     bi->w = w;
	     xx = x + ((w - bi->w) / 2);
	     yy = y + bi->y;
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
}

static void
_e_gadcon_layout_smart_init(void)
{
   if (_e_smart) return;
   _e_smart = evas_smart_new("e_gadcon_layout",
			     _e_gadcon_layout_smart_add,
			     _e_gadcon_layout_smart_del,
			     NULL, NULL, NULL, NULL, NULL,
			     _e_gadcon_layout_smart_move,
			     _e_gadcon_layout_smart_resize,
			     _e_gadcon_layout_smart_show,
			     _e_gadcon_layout_smart_hide,
			     _e_gadcon_layout_smart_color_set,
			     _e_gadcon_layout_smart_clip_set,
			     _e_gadcon_layout_smart_clip_unset,
			     NULL);
}

static void
_e_gadcon_layout_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
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
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->items) evas_object_show(sd->clip);
}

static void
_e_gadcon_layout_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_gadcon_layout_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;   
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_gadcon_layout_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_gadcon_layout_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
}  
