#include "e.h"
#include "e_mod_main.h"

/* TODO
 * - keybinding configuration
 */

#define DRAG_OFF_WINDOW

#define INPUTLEN 256
#define MATCH_LAG 0.15
#define INITIAL_MATCH_LAG 0.3

#if defined (CHECK_TIME) || defined (CHECK_REFS)
#undef DBG
#define DBG(...) ERR(__VA_ARGS__)
#else
#undef DBG
#define DBG(...)
#endif

static void _evry_matches_update(Evry_Selector *sel, int async);
static void _evry_plugin_action(Evry_Selector *sel, int finished);
static void _evry_plugin_select(Evry_State *s, Evry_Plugin *p);
static void _evry_plugin_list_insert(Evry_State *s, Evry_Plugin *p);
static int  _evry_backspace(Evry_Selector *sel);
static void _evry_update(Evry_Selector *sel, int fetch);
static void _evry_update_text_label(Evry_State *s);
static int  _evry_clear(Evry_Selector *sel);
static Eina_Bool  _evry_cb_update_timer(void *data);

static Evry_State *_evry_state_new(Evry_Selector *sel, Eina_List *plugins);
static void _evry_state_pop(Evry_Selector *sel, int immediate);
static void _evry_state_clear(Evry_Window *win);
static void _evry_hide_func(Evry_Window *win, int finished);

static Evry_Selector *_evry_selector_new(Evry_Window *win, int type);
static void _evry_selector_free(Evry_Selector *sel);
static void _evry_selector_activate(Evry_Selector *sel, int slide);
static void _evry_selector_update(Evry_Selector *sel);
static int  _evry_selector_plugins_get(Evry_Selector *sel, Evry_Item *it, const char *plugin_name);
static void _evry_selector_update_actions(Evry_Selector *sel);
static void _evry_selector_item_update(Evry_Selector *sel);
static void _evry_selector_item_clear(Evry_Selector *sel);
static void _evry_selector_label_set(Evry_Selector *sel, const char *part, const char *label);
static void _evry_selector_signal_emit(Evry_Selector *sel, const char *msg);
static int  _evry_selectors_shift(Evry_Window *win, int dir);
static int  _evry_selectors_switch(Evry_Window *win, int dir);

static Evry_Window *_evry_window_new(E_Zone *zone, E_Zone_Edge edge);
static void _evry_window_free(Evry_Window *win);
static void _evry_list_win_show(Evry_Window *win);
static void _evry_list_win_hide(Evry_Window *win);
static void _evry_list_win_update(Evry_State *s);

static int  _evry_view_create(Evry_State *s);
static void _evry_view_clear(Evry_State *s);
static int  _evry_view_update(Evry_Window *win, Evry_State *s);
static int  _evry_view_key_press(Evry_State *s, Ecore_Event_Key *ev);
static void _evry_view_show(Evry_Window *win, Evry_View *v, int slide);
static void _evry_view_hide(Evry_Window *win, Evry_View *v, int slide);


static void _evry_item_desel(Evry_State *s);
static void _evry_item_sel(Evry_State *s, Evry_Item *it);

static Eina_Bool  _evry_cb_key_down(void *data, int type, void *event);
static Eina_Bool  _evry_cb_selection_notify(void *data, int type, void *event);
static Eina_Bool  _evry_cb_mouse(void *data, int type, void *event);

static Eina_List *windows = NULL;

#ifdef CHECK_TIME
double _evry_time;
#endif

#ifdef CHECK_REFS
static int _item_cnt = 0;
Eina_List *_refd = NULL;
#endif

#define SUBJ_SEL win->selectors[0]
#define ACTN_SEL win->selectors[1]
#define OBJ_SEL  win->selectors[2]
#define CUR_SEL  win->selector

int
evry_init(void)
{
   return 1;
}

int
evry_shutdown(void)
{
   while(windows)
     evry_hide(windows->data, 0);

   return 1;
}

static int
_evry_aggregator_fetch(Evry_State *s)
{
   Eina_List *l;

   if (!s)
     {
	ERR("no state");
   	return 0;
     }
   
   if (s->aggregator->fetch(s->aggregator, s->input))
     {
	l = eina_list_data_find_list(s->cur_plugins, s->aggregator);

	if (!l)
	  s->cur_plugins = eina_list_prepend(s->cur_plugins, s->aggregator);
	else
	  s->cur_plugins = eina_list_promote_list(s->cur_plugins, l);

	return 1;
     }

   s->cur_plugins = eina_list_remove(s->cur_plugins, s->aggregator);
   
   return 1;
}

static Eina_Bool
_evry_cb_item_changed(__UNUSED__ void *data, __UNUSED__ int type, void *event)
{
   Evry_Event_Item_Changed *ev = event;
   Evry_Selector *sel;
   Evry_Item *it = ev->item;

   if (!it || !it->plugin || !it->plugin->state)
     return ECORE_CALLBACK_PASS_ON;

   sel = it->plugin->state->selector;

   if (sel->state && sel->state->cur_item == it)
     {
	_evry_selector_update(sel);
     }

   return ECORE_CALLBACK_PASS_ON;
}

Evry_Window *
evry_show(E_Zone *zone, E_Zone_Edge edge, const char *params, Eina_Bool popup)
{
   Evry_Window *win;
   Evry_Selector *sel;

#ifdef CHECK_TIME
   _evry_time = ecore_time_get();
   DBG("_____evry_show______");
#endif

   E_OBJECT_CHECK_RETURN(zone, 0);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, 0);

   if (!(win = _evry_window_new(zone, edge)))
     return NULL;

   if (popup)
     {
	e_win_layer_set(win->ewin, 255);
	ecore_x_netwm_window_type_set(win->ewin->evas_win,
				      ECORE_X_WINDOW_TYPE_UTILITY);

	ecore_evas_name_class_set(win->ewin->ecore_evas, "E", "everything");

	ecore_evas_show(win->ewin->ecore_evas);

	if (e_grabinput_get(win->ewin->evas_win, 0, win->ewin->evas_win))
	  win->grab = 1;
	else
	  ERR("could not acquire grab");
     }

   evry_history_load();

   if (params)
     win->plugin_dedicated = EINA_TRUE;

   win->sel_list = E_NEW(Evry_Selector*, 4);
   win->sel_list[3] = NULL;
   win->selectors = win->sel_list;
   _evry_selector_new(win, EVRY_PLUGIN_SUBJECT);
   _evry_selector_new(win, EVRY_PLUGIN_ACTION);
   _evry_selector_new(win, EVRY_PLUGIN_OBJECT);

   win->handlers = eina_list_append
     (win->handlers, ecore_event_handler_add
      (ECORE_EVENT_KEY_DOWN,
       _evry_cb_key_down, win));

   win->handlers = eina_list_append
     (win->handlers, ecore_event_handler_add
      (ECORE_X_EVENT_SELECTION_NOTIFY,
       _evry_cb_selection_notify, win));

   win->handlers = eina_list_append
     (win->handlers, evry_event_handler_add
      (EVRY_EVENT_ITEM_CHANGED,
       _evry_cb_item_changed, win));

   win->handlers = eina_list_append
     (win->handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_BUTTON_DOWN,
       _evry_cb_mouse, win));

   win->handlers = eina_list_append
     (win->handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_BUTTON_UP,
       _evry_cb_mouse, win));
#if 0
   win->handlers = eina_list_append
     (win->handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_MOVE,
       _evry_cb_mouse, win));

   win->handlers = eina_list_append
     (win->handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_WHEEL,
       _evry_cb_mouse, win));

   win->handlers = eina_list_append
     (win->handlers, ecore_event_handler_add
      (ECORE_X_EVENT_MOUSE_IN,
       _evry_cb_mouse_in, win));

   win->handlers = eina_list_append
     (win->handlers, ecore_event_handler_add
      (ECORE_X_EVENT_MOUSE_OUT,
       _evry_cb_mouse_out, win));
#endif
   _evry_selector_plugins_get(SUBJ_SEL, NULL, params);
   _evry_selector_update(SUBJ_SEL);

   windows = eina_list_append(windows, win);

   if (!evry_conf->hide_list || edge)
     {
	_evry_selector_activate(SUBJ_SEL, 0);
	sel = CUR_SEL;

	if (sel && sel->state && evry_conf->views)
	  {
	     if (evry_conf->first_run)
	       {
		  evry_view_toggle(sel->state, "?");
		  evry_conf->first_run = EINA_FALSE;
	       }

	     edje_object_signal_emit(win->o_main, "list:e,state,list_show", "e");
	     edje_object_signal_emit(win->o_main, "list:e,state,entry_show", "e");
	     win->visible = EINA_TRUE;
	  }
     }
   else
     {
	_evry_selector_activate(SUBJ_SEL, 0);
     }

   win->func.hide = &_evry_hide_func;

   return win;
}

static void
_evry_hide_func(Evry_Window *win, int finished)
{
   if ((finished) && (win->grab)) 
     evry_hide(win, 0);
}

void
evry_hide(Evry_Window *win, int clear)
{
   Ecore_Event_Handler *ev;
   int i;

   if (!win) return;

   _evry_state_clear(win);

   if ((clear && CUR_SEL) &&
       /* pop states */
       ((eina_list_count((SUBJ_SEL)->states) > 1) ||
	/* move to subject selector */
	(CUR_SEL != SUBJ_SEL) ||
	/* clear input */
	(((CUR_SEL)->state) && ((CUR_SEL)->state->input[0]))))
     {
	int slide = 0;
	Evry_Selector *sel;
	Evry_State *s;

	if (CUR_SEL != SUBJ_SEL)
	  {
	     if (CUR_SEL == ACTN_SEL)
	       _evry_selectors_switch(win, -1);
	     else if (CUR_SEL == OBJ_SEL)
	       _evry_selectors_switch(win, 1);
	  }

	/* just to be sure */
	CUR_SEL = SUBJ_SEL;

	while ((CUR_SEL)->states->next)
	  {
	     slide = SLIDE_RIGHT;
	     _evry_state_pop(CUR_SEL, 1);
	  }

	sel = CUR_SEL;
	s = sel->state;
	_evry_clear(sel);
	_evry_clear(sel);

	_evry_aggregator_fetch(s);
	_evry_selector_update(sel);
	_evry_update_text_label(s);
	if (s->view)
	  {
	     _evry_view_show(win, s->view, slide);
	     s->view->update(s->view);
	  }	
	return;
     }

   if (_evry_selectors_shift(win, -1))
     return;

   win->visible = EINA_FALSE;

   for (i = 0; win->sel_list[i]; i++)
     _evry_selector_free(win->sel_list[i]);

   E_FREE(win->sel_list);

   EINA_LIST_FREE(win->handlers, ev)
     ecore_event_handler_del(ev);

   if (win->grab)
     e_grabinput_release(win->ewin->evas_win,
			 win->ewin->evas_win);

   windows = eina_list_remove(windows, win);

   _evry_window_free(win);

   evry_history_unload();

#ifdef CHECK_REFS
   Evry_Item *_it;
   int _cnt = eina_list_count(_refd);
   Eina_List *_l;
   printf("__________________________");
   EINA_LIST_FOREACH(_refd, _l, _it)
     printf("%d %s\n", _it->ref, _it->label);
   printf("sum: %d", _cnt);
#endif
}

