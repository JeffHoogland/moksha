/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_gadman_free(E_Gadman *gm);
static void _e_gadman_client_free(E_Gadman_Client *gmc);
static void _e_gadman_client_edit_begin(E_Gadman_Client *gmc);
static void _e_gadman_client_edit_end(E_Gadman_Client *gmc);


static void _e_gadman_cb_signal_move_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadman_cb_signal_move_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadman_cb_signal_move_go(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadman_cb_signal_resize_left_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadman_cb_signal_resize_left_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadman_cb_signal_resize_left_go(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadman_cb_signal_resize_right_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadman_cb_signal_resize_right_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadman_cb_signal_resize_right_go(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadman_cb_signal_resize_up_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadman_cb_signal_resize_up_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadman_cb_signal_resize_up_go(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadman_cb_signal_resize_down_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadman_cb_signal_resize_down_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_gadman_cb_signal_resize_down_go(void *data, Evas_Object *obj, const char *emission, const char *source);

/* externally accessible functions */
int
e_gadman_init(void)
{
   return 1;
}

int
e_gadman_shutdown(void)
{
   return 1;
}

E_Gadman *
e_gadman_new(E_Container *con)
{
   E_Gadman    *gm;

   gm = E_OBJECT_ALLOC(E_Gadman, _e_gadman_free);
   if (!gm) return NULL;
   gm->container = con;
   return gm;
}

E_Gadman_Client *
e_gadman_client_new(E_Gadman *gm)
{
   E_Gadman_Client *gmc;
   E_OBJECT_CHECK_RETURN(gm, NULL);
   
   gmc = E_OBJECT_ALLOC(E_Gadman_Client, _e_gadman_client_free);
   if (!gmc) return NULL;
   gmc->gadman = gm;
   gmc->policy = E_GADMAN_POLICY_ANYWHERE;
   gmc->zone = e_zone_current_get(gm->container);
   gmc->minw = 1;
   gmc->minh = 1;
   gmc->maxw = 0;
   gmc->maxh = 0;
   gmc->ax = 0.0;
   gmc->ay = 1.0;
   gmc->mina = 0.0;
   gmc->maxa = 9999999.0;
   gm->clients = evas_list_append(gm->clients, gmc);
   return gmc;
}

void
e_gadman_mode_set(E_Gadman *gm, E_Gadman_Mode mode)
{
   Evas_List *l;
   
   E_OBJECT_CHECK(gm);
   if (gm->mode == mode) return;
   gm->mode = mode;
   if (gm->mode == E_GADMAN_MODE_EDIT)
     {
	for (l = gm->clients; l; l = l->next)
	  _e_gadman_client_edit_begin(l->data);
     }
   else if (gm->mode == E_GADMAN_MODE_NORMAL)
     {
	for (l = gm->clients; l; l = l->next)
	  _e_gadman_client_edit_end(l->data);
     }
}

E_Gadman_Mode
e_gadman_mode_get(E_Gadman *gm)
{
   E_OBJECT_CHECK_RETURN(gm, E_GADMAN_MODE_NORMAL);
   return gm->mode;
}

void
e_gadman_client_save(E_Gadman_Client *gmc)
{
   E_OBJECT_CHECK(gmc);
   /* save all values */
}

void
e_gadman_client_load(E_Gadman_Client *gmc)
{
   E_OBJECT_CHECK(gmc);
   /* load all the vales */
   /* implement all the values */
}

void
e_gadman_client_domain_set(E_Gadman_Client *gmc, char *domain, int instance)
{
   E_OBJECT_CHECK(gmc);
   if (gmc->domain) free(gmc->domain);
   gmc->domain = strdup(domain);
   gmc->instance = instance;
}

void
e_gadman_client_zone_set(E_Gadman_Client *gmc, E_Zone *zone)
{
   E_OBJECT_CHECK(gmc);
   gmc->zone = zone;
}

void
e_gadman_client_policy_set(E_Gadman_Client *gmc, E_Gadman_Policy pol)
{
   E_OBJECT_CHECK(gmc);
   gmc->policy = pol;
}

void
e_gadman_client_min_size_set(E_Gadman_Client *gmc, Evas_Coord minw, Evas_Coord minh)
{
   E_OBJECT_CHECK(gmc);
   gmc->minw = minw;
   gmc->minh = minh;
}

void
e_gadman_client_max_size_set(E_Gadman_Client *gmc, Evas_Coord maxw, Evas_Coord maxh)
{
   E_OBJECT_CHECK(gmc);
   gmc->maxw = maxw;
   gmc->maxh = maxh;
}

void
e_gadman_client_align_set(E_Gadman_Client *gmc, double xalign, double yalign)
{
   E_OBJECT_CHECK(gmc);
   gmc->ax = xalign;
   gmc->ay = yalign;
}

void
e_gadman_client_aspect_set(E_Gadman_Client *gmc, double mina, double maxa)
{
   E_OBJECT_CHECK(gmc);
   gmc->mina = mina;
   gmc->maxa = maxa;
}

void
e_gadman_client_geometry_get(E_Gadman_Client *gmc, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   E_OBJECT_CHECK(gmc);
   if (x) *x = gmc->x;
   if (y) *y = gmc->y;
   if (w) *w = gmc->w;
   if (h) *h = gmc->h;
}

void
e_gadman_client_change_func_set(E_Gadman_Client *gmc, void (*func) (void *data, E_Gadman_Client *gmc, E_Gadman_Change change), void *data)
{
   E_OBJECT_CHECK(gmc);
   gmc->func = func;
   gmc->data = data;
}

/* local subsystem functions */
static void
_e_gadman_free(E_Gadman *gm)
{
   free(gm);
}

static void
_e_gadman_client_free(E_Gadman_Client *gmc)
{
   gmc->gadman->clients = evas_list_remove(gmc->gadman->clients, gmc);
   if (gmc->domain) free(gmc->domain);
   free(gmc);
}

static void
_e_gadman_client_edit_begin(E_Gadman_Client *gmc)
{
   gmc->control_object = edje_object_add(gmc->gadman->container->bg_evas);
   evas_object_layer_set(gmc->control_object, 100);
   evas_object_move(gmc->control_object, gmc->x, gmc->y);
   evas_object_resize(gmc->control_object, gmc->w, gmc->h);
   edje_object_file_set(gmc->control_object,
			/* FIXME: "default.eet" needs to come from conf */
			e_path_find(path_themes, "default.eet"),
			"gadman/control");
   edje_object_signal_callback_add(gmc->control_object, "move_start", "",
				   _e_gadman_cb_signal_move_start, gmc);
   edje_object_signal_callback_add(gmc->control_object, "move_stop", "",
				   _e_gadman_cb_signal_move_stop, gmc);
   edje_object_signal_callback_add(gmc->control_object, "move_go", "",
				   _e_gadman_cb_signal_move_go, gmc);
   edje_object_signal_callback_add(gmc->control_object, "resize_start", "left",
				   _e_gadman_cb_signal_resize_left_start, gmc);
   edje_object_signal_callback_add(gmc->control_object, "resize_stop", "left",
				   _e_gadman_cb_signal_resize_left_stop, gmc);
   edje_object_signal_callback_add(gmc->control_object, "resize_go", "left",
				   _e_gadman_cb_signal_resize_left_go, gmc);
   edje_object_signal_callback_add(gmc->control_object, "resize_start", "right",
				   _e_gadman_cb_signal_resize_right_start, gmc);
   edje_object_signal_callback_add(gmc->control_object, "resize_stop", "right",
				   _e_gadman_cb_signal_resize_right_stop, gmc);
   edje_object_signal_callback_add(gmc->control_object, "resize_go", "right",
				   _e_gadman_cb_signal_resize_right_go, gmc);
   edje_object_signal_callback_add(gmc->control_object, "resize_start", "up",
				   _e_gadman_cb_signal_resize_up_start, gmc);
   edje_object_signal_callback_add(gmc->control_object, "resize_stop", "up",
				   _e_gadman_cb_signal_resize_up_stop, gmc);
   edje_object_signal_callback_add(gmc->control_object, "resize_go", "up",
				   _e_gadman_cb_signal_resize_up_go, gmc);
   edje_object_signal_callback_add(gmc->control_object, "resize_start", "down",
				   _e_gadman_cb_signal_resize_down_start, gmc);
   edje_object_signal_callback_add(gmc->control_object, "resize_stop", "down",
				   _e_gadman_cb_signal_resize_down_stop, gmc);
   edje_object_signal_callback_add(gmc->control_object, "resize_go", "down",
				   _e_gadman_cb_signal_resize_down_go, gmc);
   evas_object_show(gmc->control_object);
}

static void
_e_gadman_client_edit_end(E_Gadman_Client *gmc)
{
   evas_object_del(gmc->control_object);
}

static void
_e_gadman_cb_signal_move_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   if ((gmc->moving) || 
       (gmc->resizing_l) || (gmc->resizing_r) || 
       (gmc->resizing_u) || (gmc->resizing_d)) return;
   gmc->moving = 1;
   gmc->down_store_x = gmc->x;
   gmc->down_store_y = gmc->y;
   gmc->down_store_w = gmc->w;
   gmc->down_store_h = gmc->h;
   evas_pointer_canvas_xy_get(gmc->gadman->container->bg_evas, &gmc->down_x, &gmc->down_y);
}

static void
_e_gadman_cb_signal_move_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   gmc->moving = 0;
   e_gadman_client_save(gmc);
}

static void
_e_gadman_cb_signal_move_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   Evas_Coord x, y;
   
   gmc = data;
   if (!gmc->moving) return;
   evas_pointer_canvas_xy_get(gmc->gadman->container->bg_evas, &x, &y);
   gmc->x = gmc->down_store_x + (x - gmc->down_x);
   gmc->y = gmc->down_store_y + (y - gmc->down_y);
   gmc->w = gmc->down_store_w;
   gmc->h = gmc->down_store_h;
   /* FIXME: limit to zone, edge, or handle zone jump */
   /* limit to the zone */
   if (gmc->x < gmc->zone->x)
     gmc->x = gmc->zone->x;
   else if ((gmc->x + gmc->w) > (gmc->zone->x + gmc->zone->w))
     gmc->x = gmc->zone->x + gmc->zone->w - gmc->w;
   if (gmc->y < gmc->zone->y)
     gmc->y = gmc->zone->y;
   else if ((gmc->y + gmc->h) > (gmc->zone->y + gmc->zone->h))
     gmc->y = gmc->zone->y + gmc->zone->h - gmc->h;
   evas_object_move(gmc->control_object, gmc->x, gmc->y);
   evas_object_resize(gmc->control_object, gmc->w, gmc->h);
}

static void
_e_gadman_cb_signal_resize_left_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   if ((gmc->moving) || 
       (gmc->resizing_l) || (gmc->resizing_r) || 
       (gmc->resizing_u) || (gmc->resizing_d)) return;
   gmc->resizing_l = 1;
   gmc->down_store_x = gmc->x;
   gmc->down_store_y = gmc->y;
   gmc->down_store_w = gmc->w;
   gmc->down_store_h = gmc->h;
   evas_pointer_canvas_xy_get(gmc->gadman->container->bg_evas, &gmc->down_x, &gmc->down_y);
}

