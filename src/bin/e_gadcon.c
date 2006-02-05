/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _e_gadcon_free(E_Gadcon *gc);
static void _e_gadcon_client_free(E_Gadcon_Client *gcc);

static Evas_Object *e_gadcon_layout_add(Evas *evas);
static void e_gadcon_layout_orientation_set(Evas_Object *obj, int horizontal);
static void e_gadcon_layout_min_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
static void e_gadcon_layout_asked_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
static int e_gadcon_layout_pack(Evas_Object *obj, Evas_Object *child);
static void e_gadcon_layout_pack_size_set(Evas_Object *obj, int size);
static void e_gadcon_layout_pack_request_set(Evas_Object *obj, int pos, int size);
static void e_gadcon_layout_pack_options_set(Evas_Object *obj, int pos, int size, int res);
static void e_gadcon_layout_unpack(Evas_Object *obj);

static Evas_Hash *providers = NULL;

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
	       "start",
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
	       "pager",
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
   gc->edje.o_parent = obj;
   gc->edje.swallow_name = evas_stringshare_add(swallow_name);

   gc->orient = E_GADCON_ORIENT_HORIZ;
   gc->evas = evas_object_evas_get(obj);
   gc->o_container = e_gadcon_layout_add(gc->evas);
   evas_object_show(gc->o_container);
   edje_object_part_swallow(gc->edje.o_parent, gc->edje.swallow_name, gc->o_container);
   return gc;
}

