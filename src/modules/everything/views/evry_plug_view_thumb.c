#include "e_mod_main.h"

typedef struct _View View;
typedef struct _Smart_Data Smart_Data;
typedef struct _Item Item;

struct _View
{
  Evry_View view;
  Evas *evas;
  const Evry_State *state;
  Tab_View *tabs;

  Evas_Object *bg, *sframe, *span;
  int          iw, ih;
};

/* smart object based on wallpaper module */
struct _Smart_Data
{
  View        *view;
  Eina_List   *items;
  Item        *sel_item;
  Ecore_Idle_Enterer *idle_enter;
  Ecore_Idle_Enterer *thumb_idler;
  Ecore_Animator *animator;
  Evas_Coord   x, y, w, h;
  Evas_Coord   cx, cy, cw, ch;
  Evas_Coord   sx, sy;
  double       selmove;
  Eina_Bool    update : 1;

  int zoom;
};

struct _Item
{
  Evry_Item *item;
  Evas_Object *obj;
  Evas_Coord x, y, w, h;
  Evas_Object *frame, *image, *thumb;
  Eina_Bool selected : 1;
  Eina_Bool have_thumb : 1;
  Eina_Bool do_thumb : 1;
  Eina_Bool get_thumb : 1;
  Eina_Bool showing : 1;
  Eina_Bool visible : 1;
  Eina_Bool changed : 1;
  int pos;
};

static Evry_View *view = NULL;
static const char *view_types = NULL;

static void
_thumb_gen(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Coord w, h;
   Item *it = data;

   if (!it->frame) return;

   e_icon_size_get(it->thumb, &w, &h);
   edje_extern_object_aspect_set(it->thumb, EDJE_ASPECT_CONTROL_BOTH, w, h);
   edje_object_part_swallow(it->frame, "e.swallow.thumb", it->thumb);
   evas_object_show(it->thumb);
   it->have_thumb = EINA_TRUE;
   it->do_thumb = EINA_FALSE;

   if (it->image) evas_object_del(it->image);
   it->image = NULL;

}

static int
_thumb_idler(void *data)
{
   Smart_Data *sd = data;
   Eina_List *l;
   Item *it;
   int cnt = 0;

   EINA_LIST_FOREACH(sd->items, l, it)
     {
	if (it->thumb && !(it->have_thumb || it->do_thumb))
	  {
	     ITEM_FILE(file, it->item);

	     evas_object_smart_callback_add(it->thumb, "e_thumb_gen", _thumb_gen, it);

	     e_thumb_icon_file_set(it->thumb, file->uri, NULL);
	     e_thumb_icon_size_set(it->thumb, it->w, it->h);
	     e_thumb_icon_begin(it->thumb);
	     it->do_thumb = EINA_TRUE;

	     cnt++;
	  }

	if (cnt > 4)
	  {
	     e_util_wakeup();
	     return 1;
	  }
     }

   sd->thumb_idler = NULL;

   return 0;
}