static void
_e_gadman_cb_signal_resize_left_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   gmc->resizing_l = 0;
   e_gadman_client_save(gmc);
}

static void
_e_gadman_cb_signal_resize_left_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   Evas_Coord x, y;
   
   gmc = data;
   if (!gmc->resizing_l) return;
   evas_pointer_canvas_xy_get(gmc->gadman->container->bg_evas, &x, &y);
   gmc->x = gmc->down_store_x + (x - gmc->down_x);
   gmc->y = gmc->down_store_y;
   gmc->w = gmc->down_store_w - (x - gmc->down_x);
   gmc->h = gmc->down_store_h;
   /* limit to zone left edge */
   if (gmc->x < gmc->zone->x)
     {
	gmc->w = (gmc->down_store_x + gmc->down_store_w) - gmc->zone->x;
	gmc->x = gmc->zone->x;
     }
   /* limit to min size */
   if (gmc->w < gmc->minw)
     {
	gmc->x += (gmc->minw - gmc->w);
	gmc->w = gmc->minw;
     }
   /* limit to max size */
   if (gmc->maxw > 0)
     {
	if (gmc->w > gmc->minw)
	  {
	     gmc->x -= (gmc->maxw - gmc->w);
	     gmc->w = gmc->maxw;
	  }
     }
   /* FIXME: detect that the user ALMOST resized to full zone width/height */
   /* and jump to it */
   evas_object_move(gmc->control_object, gmc->x, gmc->y);
   evas_object_resize(gmc->control_object, gmc->w, gmc->h);
}

