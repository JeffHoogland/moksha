/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* FIXME: corner case if module is sized to full screen... cant stop edit or */
/*        re-enter edit mode (cant access root menu) */
/* FIXME: resist can still jump on top of other gads... */

/* local subsystem functions */

typedef struct _Gadman_Client_Config Gadman_Client_Config;

struct _Gadman_Client_Config
{
   struct {
      int x, y, w, h;
   } pos;
   struct {
      int l, r, t, b;
   } pad;
   int w, h;
   int edge;
   int zone;
   int use_autow, use_autoh;
   int allow_overlap, always_on_top;
   struct {
      int w, h;
      Evas_List *others;
   } res;
};

static void _e_gadman_free(E_Gadman *gm);
static void _e_gadman_client_free(E_Gadman_Client *gmc);
static void _e_gadman_client_edit_begin(E_Gadman_Client *gmc);
static void _e_gadman_client_edit_end(E_Gadman_Client *gmc);
static void _e_gadman_client_overlap_deny(E_Gadman_Client *gmc);
static void _e_gadman_client_down_store(E_Gadman_Client *gmc);
static int  _e_gadman_client_is_being_modified(E_Gadman_Client *gmc);
static void _e_gadman_client_geometry_to_align(E_Gadman_Client *gmc);
static void _e_gadman_client_aspect_enforce(E_Gadman_Client *gmc, double cx, double cy, int use_horiz);
static void _e_gadman_client_geometry_apply(E_Gadman_Client *gmc);
static void _e_gadman_client_callback_call(E_Gadman_Client *gmc, E_Gadman_Change change);