static int
_evry_selectors_shift(Evry_Window *win, int dir)
{
   if ((dir > 0) && (win->level == 0))
     {
	void *new_sel;
	Evry_Selector *sel;
	Evry_State *s;
	int i;

	for (i = 1; i < 3; i++)
	  _evry_selector_item_clear(win->selectors[i]);

	if (!(new_sel = realloc(win->sel_list, sizeof(Evry_Selector*) * 6)))
	  return 0;

	win->sel_list = new_sel;

	edje_object_signal_emit(win->o_main,
				"e,state,object_selector_hide", "e");

	win->sel_list[5] = NULL;
	win->selectors = win->sel_list + 2;
	_evry_selector_new(win, EVRY_PLUGIN_ACTION);
	_evry_selector_new(win, EVRY_PLUGIN_OBJECT);

	CUR_SEL = SUBJ_SEL;
	sel = CUR_SEL;

	_evry_selector_signal_emit(sel, "e,state,selected");

	_evry_selector_item_update(SUBJ_SEL);
	_evry_selector_item_update(ACTN_SEL);
	_evry_selector_item_update(OBJ_SEL);

	/* was checked before. anyway */
	if ((s = sel->state) && (s->cur_item))
	  _evry_selector_update_actions(sel);

	win->level++;

	return 1;
     }
   else if ((dir < 0) && (win->level > 0))
     {
	_evry_selector_item_clear(SUBJ_SEL);
	_evry_selector_free(ACTN_SEL);
	_evry_selector_free(OBJ_SEL);

	win->selectors = win->sel_list;
	win->sel_list[3] = NULL;
	CUR_SEL = NULL;

	edje_object_signal_emit(win->o_main,
				"e,state,object_selector_show", "e");
	_evry_selector_item_update(SUBJ_SEL);
	_evry_selector_item_update(ACTN_SEL);
	_evry_selector_item_update(OBJ_SEL);
	_evry_selector_activate(OBJ_SEL, 0);

	win->level = 0;

	return 1;
     }

   return 0;
}

Evry_Item *
evry_item_new(Evry_Item *it, Evry_Plugin *p, const char *label,
	      Evas_Object *(*icon_get) (Evry_Item *it, Evas *e),
	      void (*cb_free) (Evry_Item *item))
{
   if (!it)
     {
	it = E_NEW(Evry_Item, 1);
	if (!it) return NULL;
     }

   if (p && EVRY_ITEM(p)->subtype)
     it->type = EVRY_ITEM(p)->subtype;

   it->plugin = p;

   if (label) it->label = eina_stringshare_add(label);
   it->free = cb_free;
   it->icon_get = icon_get;

   it->ref = 1;

#ifdef CHECK_REFS
   _item_cnt++;
   _refd = eina_list_append(_refd, it);
#endif
#ifdef PRINT_REFS
   printf("%d, %d\t new : %s\n", it->ref, _item_cnt, it->label);
#endif
   return it;
}

void
evry_item_free(Evry_Item *it)
{
   if (!it) return;

   it->ref--;

#ifdef PRINT_REFS
    printf("%d, %d\t unref: %s\n", it->ref, _item_cnt, it->label);
#endif

   if (it->ref > 0) return;

#ifdef CHECK_REFS
   _item_cnt--;
   _refd = eina_list_remove(_refd, it);
#endif

   IF_RELEASE(it->label);
   IF_RELEASE(it->id);
   IF_RELEASE(it->context);
   IF_RELEASE(it->detail);
   IF_RELEASE(it->icon);

   if (it->free)
     it->free(it);
   else
     E_FREE(it);
}

void
evry_item_ref(Evry_Item *it)
{
   it->ref++;
#ifdef PRINT_REFS
   printf("%d, %d\t ref : %s\n", it->ref, _item_cnt, it->label);
#endif

}

static int
_evry_selector_update_actions_do(Evry_Selector *sel)
{
   Evry_State *s;
   Evry_Window *win = sel->win;

   if (sel->action_timer)
     {
	ecore_timer_del(sel->action_timer);
	sel->action_timer = NULL;
     }

   if ((s = (SUBJ_SEL)->state))
     _evry_selector_plugins_get(sel, s->cur_item, NULL);

   _evry_selector_update(sel);

   return 1;
}

static Eina_Bool
_evry_timer_cb_actions_get(void *data)
{
   Evry_Selector *sel = data;
   Evry_Window *win = sel->win;
   Evry_State *s;

   _evry_selector_update_actions_do(sel);

   if ((CUR_SEL == sel) && (s = sel->state))
     {
	if (_evry_view_update(win, s))
	  _evry_view_show(win, s->view, 0);
     }

   return ECORE_CALLBACK_CANCEL;
}

static void
_evry_selector_update_actions(Evry_Selector *sel)
{
   if (sel->action_timer)
     ecore_timer_del(sel->action_timer);

   _evry_selector_item_clear(sel);
   sel->action_timer = ecore_timer_add(0.2, _evry_timer_cb_actions_get, sel);
}

void
evry_item_select(const Evry_State *state, Evry_Item *it)
{
   Evry_State *s = (Evry_State *)state;
   Evry_Selector *sel;
   Evry_Window *win;

   if ((!s) || (s->delete_me))
     return;

   sel = s->selector;
   win = sel->win;

   s->plugin_auto_selected = EINA_FALSE;
   s->item_auto_selected = EINA_FALSE;

   _evry_item_sel(s, it);

   if (s == sel->state)
     {
	_evry_selector_update(sel);

	if (CUR_SEL == SUBJ_SEL)
	  _evry_selector_update_actions(ACTN_SEL);

	if (CUR_SEL == ACTN_SEL)
	  {
	     while (OBJ_SEL->state)
	       _evry_state_pop(OBJ_SEL, 1);
	  }
     }
}

void
evry_item_mark(const Evry_State *state, Evry_Item *it, Eina_Bool mark)
{
   Evry_State *s = (Evry_State *)state;

   if ((!s) || (s->delete_me))
     return;

   if (mark && !it->marked)
     {
	it->marked = EINA_TRUE;
	s->sel_items = eina_list_append(s->sel_items, it);
     }
   else if (it->marked)
     {
	it->marked = EINA_FALSE;
	s->sel_items = eina_list_remove(s->sel_items, it);
     }
}

void
evry_plugin_update(Evry_Plugin *p, int action)
{
   Evry_State *s;
   Evry_Selector *sel;
   Evry_Window *win;

   if (!(s = p->state) || (s->delete_me))
     return;

   if (!(sel = s->selector))
     return;

   if (!(win = sel->win))
     return;

   if (sel->state->request != p->request)
     return;

   DBG("update %d %d %s", s->request, p->request, p->name);

   if (action == EVRY_UPDATE_ADD)
     {
	/* clear marked items */
	if (s->sel_items)
	  {
	     eina_list_free(s->sel_items);
	     s->sel_items = NULL;
	  }

	if ((!p->items) && (!s->trigger_active))
	  {
	     /* remove plugin */
	     if (!(eina_list_data_find(s->cur_plugins, p)))
	       return;

	     s->cur_plugins = eina_list_remove(s->cur_plugins, p);

	     if (p == s->plugin)
	       _evry_plugin_select(s, NULL);
	  }
	else
	  {
	     /* add plugin to current plugins*/
	     _evry_plugin_list_insert(s, p);
	  }

	if (sel->state == s)
	  {
	     _evry_aggregator_fetch(s);

	     /* select first plugin */
	     if ((!s->plugin) || (s->plugin_auto_selected) ||
		 !(eina_list_data_find(s->cur_plugins, s->plugin)))
	       {
		  _evry_plugin_select(s, NULL);
	       }

	     if ((s->plugin && (sel->state == s)) &&
		 /* plugin is current */
		 ((s->plugin == p) ||
		  /* plugin items are shown in aggregator */
		  ((s->plugin == s->aggregator) && p->config->aggregate)))
	       {
		  _evry_selector_update(sel);

		  if (_evry_view_update(win, s))
		    /* XXX when there is a case where view disappears
		       check this one! is swallow always visible ? */
		    _evry_view_show(win, s->view, 0);
	       }

	     /* switch back to subject selector when no current items */
	     if ((sel == SUBJ_SEL) &&
		 (!(s->plugin) || !(s->plugin->items)) &&
		 (CUR_SEL == ACTN_SEL))
	       {
		  _evry_selectors_switch(win, -1);
		  _evry_clear(SUBJ_SEL);
	       }
	  }
	
     }
   else if (action == EVRY_UPDATE_REFRESH)
     {
	_evry_view_clear(s);
	_evry_view_update(win, s);
     }
}

static void
_evry_list_win_show(Evry_Window *win)
{
   if (win->visible) return;

   win->visible = EINA_TRUE;
   _evry_list_win_update((CUR_SEL)->state);

   edje_object_signal_emit(win->o_main, "list:e,state,list_show", "e");
   edje_object_signal_emit(win->o_main, "list:e,state,entry_show", "e");
}

static void
_evry_list_win_hide(Evry_Window *win)
{
   Evry_Selector *sel = CUR_SEL;

   if (!win->visible)
     return;

   if (!evry_conf->hide_list)
     return;

   if (sel->state)
     _evry_view_clear(sel->state);

   win->visible = EINA_FALSE;
   edje_object_signal_emit(win->o_main, "list:e,state,list_hide", "e");

   if (evry_conf->hide_input && (!(sel->state) || (sel->state->input[0])))
     edje_object_signal_emit(win->o_main, "list:e,state,entry_hide", "e");
}

static void
_evry_cb_win_delete(E_Win *ewin)
{
   Evry_Window *win = ewin->data;
   evry_hide(win, 0);
}