static void
_e_gadman_cb_signal_resize_right_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   if ((gmc->moving) || 
       (gmc->resizing_l) || (gmc->resizing_r) || 
       (gmc->resizing_u) || (gmc->resizing_d)) return;
   gmc->resizing_r = 1;
   gmc->down_store_x = gmc->x;
   gmc->down_store_y = gmc->y;
   gmc->down_store_w = gmc->w;
   gmc->down_store_h = gmc->h;
   evas_pointer_canvas_xy_get(gmc->gadman->container->bg_evas, &gmc->down_x, &gmc->down_y);
}

static void
_e_gadman_cb_signal_resize_right_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   gmc->resizing_r = 0;
   e_gadman_client_save(gmc);
}

static void
_e_gadman_cb_signal_resize_right_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   Evas_Coord x, y;
   
   gmc = data;
   if (!gmc->resizing_r) return;
   evas_pointer_canvas_xy_get(gmc->gadman->container->bg_evas, &x, &y);
   gmc->x = gmc->down_store_x;
   gmc->y = gmc->down_store_y;
   gmc->w = gmc->down_store_w + (x - gmc->down_x);
   gmc->h = gmc->down_store_h;
   /* limit to zone right edge */
   if ((gmc->x + gmc->w) > (gmc->zone->x + gmc->zone->w))
     {
	gmc->w = (gmc->zone->x + gmc->zone->w) - gmc->x;
     }
   /* limit to min size */
   if (gmc->w < gmc->minw)
     {
	gmc->w = gmc->minw;
     }
   /* limit to max size */
   if (gmc->maxw > 0)
     {
	if (gmc->w > gmc->maxw)
	  {
	     gmc->w = gmc->maxw;
	  }
     }
   /* FIXME: detect that the user ALMOST resized to full zone width/height */
   /* and jump to it */
   evas_object_move(gmc->control_object, gmc->x, gmc->y);
   evas_object_resize(gmc->control_object, gmc->w, gmc->h);
}

