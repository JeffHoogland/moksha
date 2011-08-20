#include "e.h"
#include "e_kbd_buf.h"
#include "e_kbd_int.h"
#include "e_kbd_send.h"
#include "e_mod_config.h"

enum 
{
   NORMAL = 0,
   SHIFT = (1 << 0),
   CAPSLOCK = (1 << 1),
   CTRL = (1 << 2),
   ALT = (1 << 3),
   ALTGR = (1 << 4)
};

static Evas_Object *_theme_obj_new(Evas *e, const char *custom_dir, const char *group);

static void _e_kbd_int_layout_next(E_Kbd_Int *ki);
static void _e_kbd_int_zoomkey_down(E_Kbd_Int *ki);
static void _e_kbd_int_matches_update(void *data);
static void _e_kbd_int_dictlist_down(E_Kbd_Int *ki);
static void _e_kbd_int_matchlist_down(E_Kbd_Int *ki);
static Eina_Bool _e_kbd_int_cb_border_move(void *data, int type, void *event);

static void
_e_kbd_int_cb_resize(E_Win *win)
{
   E_Kbd_Int *ki;
   
   ki = win->data;
   evas_object_resize(ki->base_obj, ki->win->w, ki->win->h);
   _e_kbd_int_zoomkey_down(ki);
   _e_kbd_int_dictlist_down(ki);
   _e_kbd_int_matchlist_down(ki);
}

static const char *
_e_kbd_int_str_unquote(const char *str)
{
   static char buf[256];
   char *p;
   
   snprintf(buf, sizeof(buf), "%s", str + 1);
   p = strrchr(buf, '"');
   if (p) *p = 0;
   return buf;
}

static E_Kbd_Int_Key *
_e_kbd_int_at_coord_get(E_Kbd_Int *ki, Evas_Coord x, Evas_Coord y)
{
   Eina_List *l;
   Evas_Coord dist;
   E_Kbd_Int_Key *ky;  
   E_Kbd_Int_Key *closest_ky = NULL;

   EINA_LIST_FOREACH(ki->layout.keys, l, ky)
     if ((x >= ky->x) && (y >= ky->y) &&
         (x < (ky->x + ky->w)) && (y < (ky->y + ky->h)))
       return ky;
   dist = 0x7fffffff;
   EINA_LIST_FOREACH(ki->layout.keys, l, ky)
     {
	Evas_Coord dx, dy;

	dx = x - (ky->x + (ky->w / 2));
	dy = y - (ky->y + (ky->h / 2));
	dx = (dx * dx) + (dy * dy);
	if (dx < dist)
	  {
	     dist = dx;
	     closest_ky = ky;
	  }
     }
   return closest_ky;
}

static E_Kbd_Int_Key_State *
_e_kbd_int_key_state_get(E_Kbd_Int *ki, E_Kbd_Int_Key *ky)
{
   E_Kbd_Int_Key_State *found = NULL;
   E_Kbd_Int_Key_State *st;
   Eina_List *l;

   EINA_LIST_FOREACH(ky->states, l, st)
     {
	if (st->state & ki->layout.state) return st;
	if (!found && st->state == NORMAL) found = st;
     }
   return found;
}

static void
_e_kbd_int_layout_buf_update(E_Kbd_Int *ki)
{
   E_Kbd_Int_Key *ky;
   Eina_List *l, *l2;

   e_kbd_buf_layout_clear(ki->kbuf);
   e_kbd_buf_layout_size_set(ki->kbuf, ki->layout.w, ki->layout.h);
   e_kbd_buf_layout_fuzz_set(ki->kbuf, ki->layout.fuzz);
   EINA_LIST_FOREACH(ki->layout.keys, l, ky)
     {
	E_Kbd_Int_Key_State *st;
	const char *out, *out_shift, *out_capslock, *out_altgr;

	out = NULL;
	out_shift = NULL;
	out_capslock = NULL;
	out_altgr = NULL;

	EINA_LIST_FOREACH(ky->states, l2, st)
	  {
	     if (st->state == NORMAL)
	       out = st->out;
	     else if (st->state == SHIFT)
	       out_shift = st->out;
	     else if (st->state == CAPSLOCK)
	       out_capslock = st->out;
	     else if (st->state == ALTGR)
	       out_altgr = st->out;
	  }
	if (out)
	  {
	     char *s1 = NULL, *s2 = NULL, *s3 = NULL;

	     if ((out) && (out[0] == '"')) 
	       s1 = strdup(_e_kbd_int_str_unquote(out));
	     if ((out_shift) && (out_shift[0] == '"')) 
	       s2 = strdup(_e_kbd_int_str_unquote(out_shift));
	     if ((out_altgr) && (out_altgr[0] == '"')) 
	       s2 = strdup(_e_kbd_int_str_unquote(out_altgr));
	     if ((out_capslock) && (out_capslock[0] == '"')) 
	       s3 = strdup(_e_kbd_int_str_unquote(out_capslock));
	     e_kbd_buf_layout_key_add(ki->kbuf, s1, s2, s3, 
				      ky->x, ky->y, ky->w, ky->h);
	     if (s1) free(s1);
	     if (s2) free(s2);
	     if (s3) free(s3);
	  }
     }
}

static void
_e_kbd_int_layout_state_update(E_Kbd_Int *ki)
{
   E_Kbd_Int_Key *ky;
   Eina_List *l;

   EINA_LIST_FOREACH(ki->layout.keys, l, ky)
     {
	E_Kbd_Int_Key_State *st;
	int selected;

	st = _e_kbd_int_key_state_get(ki, ky);
	if (st)
	  {
	     if (st->label)
	       edje_object_part_text_set(ky->obj, "e.text.label", st->label);
	     else
	       edje_object_part_text_set(ky->obj, "e.text.label", "");
	     if (st->icon)
	       {
		  char buf[PATH_MAX];
		  char *p;

		  snprintf(buf, sizeof(buf), "%s/%s", ki->layout.directory, st->icon);
		  p = strrchr(st->icon, '.');
		  if (!strcmp(p, ".edj"))
		    e_icon_file_edje_set(ky->icon_obj, buf, "icon");
		  else
		    e_icon_file_set(ky->icon_obj, buf);
	       }
	     else
	       e_icon_file_set(ky->icon_obj, NULL);
	  }
	selected = 0;
	if ((ki->layout.state & SHIFT) && (ky->is_shift)) selected = 1;
	if ((ki->layout.state & CTRL) && (ky->is_ctrl)) selected = 1;
	if ((ki->layout.state & ALT) && (ky->is_alt)) selected = 1;
	if ((ki->layout.state & ALTGR) && (ky->is_altgr)) selected = 1;
	if ((ki->layout.state & CAPSLOCK) && (ky->is_capslock)) selected = 1;
	if ((ki->layout.state & (SHIFT | CAPSLOCK)) && (ky->is_multi_shift)) 
          selected = 1;
	if (selected)
	  {
	     if (!ky->selected)
	       {
		  edje_object_signal_emit(ky->obj, "e,state,selected", "e");
		  ky->selected = 1;
	       }
	  }
	if (!selected)
	  {
	     if (ky->selected)
	       {
		  edje_object_signal_emit(ky->obj, "e,state,unselected", "e");
		  ky->selected = 0;
	       }
	  }
     }
}

static void
_e_kbd_int_string_send(E_Kbd_Int *ki, const char *str)
{
   int pos, newpos, glyph;

   pos = 0;
   e_kbd_buf_word_use(ki->kbuf, str);
   for (;;)
     {
	char buf[16];

	newpos = evas_string_char_next_get(str, pos, &glyph);
	if (glyph <= 0) return;
	strncpy(buf, str + pos, newpos - pos);
	buf[newpos - pos] = 0;
	e_kbd_send_string_press(buf, 0);
	pos = newpos;
     }
}

static void
_e_kbd_int_buf_send(E_Kbd_Int *ki)
{
   const char *str = NULL;
   const Eina_List *matches;

   matches = e_kbd_buf_string_matches_get(ki->kbuf);
   if (matches) str = matches->data;
   else str = e_kbd_buf_actual_string_get(ki->kbuf);
   if (str) _e_kbd_int_string_send(ki, str);
}

static void
_e_kbd_int_cb_match_select(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Kbd_Int_Match *km;

   km = data;
   _e_kbd_int_string_send(km->ki, km->str);
   e_kbd_buf_clear(km->ki->kbuf);
   e_kbd_send_keysym_press("space", 0);
   if (km->ki->layout.state & (SHIFT | CTRL | ALT | ALTGR))
     {
	km->ki->layout.state &= (~(SHIFT | CTRL | ALT | ALTGR));
	_e_kbd_int_layout_state_update(km->ki);
     }
   _e_kbd_int_matches_update(km->ki);
}

