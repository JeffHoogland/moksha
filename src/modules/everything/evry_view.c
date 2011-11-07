#include "e_mod_main.h"

typedef struct _View View;
typedef struct _Smart_Data Smart_Data;
typedef struct _Item Item;

#define SIZE_LIST 28
#define SIZE_DETAIL 36


struct _View
{
  Evry_View view;
  Tab_View *tabs;

  const Evry_State *state;
  const Evry_Plugin *plugin;

  Evas *evas;
  Evas_Object *bg, *sframe, *span;
  int          iw, ih;
  int          zoom;
  int         mode;
  int         mode_prev;

  Eina_List *handlers;

  Eina_Bool hiding;
};

/* smart object based on wallpaper module */
struct _Smart_Data
{
  View        *view;
  Eina_List   *items;
  Item        *cur_item;
  Ecore_Idle_Enterer *idle_enter;
  Evas_Coord   x, y, w, h;
  Evas_Coord   cx, cy, cw, ch;
  Evas_Coord   sx, sy;

  double last_select;
  double scroll_align;
  double scroll_align_to;
  Ecore_Animator *animator;

  int slide_offset;
  double slide;
  double slide_to;

  int    sliding;
  int    mouse_act;
  int    mouse_x;
  int    mouse_y;
  int    mouse_button;
  Item  *it_down;

  Eina_Bool place;
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
  int max_w, max_h;
};

static View *view = NULL;

static void _view_clear(Evry_View *view);
static void _pan_item_select(Evas_Object *obj, Item *it, int scroll);
static void _animator_del(Evas_Object *obj);
static Eina_Bool _animator(void *data);

static void
_cb_thumb_gen(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Coord w, h;
   Item *it = data;

   if (!it->frame) return;

   e_icon_size_get(it->thumb, &w, &h);
   edje_extern_object_aspect_set(it->thumb, EDJE_ASPECT_CONTROL_BOTH, w, h);
   evas_object_size_hint_max_set(it->thumb, w, h);
   edje_object_part_unswallow(it->frame, it->image);
   edje_object_part_swallow(it->frame, "e.swallow.thumb", it->thumb);
   evas_object_show(it->thumb);
   edje_object_signal_emit(it->frame, "e,action,thumb,show_delayed", "e");
   edje_object_message_signal_process(it->frame);
   it->have_thumb = EINA_TRUE;
   it->do_thumb = EINA_FALSE;

   if (it->image) evas_object_del(it->image);
   it->image = NULL;
}

static void
_cb_preload(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Item *it = data;

   if (!it->frame) return;
   printf("preload callback!!!!\n");

   edje_object_part_swallow(it->frame, "e.swallow.icon", it->image);
   /* evas_object_show(it->image); */
}

static int
_check_item(const Evry_Item *it)
{
   char *suffix;

   GET_FILE(file, it);

   if (!evry_file_path_get(file) || !file->mime) return 0;

   if (!strncmp(file->mime, "image/", 6))
     return 1;

   if ((suffix = strrchr(it->label, '.')))
     if (!strncmp(suffix, ".edj", 4))
       return 1;

   return 0;
}

static void
_item_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Item *it = data;
   Smart_Data *sd = evas_object_smart_data_get(it->obj);
   const Evry_State *s;
   if (!sd) return;

   sd->mouse_act = 1;
   sd->it_down = it;
   sd->mouse_button = ev->button;

   s = sd->view->state;

   if ((ev->button == 1) && (ev->flags & EVAS_BUTTON_DOUBLE_CLICK))
     {
	if (it != sd->cur_item)
	  {
	     evry_item_select(s, it->item);
	     _pan_item_select(it->obj, it, 0);
	  }

	if (it->item->browseable)
	  evry_browse_item(it->item);
	else
	  evry_plugin_action(s->selector->win, 1);
     }
   else
     {
	sd->mouse_x = ev->canvas.x;
	sd->mouse_y = ev->canvas.y;
     }
}

static void
_item_up(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;
   Item *it = data;
   Smart_Data *sd = evas_object_smart_data_get(it->obj);
   const Evry_State *s;
   if (!sd) return;

   sd->mouse_x = 0;
   sd->mouse_y = 0;
   if (!sd->it_down)
     return;

   edje_object_signal_emit(sd->view->bg, "e,action,hide,into", "e");
   edje_object_signal_emit(sd->view->bg, "e,action,hide,back", "e");
   sd->it_down = NULL;

   s = sd->view->state;

   if (ev->button == 1)
     {
	if (!(ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) &&
	    (it != sd->cur_item))
	  {
	     evry_item_select(s, it->item);
	     _pan_item_select(it->obj, it, 0);
	  }
     }
   else if (ev->button == 3)
     {
	evry_item_select(s, it->item);
	_pan_item_select(it->obj, it, 0);

	evry_plugin_action(s->selector->win, 0);
     }
}

static void
_item_select(Item *it)
{
   it->selected = EINA_TRUE;
   edje_object_signal_emit(it->frame, "e,state,selected", "e");

   if (it->thumb)
     {
	if (strcmp(evas_object_type_get(it->thumb), "e_icon"))
	  edje_object_signal_emit(it->thumb, "e,state,selected", "e");
	else
	  e_icon_selected_set(it->thumb, EINA_TRUE);
     }

   if (it->image)
     {
	if (strcmp(evas_object_type_get(it->image), "e_icon"))
	  edje_object_signal_emit(it->image, "e,state,selected", "e");
	else
	  e_icon_selected_set(it->image, EINA_TRUE);
     }
}

