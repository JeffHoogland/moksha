#include "e.h"
#include "e_mod_main.h"

typedef struct _Instance Instance;

struct _Instance
{
  EINA_INLIST;

  E_Gadcon_Client *gcc;
  Evas_Object     *o_button;

  E_Object_Delfn *del_fn;
  Evry_Window *win;
  Gadget_Config *cfg;
  E_Config_Dialog *cfd;
  E_Menu *menu;

  int mouse_down;

  Ecore_Animator *hide_animator;
  double hide_start;
  int hide_x, hide_y;

  Eina_List *handlers;

  Eina_Bool hidden;
  Eina_Bool illume_mode;
};

/* static void _button_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info); */
static void _button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static Eina_Bool _cb_focus_out(void *data, int type __UNUSED__, void *event);

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *client_class);
static Gadget_Config *_conf_item_get(const char *id);

static void _conf_dialog(Instance *inst);
static Eina_Inlist *instances = NULL;

static const E_Gadcon_Client_Class _gadcon_class =
{
    GADCON_CLIENT_CLASS_VERSION,
    "evry-starter",
    {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
      e_gadcon_site_is_not_toolbar
    },
    E_GADCON_CLIENT_STYLE_PLAIN
};

static Eina_Bool
_illume_running()
{
   /* hack to find out out if illume is running, dont grab if
      this is the case... */

   Eina_List *l;
   E_Module *m;

   EINA_LIST_FOREACH(e_module_list(), l, m)
     if (!strcmp(m->name, "illume2") && m->enabled)
       return EINA_TRUE;

   return EINA_FALSE;
}

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;
   Evry_Plugin *p;

   inst = E_NEW(Instance, 1);
   inst->cfg = _conf_item_get(id);

   o = edje_object_add(gc->evas);
   e_theme_edje_object_set(o, "base/theme/modules/everything", "e/modules/everything/gadget");
   
   if ((p = evry_plugin_find(inst->cfg->plugin)))
     {
	Evas_Object *oo = evry_util_icon_get(EVRY_ITEM(p), gc->evas);
	if (oo) edje_object_part_swallow(o, "e.swallow.icon", oo); 
     }

   edje_object_signal_emit(o, "e,state,unfocused", "e");

   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;

   inst->gcc = gcc;
   inst->o_button = o;

   /* evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP,
    * 				  _button_cb_mouse_up, inst); */

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  _button_cb_mouse_down, inst);


   if (_illume_running())
     {
	inst->illume_mode = EINA_TRUE;

	inst->handlers = eina_list_append(inst->handlers,
					  ecore_event_handler_add(E_EVENT_BORDER_FOCUS_OUT,
								  _cb_focus_out, inst));
     }
   
   instances = eina_inlist_append(instances, EINA_INLIST_GET(inst));

   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;
   Ecore_Event_Handler *h;
   
   inst = gcc->data;

   instances = eina_inlist_remove(instances, EINA_INLIST_GET(inst));

   EINA_LIST_FREE(inst->handlers, h)
     ecore_event_handler_del(h);

   if (inst->del_fn && inst->win)
     {
	e_object_delfn_del(E_OBJECT(inst->win->ewin), inst->del_fn);
	evry_hide(inst->win, 0);
     }
   
   evas_object_del(inst->o_button);
   free(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   Instance *inst;
   Evas_Coord mw, mh;

   inst = gcc->data;
   mw = 0, mh = 0;
   edje_object_size_min_get(inst->o_button, &mw, &mh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(inst->o_button, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   e_gadcon_client_aspect_set(gcc, mw, mh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("Everything Starter");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas __UNUSED__)
{
   /* Evas_Object *o; */
   /* char buf[4096];
    *
    * o = edje_object_add(evas);
    * snprintf(buf, sizeof(buf), "%s/e-module-start.edj",
    * 	    e_module_dir_get(start_module));
    * edje_object_file_set(o, buf, "icon"); */
   return NULL;
}

static Gadget_Config *
_conf_item_get(const char *id)
{
   Gadget_Config *ci;

   GADCON_CLIENT_CONFIG_GET(Gadget_Config, evry_conf->gadgets, _gadcon_class, id);

   ci = E_NEW(Gadget_Config, 1);
   ci->id = eina_stringshare_add(id);

   evry_conf->gadgets = eina_list_append(evry_conf->gadgets, ci);

   e_config_save_queue();

   return ci;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   Gadget_Config *gc = NULL;

   gc = _conf_item_get(NULL);

   return gc->id;
}

/***************************************************************************/


static void _del_func(void *data, void *obj __UNUSED__)
{
   Instance *inst = data;

   e_gadcon_locked_set(inst->gcc->gadcon, 0);
   e_object_delfn_del(E_OBJECT(inst->win->ewin), inst->del_fn);

   if (inst->hide_animator) ecore_animator_del(inst->hide_animator);
   inst->del_fn = NULL;
   inst->win = NULL;
   edje_object_signal_emit(inst->o_button, "e,state,unfocused", "e");
}

static void
_cb_menu_post(void *data, E_Menu *m __UNUSED__)
{
   Instance *inst = data;

   if (!inst->menu) return;
   /* e_object_del(E_OBJECT(inst->menu)); */
   inst->menu = NULL;
}

static void
_cb_menu_configure(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   _conf_dialog(data);
}

static Eina_Bool
_hide_animator(void *data)
{
   Instance *inst = data;
   E_Win *ewin = inst->win->ewin;
   double val;
   int finished = 0;

   if (!inst->hide_start)
     inst->hide_start = ecore_loop_time_get();

   val = (ecore_loop_time_get() - inst->hide_start) / 0.4;
   if (val > 0.99) finished = 1;

   val = ecore_animator_pos_map(val, ECORE_POS_MAP_DECELERATE, 0.0, 0.0);

   e_border_fx_offset(ewin->border,
		      (val * (inst->hide_x * ewin->w)),
		      (val * (inst->hide_y * ewin->h)));

   if (finished)
     {
	/* go bac to subject selector */
	evry_selectors_switch(inst->win, -1, 0);
	evry_selectors_switch(inst->win, -1, 0);

	inst->hide_animator = NULL;
	e_border_iconify(ewin->border);
	e_border_fx_offset(ewin->border, 0, 0);
	return EINA_FALSE;
     }

   return EINA_TRUE;
}

static void
_evry_hide_func(Evry_Window *win, int finished __UNUSED__)
{
   Instance *inst = win->data;

   inst->hide_start = 0;
   inst->hide_animator = ecore_animator_add(_hide_animator, inst);
   inst->hidden = EINA_TRUE;
}

static Eina_Bool
_cb_focus_out(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Focus_Out *ev;
   Instance *inst;

   ev = event;

   EINA_INLIST_FOREACH(instances, inst)
     if (inst == data) break;

   if ((!inst) || (!inst->win))
     return ECORE_CALLBACK_PASS_ON;

   if (inst->hide_animator)
     return ECORE_CALLBACK_PASS_ON;

   if (inst->win->ewin->border != ev->border)
     return ECORE_CALLBACK_PASS_ON;

   _evry_hide_func(inst->win, 0);

   return ECORE_CALLBACK_PASS_ON;
}
static void
_gadget_popup_show(Instance *inst)
{
   Evas_Coord x, y, w, h;
   int cx, cy, pw, ph;
   E_Win *ewin = inst->win->ewin;

   pw = ewin->w;
   ph = ewin->h;

   evas_object_geometry_get(inst->o_button, &x, &y, &w, &h);
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy, NULL, NULL);
   x += cx;
   y += cy;

   switch (inst->gcc->gadcon->orient)
     {
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
	 e_win_move(ewin, x, y + h);
	 inst->hide_y = -1;
	 break;
      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_BR:
      case E_GADCON_ORIENT_CORNER_BL:
	 e_win_move(ewin, x, y - ph);
	 inst->hide_y = 1;
	 break;
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_CORNER_LB:
	 e_win_move(ewin, x + w, y);
	 inst->hide_x = -1;
	 break;
      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_RT:
      case E_GADCON_ORIENT_CORNER_RB:
	 e_win_move(ewin, x - pw, y);
	 inst->hide_x = 1;
	 break;
      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_VERT:
      default:
	 break;
     }

   if (ewin->x + pw > inst->win->zone->w)
     e_win_move(ewin, inst->win->zone->w - pw, ewin->y);

   if (ewin->y + ph > inst->win->zone->h)
     e_win_move(ewin, ewin->x, inst->win->zone->h - ph);

}

static void
_gadget_window_show(Instance *inst)
{
   int zx, zy, zw, zh;
   int gx, gy, gw, gh;
   int cx, cy;
   int pw, ph;

   E_Win *ewin = inst->win->ewin;

   inst->win->func.hide = &_evry_hide_func;
   
   e_zone_useful_geometry_get(inst->win->zone, &zx, &zy, &zw, &zh);

   evas_object_geometry_get(inst->o_button, &gx, &gy, &gw, &gh);
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy, NULL, NULL);
   gx += cx;
   gy += cy;

   switch (inst->gcc->gadcon->orient)
     {
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
	 pw = zw/2;
	 ph = zh/2;
	 inst->hide_y = -1;
	 e_win_move(ewin, zx, gy + gh);
	 break;
      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_BR:
      case E_GADCON_ORIENT_CORNER_BL:
	 pw = zw/2;
	 ph = zh/2;
	 inst->hide_y = 1;
	 e_win_move(ewin, zx, gy - ph);
	 break;
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_CORNER_LB:
	 pw = zw/2.5;
	 ph = zh;
	 inst->hide_x = -1;
	 e_win_move(ewin, gx + gw, zy);
	 break;
      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_RT:
      case E_GADCON_ORIENT_CORNER_RB:
	 pw = zw/2.5;
	 ph = zh;
	 inst->hide_x = 1;
	 e_win_move(ewin, gx - pw, zy);
	 break;
      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_VERT:
      default:
	 break;
     }

   e_win_resize(ewin, pw, ph);
   e_win_show(ewin);

   e_border_focus_set(ewin->border, 1, 1);
   ewin->border->client.netwm.state.skip_pager = 1;
   ewin->border->client.netwm.state.skip_taskbar = 1;
   ewin->border->sticky = 1;

   inst->hidden = EINA_FALSE;
}

static void
_button_cb_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Instance *inst;
   Evas_Event_Mouse_Down *ev;

   inst = data;
   /* if (!inst->mouse_down)
    *   return; */

   inst->mouse_down = 0;

   ev = event_info;
   if (ev->button == 1)
     {
	Evry_Window *win;
	E_Border *bd;

	if (inst->win)
	  {
	     win = inst->win;
	     bd = win->ewin->border;

	     if (inst->hide_animator)
	       {
		  ecore_animator_del(inst->hide_animator);
		  inst->hide_animator = NULL;
	       }

	     if (inst->hidden || !bd->focused)
	       {
		  e_border_fx_offset(bd, 0, 0);
 		  e_border_uniconify(bd);
		  e_border_raise(bd);
	     	  e_border_focus_set(bd, 1, 1);
		  inst->hidden = EINA_FALSE;
		  return;
	       }
	     else
	       {
	     	  evry_hide(win, 1);
		  return;
	       }
	  }

	win = evry_show(e_util_zone_current_get(e_manager_current_get()),
			0, inst->cfg->plugin, !inst->illume_mode);
	if (!win) return;

	inst->win = win;
	win->data = inst;

	ecore_evas_name_class_set(win->ewin->ecore_evas, "E", "everything-window");

	if (inst->illume_mode)
	  _gadget_window_show(inst);
	else
	  _gadget_popup_show(inst);

	e_gadcon_locked_set(inst->gcc->gadcon, 1);

	inst->del_fn = e_object_delfn_add(E_OBJECT(win->ewin), _del_func, inst);

	edje_object_signal_emit(inst->o_button, "e,state,focused", "e");
     }
   else if ((ev->button == 3) && (!inst->menu))
     {
	E_Menu *m;
	E_Menu_Item *mi;
	int cx, cy;

	m = e_menu_new();
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Settings"));
	e_util_menu_item_theme_icon_set(mi, "configure");
	e_menu_item_callback_set(mi, _cb_menu_configure, inst);

	m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
	e_menu_post_deactivate_callback_set(m, _cb_menu_post, inst);
	inst->menu = m;

	e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy,
                                          NULL, NULL);
	e_menu_activate_mouse(m,
			      e_util_zone_current_get(e_manager_current_get()),
			      cx + ev->output.x, cy + ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
     }
}