static void
_e_kbd_int_matches_add(E_Kbd_Int *ki, const char *str, int num)
{
   E_Kbd_Int_Match *km;
   Evas_Object *o;
   Evas_Coord mw, mh;

   km = E_NEW(E_Kbd_Int_Match, 1);
   if (!km) return;
   o = _theme_obj_new(ki->win->evas, ki->themedir,
		      "e/modules/kbd/match/word");
   km->ki = ki;
   km->str = eina_stringshare_add(str);
   km->obj = o;
   ki->matches = eina_list_append(ki->matches, km);
   edje_object_part_text_set(o, "e.text.label", str);
   edje_object_size_min_calc(o, &mw, &mh);
   if (mw < 32) mw = 32;
   if (num & 0x1) e_box_pack_start(ki->box_obj, o);
   else e_box_pack_end(ki->box_obj, o);
   e_box_pack_options_set(o, 1, 1, 1, 1, 0.5, 0.5, mw, mh, 9999, 9999);
   if (num == 0)
     edje_object_signal_emit(o, "e,state,selected", "e");
   edje_object_signal_callback_add(o, "e,action,do,select", "",
				   _e_kbd_int_cb_match_select, km);
   evas_object_show(o);
}

static void
_e_kbd_int_matches_free(E_Kbd_Int *ki)
{
   E_Kbd_Int_Match *km;

   EINA_LIST_FREE(ki->matches, km)
     {
	if (km->str) eina_stringshare_del(km->str);
	evas_object_del(km->obj);
	free(km);
     }
}

static void
_e_kbd_int_matches_update(void *data)
{
   E_Kbd_Int *ki;
   const Eina_List *l, *matches;
   const char *actual;
   Evas_Coord mw, mh, vw, vh;

   if (!(ki = data)) return;
   evas_event_freeze(ki->win->evas);
   e_box_freeze(ki->box_obj);
   _e_kbd_int_matches_free(ki);
   matches = e_kbd_buf_string_matches_get(ki->kbuf);
   if (!matches)
     {
	actual = e_kbd_buf_actual_string_get(ki->kbuf);
	if (actual) _e_kbd_int_matches_add(ki, actual, 0);
     }
   else
     {
	int i = 0;

	for (i = 0, l = matches; l; l = l->next, i++)
	  {
	     _e_kbd_int_matches_add(ki, l->data, i);
	     e_box_size_min_get(ki->box_obj, &mw, &mh);
	     edje_object_part_geometry_get(ki->base_obj, "e.swallow.label", 
					   NULL, NULL, &vw, &vh);
	     if (mw > vw) break;
	  }

	if (!l)
	  {
	     actual = e_kbd_buf_actual_string_get(ki->kbuf);
	     if (actual)
	       {
		  const char *str;

		  EINA_LIST_FOREACH(matches, l, str)
		    if (!strcmp(str, actual)) break;
		  if (!l) _e_kbd_int_matches_add(ki, actual, i);
	       }
	  }
     }
   e_box_thaw(ki->box_obj);
   e_box_size_min_get(ki->box_obj, &mw, &mh);
   edje_extern_object_min_size_set(ki->box_obj, 0, mh);
   edje_object_part_swallow(ki->base_obj, "e.swallow.label", ki->box_obj);
   evas_event_thaw(ki->win->evas);

   _e_kbd_int_matchlist_down(ki);
}

static void
_e_kbd_int_key_press_handle(E_Kbd_Int *ki, E_Kbd_Int_Key *ky)
{
   E_Kbd_Int_Key_State *st;
   const char *out = NULL;

   if (!ky) return;

   if (ky->is_shift)
     {
	if (ki->layout.state & SHIFT) ki->layout.state &= (~(SHIFT));
	else ki->layout.state |= SHIFT;
	_e_kbd_int_layout_state_update(ki);
	return;
     }
   if (ky->is_multi_shift)
     {
	if (ki->layout.state & SHIFT)
          {
             ki->layout.state &= (~(SHIFT));
             ki->layout.state |= CAPSLOCK;
          }
	else if (ki->layout.state & CAPSLOCK)
          ki->layout.state &= (~(CAPSLOCK));
        else
          ki->layout.state |= SHIFT;
	_e_kbd_int_layout_state_update(ki);
	return;
     }
   if (ky->is_ctrl)
     {
	if (ki->layout.state & CTRL) ki->layout.state &= (~(CTRL));
	else ki->layout.state |= CTRL;
	if (e_kbd_buf_actual_string_get(ki->kbuf)) _e_kbd_int_buf_send(ki);
	e_kbd_buf_clear(ki->kbuf);
	_e_kbd_int_layout_state_update(ki);
	_e_kbd_int_matches_update(ki);
	return;
     }
   if (ky->is_alt)
     {
	if (ki->layout.state & ALT) ki->layout.state &= (~(ALT));
	else ki->layout.state |= ALT;
	if (e_kbd_buf_actual_string_get(ki->kbuf)) _e_kbd_int_buf_send(ki);
	e_kbd_buf_clear(ki->kbuf);
	_e_kbd_int_layout_state_update(ki);
	_e_kbd_int_matches_update(ki);
	return;
     }
   if (ky->is_altgr)
     {
	if (ki->layout.state & ALTGR) ki->layout.state &= (~(ALTGR));
	else ki->layout.state |= ALTGR;
	_e_kbd_int_layout_state_update(ki);
	return;
     }
   if (ky->is_capslock)
     {
	if (ki->layout.state & CAPSLOCK) ki->layout.state &= (~CAPSLOCK);
	else ki->layout.state |= CAPSLOCK;
	_e_kbd_int_layout_state_update(ki);
	return;
     }
   st = _e_kbd_int_key_state_get(ki, ky);
   if (st) out = st->out;
   if (ki->layout.state & (CTRL | ALT))
     {
	if (out)
	  {
	     Kbd_Mod mods = 0;

	     if (ki->layout.state & CTRL) mods |= KBD_MOD_CTRL;
	     if (ki->layout.state & ALT) mods |= KBD_MOD_ALT;
	     if (out[0] == '"')
               e_kbd_send_string_press(_e_kbd_int_str_unquote(out), mods);
	     else
	       e_kbd_send_keysym_press(out, mods);
	  }
	ki->layout.state &= (~(SHIFT | CTRL | ALT | ALTGR));
	_e_kbd_int_layout_state_update(ki);
	e_kbd_buf_lookup(ki->kbuf, _e_kbd_int_matches_update, ki);
	return;
     }
   if (out)
     {
	if (out[0] == '"')
	  {
	     if (ki->down.zoom)
	       e_kbd_buf_pressed_key_add(ki->kbuf,
					 _e_kbd_int_str_unquote(out), 
					 ki->layout.state & SHIFT, 
					 ki->layout.state & CAPSLOCK);
	     else
	       e_kbd_buf_pressed_point_add(ki->kbuf,
                                           ky->x + (ky->w / 2),
                                           ky->y + (ky->h / 2),
					   ki->layout.state & SHIFT, 
					   ki->layout.state & CAPSLOCK);
	     e_kbd_buf_lookup(ki->kbuf, _e_kbd_int_matches_update, ki);
	  }
	else
	  {
	     if (e_kbd_buf_actual_string_get(ki->kbuf)) 
               _e_kbd_int_buf_send(ki);
	     e_kbd_buf_clear(ki->kbuf);
	     e_kbd_send_keysym_press(out, 0);
	     _e_kbd_int_matches_update(ki);
	  }
     }
   if (ki->layout.state & (SHIFT | CTRL | ALT | ALTGR))
     {
	printf("CLEARING STATE\n");
	if (!ky->is_multi_shift) 
          ki->layout.state &= (~(SHIFT | CTRL | ALT | ALTGR));
	_e_kbd_int_layout_state_update(ki);
     }
}
    