static int
_e_smart_reconfigure_do(void *data)
{
   Evas_Object *obj = data;
   Smart_Data *sd = evas_object_smart_data_get(obj);
   Eina_List *l;
   Item *it;
   int iw, redo = 0, changed = 0;
   static int recursion = 0;
   Evas_Coord x, y, xx, yy, ww, hh, mw, mh, ox, oy, dd;

   if (!sd) return 0;
   if (sd->cx > (sd->cw - sd->w)) sd->cx = sd->cw - sd->w;
   if (sd->cy > (sd->ch - sd->h)) sd->cy = sd->ch - sd->h;
   if (sd->cx < 0) sd->cx = 0;
   if (sd->cy < 0) sd->cy = 0;
   e_scrollframe_child_viewport_size_get(sd->view->sframe,
					 &sd->view->iw,
					 &sd->view->ih);

   if (sd->zoom == 0)
     {
	int cnt = eina_list_count(sd->items);

	sd->view->iw *= 3;
	sd->view->iw /= 4;

	if (cnt < 3)
	  iw = (double)sd->w / 2.5;
	else if (cnt < 7)
	  iw = sd->w / 3;
	else
	  iw = sd->w / 4;
     }
   else if (sd->zoom == 1)
     {
	sd->view->iw *= 2;
	sd->view->iw /= 3;
	iw = sd->w / 3;
     }
   else /* if (sd->zoom == 2) */
     {
	iw = sd->w;
     }

   if (sd->view->iw <= 0) sd->view->iw = 1;
   if (sd->view->ih <= 0) sd->view->ih = 1;

   x = 0;
   y = 0;
   ww = iw;
   hh = (sd->view->ih * iw) / (sd->view->iw);

   mw = mh = 0;
   EINA_LIST_FOREACH(sd->items, l, it)
     {
        /* xx = sd->x - sd->cx + x; */
        if (x > (sd->w - ww))
          {
             x = 0;
             y += hh;
             xx = sd->x - sd->cx + x;
          }
        /* yy = sd->y - sd->cy + y; */
        it->x = x;
        it->y = y;
        it->w = ww;
        it->h = hh;
        if (it->selected)
          {
             sd->sx = it->x + (it->w / 2);
             sd->sy = it->y + (it->h / 2);
          }
        if ((x + ww) > mw)mw = x + ww;
        if ((y + hh) > mh) mh = y + hh;
        x += ww;
     }
   if ((mw != sd->cw) || (mh != sd->ch))
     {
        sd->cw = mw;
        sd->ch = mh;
        if (sd->cx > (sd->cw - sd->w))
          {
             sd->cx = sd->cw - sd->w;
             redo = 1;
          }
        if (sd->cy > (sd->ch - sd->h))
          {
             sd->cy = sd->ch - sd->h;
             redo = 1;
          }
        if (sd->cx < 0)
          {
             sd->cx = 0;
             redo = 1;
          }
        if (sd->cy < 0)
          {
             sd->cy = 0;
             redo = 1;
          }
        if (redo)
	  {
	     recursion = 1;
	     _e_smart_reconfigure_do(obj);
	     recursion = 0;
	  }
        changed = 1;
     }

   ox = 0;
   if (sd->w > sd->cw) ox = (sd->w - sd->cw) / 2;
   oy = 0;
   if (sd->h > sd->ch) oy = (sd->h - sd->ch) / 2;

   if (sd->sel_item && !sd->update)
     {
   	int y, h;
   	it = sd->sel_item;

   	e_scrollframe_child_pos_get(sd->view->sframe, NULL, &y);
   	e_scrollframe_child_viewport_size_get(sd->view->sframe, NULL, &h);

	if ((it->y + it->h) - y > h)
	  e_scrollframe_child_pos_set(sd->view->sframe, 0, it->y - (2 - sd->zoom) * it->h);
	else if (it->y < y)
	  e_scrollframe_child_pos_set(sd->view->sframe, 0, it->y);
     }

   EINA_LIST_FOREACH(sd->items, l, it)
     {
        Evas_Coord dx, dy, vw, vh;

        dx = dy = 0;
        if ((sd->sx >= 0) &&
            (sd->selmove > 0.0))
          {
             double a, d;

             // -----0X0+++++
             dx = (it->x + (it->w / 2)) - sd->sx;
             dy = (it->y + (it->h / 2)) - sd->sy;
             if (dx > 0)
               {
                  /* |/
                   * +-- */
                  if (dy < 0)
                    a = -atan(-(double)dy / (double)dx);
                  /* +--
                   * |\ */
                  else
                    a = atan((double)dy / (double)dx);
               }
             else if (dx == 0)
               {
                    /* |
                     * + */
                  if (dy < 0) a = -M_PI / 2;
                    /* +
                     * | */
                  else a = M_PI / 2;
               }
             else
               {
                  /*  \|
                   * --+ */
                  if (dy < 0)
                    a = -M_PI + atan((double)dy / (double)dx);
                  /* --+
                   *  /| */
                  else
                    a = M_PI - atan(-(double)dy / (double)dx);
               }
             d = sqrt((double)(dx * dx) + (double)(dy * dy));
	     /* dy = 0; */

             xx = sd->sx - sd->cx + ox;
             yy = sd->sy - sd->cy + oy;

	     if (xx < (sd->w / 2))
	       dx = sd->w - xx;
             else
	       dx = xx;

	     if (yy < (sd->h / 2))
	       dy = sd->h - yy;
             else
	       dy = yy;

             dd = dx - d;
             if (dy > dx) dd = dy - d;
             if (dd < 0) dd = 0;
             dy = sin(a) * sd->selmove * (dd * 0.9);
             dx = cos(a) * sd->selmove * (dd * 0.9);
          }
        xx = sd->x - sd->cx + it->x + ox;
        yy = sd->y - sd->cy + it->y + oy;

	evas_output_viewport_get(evas_object_evas_get(obj), NULL, NULL, &vw, &vh);
        if (E_INTERSECTS(xx, yy, it->w, it->h, 0, 0, vw, vh))
          {
	     if (!it->visible)
	       {
		  it->frame = edje_object_add(sd->view->evas);

		  e_theme_edje_object_set(it->frame, "base/theme/widgets",
					  "e/modules/everything/thumbview/main/mini");

		  evas_object_smart_member_add(it->frame, obj);
		  evas_object_clip_set(it->frame, evas_object_clip_get(obj));

		  edje_object_part_text_set(it->frame, "e.text.label", it->item->label);

		  if (!it->image && !it->have_thumb &&
		      it->item->plugin && it->item->plugin->icon_get)
		    {
		       it->image = it->item->plugin->icon_get
			 (it->item->plugin, it->item, sd->view->evas);

		       edje_object_part_swallow(it->frame, "e.swallow.icon", it->image);
		       evas_object_show(it->image);
		    }

		  evas_object_show(it->frame);

		  if (sd->update && !it->visible)
		    edje_object_signal_emit(it->frame, "e,action,thumb,show_delayed", "e");
		  else if (!it->visible)
		    edje_object_signal_emit(it->frame, "e,action,thumb,show", "e");

		  it->visible = EINA_TRUE;
	       }

	     if (it->selected && sd->zoom < 2)
	       edje_object_signal_emit(it->frame, "e,state,selected", "e");
	     
	     evas_object_move(it->frame, xx + dx, yy + dy);
	     evas_object_resize(it->frame, it->w, it->h);
	     
	     if (it->get_thumb && !it->thumb)
	       {
		  it->thumb = e_thumb_icon_add(sd->view->evas);
		  
		  if (!sd->thumb_idler)
		    sd->thumb_idler = ecore_idle_enterer_before_add(_thumb_idler, sd);
	       }
          }
        else if (it->visible)
	  {
	     if (it->do_thumb) e_thumb_icon_end(it->thumb);
	     if (it->thumb) evas_object_del(it->thumb);
	     if (it->image) evas_object_del(it->image);
	     if (it->frame) evas_object_del(it->frame);

	     it->thumb = NULL;
	     it->image = NULL;
	     it->frame = NULL;

	     it->have_thumb = EINA_FALSE;
	     it->do_thumb = EINA_FALSE;
	     it->visible = EINA_FALSE;
	     it->changed = TRUE;
	  }
     }

   if (changed)
     evas_object_smart_callback_call(obj, "changed", NULL);

   sd->update = EINA_FALSE;

   if (recursion == 0)
     sd->idle_enter = NULL;
   return 0;
}