EAPI void
e_gadcon_populate(E_Gadcon *gc)
{
   Evas_List *l;
   int i;
   
   E_OBJECT_CHECK_RETURN(gc, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(gc, E_GADCON_TYPE, NULL);

   for (i = 0; i < 6; i++)
     {
	E_Gadcon_Client_Class *cc;
	char *name, *id;
	int pos, size, res;

	/* FIXME: a hardcoded sample that woudl cnormally be saved config
	 */
	if (i == 0) {
	   name = "ibar"; id = "0";
	   size = 280; pos = 400 - (size / 2); res = 800;
	} else if (i == 1) {
	   name = "start"; id = "0";
	   size = 32; pos = 0; res = 800;
	} else if (i == 2) {
	   name = "pager"; id = "0";
	   size = 200; pos = 0; res = 800;
	} else if (i == 3) {
	   name = "clock"; id = "0";
	   size = 32; pos = 800 - size; res = 800;
	} else if (i == 4) {
	   name = "clock"; id = "1";
	   size = 32; pos = 800 - size; res = 800;
	} else if (i == 5) {
	   name = "clock"; id = "2";
	   size = 32; pos = 800 - size; res = 800;
	}
	cc = evas_hash_find(providers, name);
	if (cc)
	  {
	     E_Gadcon_Client *gcc;
	     
	     gcc = cc->func.init(gc, cc->name, id);
	     if (gcc)
	       {
		  gcc->client_class = *cc;
		  e_gadcon_layout_pack_options_set(gcc->o_base, pos, size, res);
		  if (gcc->client_class.func.orient)
		    gcc->client_class.func.orient(gcc);
	       }
	  }
     }
}

EAPI void
e_gadcon_orient(E_Gadcon *gc, E_Gadcon_Orient orient)
{
   Evas_List *l;
   
   if (gc->orient == orient) return;
   gc->orient = orient;
   for (l = gc->clients; l; l = l->next)
     {
	E_Gadcon_Client *gcc;
	
	gcc = l->data;
	if (gcc->client_class.func.orient)
	  gcc->client_class.func.orient(gcc);
     }
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
}

EAPI void
e_gadcon_client_max_size_set(E_Gadcon_Client *gcc, Evas_Coord w, Evas_Coord h)
{
   E_OBJECT_CHECK(gcc);
   E_OBJECT_TYPE_CHECK(gcc, E_GADCON_CLIENT_TYPE);
}

/* local subsystem functions */
static void
_e_gadcon_free(E_Gadcon *gc)
{
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




/* a smart object JUST for gadcon */

typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Gadcon_Layout_Item  E_Gadcon_Layout_Item;

struct _E_Smart_Data
{ 
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   Evas_Object     *clip;
   unsigned char    horizontal : 1;
   Evas_List       *items;
}; 

struct _E_Gadcon_Layout_Item
{
   E_Smart_Data    *sd;
   struct {
      int           pos, size, res;
   } ask;
   int              hookp;
   struct {
      Evas_Coord    w, h;
   } min, max;
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
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

int
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

static void
_e_gadcon_layout_smart_reconfigure(E_Smart_Data *sd)
{
   Evas_Coord x, y, w, h, xx, yy;
   Evas_List *l, *l2;
   int minw, minh, wdif, hdif;
   int count, expand;
   Evas_List *list_s = NULL, *list_m = NULL, *list_e = NULL;

   x = sd->x;
   y = sd->y;
   w = sd->w;
   h = sd->h;

   for (l = sd->items; l; l = l->next)
     {
	E_Gadcon_Layout_Item *bi;
	Evas_Object *obj;
	
	obj = l->data;
	bi = evas_object_data_get(obj, "e_gadcon_layout_data");
	if (sd->horizontal)
	  {
	     xx = bi->ask.pos + (bi->ask.size / 2);
	     if (xx < (bi->ask.res / 3))
	       { /* hooked to start */
		  bi->x = bi->ask.pos;
		  bi->w = bi->ask.size;
		  list_s = evas_list_append(list_s, obj);
		  bi->hookp = 0;
	       }
	     else if (xx > ((2 * bi->ask.res) / 3))
	       { /* hooked to end */
		  bi->x = (bi->ask.pos - bi->ask.res) + w;
		  bi->w = bi->ask.size;
		  list_e = evas_list_append(list_e, obj);
		  bi->hookp = bi->ask.res;
	       }
	     else
	       { /* hooked to middle */
		  if ((bi->ask.pos <= (bi->ask.res / 2)) &&
		      ((bi->ask.pos + bi->ask.size) > (bi->ask.res / 2)))
		    { /* straddles middle */
		       if (bi->ask.res > 2)
			 bi->x = (w / 2) + 
			 (((bi->ask.pos + (bi->ask.size / 2) - 
			    (bi->ask.res / 2)) * 
			   (bi->ask.res / 2)) /
			  (bi->ask.res / 2)) - (bi->ask.size / 2);
		       else
			 bi->x = w / 2;
		       bi->w = bi->ask.size;
		    }
		  else
		    { /* either side of middle */
		       bi->x = (bi->ask.pos - (bi->ask.res / 2)) + (w / 2);
		       bi->w = bi->ask.size;
		    }
		  list_m = evas_list_append(list_m, obj);
		  bi->hookp = bi->ask.res / 2;
	       }
	     if (bi->x < 0) bi->x = 0;
	     else if ((bi->x + bi->w) > w) bi->x = w - bi->w;
	  }
	else
	  {
	     yy = bi->ask.pos + (bi->ask.size / 2);
	     if (yy < (bi->ask.res / 3))
	       { /* hooked to start */
		  bi->y = bi->ask.pos;
		  bi->h = bi->ask.size;
		  list_s = evas_list_append(list_s, obj);
		  bi->hookp = 0;
	       }
	     else if (yy > ((2 * bi->ask.res) / 3))
	       { /* hooked to end */
		  bi->y = (bi->ask.pos - bi->ask.res) + h;
		  bi->h = bi->ask.size;
		  list_e = evas_list_append(list_e, obj);
		  bi->hookp = bi->ask.res;
	       }
	     else
	       { /* hooked to middle */
		  if ((bi->ask.pos <= (bi->ask.res / 2)) &&
		      ((bi->ask.pos + bi->ask.size) > (bi->ask.res / 2)))
		    { /* straddles middle */
		       if (bi->ask.res > 2)
			 bi->y = (h / 2) + 
			 (((bi->ask.pos + (bi->ask.size / 2) - 
			    (bi->ask.res / 2)) * 
			   (bi->ask.res / 2)) /
			  (bi->ask.res / 2)) - (bi->ask.size / 2);
		       else
			 bi->y = h / 2;
		       bi->h = bi->ask.size;
		    }
		  else
		    { /* either side of middle */
		       bi->y = (bi->ask.pos - (bi->ask.res / 2)) + (h / 2);
		       bi->h = bi->ask.size;
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
   /* FIXME: now walk all 3 lists - if any memebrs of any list overlay another
    * then calculate how much "give" there is with items that can resize
    * down and then resize them down and reposition */
   /* FIXME: if not enough "give" is there - forcibly resize everything down
    * so it all fits */
   
   evas_list_free(list_s);
   evas_list_free(list_m);
   evas_list_free(list_e);
   
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
