#include "e.h"

/* local subsystem functions */
static void _e_gadcon_popup_free(E_Gadcon_Popup *pop);
static void _e_gadcon_popup_locked_set(E_Gadcon_Popup *pop, Eina_Bool locked);
static void _e_gadcon_popup_size_recalc(E_Gadcon_Popup *pop, Evas_Object *obj);
static void _e_gadcon_popup_position(E_Gadcon_Popup *pop);
static void _e_gadcon_popup_changed_size_hints_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

/* externally accessible functions */

EAPI E_Gadcon_Popup *
e_gadcon_popup_new(E_Gadcon_Client *gcc)
{
   E_Gadcon_Popup *pop;
   Evas_Object *o;
   E_Zone *zone;

   pop = E_OBJECT_ALLOC(E_Gadcon_Popup, E_GADCON_POPUP_TYPE, _e_gadcon_popup_free);
   if (!pop) return NULL;
   zone = e_gadcon_client_zone_get(gcc);
   pop->win = e_popup_new(zone, 0, 0, 0, 0);
   e_popup_layer_set(pop->win, 350);

   o = edje_object_add(pop->win->evas);
   e_theme_edje_object_set(o, "base/theme/gadman", "e/gadman/popup");
   evas_object_show(o);
   evas_object_move(o, 0, 0);
   e_popup_edje_bg_object_set(pop->win, o);
   pop->o_bg = o;

   pop->gcc = gcc;
   pop->gadcon_lock = 1;
   pop->gadcon_was_locked = 0;

   return pop;
}

EAPI void
e_gadcon_popup_content_set(E_Gadcon_Popup *pop, Evas_Object *o)
{
   Evas_Object *old_o;

   if (!pop) return;
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_GADCON_POPUP_TYPE);

   old_o = edje_object_part_swallow_get(pop->o_bg, "e.swallow.content");
   if (old_o != o)
     {
        if (old_o)
          {
             edje_object_part_unswallow(pop->o_bg, old_o);
             evas_object_del(old_o);
          }
        edje_object_part_swallow(pop->o_bg, "e.swallow.content", o);
        evas_object_event_callback_add(o, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _e_gadcon_popup_changed_size_hints_cb, pop);
     }

   _e_gadcon_popup_size_recalc(pop, o);
}

EAPI void
e_gadcon_popup_show(E_Gadcon_Popup *pop)
{
   if (!pop) return;
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_GADCON_POPUP_TYPE);

   if (pop->win->visible) return;

   e_popup_show(pop->win);

   _e_gadcon_popup_position(pop);
}

EAPI void
e_gadcon_popup_hide(E_Gadcon_Popup *pop)
{
   if (!pop) return;
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_GADCON_POPUP_TYPE);
   if (pop->pinned) return;
   e_popup_hide(pop->win);
   if (pop->gadcon_was_locked)
     _e_gadcon_popup_locked_set(pop, 0);
}

EAPI void
e_gadcon_popup_toggle_pinned(E_Gadcon_Popup *pop)
{
   if (!pop) return;
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_GADCON_POPUP_TYPE);

   if (pop->pinned)
     {
        pop->pinned = 0;
        edje_object_signal_emit(pop->o_bg, "e,state,unpinned", "e");
     }
   else
     {
        pop->pinned = 1;
        edje_object_signal_emit(pop->o_bg, "e,state,pinned", "e");
     }
}

EAPI void
e_gadcon_popup_lock_set(E_Gadcon_Popup *pop, Eina_Bool setting)
{
   if (!pop) return;
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_GADCON_POPUP_TYPE);

   setting = !!setting;
   if (pop->gadcon_lock == setting) return;
   pop->gadcon_lock = setting;

   if (setting != pop->gadcon_was_locked)
     _e_gadcon_popup_locked_set(pop, setting);
}

/* local subsystem functions */

static void
_e_gadcon_popup_free(E_Gadcon_Popup *pop)
{
   if (pop->gadcon_was_locked)
     _e_gadcon_popup_locked_set(pop, 0);
   pop->gcc = NULL;
   e_object_del(E_OBJECT(pop->win));
   free(pop);
}

static void
_e_gadcon_popup_locked_set(E_Gadcon_Popup *pop, Eina_Bool locked)
{
   if (!pop->gcc)
     return;

   e_gadcon_locked_set(pop->gcc->gadcon, locked);
   pop->gadcon_was_locked = locked;
}

static void
_e_gadcon_popup_size_recalc(E_Gadcon_Popup *pop, Evas_Object *obj)
{
   Evas_Coord w = 0, h = 0;

   e_widget_size_min_get(obj, &w, &h);
   if ((!w) || (!h)) evas_object_size_hint_min_get(obj, &w, &h);
   if ((!w) || (!h))
     {
        edje_object_size_min_get(obj, &w, &h);
        edje_object_size_min_restricted_calc(obj, &w, &h, w, h);
     }
   edje_extern_object_min_size_set(obj, w, h);
   edje_object_size_min_calc(pop->o_bg, &pop->w, &pop->h);
   evas_object_resize(pop->o_bg, pop->w, pop->h);

   if (pop->win->visible)
     _e_gadcon_popup_position(pop);
}

static void
_e_gadcon_popup_position(E_Gadcon_Popup *pop)
{
   Evas_Coord gx, gy, gw, gh, zw, zh, zx, zy, px, py;

   /* Popup positioning */
   e_gadcon_client_geometry_get(pop->gcc, &gx, &gy, &gw, &gh);
   zx = pop->win->zone->x;
   zy = pop->win->zone->y;
   zw = pop->win->zone->w;
   zh = pop->win->zone->h;
   switch (pop->gcc->gadcon->orient)
     {
      case E_GADCON_ORIENT_CORNER_RT:
      case E_GADCON_ORIENT_CORNER_RB:
      case E_GADCON_ORIENT_RIGHT:
        px = gx - pop->w;
        py = gy;
        if (py + pop->h >= (zy + zh))
          py = gy + gh - pop->h;
        break;

      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_CORNER_LB:
        px = gx + gw;
        py = gy;
        if (py + pop->h >= (zy + zh))
          py = gy + gh - pop->h;
        break;

      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
        py = gy + gh;
        px = (gx + (gw / 2)) - (pop->w / 2);
        if ((px + pop->w) >= (zx + zw))
          px = gx + gw - pop->w;
        else if (px < zx)
          px = zx;
        break;

      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
        py = gy - pop->h;
        px = (gx + (gw / 2)) - (pop->w / 2);
        if ((px + pop->w) >= (zx + zw))
          px = gx + gw - pop->w;
        else if (px < zx)
          px = zx;
        break;

      case E_GADCON_ORIENT_FLOAT:
        px = (gx + (gw / 2)) - (pop->w / 2);
        if (gy >= (zy + (zh / 2)))
          py = gy - pop->h;
        else
          py = gy + gh;
        if ((px + pop->w) >= (zx + zw))
          px = gx + gw - pop->w;
        else if (px < zx)
          px = zx;
        break;

      default:
        e_popup_move_resize(pop->win, 50, 50, pop->w, pop->h);
        return;
     }
   if (px - zx < 0)
     px = zx;
   if (py - zy < 0)
     py = zy;
   e_popup_move_resize(pop->win, px - zx, py - zy, pop->w, pop->h);

   if (pop->gadcon_lock && (!pop->gadcon_was_locked))
     _e_gadcon_popup_locked_set(pop, 1);
}

static void
_e_gadcon_popup_changed_size_hints_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   E_Gadcon_Popup *pop;

   pop = data;
   _e_gadcon_popup_size_recalc(pop, obj);
}