static void
_e_smart_reconfigure(Evas_Object *obj)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   if (sd->idle_enter) return;
   sd->idle_enter = ecore_idle_enterer_before_add(_e_smart_reconfigure_do, obj);
}

static void
_e_smart_add(Evas_Object *obj)
{
   Smart_Data *sd = calloc(1, sizeof(Smart_Data));
   if (!sd) return;
   sd->x = sd->y = sd->w = sd->h = 0;
   sd->sx = sd->sy = -1;
   evas_object_smart_data_set(obj, sd);
}

static void
_e_smart_del(Evas_Object *obj)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   Item *it;

   if (sd->idle_enter)
     ecore_idle_enterer_del(sd->idle_enter);
   if (sd->thumb_idler)
     ecore_idle_enterer_del(sd->thumb_idler);

   // sd->view is just referenced
   // sd->child_obj is unused
   EINA_LIST_FREE(sd->items, it)
     {
	if (it->do_thumb) e_thumb_icon_end(it->thumb);
        if (it->thumb) evas_object_del(it->thumb);
        if (it->frame) evas_object_del(it->frame);
        if (it->image) evas_object_del(it->image);
	evry_item_free(it->item);
        free(it);
     }
   free(sd);
   evas_object_smart_data_set(obj, NULL);
}

static void
_e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   sd->x = x;
   sd->y = y;
   _e_smart_reconfigure(obj);
}

static void
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   sd->w = w;
   sd->h = h;
   _e_smart_reconfigure(obj);
   evas_object_smart_callback_call(obj, "changed", NULL);
}