static void
_e_kbd_int_stroke_handle(E_Kbd_Int *ki, int dir)
{
   /* If the keyboard direction is RTL switch dir 3 and 1
    * i.e, make forward backwards and the other way around */
   if (ki->layout.direction == E_KBD_INT_DIRECTION_RTL)
     {
        if (dir == 3)
          dir = 1;
	else if (dir == 1)
          dir = 3;
     }

   if (dir == 4) // up
     _e_kbd_int_layout_next(ki);
   else if (dir == 3) // left
     {
	if (e_kbd_buf_actual_string_get(ki->kbuf))
	  {
	     e_kbd_buf_backspace(ki->kbuf);
	     e_kbd_buf_lookup(ki->kbuf, _e_kbd_int_matches_update, ki);
	  }
	else
	  e_kbd_send_keysym_press("BackSpace", 0);
     }
   else if (dir == 2) // down
     {
	if (e_kbd_buf_actual_string_get(ki->kbuf)) _e_kbd_int_buf_send(ki);
	e_kbd_buf_clear(ki->kbuf);
	e_kbd_send_keysym_press("Return", 0);
	_e_kbd_int_matches_update(ki);
     }
   else if (dir == 1) // right
     {
	if (e_kbd_buf_actual_string_get(ki->kbuf)) _e_kbd_int_buf_send(ki);
	e_kbd_buf_clear(ki->kbuf);
	e_kbd_send_keysym_press("space", 0);
	_e_kbd_int_matches_update(ki);
     }
   if (ki->layout.state)
     {
	ki->layout.state = 0;
	_e_kbd_int_layout_state_update(ki);
     }
}
    
static void
_e_kbd_int_zoomkey_down(E_Kbd_Int *ki)
{
   const Eina_List *l;

   if (!ki->zoomkey.popup) return;
   e_object_del(E_OBJECT(ki->zoomkey.popup));
   ki->zoomkey.popup = NULL;
   ki->zoomkey.layout_obj = NULL;
   ki->zoomkey.sublayout_obj = NULL;
   for (l = ki->layout.keys; l; l = l->next)
     {
        E_Kbd_Int_Key *ky;

	ky = l->data;
	ky->zoom_obj = NULL;
	ky->zoom_icon_obj = NULL;
     }
}

static void
_e_kbd_int_zoomkey_up(E_Kbd_Int *ki)
{
   const Eina_List *l;
   Evas_Object *o, *o2;
   Evas_Coord mw, mh, vw, vh;
   int sx, sy, sw, sh;

   if (ki->zoomkey.popup) return;
   ki->zoomkey.popup = e_popup_new(ki->win->border->zone, -1, -1, 1, 1);
   e_popup_layer_set(ki->zoomkey.popup, 190);

   o = _theme_obj_new(ki->zoomkey.popup->evas, ki->themedir,
		      "e/modules/kbd/zoom/default");
   ki->zoomkey.base_obj = o;

   o = e_layout_add(ki->zoomkey.popup->evas);
   e_layout_virtual_size_set(o, 100, 100);
   edje_object_part_swallow(ki->zoomkey.base_obj, "e.swallow.content", o);
   evas_object_show(o);
   ki->zoomkey.layout_obj = o;

   e_layout_virtual_size_get(ki->layout_obj, &vw, &vh);

   o = e_layout_add(ki->zoomkey.popup->evas);
   e_layout_virtual_size_set(o, vw, vh);
   e_layout_pack(ki->zoomkey.layout_obj, o);
   e_layout_child_move(o, 0, 0);
   e_layout_child_resize(o,
                         vw * il_kbd_cfg->zoom_level,
                         vh * il_kbd_cfg->zoom_level);
   evas_object_show(o);
   ki->zoomkey.sublayout_obj = o;

   for (l = ki->layout.keys; l; l = l->next)
     {
	E_Kbd_Int_Key *ky;
	E_Kbd_Int_Key_State *st;
	const char *label, *icon;
	int selected;

	ky = l->data;
	o = _theme_obj_new(ki->zoomkey.popup->evas, ki->themedir,
			   "e/modules/kbd/zoomkey/default");
	label = "";
	icon = NULL;
	st = _e_kbd_int_key_state_get(ki, ky);
	if (st)
	  {
	     label = st->label;
	     icon = st->icon;
	  }

	edje_object_part_text_set(o, "e.text.label", label);

	o2 = e_icon_add(ki->zoomkey.popup->evas);
	e_icon_fill_inside_set(o2, 1);
//	e_icon_scale_up_set(o2, 0);
	edje_object_part_swallow(o, "e.swallow.content", o2);
	evas_object_show(o2);

	if (icon)
	  {
	     char buf[PATH_MAX];
	     char *p;

	     snprintf(buf, sizeof(buf), "%s/%s", ki->layout.directory, icon);
	     p = strrchr(icon, '.');
	     if (!strcmp(p, ".edj"))
	       e_icon_file_edje_set(o2, buf, "icon");
	     else
	       e_icon_file_set(o2, buf);
	  }
	selected = 0;
	if ((ki->layout.state & SHIFT) && (ky->is_shift)) selected = 1;
	if ((ki->layout.state & CTRL) && (ky->is_ctrl)) selected = 1;
	if ((ki->layout.state & ALT) && (ky->is_alt)) selected = 1;
	if ((ki->layout.state & ALTGR) && (ky->is_altgr)) selected = 1;
	if ((ki->layout.state & CAPSLOCK) && (ky->is_capslock)) selected = 1;
	if ((ki->layout.state & (SHIFT|CAPSLOCK)) && (ky->is_multi_shift)) 
          selected = 1;
	if (selected)
	  edje_object_signal_emit(o, "e,state,selected", "e");
	if (!selected)
	  edje_object_signal_emit(o, "e,state,unselected", "e");
	e_layout_pack(ki->zoomkey.sublayout_obj, o);
	e_layout_child_move(o, ky->x, ky->y);
	e_layout_child_resize(o, ky->w, ky->h);
	evas_object_show(o);
	ky->zoom_obj = o;
	ky->zoom_icon_obj = o2;
     }

   edje_object_size_min_calc(ki->zoomkey.base_obj, &vw, &vh);
   e_zone_useful_geometry_get(ki->win->border->zone, &sx, &sy, &sw, &sh);
   sh -= ki->win->h;
   mw = sw;
   if ((vw > 0) && (mw > vw)) mw = vw;
   mh = sh;
   if ((vh > 0) && (mh > vh)) mh = vh;

   e_popup_move_resize(ki->zoomkey.popup, 
		       sx + ((sw - mw) / 2), sy + ((sh - mh) / 2), mw, mh);
   evas_object_resize(ki->zoomkey.base_obj,
		      ki->zoomkey.popup->w, ki->zoomkey.popup->h);
   evas_object_show(ki->zoomkey.base_obj);
   e_popup_edje_bg_object_set(ki->zoomkey.popup, ki->zoomkey.base_obj);
   e_popup_show(ki->zoomkey.popup);
   e_popup_layer_set(ki->zoomkey.popup, 190);
}

static void
_e_kbd_int_zoomkey_update(E_Kbd_Int *ki)
{
   Evas_Coord w, h, ww, hh;
   E_Kbd_Int_Key *ky;

   evas_object_geometry_get(ki->zoomkey.layout_obj, NULL, NULL, &w, &h);
   evas_object_geometry_get(ki->layout_obj, NULL, NULL, &ww, &hh);
   e_layout_virtual_size_set(ki->zoomkey.layout_obj, w, h);
   e_layout_child_resize(ki->zoomkey.sublayout_obj,
                         ww * il_kbd_cfg->zoom_level,
                         hh * il_kbd_cfg->zoom_level);
   e_layout_child_move(ki->zoomkey.sublayout_obj, 
		       (w / 2) - (ki->down.cx * il_kbd_cfg->zoom_level),
		       (h / 2) - (ki->down.cy * il_kbd_cfg->zoom_level));
   ky = _e_kbd_int_at_coord_get(ki, ki->down.clx, ki->down.cly);
   if (ky != ki->zoomkey.pressed)
     {
	if (ki->zoomkey.pressed)
	  {
	     ki->zoomkey.pressed->pressed = 0;
	     edje_object_signal_emit(ki->zoomkey.pressed->zoom_obj,
				     "e,state,released", "e");
	     edje_object_signal_emit(ki->zoomkey.pressed->obj,
				     "e,state,released", "e");
	  }
	ki->zoomkey.pressed = ky;
	if (ki->zoomkey.pressed)
	  {
	     ki->zoomkey.pressed->pressed = 1;
	     e_layout_child_raise(ki->zoomkey.pressed->zoom_obj);
	     edje_object_signal_emit(ki->zoomkey.pressed->zoom_obj,
				     "e,state,pressed", "e");
	     e_layout_child_raise(ki->zoomkey.pressed->obj);
	     e_layout_child_raise(ki->event_obj);
	     edje_object_signal_emit(ki->zoomkey.pressed->obj,
				     "e,state,pressed", "e");
	  }
     }
}