static void
_item_unselect(Item *it)
{
   it->selected = EINA_FALSE;
   edje_object_signal_emit(it->frame, "e,state,unselected", "e");

   if (it->thumb)
     {
	if (strcmp(evas_object_type_get(it->thumb), "e_icon"))
	  edje_object_signal_emit(it->thumb, "e,state,unselected", "e");
	else
	  e_icon_selected_set(it->thumb, EINA_FALSE);
     }

   if (it->image)
     {
	if (strcmp(evas_object_type_get(it->image), "e_icon"))
	  edje_object_signal_emit(it->image, "e,state,unselected", "e");
	else
	  e_icon_selected_set(it->image, EINA_FALSE);
     }
}

//static double _icon_time;

static void
_item_show(View *v, Item *it, Evas_Object *list)
{
   if (it->visible)
     return;

   if (!it->frame)
     {
	it->frame = edje_object_add(v->evas);
	if (v->mode == VIEW_MODE_THUMB)
	  {
	     e_theme_edje_object_set(it->frame, "base/theme/modules/everything",
				     "e/modules/everything/thumbview/item/thumb");
	  }
	else
	  {
	     e_theme_edje_object_set(it->frame, "base/theme/modules/everything",
				     "e/modules/everything/thumbview/item/list");

	     if (v->mode == VIEW_MODE_DETAIL)
	       edje_object_signal_emit(it->frame, "e,state,detail,show", "e");
	  }

	evas_object_event_callback_add(it->frame, EVAS_CALLBACK_MOUSE_DOWN,
				       _item_down, it);
	evas_object_event_callback_add(it->frame, EVAS_CALLBACK_MOUSE_UP,
				       _item_up, it);
	evas_object_smart_member_add(it->frame, list);

	evas_object_clip_set(it->frame, evas_object_clip_get(list));

	if (it->item->selected)
	  _item_select(it);

	if (it->item->marked)
	  edje_object_signal_emit(it->frame, "e,state,marked", "e");
     }

   edje_object_part_text_set(it->frame, "e.text.label", it->item->label);

   if (v->mode == VIEW_MODE_DETAIL && it->item->detail)
     edje_object_part_text_set(it->frame, "e.text.detail", it->item->detail);

   evas_object_show(it->frame);

   if (it->item->browseable)
     edje_object_signal_emit(it->frame, "e,state,browseable", "e");

   if (!it->image && !it->have_thumb)
     {
	//double t = ecore_time_get();
	it->image = evry_util_icon_get(it->item, v->evas);
	//_icon_time += ecore_time_get() - t;

	if (it->image)
	  {
	     if (it->max_w == 0)
	       e_icon_size_get(it->image, &it->max_w, &it->max_h);

	     if ((it->max_w > 0) && (it->max_h > 0))
	       evas_object_size_hint_max_set(it->image, it->max_w*2, it->max_h*2);
	     else
	       it->max_w = -1;

	     if (0 && e_icon_preload_get(it->image) && !evas_object_visible_get(it->image))
	       {
		  evas_object_smart_callback_add(it->image, "preloaded", _cb_preload, it);
	       }
	     else
	       {
		  edje_object_part_swallow(it->frame, "e.swallow.icon", it->image);
		  evas_object_show(it->image);
	       }
	  }
	else it->have_thumb = EINA_TRUE;
     }


   if ((it->get_thumb) || (CHECK_TYPE(it->item, EVRY_TYPE_FILE) && _check_item(it->item)))
     {
	char *suffix;

	it->get_thumb = EINA_TRUE;

	it->thumb = e_thumb_icon_add(v->evas);

	GET_FILE(file, it->item);

	evas_object_smart_callback_add(it->thumb, "e_thumb_gen", _cb_thumb_gen, it);

	e_thumb_icon_size_set(it->thumb, it->w, it->h);

	if (it->item->icon && it->item->icon[0])
	  e_thumb_icon_file_set(it->thumb, it->item->icon, NULL);
	else if ((suffix = strrchr(file->path, '.')) && (!strncmp(suffix, ".edj", 4)))
	  {
	     e_thumb_icon_file_set(it->thumb, file->path, "e/desktop/background");
	     e_thumb_icon_size_set(it->thumb, 128, 80);
	  }
	else
	  e_thumb_icon_file_set(it->thumb, file->path, NULL);

	e_thumb_icon_begin(it->thumb);
	it->do_thumb = EINA_TRUE;
     }

   edje_object_signal_emit(it->frame, "e,action,thumb,show", "e");

   it->visible = EINA_TRUE;
}

static void
_item_hide(Item *it)
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
}