static void
_evry_cb_win_move(E_Win *ewin  __UNUSED__)
{
   /* Evry_Window *win = ewin->data; */
   /* evas_object_resize(win->o_main, ewin->w, ewin->h); */
   /* if (win->input_window)
    *   ecore_x_window_move(win->input_window, win->ewin->x, win->ewin->y); */
}

static void
_evry_cb_win_resize(E_Win *ewin)
{
   Evry_Window *win = ewin->data;
   evas_object_resize(win->o_main, ewin->w, ewin->h);
}

static Evry_Window *
_evry_window_new(E_Zone *zone, E_Zone_Edge edge)
{
   int x, y, mw, mh, h, w;
   Evry_Window *win;
   Evas_Object *o;
   const char *tmp;
   int offset_s = 0;
   const char *shape_option;

   win = E_NEW(Evry_Window, 1);
   win->ewin = e_win_new(zone->container);
   e_win_borderless_set(win->ewin, 1);
   e_win_no_remember_set(win->ewin, 1);
   e_win_placed_set(win->ewin, 1);
   ecore_evas_override_set(win->ewin->ecore_evas, 1);
   win->evas = e_win_evas_get(win->ewin);
   win->zone = zone;
   win->ewin->data = win;

   o = edje_object_add(win->evas);
   win->o_main = o;
   e_theme_edje_object_set(o, "base/theme/modules/everything",
			   "e/modules/everything/main");

   if ((shape_option = edje_object_data_get(o, "shaped")) &&
       (!strcmp(shape_option, "1")))
     {
	win->shaped = EINA_TRUE;

	if (e_config->use_composite)
	  {
	     ecore_evas_alpha_set(win->ewin->ecore_evas, 1);
	     win->ewin->evas_win = ecore_evas_software_x11_window_get(win->ewin->ecore_evas);
	     edje_object_signal_emit(o, "e,state,composited", "e");
	     edje_object_signal_emit(o, "list:e,state,composited", "e");
	     edje_object_message_signal_process(o);
	     edje_object_calc_force(o);

	     tmp = edje_object_data_get(o, "shadow_offset");
	     offset_s = tmp ? atoi(tmp) : 0;
	  }
	else
	  ecore_evas_shaped_set(win->ewin->ecore_evas, 1);
     }

   edje_object_size_min_calc(o, &mw, &mh);

   if (edge == E_ZONE_EDGE_NONE)
     {
	w = evry_conf->width;
	h = evry_conf->height;
     }
   else
     {
	w = evry_conf->edge_width;
	h = evry_conf->edge_height;
     }
   e_win_size_min_set(win->ewin, mw, mh);

   evry_conf->min_w = mw;
   if (w > mw) mw = w;

   evry_conf->min_h = mh;
   if (h > mh) mh = h;

   if (edge == E_ZONE_EDGE_NONE)
     {
	mw += offset_s*2;
	mh += offset_s*2;

	x = (zone->x + (zone->w * evry_conf->rel_x)) - (mw / 2);
	y = (zone->y + (zone->h * evry_conf->rel_y)) - (mh / 2);
     }
   else
     {
	int to_side = 0;

	switch (edge)
	  {
	   case E_ZONE_EDGE_TOP_LEFT:
	      x = to_side - offset_s;
	      y = to_side - offset_s;
	      break;
	   case E_ZONE_EDGE_TOP_RIGHT:
	      x = zone->w - (mw + offset_s + to_side);
	      y = to_side - offset_s;
	      break;
	   case E_ZONE_EDGE_BOTTOM_RIGHT:
	      x = zone->w - (mw + offset_s + to_side);
	      y = zone->h - (mh + offset_s + to_side);
	      break;
	   case E_ZONE_EDGE_BOTTOM_LEFT:
	      x = to_side - offset_s;
	      y = zone->h - (mh + offset_s + to_side);
	      break;

	   default:
	      mw += offset_s*2;
	      mh += offset_s*2;
	      x = (zone->w * evry_conf->rel_x) - (mw / 2);
	      y = (zone->h * evry_conf->rel_y) - (mh / 2);
	      break;
	  }

	x += zone->x;
	y += zone->y;

	mw += offset_s*2;
	mh += offset_s*2;
     }

   e_win_move_resize(win->ewin, x, y, mw, mh);
   win->ewin->w = mw;
   win->ewin->h = mh;

   o = win->o_main;
   evas_object_move(o, 0, 0);
   evas_object_resize(o, mw, mh);
   evas_object_show(o);

   evas_event_feed_mouse_in(win->evas, ecore_x_current_time_get(), NULL);
   evas_event_feed_mouse_move(win->evas, -1000000, -1000000,
			      ecore_x_current_time_get(), NULL);

   e_win_delete_callback_set(win->ewin, _evry_cb_win_delete);
   e_win_resize_callback_set(win->ewin, _evry_cb_win_resize);
   e_win_move_callback_set(win->ewin, _evry_cb_win_move);

   return win;
}

#if 0
static void
_evry_cb_drag_finished(E_Drag *drag, int dropped)
{
   E_FREE(drag->data);
}
#endif

static Eina_Bool
_evry_cb_mouse(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Button *ev;
   Evry_Window *win = data;
   E_Win *w;

   ev = event;

   if (!win->grab)
     return ECORE_CALLBACK_PASS_ON;

   if (ev->event_window != win->ewin->evas_win)
     return ECORE_CALLBACK_PASS_ON;

   w = win->ewin;

#if 0
   if (type == ECORE_EVENT_MOUSE_MOVE)
     {
	Ecore_Event_Mouse_Move *ev = event;
	Evry_State *s;

	if ((win->mouse_button == 3) &&
	    (s = (CUR_SEL)->state) && (s->cur_item) &&
	    (CHECK_TYPE(s->cur_item, EVRY_TYPE_FILE)) &&
	    (!E_INSIDE(ev->x, ev->y,
		       win->zone->x,
		       win->zone->y,
		       w->w, w->h)))
	  {
	     const char *drag_types[] = { "text/uri-list" };
	     E_Drag *d;
	     Evas_Object *o;
	     const char *uri;
	     int s_len, sel_length = 0;
	     char *tmp, *sel = NULL;

	     GET_FILE(file, s->cur_item);

	     if (!(uri = evry_file_url_get(file)))
	       return ECORE_CALLBACK_PASS_ON;

	     s_len = strlen(uri);
	     if (!(tmp = realloc(sel, sel_length + s_len + 2 + 1)))
	       return ECORE_CALLBACK_PASS_ON;
	     sel = tmp;
	     memcpy(sel + sel_length, uri, s_len);
	     memcpy(sel + sel_length + s_len, "\r\n", 2);
	     sel_length += s_len + 2;

	     d = e_drag_new(e_container_current_get(e_manager_current_get()),
			    ev->x, ev->y,
			    drag_types, 1, sel, sel_length, NULL,
			    _evry_cb_drag_finished);
	     e_drag_resize(d, 128, 128);
	     o = evry_util_icon_get(s->cur_item, e_drag_evas_get(d));
	     e_drag_object_set(d, o);
	     e_drag_xdnd_start(d, ev->x, ev->y);

	     evry_hide(win, 0);
	     return ECORE_CALLBACK_PASS_ON;
	  }
     }
   else if (type == ECORE_EVENT_MOUSE_WHEEL)
     {
	Ecore_Event_Mouse_Wheel *ev = event;
     }
   else
#endif
   if (type == ECORE_EVENT_MOUSE_BUTTON_DOWN)
     {
	win->mouse_out = 0;

	if (!E_INSIDE(ev->x, ev->y, 0, 0, w->w, w->h))
	  {
	     win->mouse_out = 1;
	     return ECORE_CALLBACK_PASS_ON;
	  }

	win->mouse_button = ev->buttons;
     }
   else if (type == ECORE_EVENT_MOUSE_BUTTON_UP)
     {
	win->mouse_button = 0;

	if (win->mouse_out &&
	    !E_INSIDE(ev->x, ev->y, 0, 0, w->w, w->h))
	  {
	     evry_hide(win, 0);
	     return ECORE_CALLBACK_PASS_ON;
	  }
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void
_evry_window_free(Evry_Window *win)
{
   if (win->ewin->border)
     ecore_x_window_hide(win->ewin->border->win);
   else
     ecore_x_window_hide(win->ewin->evas_win);

   evas_event_freeze(win->evas);
   evas_object_del(win->o_main);
   e_object_del(E_OBJECT(win->ewin));
   E_FREE(win);
}

static void
_evry_selector_cb_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Evry_Selector *sel = data;

   if (ev->button == 1)
     {
	if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
	  evry_plugin_action(sel->win, 1);
     }
}

static void
_evry_selector_cb_wheel(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Wheel *ev = event_info;
   Evry_Selector *sel = data;
   Evry_Window *win = sel->win;

   if (ev->z > 0)
     {
	/* FIXME dont loose selector 2 state until state 0 changed: */
	if (CUR_SEL != OBJ_SEL)
	  _evry_selectors_switch(sel->win, 1);
     }
   else if (ev->z < 0)
     {
	_evry_selectors_switch(sel->win, -1);
     }
}

static void
_evry_selector_cb_up(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;
   Evry_Selector *sel = data;
   Evry_Window *win = sel->win;

   if (sel == CUR_SEL)
     return;

   if (ev->button == 3)
     {
	evry_plugin_action(win, 0);
     }
   else if (ev->button == 1)
     {
	if (sel == SUBJ_SEL)
	  {
	     if (CUR_SEL == ACTN_SEL)
	       _evry_selectors_switch(win, -1);
	     else
	       _evry_selectors_switch(win, 1);
	  }

	else if (sel == ACTN_SEL)
	  {
	     if (CUR_SEL == SUBJ_SEL)
	       _evry_selectors_switch(win, 1);
	     else
	       _evry_selectors_switch(win, -1);
	  }
	else if (sel == OBJ_SEL)
	  {
	     if (CUR_SEL == ACTN_SEL)
	       _evry_selectors_switch(win, 1);
	  }
     }
}

static Evry_Selector *
_evry_selector_new(Evry_Window *win, int type)
{
   Plugin_Config *pc;
   Eina_List *l, *pcs = NULL;
   Evry_Selector *sel = E_NEW(Evry_Selector, 1);
   Evas_Object *o = NULL;

   sel->aggregator = evry_aggregator_new(type);

   if (type == EVRY_PLUGIN_SUBJECT)
     {
	pcs = evry_conf->conf_subjects;
	sel->edje_part = "subject_selector";
     }
   else if (type == EVRY_PLUGIN_ACTION)
     {
	pcs = evry_conf->conf_actions;
	sel->edje_part = "action_selector";
     }
   else if (type == EVRY_PLUGIN_OBJECT)
     {
	pcs = evry_conf->conf_objects;
	sel->edje_part = "object_selector";
     }

   if ((o = edje_object_part_swallow_get(win->o_main, sel->edje_part)))
     {
	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				       _evry_selector_cb_down, sel);
	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP,
				       _evry_selector_cb_up, sel);
	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_WHEEL,
				       _evry_selector_cb_wheel, sel);
     }

   EINA_LIST_FOREACH(pcs, l, pc)
     {
	if (!pc->plugin) continue;
	if (pc->plugin == sel->aggregator) continue;
	sel->plugins = eina_list_append(sel->plugins, pc->plugin);
     }

   win->selectors[type] = sel;
   sel->win = win;

   return sel;
}