static void _e_gadman_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_gadman_cb_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_gadman_cb_mouse_in(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_gadman_cb_mouse_out(void *data, Evas *evas, Evas_Object *obj, void *event_info);

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

static void _e_gadman_cb_menu_end(void *data, E_Menu *m);

static void _e_gadman_cb_half_width(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_gadman_cb_full_width(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_gadman_cb_auto_width(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_gadman_cb_center_horiz(void *data, E_Menu *m, E_Menu_Item *mi);

static void _e_gadman_cb_half_height(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_gadman_cb_full_height(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_gadman_cb_auto_height(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_gadman_cb_center_vert(void *data, E_Menu *m, E_Menu_Item *mi);

static void _e_gadman_cb_allow_overlap(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_gadman_cb_always_on_top(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_gadman_cb_end_edit_mode(void *data, E_Menu *m, E_Menu_Item *mi);

static void _e_gadman_config_free(Gadman_Client_Config *cf);
static void _e_gadman_client_geom_store(E_Gadman_Client *gmc);

static E_Config_DD *gadman_config_edd = NULL;

/* externally accessible functions */
EAPI int
e_gadman_init(void)
{
   gadman_config_edd = E_CONFIG_DD_NEW("Gadman_Client_Config", Gadman_Client_Config);
#undef T
#undef D
#define T Gadman_Client_Config
#define D gadman_config_edd
   E_CONFIG_VAL(D, T, pos.x, INT);
   E_CONFIG_VAL(D, T, pos.y, INT);
   E_CONFIG_VAL(D, T, pos.w, INT);
   E_CONFIG_VAL(D, T, pos.h, INT);
   E_CONFIG_VAL(D, T, pad.l, INT);
   E_CONFIG_VAL(D, T, pad.r, INT);
   E_CONFIG_VAL(D, T, pad.t, INT);
   E_CONFIG_VAL(D, T, pad.b, INT);
   E_CONFIG_VAL(D, T, w, INT);
   E_CONFIG_VAL(D, T, h, INT);
   E_CONFIG_VAL(D, T, edge, INT);
   E_CONFIG_VAL(D, T, zone, INT);
   E_CONFIG_VAL(D, T, use_autow, INT);
   E_CONFIG_VAL(D, T, use_autoh, INT);
   E_CONFIG_VAL(D, T, allow_overlap, INT);
   E_CONFIG_VAL(D, T, always_on_top, INT);
   E_CONFIG_VAL(D, T, res.w, INT);
   E_CONFIG_VAL(D, T, res.h, INT);
   E_CONFIG_LIST(D, T, res.others, gadman_config_edd);
   return 1;
}

EAPI int
e_gadman_shutdown(void)
{
   E_CONFIG_DD_FREE(gadman_config_edd);
   gadman_config_edd = NULL;
   return 1;
}

EAPI E_Gadman *
e_gadman_new(E_Container *con)
{
   E_Gadman    *gm;

   gm = E_OBJECT_ALLOC(E_Gadman, E_GADMAN_TYPE, _e_gadman_free);
   if (!gm) return NULL;
   gm->container = con;
   return gm;
}

EAPI void
e_gadman_mode_set(E_Gadman *gm, E_Gadman_Mode mode)
{
   Evas_List *l;
   
   E_OBJECT_CHECK(gm);
   E_OBJECT_TYPE_CHECK(gm, E_GADMAN_TYPE);
   if (gm->mode == mode) return;
   gm->mode = mode;
   if (gm->mode == E_GADMAN_MODE_EDIT)
     {
	for (l = gm->clients; l; l = l->next)
	  {
	     E_Gadman_Client *gmc;
	     
	     gmc = l->data;
	     _e_gadman_client_edit_begin(gmc);
	  }
     }
   else if (gm->mode == E_GADMAN_MODE_NORMAL)
     {
	for (l = gm->clients; l; l = l->next)
	  {
	     E_Gadman_Client *gmc;
	     
	     gmc = l->data;
	     _e_gadman_client_edit_end(gmc);
	  }
     }
}

EAPI E_Gadman_Mode
e_gadman_mode_get(E_Gadman *gm)
{
   E_OBJECT_CHECK_RETURN(gm, E_GADMAN_MODE_NORMAL);
   E_OBJECT_TYPE_CHECK_RETURN(gm, E_GADMAN_TYPE, E_GADMAN_MODE_NORMAL);
   return gm->mode;
}

EAPI void
e_gadman_all_save(E_Gadman *gm)
{
   Evas_List *l;
   
   E_OBJECT_CHECK(gm);
   for (l = gm->clients; l; l = l->next)
     {
	E_Gadman_Client *gmc;
	
	gmc = l->data;
	e_gadman_client_save(gmc);
     }
}

EAPI void
e_gadman_container_resize(E_Gadman *gm)
{
   Evas_List *l;
   
   E_OBJECT_CHECK(gm);
   for (l = gm->clients; l; l = l->next)
     {
	E_Gadman_Client *gmc;
	
	gmc = l->data;
	e_gadman_client_load(gmc);
     }
}

EAPI E_Gadman_Client *
e_gadman_client_new(E_Gadman *gm)
{
   E_Gadman_Client *gmc;
   E_OBJECT_CHECK_RETURN(gm, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(gm, E_GADMAN_TYPE, NULL);
   
   gmc = E_OBJECT_ALLOC(E_Gadman_Client, E_GADMAN_CLIENT_TYPE, _e_gadman_client_free);
   if (!gmc) return NULL;
   gmc->gadman = gm;
   gmc->policy = E_GADMAN_POLICY_ANYWHERE | E_GADMAN_POLICY_HSIZE | E_GADMAN_POLICY_VSIZE | E_GADMAN_POLICY_HMOVE | E_GADMAN_POLICY_VMOVE;
   gmc->zone = e_zone_current_get(gm->container);
   gmc->edge = E_GADMAN_EDGE_TOP;
   gmc->minw = 1;
   gmc->minh = 1;
   gmc->maxw = 0;
   gmc->maxh = 0;
   gmc->ax = 0.0;
   gmc->ay = 0.0;
   gmc->mina = 0.0;
   gmc->maxa = 9999999.0;
   gm->clients = evas_list_append(gm->clients, gmc);
   return gmc;
}

EAPI void
e_gadman_client_save(E_Gadman_Client *gmc)
{
   Gadman_Client_Config *cf, *cf2;
   Evas_List *l;
   char buf[1024];
   int found_res = 0;
   
   E_OBJECT_CHECK(gmc);
   E_OBJECT_TYPE_CHECK(gmc, E_GADMAN_CLIENT_TYPE);
   /* save all values */
   if (!gmc->config)
     {
	cf = calloc(1, sizeof(Gadman_Client_Config));
	gmc->config = cf;
     }
   cf = gmc->config;
   _e_gadman_client_geom_store(gmc);
   cf->res.w = gmc->zone->w;
   cf->res.h = gmc->zone->h;
   snprintf(buf, sizeof(buf), "gadman.%s.%i", gmc->domain, gmc->instance);
   for (l = cf->res.others; l; l = l->next)
     {
	cf2 = (Gadman_Client_Config *)l->data;
	if ((cf2->res.w == cf->res.w) && (cf2->res.h == cf->res.h))
	  {
	     *cf2 = *cf;
	     cf2->res.others = NULL;
	     found_res = 1;
	     break;
	  }
     }
   if (!found_res)
     {
	cf2 = calloc(1, sizeof(Gadman_Client_Config));
	cf->res.others = evas_list_prepend(cf->res.others, cf2);
	*cf2 = *cf;
	cf2->res.others = NULL;
     }
   e_config_domain_save(buf, gadman_config_edd, cf);
}

EAPI void
e_gadman_client_load(E_Gadman_Client *gmc)
{
   Gadman_Client_Config *cf, *cf2;
   Evas_List *l;
   char buf[1024];
   
   E_OBJECT_CHECK(gmc);
   E_OBJECT_TYPE_CHECK(gmc, E_GADMAN_CLIENT_TYPE);
   if (gmc->config)
     {
	_e_gadman_config_free(gmc->config);
	gmc->config = NULL;
     }
   snprintf(buf, sizeof(buf), "gadman.%s.%i", gmc->domain, gmc->instance);
   cf = e_config_domain_load(buf, gadman_config_edd);
   if (cf)
     {
	E_Zone *zone;
	
	for (l = cf->res.others; l; l = l->next)
	  {
	     cf2 = (Gadman_Client_Config *)l->data;
	     zone = e_container_zone_number_get(gmc->zone->container, cf2->zone);
	     if (zone)
	       {
		  if ((zone->w == cf2->res.w) && (zone->h == cf2->res.h))
		    {
		       l = cf->res.others;
		       *cf = *cf2;
		       cf->res.others = l;
		       break;
		    }
	       }
	  }
	E_CONFIG_LIMIT(cf->pos.x, 0, 10000);
	E_CONFIG_LIMIT(cf->pos.y, 0, 10000);
	E_CONFIG_LIMIT(cf->pos.w, 1, 10000);
	E_CONFIG_LIMIT(cf->pos.h, 1, 10000);
	E_CONFIG_LIMIT(cf->pad.l, 0, 1000);
	E_CONFIG_LIMIT(cf->pad.r, 0, 1000);
	E_CONFIG_LIMIT(cf->pad.t, 0, 1000);
	E_CONFIG_LIMIT(cf->pad.b, 0, 1000);
	E_CONFIG_LIMIT(cf->w, 0, 10000);
	E_CONFIG_LIMIT(cf->h, 0, 10000);
	E_CONFIG_LIMIT(cf->edge, E_GADMAN_EDGE_LEFT, E_GADMAN_EDGE_BOTTOM);
	if (cf->pos.w != cf->w)
	  gmc->ax = (double)(cf->pos.x) / (double)(cf->pos.w - cf->w);
	else
	  gmc->ax = 0.0;
	if (cf->pos.h != cf->h)
	  gmc->ay = (double)(cf->pos.y) / (double)(cf->pos.h - cf->h);
	else
	  gmc->ay = 0.0;
	gmc->w = cf->w;
	gmc->w -= (cf->pad.l + cf->pad.r);
	gmc->w += (gmc->pad.l + gmc->pad.r);
	gmc->h = cf->h;
	gmc->h -= (cf->pad.t + cf->pad.b);
	gmc->h += (gmc->pad.t + gmc->pad.b);
	gmc->edge = cf->edge;
	gmc->use_autow = cf->use_autow;
	gmc->use_autoh = cf->use_autoh;
	gmc->allow_overlap = cf->allow_overlap;
	if (gmc->allow_overlap)
	  gmc->policy |= E_GADMAN_POLICY_ALLOW_OVERLAP;
	gmc->always_on_top = cf->always_on_top;
	if (gmc->always_on_top)
	  gmc->policy |= E_GADMAN_POLICY_ALWAYS_ON_TOP;
	zone = e_container_zone_number_get(gmc->zone->container, cf->zone);
	if (zone) gmc->zone = zone;
	if (gmc->use_autow)
	  {
	     gmc->w = gmc->autow;
	     gmc->x = gmc->zone->x + ((gmc->zone->w - gmc->w) * gmc->ax);
	  }
	if (gmc->use_autoh)
	  {
	     gmc->h = gmc->autoh;
	     gmc->y = gmc->zone->y + ((gmc->zone->h - gmc->h) * gmc->ay);
	  }
	if (gmc->w > gmc->zone->w) gmc->w = gmc->zone->w;
	if (gmc->h > gmc->zone->h) gmc->h = gmc->zone->h;
//	gmc->x = gmc->zone->x + ((gmc->zone->w - gmc->w) * gmc->ax);
//	gmc->y = gmc->zone->y + ((gmc->zone->h - gmc->h) * gmc->ay);
	if (cf->pos.w != cf->w)
	  gmc->x = gmc->zone->x + ((gmc->zone->w - gmc->w) * gmc->ax);
	else
	  gmc->x = gmc->zone->x;
	if (cf->pos.h != cf->h)
	  gmc->y = gmc->zone->y + ((gmc->zone->h - gmc->h) *  gmc->ay);
	else
	  gmc->y = gmc->zone->y;
	gmc->config = cf;
     }
   else
     {
	cf = calloc(1, sizeof(Gadman_Client_Config));
	gmc->config = cf;
	_e_gadman_client_geom_store(gmc);
     }
   _e_gadman_client_overlap_deny(gmc);
   e_object_ref(E_OBJECT(gmc));
   if (!e_object_is_del(E_OBJECT(gmc)))
     _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_ZONE);
   if (!e_object_is_del(E_OBJECT(gmc)))
     _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_EDGE);
   if (!e_object_is_del(E_OBJECT(gmc)))
     _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
   e_object_unref(E_OBJECT(gmc));

   if (gmc->gadman->mode == E_GADMAN_MODE_EDIT)
     _e_gadman_client_edit_begin(gmc);
}

EAPI void
e_gadman_client_domain_set(E_Gadman_Client *gmc, char *domain, int instance)
{
   E_OBJECT_CHECK(gmc);
   E_OBJECT_TYPE_CHECK(gmc, E_GADMAN_CLIENT_TYPE);
   if (gmc->domain) evas_stringshare_del(gmc->domain);
   gmc->domain = evas_stringshare_add(domain);
   gmc->instance = instance;
}

EAPI void
e_gadman_client_zone_set(E_Gadman_Client *gmc, E_Zone *zone)
{
   E_OBJECT_CHECK(gmc);
   E_OBJECT_TYPE_CHECK(gmc, E_GADMAN_CLIENT_TYPE);
   if (zone == gmc->zone) return;
   gmc->zone = zone;
   gmc->x = gmc->zone->x + ((gmc->zone->w - gmc->w) * gmc->ax);
   gmc->y = gmc->zone->y + ((gmc->zone->h - gmc->h) * gmc->ay);
   e_object_ref(E_OBJECT(gmc));
   if (!e_object_is_del(E_OBJECT(gmc)))
     _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_ZONE);
   if (!e_object_is_del(E_OBJECT(gmc)))
     _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
   e_object_unref(E_OBJECT(gmc));
}

EAPI void
e_gadman_client_policy_set(E_Gadman_Client *gmc, E_Gadman_Policy pol)
{
   E_OBJECT_CHECK(gmc);
   E_OBJECT_TYPE_CHECK(gmc, E_GADMAN_CLIENT_TYPE);
   gmc->policy = pol;

   if (gmc->control_object)
     {
	if (gmc->policy & E_GADMAN_POLICY_HSIZE)
	  edje_object_signal_emit(gmc->control_object, "hsize", "on");
	else
	  edje_object_signal_emit(gmc->control_object, "hsize", "off");
	if (gmc->policy & E_GADMAN_POLICY_VSIZE)
	  edje_object_signal_emit(gmc->control_object, "vsize", "on");
	else
	  edje_object_signal_emit(gmc->control_object, "vsize", "off");
	if (gmc->policy & (E_GADMAN_POLICY_HMOVE | E_GADMAN_POLICY_VMOVE))
	  edje_object_signal_emit(gmc->control_object, "move", "on");
	else
	  edje_object_signal_emit(gmc->control_object, "move", "off");
     }
   
}

EAPI void
e_gadman_client_min_size_set(E_Gadman_Client *gmc, Evas_Coord minw, Evas_Coord minh)
{
   E_OBJECT_CHECK(gmc);
   E_OBJECT_TYPE_CHECK(gmc, E_GADMAN_CLIENT_TYPE);
   if ((gmc->minw == minw) && (gmc->minh == minh)) return;
   gmc->minw = minw;
   gmc->minh = minh;
   if (gmc->minw > gmc->w)
     {
	gmc->w = gmc->minw;
	gmc->x = gmc->zone->x + ((gmc->zone->w - gmc->w) * gmc->ax);
     }
   if (gmc->minh > gmc->h)
     {
	gmc->h = gmc->minh;
	gmc->y = gmc->zone->y + ((gmc->zone->h - gmc->h) * gmc->ay);
     }
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
}

EAPI void
e_gadman_client_max_size_set(E_Gadman_Client *gmc, Evas_Coord maxw, Evas_Coord maxh)
{
   E_OBJECT_CHECK(gmc);
   E_OBJECT_TYPE_CHECK(gmc, E_GADMAN_CLIENT_TYPE);
   if ((gmc->maxw == maxw) && (gmc->maxh == maxh)) return;
   gmc->maxw = maxw;
   gmc->maxh = maxh;
   if (gmc->maxw < gmc->w)
     {
	gmc->w = gmc->maxw;
	gmc->x = gmc->zone->x + ((gmc->zone->w - gmc->w) * gmc->ax);
     }
   if (gmc->maxh < gmc->h)
     {
	gmc->h = gmc->maxh;
	gmc->y = gmc->zone->y + ((gmc->zone->h - gmc->h) * gmc->ay);
     }
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
}

EAPI void
e_gadman_client_align_set(E_Gadman_Client *gmc, double xalign, double yalign)
{
   E_OBJECT_CHECK(gmc);
   E_OBJECT_TYPE_CHECK(gmc, E_GADMAN_CLIENT_TYPE);
   if ((gmc->ax == xalign) && (gmc->ay == yalign)) return;
   gmc->ax = xalign;
   gmc->ay = yalign;
   gmc->x = gmc->zone->x + ((gmc->zone->w - gmc->w) * gmc->ax);
   gmc->y = gmc->zone->y + ((gmc->zone->h - gmc->h) * gmc->ay);
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
}

EAPI void
e_gadman_client_aspect_set(E_Gadman_Client *gmc, double mina, double maxa)
{
   E_OBJECT_CHECK(gmc);
   E_OBJECT_TYPE_CHECK(gmc, E_GADMAN_CLIENT_TYPE);
   gmc->mina = mina;
   gmc->maxa = maxa;
}

EAPI void
e_gadman_client_padding_set(E_Gadman_Client *gmc, int l, int r, int t, int b)
{
   E_OBJECT_CHECK(gmc);
   E_OBJECT_TYPE_CHECK(gmc, E_GADMAN_CLIENT_TYPE);
   gmc->pad.l = l;
   gmc->pad.r = r;
   gmc->pad.t = t;
   gmc->pad.b = b;
}

EAPI void
e_gadman_client_auto_size_set(E_Gadman_Client *gmc, Evas_Coord autow, Evas_Coord autoh)
{
   E_OBJECT_CHECK(gmc);
   E_OBJECT_TYPE_CHECK(gmc, E_GADMAN_CLIENT_TYPE);
   gmc->autow = autow;
   gmc->autoh = autoh;
   if (gmc->use_autow)
     {
	gmc->w = gmc->autow;
	gmc->x = gmc->zone->x + ((gmc->zone->w - gmc->w) * gmc->ax);
     }
   if (gmc->use_autoh)
     {
	gmc->h = gmc->autoh;
	gmc->y = gmc->zone->y + ((gmc->zone->h - gmc->h) * gmc->ay);
     }
   /* FIXME: check for overlap and fix */
/*   
   for (l = gmc->zone->container->gadman->clients; l; l = l->next)
     {
	E_Gadman_Client *gmc2;
	
	gmc2 = l->data;
	if (gmc != gmc2)
	  {
	     if ((E_SPANS_COMMON(gmc->x, gmc->w, gmc2->x, gmc2->w)) &&
		 (E_SPANS_COMMON(gmc->y, gmc->h, gmc2->y, gmc2->h)))
	       {
		  // blah
	       }
	  }
     }
 */
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
}

EAPI void
e_gadman_client_edge_set(E_Gadman_Client *gmc, E_Gadman_Edge edge)
{
   E_OBJECT_CHECK(gmc);
   E_OBJECT_TYPE_CHECK(gmc, E_GADMAN_CLIENT_TYPE);
   gmc->edge = edge;
}

EAPI E_Gadman_Edge
e_gadman_client_edge_get(E_Gadman_Client *gmc)
{
   E_OBJECT_CHECK_RETURN(gmc, E_GADMAN_EDGE_TOP);
   E_OBJECT_TYPE_CHECK_RETURN(gmc, E_GADMAN_CLIENT_TYPE, E_GADMAN_EDGE_TOP);
   return gmc->edge;
}

EAPI void
e_gadman_client_geometry_get(E_Gadman_Client *gmc, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   E_OBJECT_CHECK(gmc);
   E_OBJECT_TYPE_CHECK(gmc, E_GADMAN_CLIENT_TYPE);
   if (x) *x = gmc->x;
   if (y) *y = gmc->y;
   if (w) *w = gmc->w;
   if (h) *h = gmc->h;
}

EAPI void
e_gadman_client_user_resize(E_Gadman_Client *gmc, Evas_Coord w, Evas_Coord h)
{
   E_OBJECT_CHECK(gmc);
   E_OBJECT_TYPE_CHECK(gmc, E_GADMAN_CLIENT_TYPE);
   if ((gmc->w == w) && (gmc->h == h)) return;
   gmc->w = w;
   if (gmc->w > gmc->zone->w) gmc->w = gmc->zone->w;
   gmc->x = gmc->zone->x + ((gmc->zone->w - gmc->w) * gmc->ax);
   gmc->h = h;
   if (gmc->h > gmc->zone->h) gmc->h = gmc->zone->h;
   gmc->y = gmc->zone->y + ((gmc->zone->h - gmc->h) * gmc->ay);
   _e_gadman_client_geometry_apply(gmc);
   _e_gadman_client_geometry_to_align(gmc);
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
}

EAPI void
e_gadman_client_resize(E_Gadman_Client *gmc, Evas_Coord w, Evas_Coord h)
{
   int re_adjust = 0;
   
   E_OBJECT_CHECK(gmc);
   E_OBJECT_TYPE_CHECK(gmc, E_GADMAN_CLIENT_TYPE);
   if ((gmc->w == w) && (gmc->h == h)) return;
   gmc->w = w;
   if (gmc->w > gmc->zone->w)
     {
	gmc->w = gmc->zone->w;
	re_adjust = 1;
     }
   gmc->x = gmc->zone->x + ((gmc->zone->w - gmc->w) * gmc->ax);
   gmc->h = h;
   if (gmc->h > gmc->zone->h)
     {
	gmc->h = gmc->zone->h;
	re_adjust = 1;
     }
   gmc->y = gmc->zone->y + ((gmc->zone->h - gmc->h) * gmc->ay);
   _e_gadman_client_geometry_apply(gmc);
   if (re_adjust)
     _e_gadman_client_geometry_to_align(gmc);
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
}

EAPI void
e_gadman_client_change_func_set(E_Gadman_Client *gmc, void (*func) (void *data, E_Gadman_Client *gmc, E_Gadman_Change change), void *data)
{
   E_OBJECT_CHECK(gmc);
   E_OBJECT_TYPE_CHECK(gmc, E_GADMAN_CLIENT_TYPE);
   gmc->func = func;
   gmc->data = data;
}

EAPI E_Menu *
e_gadman_client_menu_new(E_Gadman_Client *gmc)
{
   E_Menu *m;
   E_Menu_Item *mi;
   int disallow, seperator;
   const char *s;
   
   E_OBJECT_CHECK_RETURN(gmc, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(gmc, E_GADMAN_CLIENT_TYPE, NULL);
   m = e_menu_new();
   
   gmc->menu = m;

   seperator = 0;
   if (gmc->policy & E_GADMAN_POLICY_HSIZE)
     {
	seperator = 1;
	if (gmc->autow > 0)
	  {
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Automatic Width"));
	     e_menu_item_check_set(mi, 1);
	     e_menu_item_toggle_set(mi, gmc->use_autow);
	     s = e_path_find(path_icons, "default.edj");
	     e_menu_item_icon_edje_set(mi, s, "auto_width");
	     if (s) evas_stringshare_del(s);
	     e_menu_item_callback_set(mi, _e_gadman_cb_auto_width, gmc);
	     mi = e_menu_item_new(m);
	     e_menu_item_separator_set(mi, 1);
	  }
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Half Screen Width"));
	s = e_path_find(path_icons, "default.edj");
	e_menu_item_icon_edje_set(mi, s, "half_width");
	if (s) evas_stringshare_del(s);
	e_menu_item_callback_set(mi, _e_gadman_cb_half_width, gmc);
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Full Screen Width"));
	s = e_path_find(path_icons, "default.edj");
	e_menu_item_icon_edje_set(mi, s, "full_width");
	if (s) evas_stringshare_del(s);
	e_menu_item_callback_set(mi, _e_gadman_cb_full_width, gmc);
     }
   disallow = (gmc->policy & E_GADMAN_POLICY_EDGES)
	      && ((gmc->edge == E_GADMAN_EDGE_LEFT) || (gmc->edge == E_GADMAN_EDGE_RIGHT));
   if ((gmc->policy & E_GADMAN_POLICY_HMOVE) && !(disallow))
     {
	seperator = 1;
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Center Horizontally"));
	s = e_path_find(path_icons, "default.edj");
	e_menu_item_icon_edje_set(mi, s, "center_horiz");
	if (s) evas_stringshare_del(s);
	e_menu_item_callback_set(mi, _e_gadman_cb_center_horiz, gmc);
     }
   if (seperator)
     {
	mi = e_menu_item_new(m);
	e_menu_item_separator_set(mi, 1);
     }
   seperator = 0;
   if (gmc->policy & E_GADMAN_POLICY_VSIZE)
     {
	seperator = 1;
	if (gmc->autoh > 0)
	  {
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Automatic Height"));
	     e_menu_item_check_set(mi, 1);
	     e_menu_item_toggle_set(mi, gmc->use_autoh);
	     s = e_path_find(path_icons, "default.edj"),
	     e_menu_item_icon_edje_set(mi, s, "auto_height");
	     if (s) evas_stringshare_del(s);
	     e_menu_item_callback_set(mi, _e_gadman_cb_auto_height, gmc);
	     mi = e_menu_item_new(m);
	     e_menu_item_separator_set(mi, 1);
	  }
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Half Screen Height"));
	s = e_path_find(path_icons, "default.edj");
	e_menu_item_icon_edje_set(mi, s, "half_height");
	if (s) evas_stringshare_del(s);
	e_menu_item_callback_set(mi, _e_gadman_cb_half_height, gmc);
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Full Screen Height"));
	s = e_path_find(path_icons, "default.edj");
	e_menu_item_icon_edje_set(mi, s, "full_height");
	if (s) evas_stringshare_del(s);
	e_menu_item_callback_set(mi, _e_gadman_cb_full_height, gmc);
     }
   disallow = (gmc->policy & E_GADMAN_POLICY_EDGES)
	      && ((gmc->edge == E_GADMAN_EDGE_TOP) || (gmc->edge == E_GADMAN_EDGE_BOTTOM));
   if ((gmc->policy & E_GADMAN_POLICY_VMOVE) && !(disallow))
     {
	seperator = 1;
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Center Vertically"));
	s = e_path_find(path_icons, "default.edj");
	e_menu_item_icon_edje_set(mi, s, "center_vert");
	if (s) evas_stringshare_del(s);
	e_menu_item_callback_set(mi, _e_gadman_cb_center_vert, gmc);
     }
   if (seperator)
     {
	mi = e_menu_item_new(m);
	e_menu_item_separator_set(mi, 1);
     }

   /* user configurable policy flags */
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Allow Overlap"));
   e_menu_item_check_set(mi, 1);
   e_menu_item_toggle_set(mi, gmc->allow_overlap);
   s = e_path_find(path_icons, "default.edj"),
   e_menu_item_icon_edje_set(mi, s, "allow_overlap");
   if (s) evas_stringshare_del(s);
   e_menu_item_callback_set(mi, _e_gadman_cb_allow_overlap, gmc);
/* Not used yet
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Always On Top"));
   e_menu_item_check_set(mi, 1);
   e_menu_item_toggle_set(mi, gmc->always_on_top);
   s = e_path_find(path_icons, "default.edj"),
   e_menu_item_icon_edje_set(mi, s, "always_on_top");
   if (s) evas_stringshare_del(s);
   e_menu_item_callback_set(mi, _e_gadman_cb_always_on_top, gmc);
*/
   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("End Edit Mode"));
   e_menu_item_callback_set(mi, _e_gadman_cb_end_edit_mode, gmc);
   
   return m;
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
   if (gmc->menu) e_object_del(E_OBJECT(gmc->menu));
   if (gmc->control_object) evas_object_del(gmc->control_object);
   if (gmc->event_object) evas_object_del(gmc->event_object);
   gmc->gadman->clients = evas_list_remove(gmc->gadman->clients, gmc);
   if (gmc->domain) evas_stringshare_del(gmc->domain);
   _e_gadman_config_free(gmc->config);
   free(gmc);
}

static void
_e_gadman_client_edit_begin(E_Gadman_Client *gmc)
{
   gmc->control_object = edje_object_add(gmc->gadman->container->bg_evas);
   evas_object_layer_set(gmc->control_object, 100);
   evas_object_move(gmc->control_object, gmc->x, gmc->y);
   evas_object_resize(gmc->control_object, gmc->w, gmc->h);
   e_theme_edje_object_set(gmc->control_object, "base/theme/gadman",
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
   gmc->event_object = evas_object_rectangle_add(gmc->gadman->container->bg_evas);
   evas_object_color_set(gmc->event_object, 0, 0, 0, 0);
   evas_object_repeat_events_set(gmc->event_object, 1);
   evas_object_layer_set(gmc->event_object, 100);
   evas_object_move(gmc->event_object, gmc->x, gmc->y);
   evas_object_resize(gmc->event_object, gmc->w, gmc->h);
   evas_object_event_callback_add(gmc->event_object, EVAS_CALLBACK_MOUSE_DOWN, _e_gadman_cb_mouse_down, gmc);
   evas_object_event_callback_add(gmc->event_object, EVAS_CALLBACK_MOUSE_UP, _e_gadman_cb_mouse_up, gmc);
   evas_object_event_callback_add(gmc->event_object, EVAS_CALLBACK_MOUSE_IN, _e_gadman_cb_mouse_in, gmc);
   evas_object_event_callback_add(gmc->event_object, EVAS_CALLBACK_MOUSE_OUT, _e_gadman_cb_mouse_out, gmc);
   
   if (gmc->policy & E_GADMAN_POLICY_HSIZE)
     edje_object_signal_emit(gmc->control_object, "hsize", "on");
   else
     edje_object_signal_emit(gmc->control_object, "hsize", "off");
   if (gmc->policy & E_GADMAN_POLICY_VSIZE)
     edje_object_signal_emit(gmc->control_object, "vsize", "on");
   else
     edje_object_signal_emit(gmc->control_object, "vsize", "off");
   if (gmc->policy & (E_GADMAN_POLICY_HMOVE | E_GADMAN_POLICY_VMOVE))
     edje_object_signal_emit(gmc->control_object, "move", "on");
   else
     edje_object_signal_emit(gmc->control_object, "move", "off");
   
   evas_object_clip_set(gmc->event_object, gmc->zone->bg_clip_object);
   evas_object_clip_set(gmc->control_object, gmc->zone->bg_clip_object);
   evas_object_show(gmc->event_object);
   evas_object_show(gmc->control_object);
}

static void
_e_gadman_client_edit_end(E_Gadman_Client *gmc)
{
   e_gadman_client_save(gmc);
   if (gmc->moving) e_move_end();
   if ((gmc->resizing_l) || (gmc->resizing_r) ||
       (gmc->resizing_u) || (gmc->resizing_d))
       e_resize_end();
   gmc->moving = 0;
   gmc->resizing_l = 0;
   gmc->resizing_r = 0;
   gmc->resizing_u = 0;
   gmc->resizing_d = 0;
   evas_object_del(gmc->control_object);
   gmc->control_object = NULL;
   evas_object_del(gmc->event_object);
   gmc->event_object = NULL;
}

static void
_e_gadman_client_overlap_deny(E_Gadman_Client *gmc)
{
   Evas_List *l;
   Evas_Coord ox, oy;
   int ok = 0;
   int iterate = 0;
   
   ox = gmc->x;
   oy = gmc->y;
   ok = 0;
   if ((gmc->policy & 0xff) == E_GADMAN_POLICY_EDGES)
     {
	if ((gmc->edge == E_GADMAN_EDGE_LEFT) ||
	    (gmc->edge == E_GADMAN_EDGE_RIGHT))
	  {
	     for (l = gmc->zone->container->gadman->clients; l; l = l->next)
	       {
		  E_Gadman_Client *gmc2;
		  
		  gmc2 = l->data;
		  if (gmc != gmc2)
		    {
		       if ((E_SPANS_COMMON(gmc->x, gmc->w, gmc2->x, gmc2->w)) &&
			   (E_SPANS_COMMON(gmc->y, gmc->h, gmc2->y, gmc2->h)))
			 {
			    ok = 0;
			    gmc->y = gmc2->y + gmc2->h;
			 }
		    }
	       }
	     if (ok) return;
	     if ((gmc->y + gmc->h) > (gmc->zone->y + gmc->zone->h))
	       gmc->y = gmc->zone->y + gmc->zone->h - gmc->h;
	     ok = 1;
	     for (l = gmc->zone->container->gadman->clients; l; l = l->next)
	       {
		  E_Gadman_Client *gmc2;
		  
		  gmc2 = l->data;
		  if (gmc != gmc2)
		    {
		       if ((E_SPANS_COMMON(gmc->x, gmc->w, gmc2->x, gmc2->w)) &&
			   (E_SPANS_COMMON(gmc->y, gmc->h, gmc2->y, gmc2->h)))
			 {
			    ok = 0;
			    break;
			 }
		    }
	       }
	     if (ok)
	       {
		  _e_gadman_client_geometry_to_align(gmc);
		  return;
	       }
	     for (l = gmc->zone->container->gadman->clients; l; l = l->next)
	       {
		  E_Gadman_Client *gmc2;
		  
		  gmc2 = l->data;
		  if (gmc != gmc2)
		    {
		       if ((E_SPANS_COMMON(gmc->x, gmc->w, gmc2->x, gmc2->w)) &&
			   (E_SPANS_COMMON(gmc->y, gmc->h, gmc2->y, gmc2->h)))
			 {
			    gmc->y = gmc2->y - gmc->h;
			 }
		    }
	       }
	     if (gmc->y < gmc->zone->y)
	       gmc->y = gmc->zone->y;
	  }
	else if ((gmc->edge == E_GADMAN_EDGE_TOP) ||
		 (gmc->edge == E_GADMAN_EDGE_BOTTOM))
	  {
	     for (l = gmc->zone->container->gadman->clients; l; l = l->next)
	       {
		  E_Gadman_Client *gmc2;
		  
		  gmc2 = l->data;
		  if (gmc != gmc2)
		    {
		       if ((E_SPANS_COMMON(gmc->x, gmc->w, gmc2->x, gmc2->w)) &&
			   (E_SPANS_COMMON(gmc->y, gmc->h, gmc2->y, gmc2->h)))
			 {
			    ok = 0;
			    gmc->x = gmc2->x + gmc2->w;
			 }
		    }
	       }
	     if (ok) return;
	     if ((gmc->x + gmc->w) > (gmc->zone->x + gmc->zone->w))
	       gmc->x = gmc->zone->x + gmc->zone->w - gmc->w;
	     ok = 1;
	     for (l = gmc->zone->container->gadman->clients; l; l = l->next)
	       {
		  E_Gadman_Client *gmc2;
		  
		  gmc2 = l->data;
		  if (gmc != gmc2)
		    {
		       if ((E_SPANS_COMMON(gmc->x, gmc->w, gmc2->x, gmc2->w)) &&
			   (E_SPANS_COMMON(gmc->y, gmc->h, gmc2->y, gmc2->h)))
			 {
			    ok = 0;
			    break;
			 }
		    }
	       }
	     if (ok)
	       {
		  _e_gadman_client_geometry_to_align(gmc);
		  return;
	       }
	     for (l = gmc->zone->container->gadman->clients; l; l = l->next)
	       {
		  E_Gadman_Client *gmc2;
		  
		  gmc2 = l->data;
		  if (gmc != gmc2)
		    {
		       if ((E_SPANS_COMMON(gmc->x, gmc->w, gmc2->x, gmc2->w)) &&
			   (E_SPANS_COMMON(gmc->y, gmc->h, gmc2->y, gmc2->h)))
			 {
			    gmc->x = gmc2->x - gmc->w;
			 }
		    }
	       }
	     if (gmc->x < gmc->zone->x)
	       gmc->x = gmc->zone->x;
	  }
	_e_gadman_client_geometry_to_align(gmc);
	return;
     }
   while ((!ok) && (iterate < 1000))
     {
	ok = 1;
	for (l = gmc->zone->container->gadman->clients; l; l = l->next)
	  {
	     E_Gadman_Client *gmc2;
	     
	     gmc2 = l->data;
	     if (gmc != gmc2)
	       {
		  if ((E_SPANS_COMMON(gmc->x, gmc->w, gmc2->x, gmc2->w)) &&
		      (E_SPANS_COMMON(gmc->y, gmc->h, gmc2->y, gmc2->h)))
		    {
		       ok = 0;
		       gmc->x = gmc2->x + gmc2->w;
		    }
	       }
	  }
	if (ok) break;
	if ((gmc->x + gmc->w) > (gmc->zone->x + gmc->zone->w))
	  gmc->x = gmc->zone->x + gmc->zone->w - gmc->w;
	ok = 1;
	for (l = gmc->zone->container->gadman->clients; l; l = l->next)
	  {
	     E_Gadman_Client *gmc2;
	     
	     gmc2 = l->data;
	     if (gmc != gmc2)
	       {
		  if ((E_SPANS_COMMON(gmc->x, gmc->w, gmc2->x, gmc2->w)) &&
		      (E_SPANS_COMMON(gmc->y, gmc->h, gmc2->y, gmc2->h)))
		    {
		       ok = 0;
		       break;
		    }
	       }
	  }
	if (ok) break;
	for (l = gmc->zone->container->gadman->clients; l; l = l->next)
	  {
	     E_Gadman_Client *gmc2;
	     
	     gmc2 = l->data;
	     if (gmc != gmc2)
	       {
		  if ((E_SPANS_COMMON(gmc->x, gmc->w, gmc2->x, gmc2->w)) &&
		      (E_SPANS_COMMON(gmc->y, gmc->h, gmc2->y, gmc2->h)))
		    {
		       gmc->x = gmc2->x - gmc->w;
		    }
	       }
	  }
	if (gmc->x < gmc->zone->x)
	  gmc->x = gmc->zone->x;
	ok = 1;
	for (l = gmc->zone->container->gadman->clients; l; l = l->next)
	  {
	     E_Gadman_Client *gmc2;
	     
	     gmc2 = l->data;
	     if (gmc != gmc2)
	       {
		  if ((E_SPANS_COMMON(gmc->x, gmc->w, gmc2->x, gmc2->w)) &&
		      (E_SPANS_COMMON(gmc->y, gmc->h, gmc2->y, gmc2->h)))
		    {
		       ok = 0;
		       break;
		    }
	       }
	  }
	if (ok) break;
	if (gmc->y > (gmc->zone->y + (gmc->zone->h / 2)))
	  gmc->y -= 8;
	else
	  gmc->y += 8;
	if (oy < (gmc->zone->y + (gmc->zone->h / 2)))
	  {
	     if ((gmc->y - oy) > (gmc->zone->h / 3))
	       {
		  gmc->x = ox;
		  gmc->y = oy;
		  break;
	       }
	  }
	else
	  {
	     if ((oy - gmc->y) > (gmc->zone->h / 3))
	       {
		  gmc->x = ox;
		  gmc->y = oy;
		  break;
	       }
	  }
	iterate++;
     }
   _e_gadman_client_geometry_to_align(gmc);
}

static void
_e_gadman_client_down_store(E_Gadman_Client *gmc)
{
   gmc->down_store_x = gmc->x;
   gmc->down_store_y = gmc->y;
   gmc->down_store_w = gmc->w;
   gmc->down_store_h = gmc->h;
   evas_pointer_canvas_xy_get(gmc->gadman->container->bg_evas, &gmc->down_x, &gmc->down_y);
}

static int
_e_gadman_client_is_being_modified(E_Gadman_Client *gmc)
{
   if ((gmc->moving) || 
       (gmc->resizing_l) || (gmc->resizing_r) || 
       (gmc->resizing_u) || (gmc->resizing_d))
     return 1;
   return 0;
}

static void
_e_gadman_client_geometry_to_align(E_Gadman_Client *gmc)
{
   if (gmc->w != gmc->zone->w)
     gmc->ax = (double)(gmc->x - gmc->zone->x) / (double)(gmc->zone->w - gmc->w);
   else
     gmc->ax = 0.0;
   if (gmc->h != gmc->zone->h)
     gmc->ay = (double)(gmc->y - gmc->zone->y) / (double)(gmc->zone->h - gmc->h);
   else
     gmc->ay = 0.0;
}

static void
_e_gadman_client_aspect_enforce(E_Gadman_Client *gmc, double cx, double cy, int use_horiz)
{
   Evas_Coord neww, newh;
   double aspect;
   int change = 0;
   
   neww = gmc->w - (gmc->pad.l + gmc->pad.r);
   newh = gmc->h - (gmc->pad.t + gmc->pad.b);
   if (gmc->h > 0)
     aspect = (double)neww / (double)newh;
   else
     aspect = 0.0;

   if (aspect > gmc->maxa)
     {
	if (use_horiz)
	  newh = neww / gmc->maxa;
	else
	  neww = newh * gmc->mina;
	change = 1;
     }
   else if (aspect < gmc->mina)
     {
	if (use_horiz)
	  newh = neww / gmc->maxa;
	else
	  neww = newh * gmc->mina;
	change = 1;
     }
   if (change)
     {
	neww += (gmc->pad.l + gmc->pad.r);
	newh += (gmc->pad.t + gmc->pad.b);
	gmc->x = gmc->x + ((gmc->w - neww) * cx);
	gmc->y = gmc->y + ((gmc->h - newh) * cy);
	gmc->w = neww;
	gmc->h = newh;
     }
}

static void
_e_gadman_client_geometry_apply(E_Gadman_Client *gmc)
{
   if (gmc->event_object)
     {
	evas_object_move(gmc->event_object, gmc->x, gmc->y);
	evas_object_resize(gmc->event_object, gmc->w, gmc->h);
     }
   if (gmc->control_object)
     {
	evas_object_move(gmc->control_object, gmc->x, gmc->y);
	evas_object_resize(gmc->control_object, gmc->w, gmc->h);
     }
}

static void
_e_gadman_client_callback_call(E_Gadman_Client *gmc, E_Gadman_Change change)
{
   if (gmc->func) gmc->func(gmc->data, gmc, change);
}

static void
_e_gadman_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Gadman_Client *gmc;
   
   gmc = data;
   ev = event_info;
   if (_e_gadman_client_is_being_modified(gmc)) return;
   /* FIXME: how do we prevent this if you don't have a right mouse button */
   /*        maybe make this menu available in he modules menu? */
   if (ev->button == 3)
     {
	E_Menu *m;
	
	m = e_gadman_client_menu_new(gmc);
	if (m)
	  {
	     e_menu_post_deactivate_callback_set(m, _e_gadman_cb_menu_end, gmc);
	     e_menu_activate_mouse(m, gmc->zone, ev->output.x, ev->output.y, 1, 1,
				   E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	     e_util_container_fake_mouse_up_all_later(gmc->zone->container);
	  }
     }
}

static void
_e_gadman_cb_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   E_Gadman_Client *gmc;
   
   gmc = data;
   ev = event_info;
   if (_e_gadman_client_is_being_modified(gmc)) return;
}

static void
_e_gadman_cb_mouse_in(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_In *ev;
   E_Gadman_Client *gmc;
   
   gmc = data;
   ev = event_info;
   if (_e_gadman_client_is_being_modified(gmc)) return;
   edje_object_signal_emit(gmc->control_object, "active", "");
}

static void
_e_gadman_cb_mouse_out(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Out *ev;
   E_Gadman_Client *gmc;
   
   gmc = data;
   ev = event_info;
   if (_e_gadman_client_is_being_modified(gmc)) return;
   edje_object_signal_emit(gmc->control_object, "inactive", "");
}

static void
_e_gadman_cb_signal_move_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
 
   gmc = data;
   if (_e_gadman_client_is_being_modified(gmc)) return;
   _e_gadman_client_down_store(gmc);
   gmc->moving = 1;
   evas_object_raise(gmc->control_object);
   evas_object_raise(gmc->event_object);
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_RAISE);

   if (e_config->move_info_follows)
     e_move_resize_object_coords_set(gmc->x, gmc->y, gmc->w, gmc->h);
   else
     e_move_resize_object_coords_set(gmc->zone->x, gmc->zone->y, gmc->zone->w, gmc->zone->h);
   e_move_begin(gmc->zone, gmc->x, gmc->y);
}

static void
_e_gadman_cb_signal_move_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   gmc->moving = 0;
   _e_gadman_client_geometry_to_align(gmc);
   e_gadman_client_save(gmc);

   e_move_end();
}

static void
_e_gadman_cb_signal_move_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   Evas_Coord       x, y;
   int              new_edge = 0;
   int              nx, ny, nxx, nyy;
   int              new_zone = 0;
   Evas_List       *skiplist = NULL;

   
   gmc = data;
   if (!gmc->moving) return;
   evas_pointer_canvas_xy_get(gmc->gadman->container->bg_evas, &x, &y);
   nxx = nx = gmc->down_store_x + (x - gmc->down_x);
   nyy = ny = gmc->down_store_y + (y - gmc->down_y);
   skiplist = evas_list_append(skiplist, gmc);
   e_resist_container_gadman_position(gmc->zone->container, skiplist, 
				      gmc->x, gmc->y, gmc->w, gmc->h,
				      nx, ny, gmc->w, gmc->h,
				      &nxx, &nyy);
   evas_list_free(skiplist);
   x += (nxx - nx);
   y += (nyy - ny);
   if ((gmc->policy & 0xff) == E_GADMAN_POLICY_EDGES)
     {
	double xr, yr;
	E_Gadman_Edge ne;
	
	ne = gmc->edge;
	
	if (!(gmc->policy & E_GADMAN_POLICY_FIXED_ZONE))
	  {
	     E_Zone *nz;
	     
	     nz = e_container_zone_at_point_get(gmc->zone->container, x, y);
	     if ((nz) && (nz != gmc->zone))
	       {
		  gmc->zone = nz;
		  new_zone = 1;
		  evas_object_clip_set(gmc->event_object, gmc->zone->bg_clip_object);
		  evas_object_clip_set(gmc->control_object, gmc->zone->bg_clip_object);
	       }
	  }
	
	xr = (double)(x - gmc->zone->x) / (double)gmc->zone->w;
	yr = (double)(y - gmc->zone->y) / (double)gmc->zone->h;
	
	if ((xr + yr) <= 1.0) /* top or left */
	  {
	     if (((1.0 - yr) + xr) <= 1.0) ne = E_GADMAN_EDGE_LEFT;
	     else ne = E_GADMAN_EDGE_TOP;
	  }
	else /* bottom or right */
	  {
	     if (((1.0 - yr) + xr) <= 1.0) ne = E_GADMAN_EDGE_BOTTOM;
	     else ne = E_GADMAN_EDGE_RIGHT;
	  }
	
	if (ne != gmc->edge)
	  {
	     gmc->edge = ne;
	     new_edge = 1;
	  }
	if (gmc->edge == E_GADMAN_EDGE_LEFT)
	  {
	     gmc->x = gmc->zone->x;
	     gmc->y = gmc->down_store_y + (y - gmc->down_y);
	     if (gmc->h > gmc->zone->h) gmc->h = gmc->zone->h;
	     if (gmc->y < gmc->zone->y)
	       gmc->y = gmc->zone->y;
	     else if ((gmc->y + gmc->h) > (gmc->zone->y + gmc->zone->h))
	       gmc->y = gmc->zone->y + gmc->zone->h - gmc->h;
	  }
	else if (gmc->edge == E_GADMAN_EDGE_RIGHT)
	  {
	     gmc->x = gmc->zone->x + gmc->zone->w - gmc->w;
	     gmc->y = gmc->down_store_y + (y - gmc->down_y);
	     if (gmc->h > gmc->zone->h) gmc->h = gmc->zone->h;
	     if (gmc->y < gmc->zone->y)
	       gmc->y = gmc->zone->y;
	     else if ((gmc->y + gmc->h) > (gmc->zone->y + gmc->zone->h))
	       gmc->y = gmc->zone->y + gmc->zone->h - gmc->h;
	  }
	else if (gmc->edge == E_GADMAN_EDGE_TOP)
	  {
	     gmc->x = gmc->down_store_x + (x - gmc->down_x);
	     gmc->y = gmc->zone->y;
	     if (gmc->w > gmc->zone->w) gmc->w = gmc->zone->w;
	     if (gmc->x < gmc->zone->x)
	       gmc->x = gmc->zone->x;
	     else if ((gmc->x + gmc->w) > (gmc->zone->x + gmc->zone->w))
	       gmc->x = gmc->zone->x + gmc->zone->w - gmc->w;
	  }
	else if (gmc->edge == E_GADMAN_EDGE_BOTTOM)
	  {
	     gmc->x = gmc->down_store_x + (x - gmc->down_x);
	     gmc->y = gmc->zone->y + gmc->zone->h - gmc->h;
	     if (gmc->w > gmc->zone->w) gmc->w = gmc->zone->w;
	     if (gmc->x < gmc->zone->x)
	       gmc->x = gmc->zone->x;
	     else if ((gmc->x + gmc->w) > (gmc->zone->x + gmc->zone->w))
	       gmc->x = gmc->zone->x + gmc->zone->w - gmc->w;
	  }
     }
   else
     {
	if (gmc->policy & E_GADMAN_POLICY_HMOVE)
	  gmc->x = gmc->down_store_x + (x - gmc->down_x);
	else
	  gmc->x = gmc->down_store_x;
	if (gmc->policy & E_GADMAN_POLICY_VMOVE)
	  gmc->y = gmc->down_store_y + (y - gmc->down_y);
	else
	  gmc->y = gmc->down_store_y;
	gmc->w = gmc->down_store_w;
	gmc->h = gmc->down_store_h;
	if (!(gmc->policy & E_GADMAN_POLICY_FIXED_ZONE))
	  {
	     E_Zone *nz;
	     
	     nz = e_container_zone_at_point_get(gmc->zone->container, x, y);
	     if ((nz) && (nz != gmc->zone))
	       {
		  gmc->zone = nz;
		  new_zone = 1;
		  evas_object_clip_set(gmc->event_object, gmc->zone->bg_clip_object);
		  evas_object_clip_set(gmc->control_object, gmc->zone->bg_clip_object);
	       }
	  }
	if (gmc->h > gmc->zone->h) gmc->h = gmc->zone->h;
	if (gmc->w > gmc->zone->w) gmc->w = gmc->zone->w;
	if (gmc->x < gmc->zone->x)
	  gmc->x = gmc->zone->x;
	else if ((gmc->x + gmc->w) > (gmc->zone->x + gmc->zone->w))
	  gmc->x = gmc->zone->x + gmc->zone->w - gmc->w;
	if (gmc->y < gmc->zone->y)
	  gmc->y = gmc->zone->y;
	else if ((gmc->y + gmc->h) > (gmc->zone->y + gmc->zone->h))
	  gmc->y = gmc->zone->y + gmc->zone->h - gmc->h;
     }
   _e_gadman_client_geometry_to_align(gmc);
   _e_gadman_client_geometry_apply(gmc);
   e_object_ref(E_OBJECT(gmc));
   if (!e_object_is_del(E_OBJECT(gmc)))
     {
	if (new_zone)
	  _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_ZONE);
     }
   if (!e_object_is_del(E_OBJECT(gmc)))
     {
	if (new_edge)
	  _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_EDGE);
     }
   if (!e_object_is_del(E_OBJECT(gmc)))
     _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
   e_object_unref(E_OBJECT(gmc));

   if (e_config->move_info_follows)
     e_move_resize_object_coords_set(gmc->x, gmc->y, gmc->w, gmc->h);
   else
     e_move_resize_object_coords_set(gmc->zone->x, gmc->zone->y, gmc->zone->w, gmc->zone->h);
   e_move_update(gmc->x, gmc->y);
}

static void
_e_gadman_cb_signal_resize_left_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
 
   gmc = data;
   if (_e_gadman_client_is_being_modified(gmc)) return;
   _e_gadman_client_down_store(gmc);
   gmc->use_autow = 0;
   gmc->resizing_l = 1;

   if (e_config->resize_info_follows)
     e_move_resize_object_coords_set(gmc->x, gmc->y, gmc->w, gmc->h);
   else
     e_move_resize_object_coords_set(gmc->zone->x, gmc->zone->y, gmc->zone->w, gmc->zone->h);
   e_resize_begin(gmc->zone, gmc->w, gmc->h);
}