/* static void
 * _button_cb_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
 * {
 *    Instance *inst;
 *    Evas_Event_Mouse_Down *ev;
 * 
 *    inst = data;
 *    inst->mouse_down = 1;
 * } */


int
evry_gadget_init(void)
{
   e_gadcon_provider_register(&_gadcon_class);
   return 1;
}

void
evry_gadget_shutdown(void)
{
   e_gadcon_provider_unregister(&_gadcon_class);
}

/***************************************************************************/

struct _E_Config_Dialog_Data
{
  char *plugin;
  int hide_after_action;
  int popup;
};

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

static void
_conf_dialog(Instance *inst)
{
   E_Config_Dialog_View *v = NULL;
   E_Container *con;

   if (inst->cfd)
     return;

   /* if (e_config_dialog_find("everything-gadgets", "launcher/everything-gadgets"))
    *   return; */

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   con = e_container_current_get(e_manager_current_get());
   inst->cfd = e_config_dialog_new(con, _("Everything Gadgets"), "everything-gadgets",
				   "launcher/everything-gadgets", NULL, 0, v, inst);

   e_dialog_resizable_set(inst->cfd->dia, 0);
   /* _conf->cfd = cfd; */
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata = NULL;
   Instance *inst = cfd->data;
   Gadget_Config *gc = inst->cfg;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);