static void
_evry_selector_free(Evry_Selector *sel)
{
   Evry_Window *win = sel->win;

   _evry_selector_item_clear(sel);

   if (win->visible && (sel == CUR_SEL))
     _evry_view_clear(sel->state);

   while (sel->states)
     _evry_state_pop(sel, 1);

   EVRY_PLUGIN_FREE(sel->aggregator);

   if (sel->plugins) eina_list_free(sel->plugins);

   if (sel->update_timer)
     ecore_timer_del(sel->update_timer);

   if (sel->action_timer)
     ecore_timer_del(sel->action_timer);

   E_FREE(sel);
}

static void
_evry_selector_signal_emit(Evry_Selector *sel, const char *msg)
{
   Evry_Window *win = sel->win;
   char buf[128];
   snprintf(buf, sizeof(buf), "%s:%s", sel->edje_part, msg);
   edje_object_signal_emit(win->o_main, buf, "e");
}

static void
_evry_selector_label_set(Evry_Selector *sel, const char *part, const char *label)
{
   Evry_Window *win = sel->win;
   char buf[128];
   snprintf(buf, sizeof(buf), "%s:%s", sel->edje_part, part);
   edje_object_part_text_set(win->o_main, buf, label);
}

static void
_evry_selector_activate(Evry_Selector *sel, int slide)
{
   Evry_Window *win = sel->win;
   Evry_State *s;

   if (CUR_SEL)
     {
	Evry_Selector *cur = CUR_SEL;
	_evry_selector_signal_emit(cur, "e,state,unselected");

	if (cur->state && cur->state->view)
	  _evry_view_hide(win, cur->state->view, slide);

	if (!slide && evry_conf->hide_list)
	  _evry_list_win_hide(win);
     }

   if (!sel)
     {
	ERR("selector == NULL");
	return;
     }

   CUR_SEL = sel;

   _evry_selector_signal_emit(sel, "e,state,selected");

   /* do delayed actions fetch now */
   if (sel->action_timer)
     _evry_selector_update_actions_do(sel);

   if ((s = sel->state))
     {
	_evry_update_text_label(s);

	if (s->cur_item)
	  _evry_selector_label_set(sel, "e.text.plugin",
				   EVRY_ITEM(s->cur_item->plugin)->label);

	if (_evry_view_create(s))
	  {
	     _evry_view_show(win, s->view, slide);
	     s->view->update(s->view);
	  }
     }
}

static void
_evry_selector_thumb_gen(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Coord w, h;
   Evry_Selector *sel = data;
   Evry_Window *win = sel->win;
   char buf[64];

   if (sel->o_icon)
     {
       evas_object_del(sel->o_icon);
       sel->o_icon = NULL;
     }

   e_icon_size_get(sel->o_thumb, &w, &h);
   edje_extern_object_aspect_set(sel->o_thumb, EDJE_ASPECT_CONTROL_BOTH, w, h);

   snprintf(buf, sizeof(buf), "%s:e.swallow.thumb", sel->edje_part);
   edje_object_part_swallow(win->o_main, buf, sel->o_thumb);

   evas_object_show(sel->o_thumb);
   _evry_selector_signal_emit(sel, "e,action,thumb,show");
   /* edje_object_signal_emit(sel->o_main, "e,action,thumb,show", "e"); */
   sel->do_thumb = EINA_FALSE;
}

static int
_evry_selector_thumb(Evry_Selector *sel, const Evry_Item *it)
{
   Evry_Window *win = sel->win;
   char *suffix = NULL;

   if (sel->do_thumb)
     e_thumb_icon_end(sel->o_thumb);

   if (sel->o_thumb)
     evas_object_del(sel->o_thumb);
   sel->o_thumb = NULL;

   if (it->type != EVRY_TYPE_FILE)
     return 0;

   GET_FILE(file, it);

   if (!file->mime)
     return 0;

   if (!(evry_file_path_get(file)))
     return 0;

   if ((!strncmp(file->mime, "image/", 6)) ||
       ((suffix = strrchr(file->path, '.')) && (!strncmp(suffix, ".edj", 4))))
     {
   	sel->o_thumb = e_thumb_icon_add(win->evas);
   	e_thumb_icon_size_set(sel->o_thumb, 128, 128);
   	evas_object_smart_callback_add(sel->o_thumb, "e_thumb_gen",
				       _evry_selector_thumb_gen, sel);
	if (suffix)
	  {
	     e_thumb_icon_file_set(sel->o_thumb, file->path, "e/desktop/background");
	     e_thumb_icon_size_set(sel->o_thumb, 128, 80);
	  }
	else
	  e_thumb_icon_file_set(sel->o_thumb, file->path, NULL);

   	e_thumb_icon_begin(sel->o_thumb);
   	sel->do_thumb = EINA_TRUE;
   	return 1;
     }

   return 0;
}

static void
_evry_selector_item_clear(Evry_Selector *sel)
{
   if (sel->o_icon)
     {
	evas_object_del(sel->o_icon);
	sel->o_icon = NULL;
     }

   if (sel->o_thumb)
     {
	if (sel->do_thumb)
	  e_thumb_icon_end(sel->o_thumb);

	evas_object_del(sel->o_thumb);
	sel->o_thumb = NULL;
     }
}

static void
_evry_selector_item_update(Evry_Selector *sel)
{
   Evry_State *s = sel->state;
   Evry_Item *it = NULL;
   Evas_Object *o = NULL;
   Evry_Window *win = sel->win;

   _evry_selector_item_clear(sel);

   if ((!s) || !(s->cur_item))
     {
	/* no items for this state - clear selector */
	_evry_selector_label_set(sel, "e.text.label","");

	if (sel == CUR_SEL && s && s->plugin)
	  _evry_selector_label_set(sel, "e.text.plugin",
				   EVRY_ITEM(s->plugin)->label);
	else
	  _evry_selector_label_set(sel, "e.text.plugin", "");

	if (!s) return;
     }

   if ((it = s->cur_item))
     {
	_evry_selector_label_set(sel, "e.text.label", it->label);
	_evry_selector_label_set(sel, "e.text.plugin",
				 EVRY_ITEM(it->plugin)->label);

	if (!_evry_selector_thumb(sel, it))
	  {
	     o = evry_util_icon_get(it, win->evas);

	     if ((!o) && it->plugin)
	       o = evry_util_icon_get(EVRY_ITEM(it->plugin), win->evas);
	  }
     }

   if ((!o) && (s->plugin && (EVRY_ITEM(s->plugin)->icon)))
     {
	o = evry_icon_theme_get(EVRY_ITEM(s->plugin)->icon, win->evas);
     }

   if (o)
     {
	char buf[64];
	snprintf(buf, sizeof(buf), "%s:e.swallow.icons", sel->edje_part);
	edje_object_part_swallow(win->o_main, buf, o);
	evas_object_show(o);
	sel->o_icon = o;
     }
}

static void
_evry_selector_update(Evry_Selector *sel)
{
   Evry_State *s = sel->state;
   Evry_Item *it = NULL;
   Eina_Bool item_changed = EINA_FALSE;
   Evry_Window *win = sel->win;

   DBG("%p", sel);

   if (!s)
     {
	_evry_selector_item_update(sel);
	return;
     }

   if ((it = s->cur_item))
     {
	if (!s->plugin || !eina_list_data_find_list(s->plugin->items, it))
	  {
	     _evry_item_desel(s);
	     item_changed = EINA_TRUE;
	  }
     }

   /* get first item */
   if (s->plugin && ((!s->cur_item) || s->item_auto_selected))
     {
	it = eina_list_data_get(s->plugin->items);
	s->item_auto_selected = EINA_TRUE;
	if (!s->cur_plugins || (it != s->cur_item))
	  item_changed = EINA_TRUE;

	_evry_item_sel(s, it);
     }

   _evry_selector_item_update(sel);

   if ((sel == SUBJ_SEL) && (item_changed))
     _evry_selector_update_actions(ACTN_SEL);
}

static void
_evry_list_win_update(Evry_State *s)
{
   Evry_Window *win = s->selector->win;

   if (s != (CUR_SEL)->state) return;
   if (!win->visible) return;

   if (_evry_view_update(win, s))
     _evry_view_show(win, s->view, 0);
}

static int
_evry_selector_plugins_get(Evry_Selector *sel, Evry_Item *it, const char *plugin_name)
{
   Eina_List *l, *plugins = NULL;
   Evry_Plugin *p, *pp;
   Evry_Window *win = sel->win;
   Evry_Action *act = NULL;

   while (sel->state)
     _evry_state_pop(sel, 1);

   if ((sel != SUBJ_SEL) && (!it))
     return 0;

   if (it && CHECK_TYPE(it, EVRY_TYPE_ACTION))
     act = (Evry_Action *)it;

   EINA_LIST_FOREACH(sel->plugins, l, p)
     {
	if (!p->config->enabled && !win->plugin_dedicated)
	  continue;

	if (plugin_name && strcmp(plugin_name, p->name))
	  continue;

	if (act && (!CHECK_SUBTYPE(p, act->it2.type)))
	  continue;

	if (p->begin && (pp = p->begin(p, it)))
	  plugins = eina_list_append(plugins, pp);
     }

   _evry_state_new(sel, plugins);

   if (plugins)
     _evry_matches_update(sel, 1);

   return 1;
}

static Evry_State *
_evry_state_new(Evry_Selector *sel, Eina_List *plugins)
{
   Evry_State *s = E_NEW(Evry_State, 1);
   Eina_List *l;
   Evry_Plugin *p;

   if (!s) return NULL;

   s->inp = malloc(INPUTLEN);
   s->inp[0] = 0;
   s->input = s->inp;
   s->plugins = plugins;
   s->selector = sel;

   sel->states = eina_list_prepend(sel->states, s);
   sel->state = s;

   p = sel->aggregator->begin(sel->aggregator, NULL);
   s->plugins = eina_list_append(s->plugins, p);
   s->aggregator = p;

   EINA_LIST_FOREACH(s->plugins, l, p)
     p->state = s;

   return s;
}