static void
_e_gadman_cb_signal_resize_left_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   gmc->resizing_l = 0;
   e_gadman_client_save(gmc);

   e_resize_end();
}

static void
_e_gadman_cb_signal_resize_left_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   Evas_Coord       x, y;
   
   gmc = data;
   if (!gmc->resizing_l) return;
   if (!(gmc->policy & E_GADMAN_POLICY_HSIZE)) return;
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
	gmc->x = (gmc->down_store_x + gmc->down_store_w) - gmc->minw;
	gmc->w = gmc->minw;
     }
   /* if we are atthe edge or beyond. assyme we want to be all the way there */
   if (x <= gmc->zone->x)
     {
	gmc->w = (gmc->x + gmc->w) - gmc->zone->x;
	gmc->x = gmc->zone->x;
     }
   /* limit to max size */
   if (gmc->maxw > 0)
     {
	if (gmc->w > gmc->maxw)
	  {
	     gmc->x -= (gmc->maxw - gmc->w);
	     gmc->w = gmc->maxw;
	  }
     }
   _e_gadman_client_aspect_enforce(gmc, 1.0, 0.5, 1);
   if (gmc->x < gmc->zone->x)
     gmc->x = gmc->zone->x;
   else if ((gmc->x + gmc->w) > (gmc->zone->x + gmc->zone->w))
     gmc->x = gmc->zone->x + gmc->zone->w - gmc->w;
   if (gmc->y < gmc->zone->y)
     gmc->y = gmc->zone->y;
   else if ((gmc->y + gmc->h) > (gmc->zone->y + gmc->zone->h))
     gmc->y = gmc->zone->y + gmc->zone->h - gmc->h;
   _e_gadman_client_geometry_to_align(gmc);
   _e_gadman_client_geometry_apply(gmc);
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);

   if (e_config->resize_info_follows)
     e_move_resize_object_coords_set(gmc->x, gmc->y, gmc->w, gmc->h);
   else
     e_move_resize_object_coords_set(gmc->zone->x, gmc->zone->y, gmc->zone->w, gmc->zone->h);
   e_resize_update(gmc->w, gmc->h);
}