static Eina_Bool
_e_kbd_int_cb_hold_timeout(void *data)
{
   E_Kbd_Int *ki;

   ki = data;
   ki->down.hold_timer = NULL;
   ki->down.zoom = 1;
   if (ki->layout.pressed)
     {
	ki->layout.pressed->pressed = 0;
	edje_object_signal_emit(ki->layout.pressed->obj,
				"e,state,released", "e");
	ki->layout.pressed = NULL;
     }
   _e_kbd_int_zoomkey_up(ki);
   _e_kbd_int_zoomkey_update(ki);
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_kbd_int_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Kbd_Int *ki;
   Evas_Coord x, y, w, h;
   E_Kbd_Int_Key *ky;

   ev = event_info;
   if (ev->button != 1) return;
   ki = data;
   ki->down.x = ev->canvas.x;
   ki->down.y = ev->canvas.y;
   ki->down.down = 1;
   ki->down.stroke = 0;

   evas_object_geometry_get(ki->event_obj, &x, &y, &w, &h);
   x = ev->canvas.x - x;
   y = ev->canvas.y - y;
   ki->down.cx = x;
   ki->down.cy = y;
   x = (x * ki->layout.w) / w;
   y = (y * ki->layout.h) / h;

   ki->down.lx = x;
   ki->down.ly = y;
   ki->down.clx = x;
   ki->down.cly = y;

   if (ki->down.hold_timer) ecore_timer_del(ki->down.hold_timer);
   ki->down.hold_timer = ecore_timer_add(il_kbd_cfg->hold_timer,
                                         _e_kbd_int_cb_hold_timeout, ki);

   ky = _e_kbd_int_at_coord_get(ki, x, y);
   ki->layout.pressed = ky;
   if (ky)
     {
	ky->pressed = 1;
	e_layout_child_raise(ky->obj);
	e_layout_child_raise(ki->event_obj);
	edje_object_signal_emit(ky->obj, "e,state,pressed", "e");
     }
}

static void
_e_kbd_int_cb_mouse_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   E_Kbd_Int *ki;
   Evas_Coord dx, dy, x, w, y, h;

   ev = event_info;
   ki = data;

   if (ki->down.zoom)
     {
	evas_object_geometry_get(ki->event_obj, &x, &y, &w, &h);
	x = ev->cur.canvas.x - x;
	y = ev->cur.canvas.y - y;
	ki->down.cx = x;
	ki->down.cy = y;
	x = (x * ki->layout.w) / w;
	y = (y * ki->layout.h) / h;
	ki->down.clx = x;
	ki->down.cly = y;
	_e_kbd_int_zoomkey_update(ki);
	return;
     }
   if (ki->down.stroke) return;

   dx = ev->cur.canvas.x - ki->down.x;
   dy = ev->cur.canvas.y - ki->down.y;
   evas_object_geometry_get(ki->event_obj, &x, &y, &w, &h);
   dx = (dx * ki->layout.w) / w;
   dy = (dy * ki->layout.h) / h;
   if ((dx > 0) && (dx > (ki->layout.w / il_kbd_cfg->slide_dim)))
     ki->down.stroke = 1;
   else if ((dx < 0) && (-dx > (ki->layout.w / il_kbd_cfg->slide_dim)))
     ki->down.stroke = 1;
   if ((dy > 0) && (dy > (ki->layout.h / il_kbd_cfg->slide_dim)))
     ki->down.stroke = 1;
   else if ((dy < 0) && (-dy > (ki->layout.w / il_kbd_cfg->slide_dim)))
     ki->down.stroke = 1;
   if ((ki->down.stroke) && (ki->down.hold_timer))
     {
	ecore_timer_del(ki->down.hold_timer);
	ki->down.hold_timer = NULL;
     }
}

static void
_e_kbd_int_cb_mouse_up(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   E_Kbd_Int *ki;
   E_Kbd_Int_Key *ky;
   int dir = 0;

   ev = event_info;
   if (ev->button != 1) return;
   ki = data;

   if (ki->down.zoom)
     {
        ky = _e_kbd_int_at_coord_get(ki, ki->down.clx, ki->down.cly);
	_e_kbd_int_key_press_handle(ki, ky);
	_e_kbd_int_zoomkey_down(ki);
	ki->down.zoom = 0;
     }
   else if (!ki->down.down)
     {
        /* case of mouse up event without mouse down when popup appear */
        Evas_Coord x, y, w, h;
        evas_object_geometry_get(ki->event_obj, &x, &y, &w, &h);
        x = ev->canvas.x - x;
        y = ev->canvas.y - y;
        x = (x * ki->layout.w) / w;
        y = (y * ki->layout.h) / h;
        ky = _e_kbd_int_at_coord_get(ki, x, y);
        ki->layout.pressed = ky;
        _e_kbd_int_key_press_handle(ki, ki->layout.pressed);
        edje_object_signal_emit(ky->obj,
                                "e,state,pressed", "e");
        edje_object_message_signal_process(ky->obj);
     }
   else if (!ki->down.stroke)
     {

        ky = _e_kbd_int_at_coord_get(ki, ki->down.lx, ki->down.ly);
        _e_kbd_int_key_press_handle(ki, ky);
     }
   else
     {
	Evas_Coord dx, dy;

	dx = ev->canvas.x - ki->down.x;
	dy = ev->canvas.y - ki->down.y;
	if (dx > 0)
	  {
	     if (dy > 0)
	       {
		  if (dx > dy) dir = 1; // right
		  else dir = 2; // down
	       }
	     else
	       {
		  if (dx > -dy) dir = 1; // right
		  else dir = 4; // up
	       }
	  }
	else
	  {
	     if (dy > 0)
	       {
		  if (-dx > dy) dir = 3; // left
		  else dir = 2; // down
	       }
	     else
	       {
		  if (-dx > -dy) dir = 3; // left
		  else dir = 4; // up
	       }
	  }
     }

   ky = ki->layout.pressed;
   if (ky)
     {
	ky->pressed = 0;
	edje_object_signal_emit(ky->obj, "e,state,released", "e");
	ki->layout.pressed = NULL;
     }
   ky = ki->zoomkey.pressed;
   if (ky)
     {
	ky->pressed = 0;
	edje_object_signal_emit(ky->obj, "e,state,released", "e");
	ki->zoomkey.pressed = NULL;
     }

   ki->down.down = 0;
   ki->down.stroke = 0;
   if (ki->down.hold_timer)
     {
	ecore_timer_del(ki->down.hold_timer);
	ki->down.hold_timer = NULL;
     }

   if (dir > 0) _e_kbd_int_stroke_handle(ki, dir);
}

static E_Kbd_Int_Layout *
_e_kbd_int_layouts_list_default_get(E_Kbd_Int *ki)
{
   E_Kbd_Int_Layout *kil;
   Eina_List *l;

   EINA_LIST_FOREACH(ki->layouts, l, kil)
     if ((!strcmp(ecore_file_file_get(kil->path), "Default.kbd")))
       return kil;
   return NULL;
}

static E_Kbd_Int_Layout *
_e_kbd_int_layouts_type_get(E_Kbd_Int *ki, int type)
{
   E_Kbd_Int_Layout *kil;
   Eina_List *l;

   EINA_LIST_FOREACH(ki->layouts, l, kil)
     if (kil->type == type) return kil;
   return NULL;
}

static void
_e_kbd_int_layout_free(E_Kbd_Int *ki)
{
   E_Kbd_Int_Key *ky;

   if (ki->layout.directory) free(ki->layout.directory);
   if (ki->layout.file) eina_stringshare_del(ki->layout.file);
   ki->layout.directory = NULL;
   ki->layout.file = NULL;
   EINA_LIST_FREE(ki->layout.keys, ky)
     {
        E_Kbd_Int_Key_State *st;

	EINA_LIST_FREE(ky->states, st)
	  {
	     if (st->label) eina_stringshare_del(st->label);
	     if (st->icon) eina_stringshare_del(st->icon);
	     if (st->out) eina_stringshare_del(st->out);
	     free(st);
	  }
	if (ky->obj) evas_object_del(ky->obj);
	if (ky->icon_obj) evas_object_del(ky->icon_obj);
	free(ky);
     }
   if (ki->event_obj) evas_object_del(ki->event_obj);
   ki->event_obj = NULL;
}