static int
_place_items(Smart_Data *sd)
{
   Eina_List *l;
   Item *it;
   int div;
   Evas_Coord x = 0, y = 0, ww, hh, mw = 0, mh = 0;

   if (sd->view->mode == VIEW_MODE_LIST)
     {
	ww = sd->w;
	hh = SIZE_LIST;
     }
   else if (sd->view->mode == VIEW_MODE_DETAIL)
     {
	ww = sd->w;
	hh = SIZE_DETAIL;
     }
   else
     {
	int w, h;
	Evas_Object *o = edje_object_add(sd->view->evas);
	e_theme_edje_object_set(o, "base/theme/modules/everything",
				"e/modules/everything/thumbview/item/thumb");
	edje_object_size_min_get(o, &w, &h);
	evas_object_del(o);

	if ((w > 0) && (h > 0))
	  {
	     div = sd->w / w;
	     if (div < 1) div = 1;
	     ww = w + (sd->w - div * w) / div;
	     hh = ((double)h/(double)w * (double)ww);
	  }
	else
	  {
	     if (sd->view->zoom == 0)
	       ww = 96;
	     else if (sd->view->zoom == 1)
	       ww = 128;
	     else
	       ww = 192;

	     div = sd->w / ww;
	     if (div < 1) div = 1;
	     ww += (sd->w - div * ww) / div;

	     div = sd->h / ww;
	     if (div < 1) div = 1;
	     hh = ww + (sd->h - div * ww) / div;

	     if (hh > ww)
	       hh = ww + (sd->h - (div + 1) * ww) / (div + 1);
	  }
     }

   EINA_LIST_FOREACH(sd->items, l, it)
     {
	it->x = x;
	it->y = y;
	it->w = ww;
	it->h = hh;

	if ((x + ww) > mw) mw = x + ww;
	if ((y + hh) > mh) mh = y + hh;
	x += ww;

	if (x <= (sd->w - ww))
	  continue;

	x = 0;
	y += hh;
     }

   if ((sd->view->mode == VIEW_MODE_LIST) ||
       (sd->view->mode == VIEW_MODE_DETAIL))
     mh += sd->h % hh;

   if ((mw != sd->cw) || (mh != sd->ch))
     {
	sd->cw = mw;
	sd->ch = mh;

	if (sd->cx > (sd->cw - sd->w))
	  sd->cx = sd->cw - sd->w;
	if (sd->cy > (sd->ch - sd->h))
	  sd->cy = sd->ch - sd->h;
	if (sd->cx < 0)
	  sd->cx = 0;
	if (sd->cy < 0)
	  sd->cy = 0;

	return 1;
     }

   return 0;
}

static Eina_Bool
_e_smart_reconfigure_do(void *data)
{
   Evas_Object *obj = data;
   Smart_Data *sd = evas_object_smart_data_get(obj);
   Eina_List *l;
   Item *it;
   Evas_Coord xx, yy;
   double time;
   
   if (!sd)
     return ECORE_CALLBACK_CANCEL;

   sd->idle_enter = NULL;

   if (sd->w < 1)
     return ECORE_CALLBACK_CANCEL;

   if (sd->view->hiding)
     return ECORE_CALLBACK_CANCEL;

   if (sd->cx > (sd->cw - sd->w)) sd->cx = sd->cw - sd->w;
   if (sd->cy > (sd->ch - sd->h)) sd->cy = sd->ch - sd->h;
   if (sd->cx < 0) sd->cx = 0;
   if (sd->cy < 0) sd->cy = 0;

   if (sd->place && _place_items(sd))
     {	
	evas_object_smart_callback_call(obj, "changed", NULL);   
	return ECORE_CALLBACK_RENEW;
     }

   time = ecore_time_get();
   
   EINA_LIST_FOREACH(sd->items, l, it)
     {
	xx = sd->x - sd->cx + it->x;
	yy = sd->y - sd->cy + it->y;

	if (E_INTERSECTS(xx, yy, it->w, it->h, 0, sd->y - it->h*3,
			 sd->x + sd->w, sd->y + sd->h + it->h*6))
	  {
	     if (!it->visible)
	       _item_show(sd->view, it, obj);

	     evas_object_move(it->frame, xx, yy);
	     evas_object_resize(it->frame, it->w, it->h);
	  }
	else if (it->visible)
	  {
	     _item_hide(it);
	  }
	it->changed = EINA_FALSE;

	if (ecore_time_get() - time > 0.03)
	  return ECORE_CALLBACK_RENEW;
     }

   sd->idle_enter = NULL;

   return ECORE_CALLBACK_CANCEL;
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

   sd->sx = sd->sy = -1;
   evas_object_smart_data_set(obj, sd);
}

static void
_e_smart_del(Evas_Object *obj)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   if (sd->idle_enter)
     ecore_idle_enterer_del(sd->idle_enter);

   _animator_del(obj);

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

   sd->place = EINA_TRUE;
   _e_smart_reconfigure(obj);
   /* evas_object_smart_callback_call(obj, "changed", NULL); */
}

static void
_e_smart_show(Evas_Object *obj __UNUSED__){}

static void
_e_smart_hide(Evas_Object *obj __UNUSED__){}

static void
_e_smart_color_set(Evas_Object *obj __UNUSED__, int r __UNUSED__, int g __UNUSED__, int b __UNUSED__, int a __UNUSED__){}

static void
_e_smart_clip_set(Evas_Object *obj __UNUSED__, Evas_Object * clip __UNUSED__){}

