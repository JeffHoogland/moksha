#include <e.h>
#include "e_mod_main.h"

typedef struct _Instance Instance;

struct _Instance
{
  E_Gadcon_Client *gcc;

  E_Toolbar *tbar;

  Evas_Object *o_base, *o_box, *o_fm, *o_scroll;

  // buttons
  Eina_List *l_buttons;
  Eina_List *history, *current;
  int ignore_dir;

  const char *theme;

  Ecore_Idle_Enterer *idler;
};


/* local function protos */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *client_class);
static void _cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _cb_key_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _cb_back_click(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _cb_forward_click(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _cb_up_click(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _cb_refresh_click(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _cb_favorites_click(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _cb_changed(void *data, Evas_Object *obj, void *event_info);
static void _cb_dir_changed(void *data, Evas_Object *obj, void *event_info);
static void _cb_button_click(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _box_button_append(Instance *inst, const char *label, void (*func)(void *data, Evas_Object *obj, const char *emission, const char *source));

static Eina_List *instances = NULL;
static const char *_nav_mod_dir = NULL;

/* local gadcon functions */
static const E_Gadcon_Client_Class _gc_class =
{
   GADCON_CLIENT_CLASS_VERSION, "efm_navigation",
     {
        _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
          e_gadcon_site_is_efm_toolbar
     }, E_GADCON_CLIENT_STYLE_PLAIN
};

Eina_Bool
e_fwin_nav_init(void)
{
   e_gadcon_provider_register(&_gc_class);

   return EINA_TRUE;
}

Eina_Bool
e_fwin_nav_shutdown(void)
{
   e_gadcon_provider_unregister(&_gc_class);

   return EINA_TRUE;
}

static void _cb_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Instance *inst;
   int w, h;

   inst = data;
   evas_object_geometry_get(inst->gcc->gadcon->o_container, 0, 0, &w, &h);
   e_gadcon_client_min_size_set(inst->gcc, w, h);
   e_gadcon_client_aspect_set(inst->gcc, w, h);
}

static Eina_Bool
_cb_initial_dir(void *data)
{
   Instance *inst = data;

   _cb_dir_changed(inst, NULL, NULL);

   return ECORE_CALLBACK_CANCEL;
}

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Instance *inst = NULL;
   char buf[PATH_MAX];
   int w, h;
   E_Toolbar *tbar;
   Eina_List *l;
   Evas_Object *o_fm;

   tbar = e_gadcon_toolbar_get(gc);
   if (!tbar) return NULL;

   o_fm = e_toolbar_fm2_get(tbar);
   if (!o_fm) return NULL;

   /* make sure only one instance exists in toolbar */
   EINA_LIST_FOREACH(instances, l, inst)
     if (inst->tbar == tbar) return NULL;

   inst = E_NEW(Instance, 1);
   if (!inst) return NULL;

   inst->tbar = tbar;
   inst->o_fm = o_fm;

   snprintf(buf, sizeof(buf), "%s/e-module-efm_nav.edj", _nav_mod_dir);
   inst->theme = eina_stringshare_add(buf);

   inst->o_base = edje_object_add(gc->evas);
   e_theme_edje_object_set(inst->o_base, "base/theme/modules/efm_navigation",
			   "modules/efm_navigation/main");

   edje_object_signal_callback_add(inst->o_base, "e,action,back,click", "",
				   _cb_back_click, inst);
   edje_object_signal_callback_add(inst->o_base, "e,action,forward,click", "",
				   _cb_forward_click, inst);
   edje_object_signal_callback_add(inst->o_base, "e,action,up,click", "",
				   _cb_up_click, inst);
   edje_object_signal_callback_add(inst->o_base, "e,action,refresh,click", "",
				   _cb_refresh_click, inst);
   edje_object_signal_callback_add(inst->o_base, "e,action,favorites,click", "",
				   _cb_favorites_click, inst);
   evas_object_show(inst->o_base);

   inst->o_scroll = e_scrollframe_add(gc->evas);
   e_scrollframe_custom_theme_set(inst->o_scroll,
				       "base/theme/modules/efm_navigation",
				  "modules/efm_navigation/pathbar_scrollframe");

   e_scrollframe_single_dir_set(inst->o_scroll, 1);
   e_scrollframe_policy_set(inst->o_scroll, E_SCROLLFRAME_POLICY_AUTO,
			    E_SCROLLFRAME_POLICY_OFF);
   e_scrollframe_thumbscroll_force(inst->o_scroll, 1);
   evas_object_show(inst->o_scroll);

   inst->o_box = e_box_add(gc->evas);
   e_box_orientation_set(inst->o_box, 1);
   e_box_homogenous_set(inst->o_box, 0);
   e_scrollframe_child_set(inst->o_scroll, inst->o_box);
   evas_object_show(inst->o_box);

   edje_object_part_swallow(inst->o_base, "e.swallow.pathbar", inst->o_scroll);

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->o_base);
   inst->gcc->data = inst;

   /* add the hooks to get signals from efm */
   evas_object_event_callback_add(inst->o_fm, EVAS_CALLBACK_KEY_DOWN,
				  _cb_key_down, inst);
   evas_object_smart_callback_add(inst->o_fm, "changed",
				  _cb_changed, inst);
   evas_object_smart_callback_add(inst->o_fm, "dir_changed",
				  _cb_dir_changed, inst);

   evas_object_event_callback_add(inst->o_base,
				  EVAS_CALLBACK_MOUSE_DOWN,
				  _cb_mouse_down, inst);
   if (!inst->gcc->resizable)
     {
	evas_object_geometry_get(inst->gcc->gadcon->o_container, 0, 0, &w, &h);
	e_gadcon_client_min_size_set(inst->gcc, w, h);
	e_gadcon_client_aspect_set(inst->gcc, w, h);
	evas_object_event_callback_add(inst->gcc->gadcon->o_container,
				       EVAS_CALLBACK_RESIZE,
				       _cb_resize, inst);
     }

   edje_object_signal_emit(inst->o_base, "e,state,back,disabled", "e");
   edje_object_signal_emit(inst->o_base, "e,state,forward,disabled", "e");
   edje_object_message_signal_process(inst->o_base);

   instances = eina_list_append(instances, inst);

   inst->idler = ecore_idle_enterer_add(_cb_initial_dir, inst);

   return inst->gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst = NULL;
   const char *s;
   Evas_Object *btn;

   inst = gcc->data;
   if (!inst) return;
   instances = eina_list_remove(instances, inst);

   evas_object_event_callback_del_full(inst->o_fm,
				       EVAS_CALLBACK_KEY_DOWN,
				       _cb_key_down, inst);

   evas_object_smart_callback_del(inst->o_fm, "changed", _cb_changed);
   evas_object_smart_callback_del(inst->o_fm, "dir_changed", _cb_dir_changed);

   EINA_LIST_FREE(inst->history, s)
     eina_stringshare_del(s);

   if (gcc->gadcon->o_container)
     evas_object_event_callback_del_full(gcc->gadcon->o_container,
					 EVAS_CALLBACK_RESIZE,
					 _cb_resize, inst);

   EINA_LIST_FREE(inst->l_buttons, btn)
     {
        e_box_unpack(btn);
        evas_object_del(btn);
     }

   if (inst->o_base) evas_object_del(inst->o_base);
   if (inst->o_box) evas_object_del(inst->o_box);
   if (inst->o_scroll) evas_object_del(inst->o_scroll);

   eina_stringshare_del(inst->theme);

   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   Instance *inst;

   inst = gcc->data;
   switch (orient)
     {
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_BOTTOM:
	 /* e_gadcon_client_aspect_set(gcc, 16 * 4, 16); */
	break;
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_RIGHT:
	/* e_gadcon_client_aspect_set(gcc, 16, 16 * 4); */
	break;
      default:
	break;
     }
   /* e_gadcon_client_min_size_set(gcc, 16, 16); */
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("EFM Navigation");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   return e_util_icon_theme_icon_add("system-file-manager", 48, evas);
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf), "%s.%d", _gc_class.name,
	    (eina_list_count(instances) + 1));
   return strdup(buf);
}

/* local functions */
static void
_cb_key_down(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info)
{
   Instance *inst;
   Evas_Event_Key_Down *ev;

   inst = data;
   ev = event_info;
   if (evas_key_modifier_is_set(ev->modifiers, "Alt"))
     {
        if (!strcmp(ev->key, "Left"))
          _cb_back_click(inst, obj, "e,action,click", "e");
        else if (!strcmp(ev->key, "Right"))
          _cb_forward_click(inst, obj, "e,action,click", "e");
        else if (!strcmp(ev->key, "Up"))
          _cb_up_click(inst, obj, "e,action,click", "e");
     }
   else if (evas_key_modifier_is_set(ev->modifiers, "Control"))
     {
        if (!strcmp(ev->key, "r"))
          _cb_refresh_click(inst, obj, "e,action,click", "e");
     }
}

static void
_cb_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Instance *inst;
   Evas_Event_Mouse_Down *ev;
   E_Menu *m;
   E_Zone *zone;
   int x, y;

   inst = data;
   ev = event_info;
   if ((ev->button != 3) || (inst->gcc->menu)) return;
   zone = e_util_zone_current_get(e_manager_current_get());

   m = e_menu_new();
   m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
   ecore_x_pointer_xy_get(zone->container->win, &x, &y);
   e_menu_activate_mouse(m, zone, x, y, 1, 1,
			 E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
}

static void
_cb_back_click(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Instance *inst;

   inst = data;

   if ((!inst->current) || (inst->current == eina_list_last(inst->history))) return;
   inst->current = eina_list_next(inst->current);

   inst->ignore_dir = 1;
   e_fm2_path_set(inst->o_fm, eina_list_data_get(inst->current), "/");
}

static void
_cb_forward_click(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Instance *inst;

   inst = data;

   if ((!inst->current) || (inst->current == inst->history)) return;
   inst->current = eina_list_prev(inst->current);

   inst->ignore_dir = 1;
   e_fm2_path_set(inst->o_fm, eina_list_data_get(inst->current), "/");
}

static void
_cb_refresh_click(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Instance *inst;

   inst = data;

   // Don't destroy forward history when refreshing
   inst->ignore_dir = 1;
   e_fm2_path_set(inst->o_fm, NULL, e_fm2_real_path_get(inst->o_fm));
}

static void
_cb_up_click(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Instance *inst;
   char *p, *t;

   inst = data;

   t = strdup(e_fm2_real_path_get(inst->o_fm));
   p = strrchr(t, '/');
   if (p)
     {
        *p = 0;
        p = t;
        if (p[0] == 0) p = "/";
        e_fm2_path_set(inst->o_fm, NULL, p);
     }
   else
     {
        edje_object_signal_emit(inst->o_base, "e,state,up,disabled", "e");
     }

   free(t);
}

static void
_cb_favorites_click(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Instance *inst;

   inst = data;

   e_fm2_path_set(inst->o_fm, "favorites", "/");
}

static void
_cb_changed(void *data, Evas_Object *obj __UNUSED__, void *event_info)
{
   Instance *inst;

   inst = data;
   inst->tbar = event_info;
}

static void
_cb_button_click(void *data, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Instance *inst = data;
   Eina_List *l;
   Evas_Object *btn;
   char path[PATH_MAX] = "";

   EINA_LIST_FOREACH(inst->l_buttons, l, btn)
     {
        strcat(path, edje_object_part_text_get(btn, "e.text.label"));
        if (btn == obj) break;
        strcat(path, "/");
     }
   e_fm2_path_set(inst->o_fm, "/", path);
}

static void
_box_button_append(Instance *inst, const char *label, void (*func)(void *data, Evas_Object *obj, const char *emission, const char *source))
{
   Evas_Object *o;
   Evas_Coord mw = 0, mh = 0;

   if (!inst || !label || !*label || !func)
     return;

   o = edje_object_add(evas_object_evas_get(inst->o_box));

   e_theme_edje_object_set(o, "base/theme/modules/efm_navigation",
			   "modules/efm_navigation/pathbar_button");

   edje_object_signal_callback_add(o, "e,action,click", "", func, inst);
   edje_object_part_text_set(o, "e.text.label", label);
   edje_object_size_min_calc(o, &mw, &mh);
   e_box_pack_end(inst->o_box, o);
   evas_object_show(o);
   e_box_pack_options_set(o, 1, 0, 0, 0, 0.5, 0.5, mw, mh, 9999, 9999);
   e_box_size_min_get(inst->o_box, &mw, NULL);
   evas_object_geometry_get(inst->o_scroll, NULL, NULL, NULL, &mh);
   evas_object_resize(inst->o_box, mw, mh);

   inst->l_buttons = eina_list_append(inst->l_buttons, o);
}

static void
_cb_dir_changed(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Instance *inst;
   const char *realpath;
   char *path, *dir, *p;
   Eina_List *l, *ll, *sel = NULL;
   Evas_Object *btn;
   int mw, sw, changed = 0;

   inst = data;

   if (!(realpath = e_fm2_real_path_get(inst->o_fm))) return;

   /* update pathbar */
   if (!inst->l_buttons)
     _box_button_append(inst, "/", _cb_button_click);
   
   sel = inst->l_buttons;
   l = eina_list_next(sel);
   p = path = ecore_file_realpath(realpath);

   while (p)
     {
	dir = strsep(&p, "/");

        if (!(*dir)) continue;

	if (l && (btn = eina_list_data_get(l)))
	  {
	     if (strcmp(dir, edje_object_part_text_get(btn, "e.text.label")))
	       {
		  changed = 1;

		  while (l)
		    {
		       e_box_unpack(btn);
		       evas_object_del(btn);
		       ll = l;
		       l = eina_list_next(l);
		       btn = eina_list_data_get(l);

		       inst->l_buttons =
			 eina_list_remove_list(inst->l_buttons, ll);
		    }
	       }
	     else
	       {
		  if (!p) sel = l;
		  l = eina_list_next(l);
		  continue;
	       }
	  }

	_box_button_append(inst, dir, _cb_button_click);
	if (!p)	sel = eina_list_last(inst->l_buttons);
	changed = 1;
     }

   free(path);

   if (changed)
     {
	evas_object_geometry_get(inst->o_box, NULL, NULL, &mw, NULL);
	edje_object_size_min_calc(e_scrollframe_edje_object_get(inst->o_scroll), &sw, NULL);

	evas_object_size_hint_max_set(inst->o_scroll, mw + sw, 32);
     }

   EINA_LIST_FOREACH(inst->l_buttons, l, btn)
     if (l == sel)
       edje_object_signal_emit(btn, "e,state,selected", "e");
     else
       edje_object_signal_emit(btn, "e,state,default", "e");

   /* scroll to selected button */
   if (sel)
     {
        Evas_Coord x, y, w, h, xx, yy, ww = 1;
        btn = eina_list_data_get(sel);
        evas_object_geometry_get(btn, &x, &y, &w, &h);

	/* show buttons around selected */
	if (sel->next)
	  {
	     btn = eina_list_data_get(sel->next);
	     evas_object_geometry_get(btn, NULL, NULL, &ww, NULL);
	     w += ww;
	  }
	if (sel->prev)
	  {
	     btn = eina_list_data_get(sel->prev);
	     evas_object_geometry_get(btn, NULL, NULL, &ww, NULL);
	     x -= ww;
	     w += ww;
	  }

	evas_object_geometry_get(inst->o_box, &xx, &yy, NULL, NULL);
        e_scrollframe_child_region_show(inst->o_scroll, x - xx, y - yy, w, h);
     }

   /* update history */
   if ((!inst->ignore_dir) && (eina_list_data_get(inst->current) != realpath))
     {
	if (inst->current)
	  {
	     while (inst->history != inst->current)
	       {
	     	  eina_stringshare_del(eina_list_data_get(inst->history));
	     	  inst->history =
		    eina_list_remove_list(inst->history, inst->history);
	       }
	  }
	inst->history =
	  eina_list_prepend(inst->history, eina_stringshare_ref(realpath));
	inst->current = inst->history;
     }
   inst->ignore_dir = 0;

   if (!strcmp(realpath, "/"))
     edje_object_signal_emit(inst->o_base, "e,state,up,disabled", "e");
   else
     edje_object_signal_emit(inst->o_base, "e,state,up,enabled", "e");

   if ((!inst->history) || (eina_list_last(inst->history) == inst->current))
     edje_object_signal_emit(inst->o_base, "e,state,back,disabled", "e");
   else
     edje_object_signal_emit(inst->o_base, "e,state,back,enabled", "e");

   if ((!inst->history) || (inst->history == inst->current))
     edje_object_signal_emit(inst->o_base, "e,state,forward,disabled", "e");
   else
     edje_object_signal_emit(inst->o_base, "e,state,forward,enabled", "e");
}