static void
_e_kbd_int_layout_parse(E_Kbd_Int *ki, const char *layout)
{
   FILE *f;
   char buf[PATH_MAX];
   int isok = 0;
   E_Kbd_Int_Key *ky = NULL;
   E_Kbd_Int_Key_State *st = NULL;

   if (!(f = fopen(layout, "r"))) return;

   ki->layout.directory = ecore_file_dir_get(layout);
   ki->layout.file = eina_stringshare_add(layout);

   /* Make the default direction LTR */
   ki->layout.direction = E_KBD_INT_DIRECTION_LTR;

   while (fgets(buf, sizeof(buf), f))
     {
	int len;
	char str[PATH_MAX];

	if (!isok)
	  {
	     if (!strcmp(buf, "##KBDCONF-1.0\n")) isok = 1;
	  }
	if (!isok) break;
	if (buf[0] == '#') continue;
	len = strlen(buf);
	if (len > 0)
	  {
	     if (buf[len - 1] == '\n') buf[len - 1] = 0;
	  }
	if (sscanf(buf, "%4000s", str) != 1) continue;
	if (!strcmp(str, "kbd"))
	  {
	     if (sscanf(buf, "%*s %i %i\n", &(ki->layout.w), &(ki->layout.orig_h)) != 2)
	       continue;
	  }
	if (!strcmp(str, "fuzz"))
	  {
	     sscanf(buf, "%*s %i\n", &(ki->layout.fuzz));
	     continue;
	  }
	if (!strcmp(str, "direction"))
	  {
	     char direction[4];

	     sscanf(buf, "%*s %3s\n", direction);

	     /* If rtl mark as rtl, otherwise make it ltr */
	     if (!strcmp(direction, "rtl"))
               ki->layout.direction = E_KBD_INT_DIRECTION_RTL;
	     else
               ki->layout.direction = E_KBD_INT_DIRECTION_LTR;
	     continue;
	  }
	if (!strcmp(str, "key"))
	  {
	     ky = calloc(1, sizeof(E_Kbd_Int_Key));
	     if (!ky) continue;
	     if (sscanf(buf, "%*s %i %i %i %i\n", &(ky->x), &(ky->orig_y), &(ky->w), &(ky->orig_h)) != 4)
	       {
		  free(ky);
		  ky = NULL;
		  continue;
	       }
	     ki->layout.keys = eina_list_append(ki->layout.keys, ky);
	  }
	if (!ky) continue;
	if ((!strcmp(str, "normal")) || (!strcmp(str, "shift")) ||
	    (!strcmp(str, "capslock")) || (!strcmp(str, "altgr")))
	  {
	     char *p;
	     char label[PATH_MAX];

	     if (sscanf(buf, "%*s %4000s", label) != 1) continue;
	     st = calloc(1, sizeof(E_Kbd_Int_Key_State));
	     if (!st) continue;
	     ky->states = eina_list_append(ky->states, st);
	     if (!strcmp(str, "normal")) st->state = NORMAL;
	     if (!strcmp(str, "shift")) st->state = SHIFT;
	     if (!strcmp(str, "capslock")) st->state = CAPSLOCK;
	     if (!strcmp(str, "altgr")) st->state = ALTGR;
	     p = strrchr(label, '.');
	     if ((p) && (!strcmp(p, ".png")))
	       st->icon = eina_stringshare_add(label);
	     else if ((p) && (!strcmp(p, ".edj")))
	       st->icon = eina_stringshare_add(label);
	     else
	       st->label = eina_stringshare_add(label);
	     if (sscanf(buf, "%*s %*s %4000s", str) != 1) continue;
	     st->out = eina_stringshare_add(str);
	  }
	if (!strcmp(str, "is_shift")) ky->is_shift = 1;
	if (!strcmp(str, "is_multi_shift")) ky->is_multi_shift = 1;
	if (!strcmp(str, "is_ctrl")) ky->is_ctrl = 1;
	if (!strcmp(str, "is_alt")) ky->is_alt = 1;
	if (!strcmp(str, "is_altgr")) ky->is_altgr = 1;
	if (!strcmp(str, "is_capslock")) ky->is_capslock = 1;
     }
   fclose(f);
}

static void
_e_kbd_int_layout_build(E_Kbd_Int *ki)
{
   E_Kbd_Int_Key *ky;
   Evas_Object *o, *o2;
   Evas_Coord lw, lh;
   Eina_List *l;

   ki->layout.h = ki->layout.orig_h * il_kbd_cfg->scale_height;

   evas_event_freeze(ki->win->evas);
   e_layout_virtual_size_set(ki->layout_obj, ki->layout.w, ki->layout.h);
   edje_extern_object_aspect_set(ki->layout_obj, EDJE_ASPECT_CONTROL_BOTH, 
                                 ki->layout.w, ki->layout.h);
   edje_object_part_swallow(ki->base_obj, "e.swallow.content", ki->layout_obj);
   evas_object_resize(ki->base_obj, ki->win->w, ki->win->h);
   edje_object_part_geometry_get(ki->base_obj, "e.swallow.content", 
                                 NULL, NULL, &lw, &lh);
   lh = (ki->layout.h * lw) / ki->layout.w;
   edje_extern_object_min_size_set(ki->layout_obj, lw, lh);
   edje_extern_object_max_size_set(ki->layout_obj, ki->win->w, ki->win->h);
   edje_object_part_swallow(ki->base_obj, "e.swallow.content", ki->layout_obj);

   EINA_LIST_FOREACH(ki->layout.keys, l, ky)
     {
	E_Kbd_Int_Key_State *st;
	const char *label, *icon;

	ky->y = ky->orig_y * il_kbd_cfg->scale_height;
	ky->h = ky->orig_h * il_kbd_cfg->scale_height;

	o = _theme_obj_new(ki->win->evas, ki->themedir,
			   "e/modules/kbd/key/default");
	ky->obj = o;
	label = "";
	icon = NULL;
	st = _e_kbd_int_key_state_get(ki, ky);
	if (st)
	  {
	     label = st->label;
	     icon = st->icon;
	  }

        edje_object_part_text_set(o, "e.text.label", label);

	o2 = e_icon_add(ki->win->evas);
	e_icon_fill_inside_set(o2, 1);
	e_icon_scale_up_set(o2, 0);
	ky->icon_obj = o2;
	edje_object_part_swallow(o, "e.swallow.content", o2);
	evas_object_show(o2);

	if (icon)
	  {
	     char buf[PATH_MAX];
	     char *p;

	     snprintf(buf, sizeof(buf), "%s/%s", ki->layout.directory, icon);
	     p = strrchr(icon, '.');
	     if (!strcmp(p, ".edj"))
	       e_icon_file_edje_set(o2, buf, "icon");
	     else
	       e_icon_file_set(o2, buf);
	  }
	e_layout_pack(ki->layout_obj, o);
	e_layout_child_move(o, ky->x, ky->y);
	e_layout_child_resize(o, ky->w, ky->h);
	evas_object_show(o);
     }

   o = evas_object_rectangle_add(ki->win->evas);
   e_layout_pack(ki->layout_obj, o);
   e_layout_child_move(o, 0, 0);
   e_layout_child_resize(o, ki->layout.w, ki->layout.h);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _e_kbd_int_cb_mouse_down, ki);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, 
                                  _e_kbd_int_cb_mouse_up, ki);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, 
                                  _e_kbd_int_cb_mouse_move, ki);
   evas_object_show(o);
   ki->event_obj = o;
   evas_event_thaw(ki->win->evas);
   _e_kbd_int_matchlist_down(ki);
}

static void
_e_kbd_int_layouts_free(E_Kbd_Int *ki)
{
   E_Kbd_Int_Layout *kil;

   EINA_LIST_FREE(ki->layouts, kil)
     {
	eina_stringshare_del(kil->path);
	eina_stringshare_del(kil->dir);
	eina_stringshare_del(kil->icon);
	eina_stringshare_del(kil->name);
	free(kil);
     }
}