static void
_e_gadman_cb_signal_resize_right_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
 
   gmc = data;
   if (_e_gadman_client_is_being_modified(gmc)) return;
   _e_gadman_client_down_store(gmc);
   gmc->use_autow = 0;
   gmc->resizing_r = 1;

   if (e_config->resize_info_follows)
     e_move_resize_object_coords_set(gmc->x, gmc->y, gmc->w, gmc->h);
   else
     e_move_resize_object_coords_set(gmc->zone->x, gmc->zone->y, gmc->zone->w, gmc->zone->h);
   e_resize_begin(gmc->zone, gmc->w, gmc->h);
}

static void
_e_gadman_cb_signal_resize_right_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   gmc->resizing_r = 0;
   e_gadman_client_save(gmc);

   e_resize_end();
}

static void
_e_gadman_cb_signal_resize_right_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   Evas_Coord x, y;

   gmc = data;
   if (!gmc->resizing_r) return;
   if (!(gmc->policy & E_GADMAN_POLICY_HSIZE)) return;
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
   /* if we are atthe edge or beyond. assyme we want to be all the way there */
   if (x >= (gmc->zone->x + gmc->zone->w - 1))
     {
	gmc->w = gmc->zone->x + gmc->zone->w - gmc->x;
     }
   /* limit to max size */
   if (gmc->maxw > 0)
     {
	if (gmc->w > gmc->maxw)
	  {
	     gmc->w = gmc->maxw;
	  }
     }
   _e_gadman_client_aspect_enforce(gmc, 0.0, 0.5, 1);
   if (gmc->x < gmc->zone->x)
     gmc->x = gmc->zone->x;
   else if ((gmc->x + gmc->w) > (gmc->zone->x + gmc->zone->w))
     gmc->x = gmc->zone->x + gmc->zone->w - gmc->w;
   if (gmc->y < gmc->zone->y)
     gmc->y = gmc->zone->y;
   else if ((gmc->y + gmc->h) > (gmc->zone->y + gmc->zone->h))
     gmc->y = gmc->zone->y + gmc->zone->h - gmc->h;
   _e_gadman_client_geometry_to_align(gmc);
   _e_gadman_client_geometry_apply(gmc);
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);

   if (e_config->resize_info_follows)
     e_move_resize_object_coords_set(gmc->x, gmc->y, gmc->w, gmc->h);
   else
     e_move_resize_object_coords_set(gmc->zone->x, gmc->zone->y, gmc->zone->w, gmc->zone->h);
   e_resize_update(gmc->w, gmc->h);
}