#define CP(_name) cfdata->_name = (gc->_name ? strdup(gc->_name) : NULL);
#define C(_name) cfdata->_name = gc->_name;
   CP(plugin);
   C(hide_after_action);
   C(popup);
#undef CP
#undef C

   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Instance *inst = cfd->data;

   inst->cfd = NULL;
   if (cfdata->plugin) free(cfdata->plugin);
   E_FREE(cfdata);
}

static void
_cb_button_settings(void *data __UNUSED__, void *data2 __UNUSED__)
{
   /* evry_collection_conf_dialog(e_container_current_get(e_manager_current_get()), "Start"); */
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *e, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o = NULL, *of = NULL, *ow = NULL;
   Instance *inst = cfd->data;

   o = e_widget_list_add(e, 0, 0);

   of = e_widget_framelist_add(e, _("Plugin"), 0);
   ow = e_widget_entry_add(e, &(cfdata->plugin), NULL, NULL, NULL);
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_button_add(e, _("Settings"), NULL, _cb_button_settings, inst, NULL);
   e_widget_framelist_object_append(of, ow);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Instance *inst = cfd->data;
   Gadget_Config *gc = inst->cfg;

#define CP(_name)					\
   if (gc->_name)					\
     eina_stringshare_del(gc->_name);			\
   gc->_name = eina_stringshare_add(cfdata->_name);
#define C(_name) gc->_name = cfdata->_name;
   eina_stringshare_del(gc->plugin);			\
   if (cfdata->plugin[0])
     gc->plugin = eina_stringshare_add(cfdata->plugin);
   else
     gc->plugin = NULL;
   C(hide_after_action);
   C(popup);
#undef CP
#undef C

   e_config_save_queue();

   return 1;
}