static void
_e_kbd_int_layouts_list_update(E_Kbd_Int *ki)
{
   Eina_List *files;
   Eina_List *l;
   char buf[PATH_MAX], *p, *file, *path;
   const char *fl;
   Eina_List *kbs = NULL, *layouts = NULL;
   int ok;
   size_t len;

   len = e_user_dir_concat_static(buf, "keyboards");
   if (len + 2 >= sizeof(buf)) return;

   files = ecore_file_ls(buf);

   buf[len] = '/';
   len++;

   EINA_LIST_FREE(files, file)
     {
	p = strrchr(file, '.');
	if ((p) && (!strcmp(p, ".kbd")))
	  {
	     if (eina_strlcpy(buf + len, file, sizeof(buf) - len) >= sizeof(buf) - len)
	       continue;
	     kbs = eina_list_append(kbs, eina_stringshare_add(buf));
	  }
	free(file);
     }

   len = snprintf(buf, sizeof(buf), "%s/keyboards", ki->syskbds);
   if (len + 2 >= sizeof(buf)) return;

   files = ecore_file_ls(buf);

   buf[len] = '/';
   len++;

   EINA_LIST_FREE(files, file)
     {
	p = strrchr(file, '.');
	if ((p) && (!strcmp(p, ".kbd")))
	  {
	     ok = 1;
	     EINA_LIST_FOREACH(kbs, l, fl)
	       {
		  if (!strcmp(file, fl))
		    {
		       ok = 0;
		       break;
		    }
	       }
	     if (ok)
	       {
		  if (eina_strlcpy(buf + len, file, sizeof(buf) - len) >= sizeof(buf) - len)
		    continue;
		  kbs = eina_list_append(kbs, eina_stringshare_add(buf));
	       }
	  }
	free(file);
     }
   /* Previous loop could break before destroying all items. */
   EINA_LIST_FREE(files, file)
     free(file);

   EINA_LIST_FREE(kbs, path)
     {
	E_Kbd_Int_Layout *kil;

	kil = E_NEW(E_Kbd_Int_Layout, 1);
	if (kil)
	  {
	     char *s, *p;
	     FILE *f;

	     kil->path = path;
	     s = strdup(ecore_file_file_get(kil->path));
	     if (s)
	       {
		  p = strrchr(s, '.');
		  if (p) *p = 0;
		  kil->name = eina_stringshare_add(s);
		  free(s);
	       }
	     s = ecore_file_dir_get(kil->path);
	     if (s)
	       {
		  kil->dir = eina_stringshare_add(s);
		  free(s);
	       }
	     f = fopen(kil->path, "r");
	     if (f)
	       {
		  int isok = 0;

		  while (fgets(buf, sizeof(buf), f))
		    {
		       int len;
		       char str[4096];

		       if (!isok)
			 {
			    if (!strcmp(buf, "##KBDCONF-1.0\n")) isok = 1;
			 }
		       if (!isok) break;
		       if (buf[0] == '#') continue;
		       len = strlen(buf);
		       if (len > 0)
			 {
			    if (buf[len - 1] == '\n') buf[len - 1] = 0;
			 }
		       if (sscanf(buf, "%4000s", str) != 1) continue;
		       if (!strcmp(str, "type"))
			 {
			    sscanf(buf, "%*s %4000s\n", str);
			    if (!strcmp(str, "ALPHA"))
			      kil->type = E_KBD_INT_TYPE_ALPHA;
			    else if (!strcmp(str, "NUMERIC"))
			      kil->type = E_KBD_INT_TYPE_NUMERIC;
			    else if (!strcmp(str, "PIN"))
			      kil->type = E_KBD_INT_TYPE_PIN;
			    else if (!strcmp(str, "PHONE_NUMBER"))
			      kil->type = E_KBD_INT_TYPE_PHONE_NUMBER;
			    else if (!strcmp(str, "HEX"))
			      kil->type = E_KBD_INT_TYPE_HEX;
			    else if (!strcmp(str, "TERMINAL"))
			      kil->type = E_KBD_INT_TYPE_TERMINAL;
			    else if (!strcmp(str, "PASSWORD"))
			      kil->type = E_KBD_INT_TYPE_PASSWORD;
			    else if (!strcmp(str, "IP"))
			      kil->type = E_KBD_INT_TYPE_IP;
			    else if (!strcmp(str, "HOST"))
			      kil->type = E_KBD_INT_TYPE_HOST;
			    else if (!strcmp(str, "FILE"))
			      kil->type = E_KBD_INT_TYPE_FILE;
			    else if (!strcmp(str, "URL"))
			      kil->type = E_KBD_INT_TYPE_URL;
			    else if (!strcmp(str, "KEYPAD"))
			      kil->type = E_KBD_INT_TYPE_KEYPAD;
			    else if (!strcmp(str, "J2ME"))
			      kil->type = E_KBD_INT_TYPE_J2ME;
			    continue;
			 }
		       if (!strcmp(str, "icon"))
			 {
			    sscanf(buf, "%*s %4000s\n", str);
			    snprintf(buf, sizeof(buf), "%s/%s", kil->dir, str);
			    kil->icon = eina_stringshare_add(buf);
			    continue;
			 }
		    }
		  fclose(f);
	       }
	     layouts = eina_list_append(layouts, kil);
	  }
     }
   _e_kbd_int_layouts_free(ki);
   ki->layouts = layouts;
}

static void
_e_kbd_int_layout_select(E_Kbd_Int *ki, E_Kbd_Int_Layout *kil)
{
   _e_kbd_int_layout_free(ki);
   _e_kbd_int_layout_parse(ki, kil->path);
   _e_kbd_int_layout_build(ki);
   _e_kbd_int_layout_buf_update(ki);
   _e_kbd_int_layout_state_update(ki);

   if (!kil->icon)
     e_icon_file_set(ki->icon_obj, kil->icon);
   else
     {
	const char *p;

	p = strrchr(kil->icon, '.');
	if (!p)
	  e_icon_file_set(ki->icon_obj, kil->icon);
	else
	  {
	     if (!strcmp(p, ".edj"))
	       e_icon_file_edje_set(ki->icon_obj, kil->icon, "icon");
	     else
	       e_icon_file_set(ki->icon_obj, kil->icon);
	  }
     }
}

static void
_e_kbd_int_layout_next(E_Kbd_Int *ki)
{
   Eina_List *l, *ln = NULL;
   E_Kbd_Int_Layout *kil;

   EINA_LIST_FOREACH(ki->layouts, l, kil)
     if (!strcmp(kil->path, ki->layout.file))
       {
          ln = l->next;
          break;
       }
   if (!ln) ln = ki->layouts;
   if (!ln) return;
   kil = ln->data;
   _e_kbd_int_layout_select(ki, kil);
}

static Eina_Bool
_e_kbd_int_cb_client_message(void *data, __UNUSED__ int type, void *event)
{
   Ecore_X_Event_Client_Message *ev;
   E_Kbd_Int *ki;

   ev = event;
   ki = data;
   if ((ev->win == ki->win->evas_win) && 
       (ev->message_type == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE)) 
     {
	E_Kbd_Int_Layout *kil = NULL;

	if (ev->data.l[0] == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_OFF)
	  {
	     _e_kbd_int_zoomkey_down(ki);
	     _e_kbd_int_dictlist_down(ki);
	     _e_kbd_int_matchlist_down(ki);
	  }
	else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_ON)
	  {
	     // do nothing - leave kbd as-is
	  }
	else if (ev->data.l[0] == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_ALPHA)
          kil = _e_kbd_int_layouts_type_get(ki, E_KBD_INT_TYPE_ALPHA);
	else if (ev->data.l[0] == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_NUMERIC)
          kil = _e_kbd_int_layouts_type_get(ki, E_KBD_INT_TYPE_NUMERIC);
	else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_PIN)
          kil = _e_kbd_int_layouts_type_get(ki, E_KBD_INT_TYPE_PIN);
	else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_PHONE_NUMBER)
          kil = _e_kbd_int_layouts_type_get(ki, E_KBD_INT_TYPE_PHONE_NUMBER);
	else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_HEX)
          kil = _e_kbd_int_layouts_type_get(ki, E_KBD_INT_TYPE_HEX);
	else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_TERMINAL)
          kil = _e_kbd_int_layouts_type_get(ki, E_KBD_INT_TYPE_TERMINAL);
	else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_PASSWORD)
          kil = _e_kbd_int_layouts_type_get(ki, E_KBD_INT_TYPE_PASSWORD);
	else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_IP)
          kil = _e_kbd_int_layouts_type_get(ki, E_KBD_INT_TYPE_IP);
	else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_HOST)
          kil = _e_kbd_int_layouts_type_get(ki, E_KBD_INT_TYPE_HOST);
	else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_FILE)
          kil = _e_kbd_int_layouts_type_get(ki, E_KBD_INT_TYPE_FILE);
	else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_URL)
          kil = _e_kbd_int_layouts_type_get(ki, E_KBD_INT_TYPE_URL);
	else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_KEYPAD)
          kil = _e_kbd_int_layouts_type_get(ki, E_KBD_INT_TYPE_KEYPAD);
	else if (ev->data.l[0] == ECORE_X_VIRTUAL_KEYBOARD_STATE_J2ME)
          kil = _e_kbd_int_layouts_type_get(ki, E_KBD_INT_TYPE_J2ME);

        if (kil) _e_kbd_int_layout_select(ki, kil);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_kbd_int_dictlist_down(E_Kbd_Int *ki)
{
   char *str;

   if (!ki->dictlist.popup) return;
   e_object_del(E_OBJECT(ki->dictlist.popup));
   ki->dictlist.popup = NULL;
   EINA_LIST_FREE(ki->dictlist.matches, str)
     eina_stringshare_del(str);
}