static void
_e_gadman_cb_signal_resize_up_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
 
   gmc = data;
   if (_e_gadman_client_is_being_modified(gmc)) return;
   _e_gadman_client_down_store(gmc);
   gmc->use_autoh = 0;
   gmc->resizing_u = 1;

   if (e_config->resize_info_follows)
     e_move_resize_object_coords_set(gmc->x, gmc->y, gmc->w, gmc->h);
   else
     e_move_resize_object_coords_set(gmc->zone->x, gmc->zone->y, gmc->zone->w, gmc->zone->h);
   e_resize_begin(gmc->zone, gmc->w, gmc->h);
}

static void
_e_gadman_cb_signal_resize_up_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   gmc->resizing_u = 0;
   e_gadman_client_save(gmc);

   e_resize_end();
}

static void
_e_gadman_cb_signal_resize_up_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   Evas_Coord x, y;

   gmc = data;
   if (!gmc->resizing_u) return;
   if (!(gmc->policy & E_GADMAN_POLICY_VSIZE)) return;
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
	gmc->y = (gmc->down_store_y + gmc->down_store_h) - gmc->minh;
	gmc->h = gmc->minh;
     }
   /* if we are atthe edge or beyond. assyme we want to be all the way there */
   if (y <= gmc->zone->y)
     {
	gmc->h = (gmc->y + gmc->h) - gmc->zone->y;
	gmc->y = gmc->zone->y;
     }
   /* limit to max size */
   if (gmc->maxh > 0)
     {
	if (gmc->h > gmc->maxh)
	  {
	     gmc->y -= (gmc->maxh - gmc->h);
	     gmc->h = gmc->maxh;
	  }
     }
   _e_gadman_client_aspect_enforce(gmc, 0.5, 1.0, 0);
   if (gmc->x < gmc->zone->x)
     gmc->x = gmc->zone->x;
   else if ((gmc->x + gmc->w) > (gmc->zone->x + gmc->zone->w))
     gmc->x = gmc->zone->x + gmc->zone->w - gmc->w;
   if (gmc->y < gmc->zone->y)
     gmc->y = gmc->zone->y;
   else if ((gmc->y + gmc->h) > (gmc->zone->y + gmc->zone->h))
     gmc->y = gmc->zone->y + gmc->zone->h - gmc->h;
   _e_gadman_client_geometry_to_align(gmc);
   _e_gadman_client_geometry_apply(gmc);
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);

   if (e_config->resize_info_follows)
     e_move_resize_object_coords_set(gmc->x, gmc->y, gmc->w, gmc->h);
   else
     e_move_resize_object_coords_set(gmc->zone->x, gmc->zone->y, gmc->zone->w, gmc->zone->h);
   e_resize_update(gmc->w, gmc->h);
}