static void
_evry_state_pop(Evry_Selector *sel, int immediate)
{
   Evry_Plugin *p;
   Evry_State *prev;
   Evry_Window *win = sel->win;
   Evry_State *s = sel->state;

   _evry_item_desel(s);

   if (sel->update_timer)
     {
   	ecore_timer_del(sel->update_timer);
   	sel->update_timer = NULL;
     }

   if (sel->action_timer)
     {
   	ecore_timer_del(sel->action_timer);
   	sel->action_timer = NULL;
     }

   if (s->view)
     {
	if (immediate)
	  {
	     s->view->destroy(s->view);
	  }
	else
	  {
	     _evry_view_hide(win, s->view, SLIDE_RIGHT);
	     /* XXX delay free state? is it possible that
		view can receive events here? */
	     s->delete_me = EINA_TRUE;
	  }
     }

   if (s->sel_items)
     eina_list_free(s->sel_items);

   if (s->cur_plugins)
     eina_list_free(s->cur_plugins); 

   sel->states = eina_list_remove_list(sel->states, sel->states);
   prev = eina_list_data_get(sel->states);

   EINA_LIST_FREE(s->plugins, p)
     {
	/* skip non top-level plugins */
	if (prev && eina_list_data_find(prev->plugins, p))
	  {
	     p->state = prev;
	     continue;
	  }
	EVRY_ITEM_FREE(p);
     }

   if (!s->delete_me)
     {
	E_FREE(s->inp);
	E_FREE(s);
     }

   sel->state = prev;
}

int
evry_state_push(Evry_Selector *sel, Eina_List *plugins)
{
   Evry_State *s, *new_state;
   Eina_List *l;
   Evry_Plugin *p;
   Evry_View *view = NULL;
   Evry_Window *win = sel->win;

   s = sel->state;

   if (!(new_state = _evry_state_new(sel, plugins)))
     {
	DBG("no new state");
	return 0;
     }

   EINA_LIST_FOREACH(plugins, l, p)
     p->state = new_state;

   if (s && s->view)
     {
	_evry_view_hide(win, s->view, SLIDE_LEFT);
	view = s->view;
     }

   _evry_matches_update(sel, 1);
   s = new_state;

   _evry_selector_update(sel);

   if (view && win->visible)
     {
	s->view = view->create(view, s, win->o_main);
	if (s->view)
	  {
	     s->view->state = s;
	     _evry_view_show(win, s->view, SLIDE_LEFT);
	     s->view->update(s->view);
	  }
     }

   _evry_update_text_label(sel->state);

   return 1;
}

int
evry_browse_item(Evry_Item *it)
{
   Evry_State *s, *new_state;
   Evry_Selector *sel;
   Eina_List *l, *plugins = NULL;
   Evry_Plugin *p, *pp, *pref = NULL;
   Evry_View *view = NULL;
   int browse_aggregator = 0;
   Evry_Window *win;

   if (!(it) || !(it->plugin) || !(it->browseable))
     {
	DBG("no item");
	return 0;
     }

   if (!(s = it->plugin->state))
     {
	DBG("no state");
	return 0;
     }

   sel = s->selector;
   win = sel->win;

   /* aggregator */
   if (CHECK_TYPE(it, EVRY_TYPE_PLUGIN))
     {
	browse_aggregator = 1;
	plugins = eina_list_append(plugins, it);
     }
   else
     {
	if ((it->plugin->browse) && (pp = it->plugin->browse(it->plugin, it)))
	  {
	     plugins = eina_list_append(plugins, pp);
	     pref = pp;
	  }

	/* items of type NONE can only be browsed by their own plugin */
	if (!CHECK_TYPE(it, EVRY_TYPE_NONE))
	  {
	     EINA_LIST_FOREACH(sel->plugins, l, p)
	       {
		  if (!p->browse)
		    continue;

		  if ((pref) && (!strcmp(p->name, pref->name)))
		    continue;

		  if ((pp = p->browse(p, it)))
		    plugins = eina_list_append(plugins, pp);
	       }
	  }
     }

   if (!(plugins))
     {
	DBG("no plugins");
	return 0;
     }

   if (!(new_state = _evry_state_new(sel, plugins)))
     {
	DBG("no new state");
	return 0;
     }

   if (s->view)
     {
	_evry_view_hide(win, s->view, SLIDE_LEFT);
	view = s->view;
     }

   if (browse_aggregator)
     {
	it->hi = NULL;

	if (!EVRY_PLUGIN(it)->items)
	  {
	     evry_history_item_add(it, NULL, s->input);
	     s = new_state;
	     EVRY_PLUGIN(it)->state = s;
	     _evry_matches_update(sel, 1);
	  }
	else
	  {
	     evry_history_item_add(it, NULL, NULL);
	     strncpy(new_state->input, s->input, INPUTLEN);
	     s = new_state;
	     EVRY_PLUGIN(it)->state = s;
	     s->cur_plugins = eina_list_append(s->cur_plugins, it);
	  }

	_evry_plugin_select(s, EVRY_PLUGIN(it));
     }
   else
     {
	if (it->plugin->history)
	  evry_history_item_add(it, NULL, s->input);

	_evry_matches_update(sel, 1);
	s = new_state;
	_evry_plugin_select(s, pref);
     }

   _evry_selector_update(sel);

   if (view && win->visible)
     {
	s->view = view->create(view, s, win->o_main);
	if (s->view)
	  {
	     s->view->state = s;
	     s->view->update(s->view);
	     _evry_view_show(win, s->view, SLIDE_LEFT);
	  }
     }

   _evry_update_text_label(sel->state);

   return 1;
}

int
evry_browse_back(Evry_Selector *sel)
{
   Evry_Window *win;

   if (!sel) return 0;

   if ((!sel) || !(win = sel->win))
     return 0;

   if ((!sel->state) || (!sel->states->next))
     return 0;

   _evry_state_pop(sel, 0);

   _evry_selector_update(sel);
   _evry_update_text_label(sel->state);
   
   if (sel == SUBJ_SEL)
     _evry_selector_update_actions(ACTN_SEL);

   if (sel->state->view)
     {
	_evry_view_show(win, sel->state->view, SLIDE_RIGHT);
	sel->state->view->update(sel->state->view);
     }
   
   return 1;
}

int
evry_selectors_switch(Evry_Window *win, int dir, int slide)
{
   Evry_State *s = (CUR_SEL)->state;

   if ((CUR_SEL)->update_timer)
     {
	if ((CUR_SEL == SUBJ_SEL) || (CUR_SEL == ACTN_SEL))
	  {
	     _evry_matches_update(CUR_SEL, 0);
	     _evry_selector_update(CUR_SEL);
	  }
     }

   if (CUR_SEL != SUBJ_SEL && dir == 0)
     {
   	edje_object_signal_emit(win->o_main, "e,state,object_selector_hide", "e");
   	_evry_selector_activate(SUBJ_SEL, (slide * SLIDE_RIGHT));
   	return 1;
     }
   if (CUR_SEL == SUBJ_SEL && dir > 0)
     {
	if (s->cur_item)
	  {
	     _evry_selector_activate(ACTN_SEL, slide * SLIDE_LEFT);
	     return 1;
	  }
     }
   else if (CUR_SEL == ACTN_SEL && dir > 0)
     {
	Evry_Item *it;

	if (!s || !(it = s->cur_item) || !(CHECK_TYPE(it, EVRY_TYPE_ACTION)))
	  return 0;

	GET_ACTION(act,it);
	if (!act->it2.type)
	  return 0;

	_evry_selector_plugins_get(OBJ_SEL, it, NULL);
	_evry_selector_update(OBJ_SEL);
	edje_object_signal_emit(win->o_main, "e,state,object_selector_show", "e");

	_evry_selector_activate(OBJ_SEL, (slide * SLIDE_LEFT));
	return 1;
     }
   else if (CUR_SEL == ACTN_SEL && dir < 0)
     {
	_evry_selector_activate(SUBJ_SEL, (slide * SLIDE_RIGHT));
	edje_object_signal_emit(win->o_main, "e,state,object_selector_hide", "e");
	return 1;
     }
   else if (CUR_SEL == OBJ_SEL && dir < 0)
     {
	_evry_selector_activate(ACTN_SEL, (slide * SLIDE_RIGHT));
	return 1;
     }
   return 0;
}

static int
_evry_selectors_switch(Evry_Window *win, int dir)
{
   return evry_selectors_switch(win, dir, 0);
}

static int
_evry_input_complete(Evry_State *s)
{
   int action = 0;
   char *input = NULL;
   Evry_Item *it = s->cur_item;
   Evry_Window *win = s->selector->win;

   if (!it) return 0;

   evry_item_ref(it);

   s->item_auto_selected = EINA_FALSE;

   if (it->plugin->complete)
     action = it->plugin->complete(it->plugin, it, &input);
   else
     evry_browse_item(it);

   if (input && action == EVRY_COMPLETE_INPUT)
     {
	strncpy(s->input, input, INPUTLEN - 1);
	_evry_update_text_label(s);
	_evry_cb_update_timer(CUR_SEL);
	evry_item_select(s, it);
     }

   evry_item_free(it);
   E_FREE(input);

   return 1;
}

static int
_evry_cheat_history(Evry_State *s, int promote, int delete)
{

   History_Entry *he;
   History_Item *hi;
   History_Types *ht;
   Eina_List *l, *ll;
   Evry_Item *it = s->cur_item;

   if (!it) return 0;

   if (!(ht = evry_history_types_get(it->type)))
     return 1;

   if (!(he = eina_hash_find(ht->types, (it->id ? it->id : it->label))))
     return 1;

   EINA_LIST_FOREACH_SAFE(he->items, l, ll, hi)
     {
	if (hi->plugin != it->plugin->name)
	  continue;

	if (delete)
	  {
	     if (hi->input)
	       eina_stringshare_del(hi->input);
	     if (hi->plugin)
	       eina_stringshare_del(hi->plugin);
	     if (hi->context)
	       eina_stringshare_del(hi->context);
	     E_FREE(hi);

	     he->items = eina_list_remove_list(he->items, l);
	  }
	else if (promote)
	  {
	     hi->count += 5;
	     hi->last_used = ecore_time_unix_get();
	  }
	else /* demote */
	  {
	     hi->count -= 5;
	     if (hi->count < 0) hi->count = 1;
	  }
     }
   if (s->plugin == s->aggregator)
     _evry_aggregator_fetch(s);
   if (s->view)
     s->view->update(s->view);

   return 1;
}