static void
_e_kbd_int_cb_dictlist_item_sel(void *data)
{
   E_Kbd_Int *ki;
   const char *str;
   int i;

   ki = data;
   i = e_widget_ilist_selected_get(ki->dictlist.ilist_obj);
   str = eina_list_nth(ki->dictlist.matches, i);
   e_kbd_buf_clear(ki->kbuf);
   if (ki->layout.state & (SHIFT | CTRL | ALT | ALTGR))
     {
	ki->layout.state &= (~(SHIFT | CTRL | ALT | ALTGR));
	_e_kbd_int_layout_state_update(ki);
     }

   eina_stringshare_replace(&il_kbd_cfg->dict, str);
   e_kbd_buf_dict_set(ki->kbuf, il_kbd_cfg->dict);
   e_config_save_queue();

   _e_kbd_int_dictlist_down(ki);
}

static void
_e_kbd_int_dictlist_up(E_Kbd_Int *ki)
{
   Evas_Object *o;
   Evas_Coord mw, mh, sh;
   Eina_List *files, *l;
   char buf[PATH_MAX], *p, *file, *pp;
   const char *str;
   int used;

   if (ki->dictlist.popup) return;

   ki->dictlist.popup = e_popup_new(ki->win->border->zone, -1, -1, 1, 1);
   e_popup_layer_set(ki->dictlist.popup, 190);

   o = _theme_obj_new(ki->dictlist.popup->evas, ki->themedir,
		      "e/modules/kbd/match/default");
   ki->dictlist.base_obj = o;

   o = e_widget_ilist_add(ki->dictlist.popup->evas, 
                          (32 * e_scale), (32 * e_scale), NULL);
   e_widget_ilist_selector_set(o, 1);
   e_widget_ilist_freeze(o);
   ki->dictlist.ilist_obj = o;

   e_user_dir_concat_static(buf, "dicts");
   files = ecore_file_ls(buf);

   EINA_LIST_FREE(files, file)
     {
        p = strrchr(file, '.');
        if ((p) && (!strcmp(p, ".dic")))
          {
             const char *match;

             used = 0;
	     EINA_LIST_FOREACH(ki->dictlist.matches, l, match)
	       if (!strcmp(match, file)) used = 1;

	     if (used) goto next1;

             p = strdup(file);
	     if (!p) goto next1;

             for (pp = p; *pp; pp++)
               {
                  if (*pp == '_') *pp = ' ';
               }
             pp = strrchr(p, '.');
             *pp = 0;
             str = eina_stringshare_add(file);
             ki->dictlist.matches = eina_list_append(ki->dictlist.matches, str);
             e_widget_ilist_append(o, NULL, p, _e_kbd_int_cb_dictlist_item_sel,
                                   ki, NULL);
             free(p);
          }

        next1:
	free(file);
     }

   snprintf(buf, sizeof(buf), "%s/dicts", ki->sysdicts);
   files = ecore_file_ls(buf);
   EINA_LIST_FREE(files, file)
     {
        p = strrchr(file, '.');
        if ((p) && (!strcmp(p, ".dic")))
          {
	     const char *match;

             used = 0;
	     EINA_LIST_FOREACH(ki->dictlist.matches, l, match)
	       if (!strcmp(match, file)) used = 1;

	     if (used) goto next2;

             p = strdup(file);
	     if (!p) goto next2;

             for (pp = p; *pp; pp++)
               {
                  if (*pp == '_') *pp = ' ';
               }
             pp = strrchr(p, '.');
             *pp = 0;
             str = eina_stringshare_add(file);
             ki->dictlist.matches = eina_list_append(ki->dictlist.matches, str);
             e_widget_ilist_append(o, NULL, p, _e_kbd_int_cb_dictlist_item_sel,
                                   ki, NULL);
             free(p);
          }
        next2:
	free(file);
     }
   e_widget_ilist_thaw(o);
   e_widget_ilist_go(o);

   e_widget_ilist_preferred_size_get(o, &mw, &mh);
   if (mh < (120 *e_scale)) mh = (120 * e_scale);

   edje_extern_object_min_size_set(ki->dictlist.ilist_obj, mw, mh);
   edje_object_part_swallow(ki->dictlist.base_obj, "e.swallow.content",
                            ki->dictlist.ilist_obj);
   edje_object_size_min_calc(ki->dictlist.base_obj, &mw, &mh);

   e_zone_useful_geometry_get(ki->win->border->zone, NULL, NULL, NULL, &sh);
   mw = ki->win->w;
   if (mh > (sh - ki->win->h)) mh = (sh - ki->win->h);
   e_popup_move_resize(ki->dictlist.popup, 0, ki->win->y - mh, mw, mh);

   evas_object_resize(ki->dictlist.base_obj, 
		      ki->dictlist.popup->w, ki->dictlist.popup->h);
   evas_object_show(ki->dictlist.base_obj);
   e_popup_edje_bg_object_set(ki->dictlist.popup, ki->dictlist.base_obj);
   e_popup_show(ki->dictlist.popup);
   _e_kbd_int_matchlist_down(ki);
}

static void
_e_kbd_int_matchlist_down(E_Kbd_Int *ki)
{
   char *str;

   if (!ki->matchlist.popup) return;
   e_object_del(E_OBJECT(ki->matchlist.popup));
   ki->matchlist.popup = NULL;
   EINA_LIST_FREE(ki->matchlist.matches, str)
     eina_stringshare_del(str);
}

static void
_e_kbd_int_cb_matchlist_item_sel(void *data)
{
   E_Kbd_Int *ki;
   const char *str;

   ki = data;
   str = e_widget_ilist_selected_label_get(ki->matchlist.ilist_obj);
   _e_kbd_int_string_send(ki, str);
   e_kbd_buf_clear(ki->kbuf);
   e_kbd_send_keysym_press("space", 0);
   if (ki->layout.state & (SHIFT | CTRL | ALT | ALTGR))
     {
	ki->layout.state &= (~(SHIFT | CTRL | ALT | ALTGR));
	_e_kbd_int_layout_state_update(ki);
     }
   _e_kbd_int_matches_update(ki);
   _e_kbd_int_matchlist_down(ki);
}

static void
_e_kbd_int_matchlist_up(E_Kbd_Int *ki)
{
   const Eina_List *l;
   Evas_Object *o;
   Evas_Coord mw, mh, sh;

   if (!e_kbd_buf_string_matches_get(ki->kbuf)) return;
   if (ki->matchlist.popup) return;
   ki->matchlist.popup = e_popup_new(ki->win->border->zone, -1, -1, 1, 1);
   e_popup_layer_set(ki->matchlist.popup, 190);

   o = _theme_obj_new(ki->matchlist.popup->evas, ki->themedir,
		      "e/modules/kbd/match/default");
   ki->matchlist.base_obj = o;

   o = e_widget_ilist_add(ki->matchlist.popup->evas, 
                          (32 * e_scale), (32 * e_scale), NULL);
   e_widget_ilist_selector_set(o, 1);
   ki->matchlist.ilist_obj = o;
   edje_object_part_swallow(ki->matchlist.base_obj, "e.swallow.content", o);
   evas_object_show(o);

   for (l = e_kbd_buf_string_matches_get(ki->kbuf); l; l = l->next)
     {
	const char *str;

	if (!l->prev)
	  {
	     str = e_kbd_buf_actual_string_get(ki->kbuf);
	     if (str)
	       {
		  str = eina_stringshare_add(str);
		  ki->matchlist.matches = eina_list_append(ki->matchlist.matches, str);
		  e_widget_ilist_append(o, NULL, str, 
                                        _e_kbd_int_cb_matchlist_item_sel,
					ki, NULL);
	       }
	  }
	str = l->data;
	str = eina_stringshare_add(str);
	ki->matchlist.matches = eina_list_append(ki->matchlist.matches, str);
	e_widget_ilist_append(o, NULL, str, _e_kbd_int_cb_matchlist_item_sel,
			      ki, NULL);
     }
   e_widget_ilist_thaw(o);
   e_widget_ilist_go(o);

   e_widget_ilist_preferred_size_get(o, &mw, &mh);
   if (mh < (120 *e_scale)) mh = (120 * e_scale);

   edje_extern_object_min_size_set(ki->matchlist.ilist_obj, mw, mh);
   edje_object_part_swallow(ki->matchlist.base_obj, "e.swallow.content",
		   	    ki->matchlist.ilist_obj);
   edje_object_size_min_calc(ki->matchlist.base_obj, &mw, &mh);

   edje_extern_object_min_size_set(ki->matchlist.ilist_obj, 0, 0);
   edje_object_part_swallow(ki->matchlist.base_obj, "e.swallow.content",
		   	    ki->matchlist.ilist_obj);
   e_zone_useful_geometry_get(ki->win->border->zone, NULL, NULL, NULL, &sh);
   mw = ki->win->w;
   if (mh > (sh - ki->win->h)) mh = (sh - ki->win->h);
   e_popup_move_resize(ki->matchlist.popup,
		       ki->win->x, ki->win->y - mh, mw, mh);
   evas_object_resize(ki->matchlist.base_obj, 
		      ki->matchlist.popup->w, ki->matchlist.popup->h);
   evas_object_show(ki->matchlist.base_obj);
   e_popup_edje_bg_object_set(ki->matchlist.popup, ki->matchlist.base_obj);
   e_popup_show(ki->matchlist.popup);

   _e_kbd_int_dictlist_down(ki);
}