static void
_e_gadman_cb_signal_resize_down_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   if (_e_gadman_client_is_being_modified(gmc)) return;
   _e_gadman_client_down_store(gmc);
   gmc->use_autoh = 0;
   gmc->resizing_d = 1;

   if (e_config->resize_info_follows)
     e_move_resize_object_coords_set(gmc->x, gmc->y, gmc->w, gmc->h);
   else
     e_move_resize_object_coords_set(gmc->zone->x, gmc->zone->y, gmc->zone->w, gmc->zone->h);
   e_resize_begin(gmc->zone, gmc->w, gmc->h);
}

static void
_e_gadman_cb_signal_resize_down_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   gmc->resizing_d = 0;
   e_gadman_client_save(gmc);

   e_resize_end();
}

static void
_e_gadman_cb_signal_resize_down_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Gadman_Client *gmc;
   Evas_Coord x, y;
 
   gmc = data;
   if (!gmc->resizing_d) return;
   if (!(gmc->policy & E_GADMAN_POLICY_VSIZE)) return;
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
   /* if we are atthe edge or beyond. assyme we want to be all the way there */
   if (y >= (gmc->zone->y + gmc->zone->h - 1))
     {
	gmc->h = gmc->zone->y + gmc->zone->h - gmc->y;
     }
   /* limit to max size */
   if (gmc->maxh > 0)
     {
	if (gmc->h > gmc->maxh)
	  {
	     gmc->h = gmc->maxh;
	  }
     }
   _e_gadman_client_aspect_enforce(gmc, 0.5, 0.0, 0);
   if (gmc->x < gmc->zone->x)
     gmc->x = gmc->zone->x;
   else if ((gmc->x + gmc->w) > (gmc->zone->x + gmc->zone->w))
     gmc->x = gmc->zone->x + gmc->zone->w - gmc->w;
   if (gmc->y < gmc->zone->y)
     gmc->y = gmc->zone->y;
   else if ((gmc->y + gmc->h) > (gmc->zone->y + gmc->zone->h))
     gmc->y = gmc->zone->y + gmc->zone->h - gmc->h;
   _e_gadman_client_geometry_to_align(gmc);
   _e_gadman_client_geometry_apply(gmc);
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);

   if (e_config->resize_info_follows)
     e_move_resize_object_coords_set(gmc->x, gmc->y, gmc->w, gmc->h);
   else
     e_move_resize_object_coords_set(gmc->zone->x, gmc->zone->y, gmc->zone->w, gmc->zone->h);
   e_resize_update(gmc->w, gmc->h);
}