static void
_e_smart_clip_unset(Evas_Object *obj __UNUSED__){}

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
   it->changed = EINA_TRUE;

   evry_item_ref(item);

   sd->place = EINA_TRUE;
   _e_smart_reconfigure(obj);

   return it;
}

static void
_pan_item_remove(Evas_Object *obj, Item *it)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   sd->items = eina_list_remove(sd->items, it);

   _item_hide(it);

   evry_item_free(it->item);

   sd->place = EINA_TRUE;
   _e_smart_reconfigure(obj);

   E_FREE(it);
}

static void
_animator_del(Evas_Object *obj)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   sd->animator = NULL;
}

static Eina_Bool
_animator(void *data)
{
   Smart_Data *sd = evas_object_smart_data_get(data);
   if (!sd) return ECORE_CALLBACK_CANCEL;

   double da;
   double spd = ((25.0/ (double)e_config->framerate) /
		 (double) (1 + sd->view->zoom));
   if (spd > 0.9) spd = 0.9;

   int wait = 0;

   if (sd->scroll_align != sd->scroll_align_to)
     {
	sd->scroll_align = ((sd->scroll_align * (1.0 - spd)) +
			    (sd->scroll_align_to * spd));

	da = sd->scroll_align - sd->scroll_align_to;
	if (da < 0.0) da = -da;
	if (da < 0.02)
	  sd->scroll_align = sd->scroll_align_to;
	else
	  wait++;

	e_scrollframe_child_pos_set(sd->view->sframe,
				    0, sd->scroll_align);
     }

   if (wait)
     return ECORE_CALLBACK_RENEW;

   _animator_del(data);

   return ECORE_CALLBACK_CANCEL;

}

static int
_child_region_get(Evas_Object *obj, Evas_Coord y, Evas_Coord h)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   Evas_Coord my = 0, ch = 0, py = 0, ny;

   py = sd->cy;
   ch = sd->ch;

   if (sd->h < sd->ch)
     my = sd->ch - sd->h;
   else
     my = 0;

   ny = py;
   if (y < py) ny = y;
   else if ((y + h) > (py + (ch - my)))
     {
	ny = y + h - (ch - my);
	if (ny > y) ny = y;
     }

   if (ny < 0) ny = 0;

   return ny;
}

static void
_pan_item_select(Evas_Object *obj, Item *it, int scroll)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   double align = -1;

   if (sd->cur_item)
     {
	_item_unselect(sd->cur_item);
	sd->cur_item = NULL;
     }

   if (!it) return;

   _item_select(it);
   sd->cur_item = it;

   if (evry_conf->scroll_animate)
     {
   	double now = ecore_time_get();

   	if (now - sd->last_select < 0.08)
	  {
	     sd->scroll_align = sd->scroll_align_to;
	     scroll = 0;
	  }

   	sd->last_select = now;
     }
   else scroll = 0;

   if (sd->mouse_act)
     return;

   if (sd->view->mode == VIEW_MODE_THUMB)
     {
	if (sd->view->zoom < 2)
	  align = _child_region_get(obj, it->y - it->h, it->h * 3);
	else
	  align = _child_region_get(obj, it->y, it->h);
     }
   else
     {
	align = _child_region_get(obj, it->y - it->h * 2, it->h * 5);
     }

   if (scroll && evry_conf->scroll_animate)
     {
	sd->scroll_align_to = align;

	if (align != sd->cy && !sd->animator)
	  sd->animator = ecore_animator_add(_animator, obj);
     }
   else
     {
	sd->scroll_align = sd->scroll_align_to;

	if (align >= 0)
	  {
	     sd->scroll_align = align;
	     sd->scroll_align_to = align;
	     e_scrollframe_child_pos_set(sd->view->sframe,
					 0, sd->scroll_align);
	  }

	_animator_del(obj);
     }

   _e_smart_reconfigure(obj);
}

static void
_clear_items(Evas_Object *obj)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   Eina_List *l;
   Item *it;

   _animator_del(obj);

   EINA_LIST_FOREACH(sd->items, l, it)
     _item_hide(it);
}

static void
_view_clear(Evry_View *view)
{
   View *v = (View*) view;
   Smart_Data *sd = evas_object_smart_data_get(v->span);
   Item *it;
   if (!sd) return;

   sd->mouse_x = 0;
   sd->mouse_y = 0;
   sd->mouse_act = 0;
   sd->it_down = NULL;

   _clear_items(v->span);

   if (sd->items)
     {
	EINA_LIST_FREE(sd->items, it)
	  {
	     evry_item_free(it->item);
	     E_FREE(it);
	  }
     }

   sd->place = EINA_TRUE;
   _e_smart_reconfigure(v->span);

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
_update_frame(Evas_Object *obj)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   _animator_del(obj);

   sd->scroll_align = 0;

   e_scrollframe_child_pos_set(sd->view->sframe, 0, sd->scroll_align);

   sd->place = EINA_TRUE;
   _e_smart_reconfigure_do(obj);

   _pan_item_select(obj, sd->cur_item, 0);

   _e_smart_reconfigure(obj);

   return 0;
}