static Eina_Bool
_evry_cb_key_down(void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Key *ev = event;
   Evry_State *s;
   Evry_Selector *sel;
   Evry_Window *win = data;
   const char *old;

   if (ev->event_window != win->ewin->evas_win)
     return ECORE_CALLBACK_PASS_ON;

   if (!strcmp(ev->key, "Escape"))
     {
	evry_hide(win, 0);
	return ECORE_CALLBACK_PASS_ON;
     }
#ifdef DRAG_OFF_WINDOW
   else if (win->grab && !strcmp(ev->key, "F1"))
     {
	E_Win *ewin = win->ewin;

	e_grabinput_release(ewin->evas_win, ewin->evas_win);
	if (!win->shaped)
	  e_win_borderless_set(ewin, 0);
	ecore_evas_lower(ewin->ecore_evas);
	ewin->border = e_border_new(ewin->container, ewin->evas_win, 1, 1);
	// dont need this - special stuff - here it is needed
	ewin->border->ignore_first_unmap = 1;
	ewin->border->internal = 1;
	ewin->border->internal_ecore_evas = ewin->ecore_evas;
	ewin->border->internal_no_remember = 1;
	e_border_show(ewin->border);

	win->grab = 0;
	return ECORE_CALLBACK_PASS_ON;
     }
#endif
   else if (ev->modifiers)
     {
	Eina_List *l;
	E_Config_Binding_Key *bind;
	E_Binding_Modifier mod;

	for (l = e_config->key_bindings; l; l = l->next)
	  {
	     bind = l->data;

	     if (bind->action && strcmp(bind->action, "everything")) continue;

	     mod = 0;

	     if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)
	       mod |= E_BINDING_MODIFIER_SHIFT;
	     if (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)
	       mod |= E_BINDING_MODIFIER_CTRL;
	     if (ev->modifiers & ECORE_EVENT_MODIFIER_ALT)
	       mod |= E_BINDING_MODIFIER_ALT;
	     if (ev->modifiers & ECORE_EVENT_MODIFIER_WIN)
	       mod |= E_BINDING_MODIFIER_WIN;

	     if (!(bind->key && (!strcmp(bind->key, ev->keyname)) &&
		   (((unsigned int)bind->modifiers == mod) || (bind->any_mod))))
	       continue;

	     if (win->level > 0)
	       return ECORE_CALLBACK_PASS_ON;

	     if (!(bind->params) && (CUR_SEL == OBJ_SEL) &&
		 ((CUR_SEL)->state && (CUR_SEL)->state->cur_item))
	       {
		  _evry_selectors_shift(win, 1);
		  return ECORE_CALLBACK_PASS_ON;
	       }

	     evry_hide(win, 1);

#if 0 /* FIXME this causes segv when triggering a plugin keybinding twice */
	     if (win && CUR_SEL && bind->params)
	       {
		  Eina_List *ll;
		  Evry_Plugin *p;

		  EINA_LIST_FOREACH((SUBJ_SEL)->plugins, ll, p)
		    if (!strcmp(bind->params, p->name)) break;

		  if (p)
		    {
		       _evry_plugin_select((CUR_SEL)->state, p);
		       _evry_selector_update(CUR_SEL);
		       _evry_view_update(win, (CUR_SEL)->state);
		    }
	       }
#endif
	     return ECORE_CALLBACK_PASS_ON;
	  }
     }

   old = ev->key;
   win->request_selection = EINA_FALSE;

   if (!strcmp(ev->key, "KP_Enter"))
     {
	ev->key = "Return";
     }
   else if (ev->modifiers & ECORE_EVENT_MODIFIER_ALT)
     {
	if (!strcmp(ev->key, "Tab"))
	  {
	     ev->key = "Down";
	  }
	else if (!strcmp(ev->key, "ISO_Left_Tab") || (!strcmp(ev->key, "Tab") &&
		  (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)))
	  {
	     ev->key = "Up";
	     ev->modifiers = 0;
	  }
	else if (!strcmp(ev->key, "q"))
	  {
	     evry_hide(win, 0);
	     return 1;
	  }
	else if (!strcmp(ev->key, "w"))
	  {
	     ev->key = "Return";
	     ev->modifiers = 0;
	  }
	else if (evry_conf->quick_nav == 1)
	  {
	     if (!strcmp(ev->key, "k") || (!strcmp(ev->key, "K")))
	       ev->key = "Up";
	     else if (!strcmp(ev->key, "j") || (!strcmp(ev->key, "J")))
	       ev->key = "Down";
	     else if (!strcmp(ev->key, "n") || (!strcmp(ev->key, "N")))
	       ev->key = "Next";
	     else if (!strcmp(ev->key, "p") || (!strcmp(ev->key, "P")))
	       ev->key = "Prior";
	     else if (!strcmp(ev->key, "l") || (!strcmp(ev->key, "L")))
	       ev->key = "Right";
	     else if (!strcmp(ev->key, "h") || (!strcmp(ev->key, "H")))
	       ev->key = "Left";
	     else if (!strcmp(ev->key, "i") || (!strcmp(ev->key, "I")))
	       ev->key = "Tab";
	     else if (!strcmp(ev->key, "m") || (!strcmp(ev->key, "M")))
	       ev->key = "Return";
	  }
	else if (evry_conf->quick_nav == 3)
	  {
	     if (!strcmp(ev->key, "p") || (!strcmp(ev->key, "P")))
	       ev->key = "Up";
	     else if (!strcmp(ev->key, "n") || (!strcmp(ev->key, "N")))
	       ev->key = "Down";
	     else if (!strcmp(ev->key, "f") || (!strcmp(ev->key, "F")))
	       ev->key = "Right";
	     else if (!strcmp(ev->key, "b") || (!strcmp(ev->key, "B")))
	       ev->key = "Left";
	     else if (!strcmp(ev->key, "i") || (!strcmp(ev->key, "I")))
	       ev->key = "Tab";
	     else if (!strcmp(ev->key, "m") || (!strcmp(ev->key, "M")))
	       ev->key = "Return";
	  }
     }

   if (!win || !(sel = CUR_SEL))
     goto end;

   if (!strcmp(ev->key, "Tab") &&
       !((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) ||
	 (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)))
     {
	if (!_evry_selectors_switch(win, 1))
	  _evry_selectors_switch(win, 0);
	goto end;
     }

   if (!(s = sel->state))
     goto end;

   if (!win->visible && (!strcmp(ev->key, "Down")))
     {
	_evry_list_win_show(win);
	goto end;
     }
   else if ((!strcmp(ev->key, "ISO_Left_Tab") ||
	     (((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) ||
	       (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)) &&
	      (!strcmp(ev->key, "Tab")))))
     {
	_evry_input_complete(s);
	goto end;
     }
   else if ((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) &&
   	    (!strcmp(ev->key, "Delete") || !strcmp(ev->key, "Insert")))
     {
	int delete = (!strcmp(ev->key, "Delete") &&
		      (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT));
   	int promote = (!strcmp(ev->key, "Insert"));

	_evry_cheat_history(s, promote, delete);
	goto end;
     }
   else if (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)
     {
	if (!strcmp(ev->key, "u"))
	  {
	     if (!_evry_clear(sel))
	       evry_browse_back(sel);
	     goto end;
	  }
	else if (!strcmp(ev->key, "1"))
	  evry_view_toggle(s, NULL);
	else if (!strcmp(ev->key, "Return"))
	  _evry_plugin_action(sel, 0);
	else if (!strcmp(ev->key, "v"))
	  {
	     win->request_selection = EINA_TRUE;
	     ecore_x_selection_primary_request
	       (win->ewin->evas_win, ECORE_X_SELECTION_TARGET_UTF8_STRING);
	  }
	else
	  {
	     _evry_view_key_press(s, ev);
	  }
	goto end;
     }
   if ((s->plugin && s->plugin->cb_key_down) &&
       (s->plugin->cb_key_down(s->plugin, ev)))
     {
	/* let plugin intercept keypress */
	goto end;
     }
   else if (_evry_view_key_press(s, ev))
     {
	/* let view intercept keypress */
	goto end;
     }
   else if (!strcmp(ev->key, "Right"))
     {
	if (!evry_browse_item(sel->state->cur_item))
	  evry_selectors_switch(win, 1, EINA_TRUE);
     }
   else if (!strcmp(ev->key, "Left"))
     {
	if (!evry_browse_back(sel))
	  evry_selectors_switch(win, -1, EINA_TRUE);
     }
   else if (!strcmp(ev->key, "Return"))
     {
	if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)
	  {
	     _evry_plugin_action(sel, 0);
	  }
	else if (s->cur_item && s->cur_item->browseable)
	  {
	     evry_browse_item(sel->state->cur_item);
	  }
	else
	  {
	     _evry_plugin_action(sel, 1);
	  }
     }
   else if (!strcmp(ev->key, "BackSpace"))
     {
	if (!_evry_backspace(sel))
	  evry_browse_back(sel);
     }
   else if ((ev->compose && !(ev->modifiers & ECORE_EVENT_MODIFIER_ALT)))
     {
	unsigned int len = strlen(s->inp);

	if (len == 0 && (evry_view_toggle(s, ev->compose)))
	  goto end;

	if (len < (INPUTLEN - strlen(ev->compose)))
	  {
	     strcat(s->inp, ev->compose);

	     _evry_update(sel, 1);
	  }
     }

 end:
   ev->key = old;
   return ECORE_CALLBACK_PASS_ON;
}

static int
_evry_backspace(Evry_Selector *sel)
{
   Evry_State *s = sel->state;
   int len, val, pos;

   if (!(len = strlen(s->inp)))
     return 0;

   pos = evas_string_char_prev_get(s->inp, len, &val);
   if ((pos < len) && (pos >= 0))
     {
	val = *(s->inp + pos);

	s->inp[pos] = 0;

	if (s->trigger_active && s->inp[0] != 0)
	  s->input = s->inp + 1;
	else
	  s->input = s->inp;

	if (pos == 0)
	  s->trigger_active = EINA_FALSE;

	if ((pos == 0) || !isspace(val))
	  _evry_update(sel, 1);

	return 1;
     }

   return 0;
}

