#include "e.h"

static void _e_resize_begin(void *data, void *bd);
static void _e_resize_update(void *data, void *bd);
static void _e_resize_end(void *data, void *bd);
static void _e_resize_border_extents(E_Border *bd, int *w, int *h);
static void _e_move_begin(void *data, void *bd);
static void _e_move_update(void *data, void *bd);
static void _e_move_end(void *data, void *bd);
static void _e_move_resize_object_coords_set(int x, int y, int w, int h);

static E_Popup *_disp_pop = NULL;
static Evas_Object *_obj = NULL;
static Eina_List *hooks = NULL;
static int visible = 0;
static int obj_x = 0;
static int obj_y = 0;
static int obj_w = 0;
static int obj_h = 0;

EAPI int
e_moveresize_init(void)
{
   E_Border_Hook *h;

   h = e_border_hook_add(E_BORDER_HOOK_RESIZE_BEGIN, _e_resize_begin, NULL);
   if (h) hooks = eina_list_append(hooks, h);
   h = e_border_hook_add(E_BORDER_HOOK_RESIZE_UPDATE, _e_resize_update, NULL);
   if (h) hooks = eina_list_append(hooks, h);
   h = e_border_hook_add(E_BORDER_HOOK_RESIZE_END, _e_resize_end, NULL);
   if (h) hooks = eina_list_append(hooks, h);
   h = e_border_hook_add(E_BORDER_HOOK_MOVE_BEGIN, _e_move_begin, NULL);
   if (h) hooks = eina_list_append(hooks, h);
   h = e_border_hook_add(E_BORDER_HOOK_MOVE_UPDATE, _e_move_update, NULL);
   if (h) hooks = eina_list_append(hooks, h);
   h = e_border_hook_add(E_BORDER_HOOK_MOVE_END, _e_move_end, NULL);
   if (h) hooks = eina_list_append(hooks, h);

   return 1;
}

EAPI int
e_moveresize_shutdown(void)
{
   E_Border_Hook *h;

   EINA_LIST_FREE(hooks, h)
     e_border_hook_del(h);

   return 1;
}

static void
_e_resize_begin(void *data, void *border)
{
   E_Border *bd = border;
   Evas_Coord ew, eh;
   char buf[40];
   int w, h;

   if (_disp_pop) e_object_del(E_OBJECT(_disp_pop));
   _disp_pop = NULL;
   _obj = NULL;

   if (!e_config->resize_info_visible)
     return;

   if (e_config->resize_info_follows)
     _e_move_resize_object_coords_set(bd->x + bd->fx.x, bd->y + bd->fx.y, bd->w, bd->h);
   else
     _e_move_resize_object_coords_set(bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h);

   _e_resize_border_extents(bd, &w, &h);

   _disp_pop = e_popup_new(bd->zone, 0, 0, 1, 1);
   if (!_disp_pop) return;
   e_popup_layer_set(_disp_pop, 255);
   _obj = edje_object_add(_disp_pop->evas);
   e_theme_edje_object_set(_obj, "base/theme/borders",
			   "e/widgets/border/default/resize");
   snprintf(buf, sizeof(buf), "9999x9999");
   edje_object_part_text_set(_obj, "e.text.label", buf);

   edje_object_size_min_calc(_obj, &ew, &eh);
   evas_object_move(_obj, 0, 0);
   evas_object_resize(_obj, ew, eh);
   evas_object_show(_obj);
   e_popup_edje_bg_object_set(_disp_pop, _obj);

   if (!visible)
     {
	evas_object_show(_obj);
	e_popup_show(_disp_pop);
	visible = 1;
     }
   snprintf(buf, sizeof(buf), "%ix%i", w, h);
   edje_object_part_text_set(_obj, "e.text.label", buf);

   e_popup_move_resize(_disp_pop,
		       (obj_x - _disp_pop->zone->x) +
		       ((obj_w - ew) / 2),
		       (obj_y - _disp_pop->zone->y) +
		       ((obj_h - eh) / 2),
		       ew, eh);

   e_popup_show(_disp_pop);

   visible = 1;
}

static void
_e_resize_end(void *data, void *border)
{
   if (e_config->resize_info_visible)
     {
	if (_obj)
	  {
	     evas_object_del(_obj);
	     _obj = NULL;
	  }
	if (_disp_pop)
	  {
	     e_object_del(E_OBJECT(_disp_pop));
	     _disp_pop = NULL;
	  }
     }

   visible = 0;
}