static int
_view_update(Evry_View *view)
{
   GET_VIEW(v, view);
   Smart_Data *sd = evas_object_smart_data_get(v->span);
   Item *v_it;
   Evry_Item *p_it;
   Eina_List *l, *ll, *v_remove = NULL, *v_items = NULL;
   int pos, last_pos, last_vis = 0, first_vis = 0;
   Eina_Bool update = EINA_FALSE;
   Evry_Plugin *p = v->state->plugin;

   if (!sd) return 0;
   sd->cur_item = NULL;
   sd->it_down = NULL;
   sd->mouse_act = 0;
   sd->mouse_x = 0;
   sd->mouse_y = 0;

   v->hiding = EINA_FALSE;

   if (!p)
     {
	_view_clear(view);
	return 1;
     }

   if (p != v->plugin && (v->plugin || (v->mode == VIEW_MODE_NONE)))
     {
	if (p->config->view_mode != v->mode)
	  {
	     _clear_items(v->span);

	     if (p->config->view_mode < 0)
	       v->mode = evry_conf->view_mode;
	     else
	       v->mode = p->config->view_mode;
	  }
     }

   /* go through current view items */
   EINA_LIST_FOREACH(sd->items, l, v_it)
     {
	last_pos = v_it->pos;
	v_it->pos = 0;
	pos = 1;

	/* go through plugins current items */
	EINA_LIST_FOREACH(p->items, ll, p_it)
	  {
	     if (v_it->item == p_it)
	       {
		  if (pos != last_pos)
		    v_it->changed = EINA_TRUE;

		  v_it->pos = pos;

		  if (p_it->selected)
		    {
		       sd->cur_item = v_it;
		       v_it->selected = EINA_TRUE;
		    }
		  else
		    {
		       if (v_it->selected && v_it->frame)
			 edje_object_signal_emit
			   (v_it->frame,"e,state,unselected", "e");
		       v_it->selected = EINA_FALSE;
		    }
		  break;
	       }
	     pos++;
	  }

	if (v_it->visible)
	  {
	     if (!first_vis) first_vis = v_it->pos;
	     last_vis = v_it->pos;
	  }

	/* view item is in list of current items */
	if (v_it->pos)
	  {
	     v_items = eina_list_append(v_items, v_it->item);

	     if (v_it->visible && v_it->changed)
	       update = EINA_TRUE;
	  }
	else
	  {
	     if (v_it->visible) update = EINA_TRUE;
	     v_remove = eina_list_append(v_remove, v_it);
	  }
     }

   EINA_LIST_FREE(v_remove, v_it)
     _pan_item_remove(v->span, v_it);

   /* go through plugins current items */
   pos = 1;
   EINA_LIST_FOREACH(p->items, l, p_it)
     {
	/* item is not already in view */
	if (!eina_list_data_find_list(v_items, p_it))
	  {
	     v_it = _pan_item_add(v->span, p_it);

	     if (!v_it) continue;

	     v_it->pos = pos;

	     if (p_it == v->state->cur_item)
	       {
	     	  sd->cur_item = v_it;
	     	  v_it->selected = EINA_TRUE;
	       }

	     if (pos > first_vis && pos < last_vis)
	       update = EINA_TRUE;
	  }
	pos++;
     }
   if (v_items) eina_list_free(v_items);

   sd->items = eina_list_sort(sd->items, -1, _sort_cb);

   if (update || !last_vis || v->plugin != p)
     {
	v->plugin = p;
	_update_frame(v->span);
     }

   v->tabs->update(v->tabs);

   return 0;
}