static void
_e_gadman_cb_menu_end(void *data, E_Menu *m)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   e_object_del(E_OBJECT(m));
   gmc->menu = NULL;
}

static void
_e_gadman_cb_half_width(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   gmc->w = gmc->zone->w / 2;
   if (gmc->w < gmc->minw) gmc->w = gmc->minw;
   if (gmc->maxw > 0)
     {
	if (gmc->w > gmc->maxw) gmc->w = gmc->maxw;
     }
   gmc->use_autow = 0;
   _e_gadman_client_aspect_enforce(gmc, 0.0, 0.5, 1);
   if (gmc->x < gmc->zone->x)
     gmc->x = gmc->zone->x;
   else if ((gmc->x + gmc->w) > (gmc->zone->x + gmc->zone->w))
     gmc->x = gmc->zone->x + gmc->zone->w - gmc->w;
   if (gmc->y < gmc->zone->y)
     gmc->y = gmc->zone->y;
   else if ((gmc->y + gmc->h) > (gmc->zone->y + gmc->zone->h))
     gmc->y = gmc->zone->y + gmc->zone->h - gmc->h;
   _e_gadman_client_geometry_apply(gmc);
   _e_gadman_client_geometry_to_align(gmc);
   e_gadman_client_save(gmc);
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
}