static void
_evry_update_text_label(Evry_State *s)
{
   Evry_Window *win = s->selector->win;

   if (!win->visible && evry_conf->hide_input)
     {
	if (strlen(s->inp) > 0)
	  edje_object_signal_emit(win->o_main, "list:e,state,entry_show", "e");
	else
	  edje_object_signal_emit(win->o_main, "list:e,state,entry_hide", "e");
     }

   edje_object_part_text_set(win->o_main, "list:e.text.label", s->inp);
}

static void
_evry_update(Evry_Selector *sel, int fetch)
{
   Evry_State *s = sel->state;
   Evry_Window *win = sel->win;

   _evry_update_text_label(s);

   if (fetch)
     {
	if (sel->update_timer)
	  ecore_timer_del(sel->update_timer);

	sel->update_timer =
	  ecore_timer_add(MATCH_LAG, _evry_cb_update_timer, sel);

	edje_object_signal_emit(win->o_main, "list:e,signal,update", "e");
     }
}

static Eina_Bool
_evry_cb_update_timer(void *data)
{
   Evry_Selector *sel = data;

   _evry_matches_update(sel, 1);
   _evry_selector_update(sel);
   _evry_list_win_update(sel->state);
   sel->update_timer = NULL;

   return ECORE_CALLBACK_CANCEL;
}

static int
_evry_clear(Evry_Selector *sel)
{
   Evry_State *s = sel->state;
   Evry_Window *win = sel->win;

   if (!(s->inp) || (s->inp[0] == 0))
     return 0;

   if (s->trigger_active && s->inp[1] != 0)
     {
	s->inp[1] = 0;
	s->input = s->inp + 1;
     }
   else
     {
	s->inp[0] = 0;
	s->input = s->inp;
	s->trigger_active = EINA_FALSE;
     }

   _evry_update(sel, 1);

   if (!win->visible && evry_conf->hide_input)
     edje_object_signal_emit(win->o_main, "list:e,state,entry_hide", "e");

   return 1;
}

static void
_evry_cb_free_action_performed(void *data __UNUSED__, void *event)
{
   Evry_Event_Action_Performed *ev = event;

   if (ev->it1)
     EVRY_ITEM_FREE(ev->it1);
   if (ev->it2)
     EVRY_ITEM_FREE(ev->it2);

   IF_RELEASE(ev->action);

   E_FREE(ev);
}

static int
_evry_action_do(Evry_Action *act)
{
   Evry_Event_Action_Performed *ev;
   int ret;

   if ((ret = act->action(act)))
     {
	ev = E_NEW(Evry_Event_Action_Performed, 1);
	ev->action = eina_stringshare_ref(act->name);
	ev->it1 = act->it1.item;
	ev->it2 = act->it2.item;

	if (ev->it1)
	  EVRY_ITEM_REF(ev->it1);
	if (ev->it2)
	  EVRY_ITEM_REF(ev->it2);

	ecore_event_add(_evry_events[EVRY_EVENT_ACTION_PERFORMED], ev,
			_evry_cb_free_action_performed, NULL);
	return ret;
     }
   return 0;
}

void
evry_plugin_action(Evry_Window *win, int finished)
{
   _evry_plugin_action(CUR_SEL, finished);
}

static void
_evry_plugin_action(Evry_Selector *sel, int finished)
{
   Evry_State *s_subj, *s_act, *s_obj = NULL;
   Evry_Item *it, *it_subj, *it_act, *it_obj = NULL;
   Evry_Window *win = sel->win;
   Eina_List *l;
   int ret = 0;

   if ((SUBJ_SEL)->update_timer)
     {
	_evry_matches_update(SUBJ_SEL, 0);
	_evry_selector_update(SUBJ_SEL);
     }

   /* do delayed fetch actions now */
   if ((ACTN_SEL)->action_timer)
     _evry_selector_update_actions_do(ACTN_SEL);

   if (!(s_subj = (SUBJ_SEL)->state))
     return;

   if (!(it_subj = s_subj->cur_item))
     return;

   if ((CUR_SEL == SUBJ_SEL) && (ACTN_SEL)->update_timer)
     {
	/* _evry_selector_actions_get(it_subj); */
	_evry_selector_plugins_get(ACTN_SEL, it_subj, NULL);

	if (!(ACTN_SEL)->state)
	  return;

	_evry_selector_update(ACTN_SEL);
     }

   if (!(s_act = (ACTN_SEL)->state))
     return;

   if (!(it_act = s_act->cur_item))
     return;

   if (CHECK_TYPE(it_act, EVRY_TYPE_ACTION) ||
       CHECK_SUBTYPE(it_act, EVRY_TYPE_ACTION))
     {
	GET_ACTION(act, it_act);

	if (!act->action)
	  return;

	/* get object item for action, when required */
	if (act->it2.type)
	  {
	     /* check if object is provided */
	     if ((s_obj = (OBJ_SEL)->state) && (!s_obj->delete_me))
	       {
		  it_obj = s_obj->cur_item;
	       }

	     if (!it_obj)
	       {
		  if (SUBJ_SEL == CUR_SEL)
		    _evry_selectors_switch(win, 1);
		  if (ACTN_SEL == CUR_SEL)
		    _evry_selectors_switch(win, 1);
		  return;
	       }

	     act->it2.item = it_obj;
	  }

	if (s_obj && s_obj->sel_items  && !(act->it2.accept_list))
	  {
	     if (!(act->it1.item && CHECK_TYPE(act->it1.item, EVRY_TYPE_PLUGIN)))
	       act->it1.item = it_subj;

	     EINA_LIST_FOREACH(s_obj->sel_items, l, it)
	       {
		  if (it->type != act->it2.type)
		    continue;
		  act->it2.item = it;

		  _evry_action_do(act);
	       }
	  }
	else if (s_subj->sel_items && !(act->it1.accept_list))
	  {
	     EINA_LIST_FOREACH(s_subj->sel_items, l, it)
	       {
		  if (it->type != act->it1.type)
		    continue;
		  act->it1.item = it;

		  _evry_action_do(act);
	       }
	  }
	else
	  {
	     if (!(act->it1.item && CHECK_TYPE(act->it1.item, EVRY_TYPE_PLUGIN)))
	       {
		  act->it1.item = it_subj;
		  act->it1.items = s_subj->sel_items;
	       }

	     if (s_obj)
	       act->it2.items = s_obj->sel_items;

	     ret = _evry_action_do(act);

	     if (ret == EVRY_ACTION_OTHER)
	       return;
	  }
     }
   else return;

   if (s_subj && it_subj && it_subj->plugin->history)
     evry_history_item_add(it_subj, NULL, s_subj->input);

   if (s_act && it_act && it_act->plugin->history)
     evry_history_item_add(it_act, it_subj->context, s_act->input);

   if (s_obj && it_obj && it_obj->plugin->history)
     evry_history_item_add(it_obj, it_act->context, s_obj->input);

   if (ret == EVRY_ACTION_CONTINUE)
     return;

   if (ret == EVRY_ACTION_CLEAR)
     {
	_evry_clear(CUR_SEL);
     }
   else
     {	
	win->func.hide(win, finished);
     }
}

static void
_evry_view_list_swallow_set(Evas_Object *obj, const char *part, Evas_Object *swallow)
{
   Evas_Object *o;

   o = edje_object_part_swallow_get(obj, part);
   if (o != swallow)
     {
	evas_object_hide(o);
	edje_object_part_unswallow(obj, o);

	edje_object_part_unswallow(obj, swallow);
	edje_object_part_swallow(obj, part, swallow);
	evas_object_show(swallow);
     }
}

static void
_evry_view_show(Evry_Window *win, Evry_View *v, int slide)
{
   if (!v) return;

   if (v->o_bar)
     {
	edje_object_part_swallow(win->o_main, "list:e.swallow.bar", v->o_bar);
	evas_object_show(v->o_bar);
     }

   if (!v->o_list)
     return;

   if (slide == SLIDE_LEFT)
     {
	_evry_view_list_swallow_set(win->o_main, "list:e.swallow.list2", v->o_list);

	edje_object_signal_emit(win->o_main, "list:e,action,slide,left", "e");
	edje_object_signal_emit(v->o_list, "e,action,show,list", "e");
     }
   else if (slide == SLIDE_RIGHT)
     {
	_evry_view_list_swallow_set(win->o_main, "list:e.swallow.list", v->o_list);

	edje_object_signal_emit(win->o_main, "list:e,action,slide,right", "e");
	edje_object_signal_emit(v->o_list, "e,action,show,list", "e");
     }
   else
     {
	_evry_view_list_swallow_set(win->o_main, "list:e.swallow.list", v->o_list);

	edje_object_signal_emit(win->o_main, "list:e,action,slide,default", "e");
	edje_object_signal_emit(v->o_list, "e,action,show,list", "e");
     }
}

static Eina_Bool
_clear_timer(void *data)
{
   _evry_state_clear(data);

   return ECORE_CALLBACK_CANCEL;
}

static void
_evry_state_clear(Evry_Window *win)
{
   Evry_State *s;
   Evry_View *v;

   if (!win->state_clearing)
     return;

   s = win->state_clearing;
   win->state_clearing = NULL;

   ecore_timer_del(s->clear_timer);
   s->clear_timer = NULL;

   if (s->delete_me)
     {
	if (s->view) s->view->destroy(s->view);
	s->clear_timer = NULL;
	E_FREE(s->inp);
	E_FREE(s);
     }
   else if ((v = s->view))
     {
	v->clear(v);

	if (v->o_list)
	  {
	     evas_object_hide(v->o_list);
	     edje_object_part_unswallow(win->o_main, v->o_list);
	  }
     }

   /* replay mouse down to allow direct sliding back */
   if (win->mouse_button)
     evas_event_feed_mouse_down(win->evas, win->mouse_button, 0, 0, NULL);
}

static void
_evry_view_hide(Evry_Window *win, Evry_View *v, int slide)
{
   if (v->state->delete_me)
     {
	_evry_state_clear(win);
	return;
     }

   _evry_state_clear(win);

   if (!v) return;

   if (slide && v->o_list)
     {
	win->state_clearing = v->state;

	if (slide == SLIDE_RIGHT)
	  {
	     _evry_view_list_swallow_set(win->o_main, "list:e.swallow.list2", v->o_list);
	  }
	else /* if (slide == SLIDE_LEFT) */
	  {
	     _evry_view_list_swallow_set(win->o_main, "list:e.swallow.list", v->o_list);
	  }

	edje_object_signal_emit(v->o_list, "e,action,hide,list", "e");

	/* FIXME get signal from theme when anim finished */
	v->state->clear_timer = ecore_timer_add(1.5, _clear_timer, win);

	if (v->o_bar)
	  {
	     evas_object_hide(v->o_bar);
	     edje_object_part_unswallow(win->o_main, v->o_bar);
	  }

	return;
     }

   v->clear(v);

   if (v->o_list)
     {
	evas_object_hide(v->o_list);
	edje_object_part_unswallow(win->o_main, v->o_list);
     }

   if (v->o_bar)
     {
	evas_object_hide(v->o_bar);
	edje_object_part_unswallow(win->o_main, v->o_bar);
     }
}