static int
_cb_key_down(Evry_View *view, const Ecore_Event_Key *ev)
{
   View *v = (View *) view;
   Smart_Data *sd = evas_object_smart_data_get(v->span);
   Eina_List *l = NULL, *ll;
   Item *it = NULL;
   const Evry_State *s;
   int slide;

   if (!sd || !(s = v->state) || !(s->plugin))
     return 0;

   const char *key = ev->key;

   sd->mouse_act = 0;
   sd->mouse_x = 0;
   sd->mouse_y = 0;

   if ((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) &&
       (!strcmp(key, "2")))
     {
	if (v->mode == VIEW_MODE_LIST)
	  v->mode = VIEW_MODE_DETAIL;
	else
	  v->mode = VIEW_MODE_LIST;

	v->zoom = 0;
	_clear_items(v->span);
	_update_frame(v->span);
	goto end;
     }
   else if (((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) &&
	     (!strcmp(key, "3"))) || !strcmp(key, "XF86Forward"))
     {
	if (v->mode != VIEW_MODE_THUMB)
	  {
	     v->zoom = 0;
	     v->mode = VIEW_MODE_THUMB;
	     _clear_items(v->span);
	  }
	else
	  {
	     v->zoom++;
	     if (v->zoom > 2) v->zoom = 0;
	     if (v->zoom == 2)
	       _clear_items(v->span);
	  }
	_update_frame(v->span);
	goto end;
     }
   else if (!strcmp(key, "XF86Back"))
     {
	if (v->mode == VIEW_MODE_LIST ||
	    v->mode == VIEW_MODE_DETAIL)
	  {
	     v->zoom = 0;
	     v->mode = VIEW_MODE_THUMB;
	  }
	else
	  {
	     v->mode = VIEW_MODE_DETAIL;
	  }

	_clear_items(v->span);
	_update_frame(v->span);
	goto end;
     }

   if (((ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT) ||
	(ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)) &&
       (!strcmp(key, "Up")))
     {
	if (!sd->items) goto end;
	it = sd->items->data;

	_pan_item_select(v->span, it, 1);
	evry_item_select(s, it->item);
	goto end;
     }
   else if (((ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT) ||
	     (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)) &&
	    (!strcmp(key, "Down")))
     {
	if (!sd->items) goto end;

	it = eina_list_last(sd->items)->data;
	_pan_item_select(v->span, it, 1);
	evry_item_select(s, it->item);
	goto end;
     }

   if ((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) &&
       (!strcmp(key, "plus")))
     {
	EINA_LIST_FOREACH(sd->items, ll, it)
	  {
	     if (!it->item->marked)
	       {
		  if (it->frame)
		    edje_object_signal_emit(it->frame, "e,state,marked", "e");
		  evry_item_mark(s, it->item, 1);
	       }
	  }
	goto end;
     }
   else if ((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) &&
	    (!strcmp(key, "minus")))
     {
	EINA_LIST_FOREACH(sd->items, ll, it)
	  {
	     if (it->item->marked)
	       {
		  if (it->frame)
		    edje_object_signal_emit(it->frame, "e,state,unmarked", "e");
		  evry_item_mark(s, it->item, 0);
	       }
	  }
	goto end;
     }
   else if (!strcmp(key, "comma") || !strcmp(key, "semicolon"))
     {
	if (!sd->cur_item)
	  goto end;

	if (!sd->cur_item->item->marked)
	  {
	     edje_object_signal_emit(sd->cur_item->frame, "e,state,marked", "e");
	     evry_item_mark(s, sd->cur_item->item, 1);
	  }
	else
	  {
	     edje_object_signal_emit(sd->cur_item->frame, "e,state,unmarked", "e");
	     evry_item_mark(s, sd->cur_item->item, 0);
	  }

	if (v->mode == VIEW_MODE_THUMB)
	  {
	     if (!strcmp(key, "comma"))
	       key = "Right";
	     else
	       key = "Left";
	  }
	else
	  {
	     if (!strcmp(key, "comma"))
	       key = "Down";
	     else
	       key = "Up";
	  }
     }

   if ((slide = v->tabs->key_down(v->tabs, ev)))
     {
	/* _view_update(view, -slide); */
	_view_update(view);
	return 1;
     }

   if (sd->items)
     l = eina_list_data_find_list(sd->items, sd->cur_item);
   if (!l)
     l = sd->items;

   if (v->mode == VIEW_MODE_THUMB && !evry_conf->cycle_mode)
     {
	if (!strcmp(key, "Right"))
	  {
	     if (l && l->next)
	       it = l->next->data;

	     if (it)
	       {
		  _pan_item_select(v->span, it, 1);
		  evry_item_select(s, it->item);
	       }
	     goto end;
	  }
	else if (!strcmp(key, "Left"))
	  {
	     if (l && l->prev)
	       it = l->prev->data;

	     if (it)
	       {
		  _pan_item_select(v->span, it, 1);
		  evry_item_select(s, it->item);
	       }
	     goto end;
	  }
     }
   if (!strcmp(key, "Down"))
     {
	if (v->mode == VIEW_MODE_THUMB &&
	    (!evry_conf->cycle_mode) &&
	    (sd->cur_item))
	  {
	     EINA_LIST_FOREACH(l, ll, it)
	       {
		  if (it->y > sd->cur_item->y &&
		      it->x >= sd->cur_item->x)
		    break;
	       }
	  }

	if (!it && l && l->next)
	  it = l->next->data;

	if (it)
	  {
	     _pan_item_select(v->span, it, 1);
	     evry_item_select(s, it->item);
	  }
	goto end;
     }
   else if (!strcmp(key, "Up"))
     {
	if (v->mode == VIEW_MODE_THUMB &&
	    (!evry_conf->cycle_mode) &&
	    (sd->cur_item))
	  {
	     for(ll = l; ll; ll = ll->prev)
	       {
		  it = ll->data;

		  if (it->y < sd->cur_item->y &&
		      it->x <= sd->cur_item->x)
		    break;
	       }
	  }

	if (!it && l && l->prev)
	  it = l->prev->data;

	if (it)
	  {
	     _pan_item_select(v->span, it, 1);
	     evry_item_select(s, it->item);
	  }
	goto end;
     }
   else if ((!strcmp(key, "Prior") || (!strcmp(key, "Next"))))
     {
	int cur = 0;
	int next = (!strcmp(key, "Next"));
	if (sd->cur_item)
	  cur = sd->cur_item->y;

	EINA_LIST_FOREACH(sd->items, l, it)
	  {
	     if (next)
	       {
		  if (it->y >= cur + sd->h) break;
	       }
	     else
	       {
		  if (it->y + it->h >= cur - sd->h) break;
	       }

	     if (!l->next)
	       break;
	  }

	if (it)
	  {
	     _pan_item_select(v->span, it, 0);
	     evry_item_select(s, it->item);
	  }
	goto end;
     }
   else if (!ev->modifiers && !strcmp(key, "Return"))
     {
	if (v->mode == VIEW_MODE_THUMB)
	  {
	     if (!sd->cur_item)
	       goto end;

	     if (evry_browse_item(sd->cur_item->item))
	       goto end;
	  }
     }

   return 0;

 end:
   return 1;
}