static void
_e_kbd_int_cb_matches(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Kbd_Int *ki;

   ki = data;
   if (ki->dictlist.popup) _e_kbd_int_dictlist_down(ki);
   else if (ki->matchlist.popup) _e_kbd_int_matchlist_down(ki);
   else
     {
	if (!e_kbd_buf_actual_string_get(ki->kbuf))
	  _e_kbd_int_dictlist_up(ki);
	else
	  _e_kbd_int_matchlist_up(ki);
     }
}

static void
_e_kbd_int_cb_dicts(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Kbd_Int *ki;

   ki = data;
   if (ki->dictlist.popup) _e_kbd_int_dictlist_down(ki);
   else _e_kbd_int_dictlist_up(ki);
}

static void
_e_kbd_int_cb_layouts(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Kbd_Int *ki;

   ki = data;
   _e_kbd_int_layout_next(ki);
}

EAPI E_Kbd_Int *
e_kbd_int_new(const char *themedir, const char *syskbds, const char *sysdicts)
{
   E_Kbd_Int *ki;
   Evas_Object *o;
   Evas_Coord mw, mh;
   E_Zone *zone;
   E_Kbd_Int_Layout *kil;
   Ecore_X_Window_State states[2];

   ki = E_NEW(E_Kbd_Int, 1);
   if (!ki) return NULL;
   if (themedir) ki->themedir = eina_stringshare_add(themedir);
   if (syskbds) ki->syskbds = eina_stringshare_add(syskbds);
   if (sysdicts) ki->sysdicts = eina_stringshare_add(sysdicts);
   zone = e_util_zone_current_get(e_manager_current_get());
   ki->win = e_win_new(zone->container);
   states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
   states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
   ecore_x_netwm_window_state_set(ki->win->evas_win, states, 2);
   ecore_x_icccm_hints_set(ki->win->evas_win, 0, 0, 0, 0, 0, 0, 0);
   e_win_no_remember_set(ki->win, 1);

   if (zone->w > zone->h)
     e_win_resize(ki->win, zone->w, (zone->h / 2));
   else
     e_win_resize(ki->win, zone->w, zone->h);

   e_win_resize_callback_set(ki->win, _e_kbd_int_cb_resize);
   e_win_borderless_set(ki->win, 1);
   ki->win->data = ki;
   e_win_name_class_set(ki->win, "Virtual-Keyboard", "Virtual-Keyboard");
   e_win_title_set(ki->win, "Virtual Keyboard");

   ki->base_obj = _theme_obj_new(ki->win->evas, ki->themedir,
				 "e/modules/kbd/base/default");
   edje_object_signal_callback_add(ki->base_obj, "e,action,do,matches", "",
				   _e_kbd_int_cb_matches, ki);
   edje_object_signal_callback_add(ki->base_obj, "e,action,do,layouts", "",
				   _e_kbd_int_cb_layouts, ki);
   edje_object_signal_callback_add(ki->base_obj, "e,action,do,dicts", "",
				   _e_kbd_int_cb_dicts, ki);

   o = e_layout_add(ki->win->evas);
   edje_object_part_swallow(ki->base_obj, "e.swallow.content", o);
   evas_object_show(o);
   ki->layout_obj = o;

   o = e_icon_add(ki->win->evas);
   evas_object_pass_events_set(o, 1);
   e_icon_fill_inside_set(o, 1);
   e_icon_scale_up_set(o, 0);
   edje_object_part_swallow(ki->base_obj, "e.swallow.layout", o);
   evas_object_show(o);
   ki->icon_obj = o;

   o = e_box_add(ki->win->evas);   
   e_box_orientation_set(o, 1);
   e_box_homogenous_set(o, 1);
   edje_object_part_swallow(ki->base_obj, "e.swallow.label", o);
   evas_object_show(o);
   ki->box_obj = o;

   if (il_kbd_cfg->dict)
     ki->kbuf = e_kbd_buf_new(ki->sysdicts, il_kbd_cfg->dict);
   else
     ki->kbuf = e_kbd_buf_new(ki->sysdicts, "English_(US).dic");

   _e_kbd_int_layouts_list_update(ki);

   kil = _e_kbd_int_layouts_list_default_get(ki);
   if ((!kil) && (ki->layouts))
     kil = ki->layouts->data;

   if (kil) _e_kbd_int_layout_select(ki, kil);

   edje_object_size_min_calc(ki->base_obj, &mw, &mh);
   if (mw < 48) mw = 48;
   if (mh < 48) mh = 48;
   evas_object_move(ki->base_obj, 0, 0);
   evas_object_resize(ki->base_obj, mw, mh);
   evas_object_show(ki->base_obj);

   e_win_size_min_set(ki->win, zone->w, mh);
   ecore_x_e_virtual_keyboard_set(ki->win->evas_win, 1);

   ki->client_message_handler = 
     ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, 
                             _e_kbd_int_cb_client_message, ki);
   ki->kbd_move_hdl = 
     ecore_event_handler_add(E_EVENT_BORDER_MOVE, 
                             _e_kbd_int_cb_border_move, ki);

   e_win_show(ki->win);
   ki->win->border->user_skip_winlist = 1;

   return ki;
}

EAPI void
e_kbd_int_free(E_Kbd_Int *ki)
{
   if (ki->themedir) eina_stringshare_del(ki->themedir);
   if (ki->syskbds) eina_stringshare_del(ki->syskbds);
   if (ki->sysdicts) eina_stringshare_del(ki->sysdicts);
   _e_kbd_int_layouts_free(ki);
   _e_kbd_int_matches_free(ki);
   _e_kbd_int_layout_free(ki);
   ecore_event_handler_del(ki->client_message_handler);
   ecore_event_handler_del(ki->kbd_move_hdl);
   if (ki->down.hold_timer) ecore_timer_del(ki->down.hold_timer);
   _e_kbd_int_matchlist_down(ki);
   _e_kbd_int_zoomkey_down(ki);
   e_kbd_buf_free(ki->kbuf);
   e_object_del(E_OBJECT(ki->win));
   E_FREE(ki);
}

static Evas_Object *
_theme_obj_new(Evas *e, const char *custom_dir, const char *group)
{
   Evas_Object *o;

   o = edje_object_add(e);
   if (!e_theme_edje_object_set(o, "base/theme/modules/illume", group))
     {
	if (custom_dir)
	  {
	     char buf[PATH_MAX];

	     snprintf(buf, sizeof(buf), "%s/e-module-illume-keyboard.edj", 
                      custom_dir);
	     edje_object_file_set(o, buf, group);
	  }
     }
   return o;
}

static Eina_Bool
_e_kbd_int_cb_border_move(void *data, int type __UNUSED__, void *event) 
{
   E_Event_Border_Move *ev;
   E_Kbd_Int *ki;

   ev = event;
   if (!(ki = data)) return ECORE_CALLBACK_PASS_ON;
   if (ki->win->border != ev->border) return ECORE_CALLBACK_PASS_ON;
   _e_kbd_int_zoomkey_down(ki);
   _e_kbd_int_matchlist_down(ki);
   _e_kbd_int_dictlist_down(ki);
   return ECORE_CALLBACK_PASS_ON;
}