static void
_e_gadman_cb_signal_resize_up_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   if ((gmc->moving) || 
       (gmc->resizing_l) || (gmc->resizing_r) || 
       (gmc->resizing_u) || (gmc->resizing_d)) return;
   gmc->resizing_u = 1;
   gmc->down_store_x = gmc->x;
   gmc->down_store_y = gmc->y;
   gmc->down_store_w = gmc->w;
   gmc->down_store_h = gmc->h;
   evas_pointer_canvas_xy_get(gmc->gadman->container->bg_evas, &gmc->down_x, &gmc->down_y);
}

static void
_e_gadman_cb_signal_resize_up_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   gmc->resizing_u = 0;
   e_gadman_client_save(gmc);
}

static void
_e_gadman_cb_signal_resize_up_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   Evas_Coord x, y;
   
   gmc = data;
   if (!gmc->resizing_u) return;
   evas_pointer_canvas_xy_get(gmc->gadman->container->bg_evas, &x, &y);
   gmc->x = gmc->down_store_x;
   gmc->y = gmc->down_store_y + (y - gmc->down_y);
   gmc->w = gmc->down_store_w;
   gmc->h = gmc->down_store_h - (y - gmc->down_y);
   /* limit to zone top edge */
   if (gmc->y < gmc->zone->y)
     {
	gmc->h = (gmc->down_store_y + gmc->down_store_h) - gmc->zone->y;
	gmc->y = gmc->zone->y;
     }
   /* limit to min size */
   if (gmc->h < gmc->minh)
     {
	gmc->y += (gmc->minh - gmc->h);
	gmc->h = gmc->minh;
     }
   /* limit to max size */
   if (gmc->maxh > 0)
     {
	if (gmc->h > gmc->minh)
	  {
	     gmc->y -= (gmc->maxh - gmc->h);
	     gmc->h = gmc->maxh;
	  }
     }
   /* FIXME: detect that the user ALMOST resized to full zone width/height */
   /* and jump to it */
   evas_object_move(gmc->control_object, gmc->x, gmc->y);
   evas_object_resize(gmc->control_object, gmc->w, gmc->h);
}

static void
_e_gadman_cb_signal_resize_down_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   if ((gmc->moving) || 
       (gmc->resizing_l) || (gmc->resizing_r) || 
       (gmc->resizing_u) || (gmc->resizing_d)) return;
   gmc->resizing_d = 1;
   gmc->down_store_x = gmc->x;
   gmc->down_store_y = gmc->y;
   gmc->down_store_w = gmc->w;
   gmc->down_store_h = gmc->h;
   evas_pointer_canvas_xy_get(gmc->gadman->container->bg_evas, &gmc->down_x, &gmc->down_y);
}

static void
_e_gadman_cb_signal_resize_down_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   gmc->resizing_d = 0;
   e_gadman_client_save(gmc);
}

static void
_e_gadman_cb_signal_resize_down_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   Evas_Coord x, y;
   
   gmc = data;
   if (!gmc->resizing_d) return;
   evas_pointer_canvas_xy_get(gmc->gadman->container->bg_evas, &x, &y);
   gmc->x = gmc->down_store_x;
   gmc->y = gmc->down_store_y;
   gmc->w = gmc->down_store_w;
   gmc->h = gmc->down_store_h + (y - gmc->down_y);
   /* limit to zone right bottom */
   if ((gmc->y + gmc->h) > (gmc->zone->y + gmc->zone->h))
     {
	gmc->h = (gmc->zone->y + gmc->zone->h) - gmc->y;
     }
   /* limit to min size */
   if (gmc->h < gmc->minh)
     {
	gmc->h = gmc->minh;
     }
   /* limit to max size */
   if (gmc->maxh > 0)
     {
	if (gmc->h > gmc->maxh)
	  {
	     gmc->h = gmc->maxh;
	  }
     }
   /* FIXME: detect that the user ALMOST resized to full zone width/height */
   /* and jump to it */
   evas_object_move(gmc->control_object, gmc->x, gmc->y);
   evas_object_resize(gmc->control_object, gmc->w, gmc->h);
}