static Eina_Bool
_cb_item_changed(void *data, int type __UNUSED__, void *event)
{
   Evry_Event_Item_Changed *ev = event;
   View *v = data;
   Eina_List *l;
   Item *it;
   Smart_Data *sd = evas_object_smart_data_get(v->span);
   if (!sd) return ECORE_CALLBACK_PASS_ON;

   EINA_LIST_FOREACH(sd->items, l, it)
     if (it->item == ev->item)
       break;

   if (!it)
     return ECORE_CALLBACK_PASS_ON;

   if (ev->changed_selection)
     {
	_pan_item_select(v->span, it, 1);
	evry_item_select(v->state, ev->item);
     }

   if (!it->visible)
     return ECORE_CALLBACK_PASS_ON;

   edje_object_part_text_set(it->frame, "e.text.label", it->item->label);

   if (ev->changed_icon)
     {
	if (it->do_thumb) e_thumb_icon_end(it->thumb);
	if (it->thumb) evas_object_del(it->thumb);
	if (it->image) evas_object_del(it->image);

	it->thumb = NULL;
	it->image = NULL;

	it->have_thumb = EINA_FALSE;
	it->do_thumb = EINA_FALSE;
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_action_performed(void *data, int type __UNUSED__, void *event)
{
   Evry_Event_Action_Performed *ev = event;
   View *v = data;
   Eina_List *l;
   Item *it;
   Smart_Data *sd;

   sd = evas_object_smart_data_get(v->span);
   if (!sd) return ECORE_CALLBACK_PASS_ON;

   EINA_LIST_FOREACH(sd->items, l, it)
     {

	if ((it->item == ev->it1) ||
	    (it->item == ev->it2))
	  break;
     }

   if (it && it->visible)
     {
	evas_object_raise(it->frame);
	edje_object_signal_emit(it->frame, "e,action,go", "e");
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void
_view_cb_mouse_wheel(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Wheel *ev = event_info;
   Smart_Data *sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (ev->z)
     {
	if (sd->cur_item)
	  _item_select(sd->cur_item);
	sd->mouse_act = 1;
     }
}

static void
_view_cb_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Smart_Data *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   sd->mouse_act = 1;
   sd->mouse_button = ev->button;
   sd->mouse_x = ev->canvas.x;
   sd->mouse_y = ev->canvas.y;
}

static void
_view_cb_mouse_up(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   /* Evas_Event_Mouse_Up *ev = event_info; */
   Smart_Data *sd = evas_object_smart_data_get(data);
   if (!sd) return;
   sd->mouse_x = 0;
   sd->mouse_y = 0;
   sd->mouse_button = 0;
   edje_object_signal_emit(sd->view->bg, "e,action,hide,into", "e");
   edje_object_signal_emit(sd->view->bg, "e,action,hide,back", "e");
}

#define SLIDE_RESISTANCE 80

static void
_view_cb_mouse_move(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   Smart_Data *sd = evas_object_smart_data_get(data);
   Evry_Selector *sel;
   int diff_y, diff_x;

   if (!sd) return;

   if (!sd->mouse_x)
     goto end;

   sel = sd->view->state->selector;
   if (!sel || !sel->states)
     return;

   diff_x = abs(ev->cur.canvas.x - sd->mouse_x);
   diff_y = abs(ev->cur.canvas.y - sd->mouse_y);

   if (diff_y > 15 + (diff_x/2))
     {
	edje_object_signal_emit(sd->view->bg, "e,action,hide,into", "e");
	edje_object_signal_emit(sd->view->bg, "e,action,hide,back", "e");
	goto end;
     }

   if ((sel->states->next) || (sel != sel->win->selectors[0]))
     edje_object_signal_emit(sd->view->bg, "e,action,show,back", "e");

   if (sd->it_down)
     {
	if ((sd->it_down->item->browseable) ||
	    (sel != sel->win->selectors[2]))
	  edje_object_signal_emit(sd->view->bg, "e,action,show,into", "e");

	if ((sd->cur_item != sd->it_down) && (diff_x > 10))
	  {
	     evry_item_select(sd->view->state, sd->it_down->item);
	     _pan_item_select(data, sd->it_down, 0);
	  }
     }

   if (sd->mouse_button == 1)
     {
	if (ev->cur.canvas.x - sd->mouse_x > SLIDE_RESISTANCE)
	  {
	     sd->it_down = NULL;
	     sd->mouse_x = 0;
	     sd->mouse_y = 0;
	     if (sel->states->next)
	       evry_browse_back(sel);
	     else
	       evry_selectors_switch(sel->win, -1, EINA_TRUE);
	  }
	else if ((sd->it_down && (sd->cur_item == sd->it_down)) &&
		 (sd->mouse_x - ev->cur.canvas.x > SLIDE_RESISTANCE))
	  {
	     edje_object_signal_emit(sd->view->bg, "e,action,hide,into", "e");
	     edje_object_signal_emit(sd->view->bg, "e,action,hide,back", "e");

	     if (sd->it_down->item->browseable)
	       {
		  evry_browse_item(sd->it_down->item);
	       }
	     else
	       {
		  evry_selectors_switch(sel->win, 1, EINA_TRUE);
	       }

	     sd->it_down = NULL;
	     sd->mouse_x = 0;
	     sd->mouse_y = 0;
	  }
     }
   return;

 end:
   sd->it_down = NULL;
   sd->mouse_x = 0;
   sd->mouse_y = 0;
}
static void _cb_list_hide(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   View *v = data;

   v->hiding = EINA_TRUE;
}

static void _cb_list_show(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   View *v = data;

   v->hiding = EINA_FALSE;
}

static Evry_View *
_view_create(Evry_View *view, const Evry_State *s, const Evas_Object *swallow)
{
   GET_VIEW(parent, view);

   View *v;
   Ecore_Event_Handler *h;

   v = E_NEW(View, 1);
   v->view = *view;
   v->state = s;
   v->evas = evas_object_evas_get(swallow);

   if (s->plugin)
     {
	if ((s->selector->states->next) &&
	    ((s->plugin->config->view_mode < 0) ||
	     (!strcmp(s->plugin->name, N_("All")))))
	  v->mode = parent->mode;
	else if (s->plugin->config->view_mode >= 0)
	  v->mode = s->plugin->config->view_mode;
	else
	  v->mode = evry_conf->view_mode;
     }
   else
     {
	if (s->selector->states->next)
	  v->mode = parent->mode;
	else
	  v->mode = evry_conf->view_mode;
     }

   v->plugin = s->plugin;
   v->mode_prev = v->mode;
   v->zoom = parent->zoom;

   v->bg = edje_object_add(v->evas);
   e_theme_edje_object_set(v->bg, "base/theme/modules/everything",
			   "e/modules/everything/thumbview/main/window");
   // scrolled thumbs
   v->span = _pan_add(v->evas);
   _pan_view_set(v->span, v);
   evas_object_event_callback_add(v->span, EVAS_CALLBACK_MOUSE_WHEEL,
				  _view_cb_mouse_wheel, NULL);

   evas_object_event_callback_add(v->bg, EVAS_CALLBACK_MOUSE_MOVE,
				  _view_cb_mouse_move, v->span);
   evas_object_event_callback_add(v->bg, EVAS_CALLBACK_MOUSE_DOWN,
				  _view_cb_mouse_down, v->span);
   evas_object_event_callback_add(v->bg, EVAS_CALLBACK_MOUSE_UP,
				  _view_cb_mouse_up, v->span);

   // the scrollframe holding the scrolled thumbs
   v->sframe = e_scrollframe_add(v->evas);
   e_scrollframe_custom_theme_set(v->sframe, "base/theme/modules/everything",
				  "e/modules/everything/thumbview/main/scrollframe");
   e_scrollframe_thumbscroll_force(v->sframe, 1);

   e_scrollframe_extern_pan_set(v->sframe, v->span,
				_pan_set, _pan_get, _pan_max_get,
				_pan_child_size_get);
   edje_object_part_swallow(v->bg, "e.swallow.list", v->sframe);

   evas_object_show(v->sframe);
   evas_object_show(v->span);

   v->tabs = evry_tab_view_new(EVRY_VIEW(v), v->state, v->evas);

   EVRY_VIEW(v)->o_list = v->bg;
   EVRY_VIEW(v)->o_bar = v->tabs->o_tabs;

   h = evry_event_handler_add(EVRY_EVENT_ITEM_CHANGED, _cb_item_changed, v);
   v->handlers = eina_list_append(v->handlers, h);
   h = evry_event_handler_add(EVRY_EVENT_ACTION_PERFORMED, _cb_action_performed, v);
   v->handlers = eina_list_append(v->handlers, h);

   edje_object_signal_callback_add(v->bg, "e,action,hide,list", "e", _cb_list_hide, v);
   edje_object_signal_callback_add(v->bg, "e,action,show,list", "e", _cb_list_show, v);
   return EVRY_VIEW(v);
}

static void
_view_destroy(Evry_View *view)
{
   Ecore_Event_Handler *h;

   GET_VIEW(v, view);

   _view_clear(view);

   evas_object_del(v->span);
   evas_object_del(v->bg);
   evas_object_del(v->sframe);

   evry_tab_view_free(v->tabs);

   EINA_LIST_FREE(v->handlers, h)
     ecore_event_handler_del(h);

   E_FREE(v);
}

Eina_Bool
evry_view_init(void)
{
   if (!evry_api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   View *v = E_NEW(View, 1);

   v->view.id = EVRY_VIEW(v);
   v->view.name = "Icon View";
   v->view.create = &_view_create;
   v->view.destroy = &_view_destroy;
   v->view.update = &_view_update;
   v->view.clear = &_view_clear;
   v->view.cb_key_down = &_cb_key_down;
   v->mode = VIEW_MODE_NONE;

   evry_view_register(EVRY_VIEW(v), 1);

   view = v;

   return EINA_TRUE;
}

void
evry_view_shutdown(void)
{
   evry_view_unregister(EVRY_VIEW(view));
   E_FREE(view);
}