static void
_e_resize_update(void *data, void *border)
{
   E_Border *bd = border;
   char buf[40];
   int w, h;

   if (!_disp_pop) return;

   if (e_config->resize_info_follows)
     _e_move_resize_object_coords_set(bd->x + bd->fx.x, bd->y + bd->fx.y, bd->w, bd->h);
   else
     _e_move_resize_object_coords_set(bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h);

   _e_resize_border_extents(bd, &w, &h);

   if (!visible)
     {
	evas_object_show(_obj);
	e_popup_show(_disp_pop);
	visible = 1;
     }
   snprintf(buf, sizeof(buf), "%ix%i", w, h);
   edje_object_part_text_set(_obj, "e.text.label", buf);
}

static void
_e_resize_border_extents(E_Border *bd, int *w, int *h)
{
   if ((bd->client.icccm.base_w >= 0) &&
       (bd->client.icccm.base_h >= 0))
     {
	if (bd->client.icccm.step_w > 0)
	  *w = (bd->client.w - bd->client.icccm.base_w) / bd->client.icccm.step_w;
	else
	  *w = bd->client.w;
	if (bd->client.icccm.step_h > 0)
	  *h = (bd->client.h - bd->client.icccm.base_h) / bd->client.icccm.step_h;
	else
	  *h = bd->client.h;
     }
   else
     {
	if (bd->client.icccm.step_w > 0)
	  *w = (bd->client.w - bd->client.icccm.min_w) / bd->client.icccm.step_w;
	else
	  *w = bd->client.w;
	if (bd->client.icccm.step_h > 0)
	  *h = (bd->client.h - bd->client.icccm.min_h) / bd->client.icccm.step_h;
	else
	  *h = bd->client.h;
     }
}

static void
_e_move_begin(void *data, void *border)
{
   E_Border *bd = border;
   Evas_Coord ew, eh;
   char buf[40];

   if (_disp_pop) e_object_del(E_OBJECT(_disp_pop));
   _disp_pop = NULL;
   _obj = NULL;

   if (!e_config->move_info_visible)
     return;

   if (e_config->move_info_follows)
     _e_move_resize_object_coords_set(bd->x + bd->fx.x, bd->y + bd->fx.y, bd->w, bd->h);
   else
     _e_move_resize_object_coords_set(bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h);

   _disp_pop = e_popup_new(bd->zone, 0, 0, 1, 1);
   _obj = edje_object_add(_disp_pop->evas);
   e_theme_edje_object_set(_obj, "base/theme/borders",
			   "e/widgets/border/default/move");
   snprintf(buf, sizeof(buf), "9999 9999");
   edje_object_part_text_set(_obj, "e.text.label", buf);

   edje_object_size_min_calc(_obj, &ew, &eh);
   evas_object_move(_obj, 0, 0);
   evas_object_resize(_obj, ew, eh);
   evas_object_show(_obj);
   e_popup_edje_bg_object_set(_disp_pop, _obj);

   e_popup_move_resize(_disp_pop,
		       (obj_x - _disp_pop->zone->x) +
		       ((obj_w - ew) / 2),
		       (obj_y - _disp_pop->zone->y) +
		       ((obj_h - eh) / 2),
		       ew, eh);
}

static void
_e_move_end(void *data, void *border)
{
   if (e_config->move_info_visible)
     {
	if (_obj)
	  {
	     evas_object_del(_obj);
	     _obj = NULL;
	  }
	if (_disp_pop)
	  {
	     e_object_del(E_OBJECT(_disp_pop));
	     _disp_pop = NULL;
	  }
     }

   visible = 0;
}

static void
_e_move_update(void *data, void *border)
{
   E_Border *bd = border;
   
   char buf[40];

   if (!_disp_pop) return;

   if (e_config->move_info_follows)
     _e_move_resize_object_coords_set(bd->x + bd->fx.x, bd->y + bd->fx.y, bd->w, bd->h);
   else
     _e_move_resize_object_coords_set(bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h);

   if (!visible)
     {
	evas_object_show(_obj);
	e_popup_show(_disp_pop);
	visible = 1;
     }
   snprintf(buf, sizeof(buf), "%i %i", bd->x, bd->y);
   edje_object_part_text_set(_obj, "e.text.label", buf);
}

static void
_e_move_resize_object_coords_set(int x, int y, int w, int h)
{
   obj_x = x;
   obj_y = y;
   obj_w = w;
   obj_h = h;
   if ((_disp_pop) && (e_config->move_info_visible) && (visible))
     {
	e_popup_move(_disp_pop,
		     (obj_x - _disp_pop->zone->x) +
		     ((obj_w - _disp_pop->w) / 2),
		     (obj_y - _disp_pop->zone->y) +
		     ((obj_h - _disp_pop->h) / 2)
		     );
     }
}