static int
_evry_view_create(Evry_State *s)
{
   Evry_View *view;

   if (s->view)
     return 1;

   if (s->plugin && s->plugin->view)
     view = s->plugin->view;
   else
     view = eina_list_data_get(evry_conf->views);

   s->view = view->create(view, s, s->selector->win->o_main);

   if (s->view)
     {
	s->view->state = s;
	return 1;
     }

   return 0;
}


static int
_evry_view_update(Evry_Window *win, Evry_State *s)
{
   if (!win->visible) return 0;
   /* TODO check this again !!!!*/
   if (s->view)
     {
	s->view->update(s->view);
	return 0;
     }

   if (_evry_view_create(s))
     {
	s->view->update(s->view);
	return 1;
     }

   return 0;
}

static void
_evry_view_clear(Evry_State *s)
{
   if (!s || !s->view) return;

   s->view->clear(s->view);
}

static int
_evry_view_key_press(Evry_State *s, Ecore_Event_Key *ev)
{
   if (!s || !s->view || !s->view->cb_key_down) return 0;

   return s->view->cb_key_down(s->view, ev);
}

int
evry_view_toggle(Evry_State *s, const char *trigger)
{
   Evry_View *view, *v = NULL;
   Eina_List *l, *ll;
   Eina_Bool triggered = FALSE;
   Evry_Window *win = s->selector->win;

   if (trigger)
     {
	EINA_LIST_FOREACH(evry_conf->views, ll, view)
	  {
	     if (view->trigger && !strncmp(trigger, view->trigger, 1) &&
		 (!s->view || (view->id != s->view->id)) &&
		 (v = view->create(view, s, win->o_main)))
	       {
		  triggered = EINA_TRUE;
		  goto found;
	       }
	  }
     }
   else
     {
	if (s->view)
	  l = eina_list_data_find_list(evry_conf->views, s->view->id);
	else
	  {
	     view = evry_conf->views->data;
	     v = view->create(view, s, win->o_main);
	     goto found;
	  }

	if (l && l->next)
	  l = l->next;
	else
	  l = evry_conf->views;

	EINA_LIST_FOREACH(l, ll, view)
	  {
	     if ((!view->trigger) &&
		 ((!s->view || (view->id != s->view->id)) &&
		  (v = view->create(view, s, win->o_main))))
	       goto found;
	  }
     }

 found:
   if (!v) return 0;

   _evry_list_win_show(win);

   if (s->view)
     {
	_evry_view_hide(win, s->view, 0);
	s->view->destroy(s->view);
     }

   s->view = v;
   s->view->state = s;
   _evry_view_show(win, s->view, 0);
   view->update(s->view);

   return triggered;
}

static void
_evry_matches_update(Evry_Selector *sel, int async)
{
   Evry_State *s = sel->state;
   Evry_Plugin *p;
   Eina_List *l;
   Evry_Item *it;
   Evry_Window *win = sel->win;
   const char *input = NULL;
   int len_inp = 0;

   s->changed = 1;
   s->request++;

#ifdef CHECK_TIME
   DBG("matches update %f", ecore_time_get() - _evry_time);
#endif

   if (sel->update_timer)
     {
	ecore_timer_del(sel->update_timer);
	sel->update_timer = NULL;
     }

   if (s->sel_items)
     {
	eina_list_free(s->sel_items);
	s->sel_items = NULL;
     }

   if (s->inp[0])
     {
	len_inp = strlen(s->inp);
	input = s->inp;
     }

   /* use current plugins */
   if (s->trigger_active)
     {
	s->plugin_auto_selected = EINA_FALSE;

	EINA_LIST_FOREACH(s->cur_plugins, l, p)
	  {
	     p->request = s->request;
	     p->fetch(p, s->input);
	  }
	goto found;
     }

   EINA_LIST_FREE(s->cur_plugins, p);

   /* check if input matches plugin trigger */
   if (input)
     {
	int len_trigger = 0;

	EINA_LIST_FOREACH(s->plugins, l, p)
	  {
	     if (!p->config->trigger) continue;
	     int len = strlen(p->config->trigger);

	     if (len_trigger && len != len_trigger)
	       continue;

	     if ((len_inp >= len) &&
		 (!strncmp(s->inp, p->config->trigger, len)))
	       {
		  len_trigger = len;
		  s->cur_plugins = eina_list_append(s->cur_plugins, p);
		  p->request = s->request;
		  if(len_inp == len)
		    p->fetch(p, NULL);
		  else
		    p->fetch(p, s->input + len);
	       }
	  }
	if (s->cur_plugins)
	  {
	     s->trigger_active = EINA_TRUE;
	     /* replace trigger with indicator */
	     if (len_trigger > 1)
	       {
		  s->inp[0] = ':';

		  if (s->inp + len_trigger)
		    strcpy(s->inp + 1, s->inp + len_trigger);
		  else
		    s->inp[1] = 0;
	       }
	     s->input = s->inp + 1;
	     _evry_update_text_label(s);

	     goto found;
	  }
     }

   /* query all other plugins for this state */
   EINA_LIST_FOREACH(s->plugins, l, p)
     {
	p->request = s->request;

	if (p == s->aggregator)
	  goto next;

	/* dont wait for async plugin. use their current items */
	if (!async && p->async_fetch && p->items)
	  {
	     s->cur_plugins = eina_list_append(s->cur_plugins, p);
	     goto next;
	  }

	if (!((sel->states->next) ||
	      ((CUR_SEL == SUBJ_SEL) &&
	       (win->plugin_dedicated))))
	  {
	     /* skip plugins in toplevel which trigger-only */
	     if ((sel == SUBJ_SEL) &&
		 (p->config->top_level) &&
		 (p->config->trigger_only))
	       goto next;

	     /* skip non-toplevel plugins when input < min_query */
	     if ((!p->config->top_level) &&
		 (p->config->min_query > len_inp))
	       goto next;
	  }

	if (p->fetch(p, input))
	  {
#ifdef CHECK_TIME
	     DBG("fetch %s %f", p->name, ecore_time_get() - _evry_time);
#endif
	     s->cur_plugins = eina_list_append(s->cur_plugins, p);
	     continue;
	  }

     next:
	if (s->plugin == p)
	  s->plugin = NULL;
     }

 found:
   _evry_aggregator_fetch(s);

   if (s->plugin_auto_selected)
     s->plugin = NULL;

   _evry_plugin_select(s, s->plugin);

   if (!s->plugin)
     return;

   EINA_LIST_FOREACH(s->plugin->items, l, it)
     if (it->marked)
       s->sel_items = eina_list_append(s->sel_items, it);
}

static void
_evry_item_desel(Evry_State *s)
{
   Evry_Item *it = s->cur_item;

   if (it)
     {
	it->selected = EINA_FALSE;
	evry_item_free(it);
     }

   s->cur_item = NULL;
}

static void
_evry_item_sel(Evry_State *s, Evry_Item *it)
{
   if (s->cur_item == it) return;

   _evry_item_desel(s);

   evry_item_ref(it);
   it->selected = EINA_TRUE;

   s->cur_item = it;
}

static void
_evry_plugin_select(Evry_State *s, Evry_Plugin *p)
{
   if (!s) return;

   if (p)
     {
	s->plugin_auto_selected = EINA_FALSE;
     }
   else
     {
	p = eina_list_data_get(s->cur_plugins);
	s->plugin_auto_selected = EINA_TRUE;
     }

   s->plugin = p;

   if ((s->view && s->plugin && s->plugin->view) &&
       (s->view->name != s->plugin->view->name))
     {
	s->view->destroy(s->view);
	s->view = NULL;
	if (_evry_view_create(s))
	  {
	     _evry_view_show(s->selector->win, s->view, 0);
	     s->view->update(s->view);
	  }

     }
}

static void
_evry_cb_free_plugin_selected(void *data __UNUSED__, void *event)
{
   Evry_Event_Item_Selected *ev = event;

   evry_item_free(ev->item);
   E_FREE(ev);
}

void
evry_plugin_select(Evry_Plugin *p)
{
   Evry_Event_Item_Selected *ev;

   if (!p) return;

   if (!p->state)
     {
	ERR("no state!");
	return;
     }

   _evry_plugin_select(p->state, p);
   _evry_selector_update(p->state->selector);

   ev = E_NEW(Evry_Event_Item_Selected, 1);
   ev->item = EVRY_ITEM(p);
   EVRY_ITEM_REF(p);
   ecore_event_add(_evry_events[EVRY_EVENT_PLUGIN_SELECTED], ev,
		   _evry_cb_free_plugin_selected, NULL);
}

static void
_evry_plugin_list_insert(Evry_State *s, Evry_Plugin *p)
{
   Eina_List *l;
   Evry_Plugin *p2;

   EINA_LIST_FOREACH(s->cur_plugins, l, p2)
     {
	if (p == p2)
	  return;

	if (p->config->priority < p2->config->priority)
	  break;
     }

   if (l)
     s->cur_plugins = eina_list_prepend_relative_list(s->cur_plugins, p, l);
   else
     s->cur_plugins = eina_list_append(s->cur_plugins, p);
}

static Eina_Bool
_evry_cb_selection_notify(void *data, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Selection_Notify *ev;
   Evry_Window *win = data;
   Evry_State *s = (CUR_SEL)->state;

   if (!s || (data != win)) return ECORE_CALLBACK_PASS_ON;
   if (!win->request_selection) return ECORE_CALLBACK_PASS_ON;

   win->request_selection = EINA_FALSE;

   ev = event;
   if ((ev->selection == ECORE_X_SELECTION_CLIPBOARD) ||
       (ev->selection == ECORE_X_SELECTION_PRIMARY))
     {
	if (strcmp(ev->target, ECORE_X_SELECTION_TARGET_UTF8_STRING) == 0)
	  {
	     Ecore_X_Selection_Data_Text *text_data;

	     text_data = ev->data;

	     strncat(s->input, text_data->text, (INPUTLEN - strlen(s->input)) - 1);
	     _evry_update(CUR_SEL, 1);
	  }
     }

   return ECORE_CALLBACK_PASS_ON;
}