static void
_e_smart_show(Evas_Object *obj)
{
   /* Smart_Data *sd = evas_object_smart_data_get(obj); */
   //   evas_object_show(sd->child_obj);
}

static void
_e_smart_hide(Evas_Object *obj)
{
   /* Smart_Data *sd = evas_object_smart_data_get(obj); */
   //   evas_object_hide(sd->child_obj);
}

static void
_e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   /* Smart_Data *sd = evas_object_smart_data_get(obj); */
   //   evas_object_color_set(sd->child_obj, r, g, b, a);
}

static void
_e_smart_clip_set(Evas_Object *obj, Evas_Object * clip)
{
   /* Smart_Data *sd = evas_object_smart_data_get(obj); */
   //   evas_object_clip_set(sd->child_obj, clip);
}

static void
_e_smart_clip_unset(Evas_Object *obj)
{
   /* Smart_Data *sd = evas_object_smart_data_get(obj); */
   //   evas_object_clip_unset(sd->child_obj);
}

static Evas_Object *
_pan_add(Evas *evas)
{
   static Evas_Smart *smart = NULL;
   static const Evas_Smart_Class sc =
     {
       "wp_pan",
       EVAS_SMART_CLASS_VERSION,
       _e_smart_add,
       _e_smart_del,
       _e_smart_move,
       _e_smart_resize,
       _e_smart_show,
       _e_smart_hide,
       _e_smart_color_set,
       _e_smart_clip_set,
       _e_smart_clip_unset,
       NULL,
       NULL,
       NULL,
       NULL
     };
   smart = evas_smart_class_new(&sc);
   return evas_object_smart_add(evas, smart);
}

static void
_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   if (x > (sd->cw - sd->w)) x = sd->cw - sd->w;
   if (y > (sd->ch - sd->h)) y = sd->ch - sd->h;
   if (x < 0) x = 0;
   if (y < 0) y = 0;
   if ((sd->cx == x) && (sd->cy == y)) return;
   sd->cx = x;
   sd->cy = y;
   _e_smart_reconfigure(obj);
}

static void
_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   if (x) *x = sd->cx;
   if (y) *y = sd->cy;
}

static void
_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   if (x)
     {
        if (sd->w < sd->cw) *x = sd->cw - sd->w;
        else *x = 0;
     }
   if (y)
     {
        if (sd->h < sd->ch) *y = sd->ch - sd->h;
        else *y = 0;
     }
}

static void
_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   if (w) *w = sd->cw;
   if (h) *h = sd->ch;
}

static void
_pan_view_set(Evas_Object *obj, View *view)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   sd->view = view;
}

static int
_check_item(const Evry_Item *it)
{
   if (it->plugin->type_out != view_types) return 0;

   ITEM_FILE(file, it);

   if (!file->uri || !file->mime) return 0;

   if (!strncmp(file->mime, "image/", 6))
     return 1;

   return 0;
}

static Item *
_pan_item_add(Evas_Object *obj, Evry_Item *item)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   Item *it;

   it = E_NEW(Item, 1);
   if (!it) return NULL;

   sd->items = eina_list_append(sd->items, it);
   it->obj = obj;
   it->item = item;

   if (_check_item(item))
     it->get_thumb = EINA_TRUE;

   evry_item_ref(item);

   _e_smart_reconfigure(obj);

   return it;
}

static void
_pan_item_remove(Evas_Object *obj, Item *it)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   sd->items = eina_list_remove(sd->items, it);
   if (it->do_thumb) e_thumb_icon_end(it->thumb);
   if (it->thumb) evas_object_del(it->thumb);
   if (it->frame) evas_object_del(it->frame);
   if (it->image) evas_object_del(it->image);

   evry_item_free(it->item);
   E_FREE(it);

   _e_smart_reconfigure(obj);
}

static void
_pan_item_select(Evas_Object *obj, Item *it)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   sd->sel_item->selected = EINA_FALSE;
   edje_object_signal_emit(sd->sel_item->frame, "e,state,unselected", "e");
   sd->sel_item = it;
   sd->sel_item->selected = EINA_TRUE;

   if (sd->zoom < 2)
     edje_object_signal_emit(sd->sel_item->frame, "e,state,selected", "e");

   if (sd->idle_enter) ecore_idle_enterer_del(sd->idle_enter);
   sd->idle_enter = ecore_idle_enterer_add(_e_smart_reconfigure_do, obj);
}