static void
_e_gadman_cb_full_width(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   gmc->w = gmc->zone->w;
   if (gmc->w < gmc->minw) gmc->w = gmc->minw;
   if (gmc->maxw > 0)
     {
	if (gmc->w > gmc->maxw) gmc->w = gmc->maxw;
     }
   gmc->x = gmc->zone->x + ((gmc->zone->w - gmc->w) / 2);
   gmc->use_autow = 0;
   _e_gadman_client_aspect_enforce(gmc, 0.0, 0.5, 1);
   if (gmc->x < gmc->zone->x)
     gmc->x = gmc->zone->x;
   else if ((gmc->x + gmc->w) > (gmc->zone->x + gmc->zone->w))
     gmc->x = gmc->zone->x + gmc->zone->w - gmc->w;
   if (gmc->y < gmc->zone->y)
     gmc->y = gmc->zone->y;
   else if ((gmc->y + gmc->h) > (gmc->zone->y + gmc->zone->h))
     gmc->y = gmc->zone->y + gmc->zone->h - gmc->h;
   _e_gadman_client_geometry_apply(gmc);
   _e_gadman_client_geometry_to_align(gmc);
   e_gadman_client_save(gmc);
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
}

static void
_e_gadman_cb_auto_width(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   if (e_menu_item_toggle_get(mi))
     {
	gmc->use_autow = 1;
	gmc->w = gmc->autow;
	if (gmc->w > gmc->zone->w)
	  gmc->w = gmc->zone->w;
	if ((gmc->x + gmc->w) > (gmc->zone->x + gmc->zone->w))
	  gmc->x = (gmc->zone->x + gmc->zone->w) - gmc->w;
     }
   else
     gmc->use_autow = 0;
   _e_gadman_client_geometry_apply(gmc);
   _e_gadman_client_geometry_to_align(gmc);
   e_gadman_client_save(gmc);
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
}

static void
_e_gadman_cb_center_horiz(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   gmc->x = gmc->zone->x + ((gmc->zone->w - gmc->w) / 2);
   _e_gadman_client_geometry_apply(gmc);
   _e_gadman_client_geometry_to_align(gmc);
   e_gadman_client_save(gmc);
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
}

static void
_e_gadman_cb_half_height(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   gmc->h = gmc->zone->h / 2;
   if (gmc->h < gmc->minh) gmc->h = gmc->minh;
   if (gmc->maxh > 0)
     {
	if (gmc->h > gmc->maxh) gmc->h = gmc->maxh;
     }
   gmc->use_autoh = 0;
   _e_gadman_client_aspect_enforce(gmc, 0.5, 0.0, 0);
   if (gmc->x < gmc->zone->x)
     gmc->x = gmc->zone->x;
   else if ((gmc->x + gmc->w) > (gmc->zone->x + gmc->zone->w))
     gmc->x = gmc->zone->x + gmc->zone->w - gmc->w;
   if (gmc->y < gmc->zone->y)
     gmc->y = gmc->zone->y;
   else if ((gmc->y + gmc->h) > (gmc->zone->y + gmc->zone->h))
     gmc->y = gmc->zone->y + gmc->zone->h - gmc->h;
   _e_gadman_client_geometry_apply(gmc);
   _e_gadman_client_geometry_to_align(gmc);
   e_gadman_client_save(gmc);
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
}

static void
_e_gadman_cb_full_height(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   gmc->h = gmc->zone->h;
   if (gmc->h < gmc->minh) gmc->h = gmc->minh;
   if (gmc->maxh > 0)
     {
	if (gmc->h > gmc->maxh) gmc->h = gmc->maxh;
     }
   gmc->y = gmc->zone->y + ((gmc->zone->h - gmc->h) / 2);
   gmc->use_autoh = 0;
   _e_gadman_client_aspect_enforce(gmc, 0.5, 0.0, 0);
   if (gmc->x < gmc->zone->x)
     gmc->x = gmc->zone->x;
   else if ((gmc->x + gmc->w) > (gmc->zone->x + gmc->zone->w))
     gmc->x = gmc->zone->x + gmc->zone->w - gmc->w;
   if (gmc->y < gmc->zone->y)
     gmc->y = gmc->zone->y;
   else if ((gmc->y + gmc->h) > (gmc->zone->y + gmc->zone->h))
     gmc->y = gmc->zone->y + gmc->zone->h - gmc->h;
   _e_gadman_client_geometry_apply(gmc);
   _e_gadman_client_geometry_to_align(gmc);
   e_gadman_client_save(gmc);
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
}

static void
_e_gadman_cb_auto_height(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   if (e_menu_item_toggle_get(mi))
     {
	gmc->use_autoh = 1;
	gmc->h = gmc->autoh;
	if (gmc->h > gmc->zone->h)
	  gmc->h = gmc->zone->h;
	if ((gmc->y + gmc->h) > (gmc->zone->y + gmc->zone->h))
	  gmc->y = (gmc->zone->y + gmc->zone->h) - gmc->h;
     }
   else
     gmc->use_autoh = 0;
   _e_gadman_client_geometry_apply(gmc);
   _e_gadman_client_geometry_to_align(gmc);
   e_gadman_client_save(gmc);
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
}


static void
_e_gadman_cb_center_vert(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Gadman_Client *gmc;
   
   gmc = data;
   gmc->y = gmc->zone->y + ((gmc->zone->h - gmc->h) / 2);
   _e_gadman_client_geometry_apply(gmc);
   _e_gadman_client_geometry_to_align(gmc);
   e_gadman_client_save(gmc);
   _e_gadman_client_callback_call(gmc, E_GADMAN_CHANGE_MOVE_RESIZE);
}

static void
_e_gadman_cb_allow_overlap(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Gadman_Client *gmc;

   gmc = data;
   if (e_menu_item_toggle_get(mi))
     {
	gmc->allow_overlap = 1;
	gmc->policy |= E_GADMAN_POLICY_ALLOW_OVERLAP;
     }
   else
     {
	gmc->allow_overlap = 0;
	gmc->policy &= ~E_GADMAN_POLICY_ALLOW_OVERLAP;
     }
   e_gadman_client_save(gmc);
}

static void
_e_gadman_cb_always_on_top(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Gadman_Client *gmc;

   gmc = data;
   if (e_menu_item_toggle_get(mi))
     {
	gmc->allow_overlap = 1;
	gmc->policy |= E_GADMAN_POLICY_ALWAYS_ON_TOP;
     }
   else
     {
	gmc->allow_overlap = 0;
	gmc->policy &= ~E_GADMAN_POLICY_ALWAYS_ON_TOP;
     }
   e_gadman_client_save(gmc);
}

static void
_e_gadman_cb_end_edit_mode(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Gadman_Client *gmc;

   gmc = data;
   e_gadman_mode_set(gmc->gadman, E_GADMAN_MODE_NORMAL);
}

static void
_e_gadman_config_free(Gadman_Client_Config *cf)
{
   while (cf->res.others)
     {
	_e_gadman_config_free(cf->res.others->data);
	cf->res.others = evas_list_remove_list(cf->res.others, cf->res.others);
     }
   free(cf);
}

static void
_e_gadman_client_geom_store(E_Gadman_Client *gmc)
{
   Gadman_Client_Config *cf;
   
   cf = gmc->config;
   cf->pos.x = gmc->x - gmc->zone->x;
   cf->pos.y = gmc->y - gmc->zone->y;
   cf->pos.w = gmc->zone->w;
   cf->pos.h = gmc->zone->h;
   cf->pad.l = gmc->pad.l;
   cf->pad.r = gmc->pad.r;
   cf->pad.t = gmc->pad.t;
   cf->pad.b = gmc->pad.b;
   cf->w = gmc->w;
   cf->h = gmc->h;
   cf->edge = gmc->edge;
   cf->zone = gmc->zone->num;
   cf->use_autow = gmc->use_autow;
   cf->use_autoh = gmc->use_autoh;
   cf->allow_overlap = gmc->allow_overlap;
   cf->always_on_top = gmc->always_on_top;
}