static void
_view_clear(Evry_View *view)
{
   View *v = (View*) view;
   Smart_Data *sd = evas_object_smart_data_get(v->span);
   Item *it;

   EINA_LIST_FREE(sd->items, it)
     {
	if (it->do_thumb) e_thumb_icon_end(it->thumb);
   	if (it->thumb) evas_object_del(it->thumb);
   	if (it->frame) evas_object_del(it->frame);
   	if (it->image) evas_object_del(it->image);
   	evry_item_free(it->item);
   	E_FREE(it);
     }

   if (sd->idle_enter) ecore_idle_enterer_del(sd->idle_enter);
   sd->idle_enter = ecore_idle_enterer_add(_e_smart_reconfigure_do, v->span);

   v->tabs->clear(v->tabs);
}

static int
_sort_cb(const void *data1, const void *data2)
{
   const Item *it1 = data1;
   const Item *it2 = data2;

   return it1->pos - it2->pos;
}

static int
_view_update(Evry_View *view)
{
   VIEW(v, view);
   Smart_Data *sd = evas_object_smart_data_get(v->span);
   Item *v_it;
   Evry_Item *p_it;
   Eina_List *l, *ll, *p_items, *v_remove = NULL, *v_items = NULL;
   int pos, last_pos;

   if (!v->state->plugin)
     {
	_view_clear(view);
	return 1;
     }

   p_items = v->state->plugin->items;

   EINA_LIST_FOREACH(sd->items, l, v_it)
     {
	last_pos = v_it->pos;
	v_it->pos = 0;
	pos = 1;

	EINA_LIST_FOREACH(p_items, ll, p_it)
	  {
	     if (v_it->item == p_it)
	       {
		  if (pos != last_pos)
		    v_it->changed = EINA_TRUE;

		  v_it->pos = pos;

		  if (p_it == v->state->sel_item)
		    {
		       sd->sel_item = v_it;
		       v_it->selected = EINA_TRUE;
		    }
		  else
		    v_it->selected = EINA_FALSE;

		  break;
	       }
	     pos++;
	  }

	if(v_it->pos)
	  {
	     v_items = eina_list_append(v_items, v_it->item);
	     if (_check_item(v_it->item))
	       v_it->get_thumb = EINA_TRUE;
	  }

	else
	  v_remove = eina_list_append(v_remove, v_it);
     }

   if (v_remove)
     sd->update = EINA_TRUE;

   EINA_LIST_FREE(v_remove, v_it)
     _pan_item_remove(v->span, v_it);

   pos = 1;
   int added = 0;

   EINA_LIST_FOREACH(p_items, l, p_it)
     {
	if (!eina_list_data_find_list(v_items, p_it))
	  {
	     added = 1;
	     v_it = _pan_item_add(v->span, p_it);

	     if (!v_it) continue;

	     v_it->pos = pos;

	     if (p_it == v->state->sel_item)
	       {
		  sd->sel_item = v_it;
		  v_it->selected = EINA_TRUE;
	       }
	  }
	pos++;
     }

   sd->items = eina_list_sort(sd->items, eina_list_count(sd->items), _sort_cb);

   if (added) sd->update = EINA_TRUE;

   if (sd->idle_enter) ecore_idle_enterer_del(sd->idle_enter);
   sd->idle_enter = ecore_idle_enterer_add(_e_smart_reconfigure_do, v->span);

   if (v_items) eina_list_free(v_items);

   v->tabs->update(v->tabs);

   return 1;
}

static int
_cb_key_down(Evry_View *view, const Ecore_Event_Key *ev)
{
   View *v = (View *) view;
   Smart_Data *sd = evas_object_smart_data_get(v->span);
   Eina_List *l = NULL, *ll;
   Item *it = NULL;

   if (!v->state->plugin)
     return 0;

   if ((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) &&
       ((!strcmp(ev->key, "plus")) ||
	(!strcmp(ev->key, "z"))))
     {
	sd->zoom++;
	if (sd->zoom > 2) sd->zoom = 0;

	if (sd->zoom == 2)
	  {
	     EINA_LIST_FOREACH(sd->items, l, it)
	       {
		  if (it->have_thumb)
		    {
		       evas_object_del(it->thumb);
		       it->thumb = NULL;
		       it->have_thumb = EINA_FALSE;
		    }
		  else if (it->do_thumb)
		    {
		       e_thumb_icon_end(it->thumb);
		       evas_object_del(it->thumb);
		       it->thumb = NULL;
		       it->do_thumb = EINA_FALSE;
		    }
	       }
	  }


	if (sd->idle_enter) ecore_idle_enterer_del(sd->idle_enter);
	sd->idle_enter = ecore_idle_enterer_add(_e_smart_reconfigure_do, v->span);

	goto end;
     }

   if (v->tabs->key_down(v->tabs, ev))
     {
	_view_update(view);
	return 1;
     }

   if (sd->items)
     l = eina_list_data_find_list(sd->items, sd->sel_item);

   if (!strcmp(ev->key, "Right"))
     {
	if (l && l->next)
	  it = l->next->data;

	if (it)
	  {
	     _pan_item_select(v->span, it);
	     evry_item_select(v->state, it->item);
	  }
	goto end;
     }
   else if (!strcmp(ev->key, "Left"))
     {
	if (!sd->items) return 1;

	if (l && l->prev)
	  it = l->prev->data;

	if (it)
	  {
	     _pan_item_select(v->span, it);
	     evry_item_select(v->state, it->item);
	  }
	goto end;
     }
   else if (!strcmp(ev->key, "Down"))
     {
	if (!sd->items) return 1;

	EINA_LIST_FOREACH(l, ll, it)
	  {
	     if (it->y > sd->sel_item->y &&
		 it->x >= sd->sel_item->x)
	       break;
	  }

	if (!it && l && l->next)
	  it = l->next->data;

	if (it)
	  {
	     _pan_item_select(v->span, it);
	     evry_item_select(v->state, it->item);
	  }
	goto end;
     }
   else if (!strcmp(ev->key, "Up"))
     {
	if (!sd->items) return 1;

	EINA_LIST_REVERSE_FOREACH(l, ll, it)
	  {
	     if (it->y < sd->sel_item->y &&
		 it->x <= sd->sel_item->x)
	       break;
	  }

	if (!it && l && l->prev)
	  it = l->prev->data;

	if (it)
	  {
	     _pan_item_select(v->span, it);
	     evry_item_select(v->state, it->item);
	  }
	goto end;
     }

   return 0;

 end:
   return 1;
}

static Evry_View *
_view_create(Evry_View *view, const Evry_State *s, const Evas_Object *swallow)
{
   View *v;

   if (!s->plugin)
     return NULL;

   v = E_NEW(View, 1);
   v->view = *view;
   v->state = s;
   v->evas = evas_object_evas_get(swallow);

   v->bg = edje_object_add(v->evas);
   e_theme_edje_object_set(v->bg, "base/theme/widgets",
                           "e/modules/everything/thumbview/main/window");
   // scrolled thumbs
   v->span = _pan_add(v->evas);
   _pan_view_set(v->span, v);

   // the scrollframe holding the scrolled thumbs
   v->sframe = e_scrollframe_add(v->evas);
   e_scrollframe_custom_theme_set(v->sframe, "base/theme/widgets",
                                  "e/modules/everything/thumbview/main/scrollframe");
   e_scrollframe_extern_pan_set(v->sframe, v->span,
                                _pan_set, _pan_get, _pan_max_get,
                                _pan_child_size_get);
   edje_object_part_swallow(v->bg, "e.swallow.list", v->sframe);
   evas_object_show(v->sframe);
   evas_object_show(v->span);

   EVRY_VIEW(v)->o_list = v->bg;

   v->tabs = evry_tab_view_new(s, v->evas);
   v->view.o_bar = v->tabs->o_tabs;

   return EVRY_VIEW(v);
}

static void
_view_destroy(Evry_View *view)
{
   VIEW(v, view);

   evas_object_del(v->bg);
   evas_object_del(v->sframe);
   evas_object_del(v->span);

   evry_tab_view_free(v->tabs);

   E_FREE(v);
}

static Eina_Bool
_init(void)
{
   view = E_NEW(Evry_View, 1);
   view->id = view;
   view->name = "Icon View";
   view->create = &_view_create;
   view->destroy = &_view_destroy;
   view->update = &_view_update;
   view->clear = &_view_clear;
   view->cb_key_down = &_cb_key_down;
   evry_view_register(view, 2);

   view_types = eina_stringshare_add("FILE");

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   eina_stringshare_del(view_types);
   evry_view_unregister(view);
   E_FREE(view);
}


EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);


