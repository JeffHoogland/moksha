/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "e.h"
#include "e_mod_main.h"

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *client_class);
/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
     "pager",
     {
	_gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL
     },
   E_GADCON_CLIENT_STYLE_INSET
};

/* actual module specifics */
typedef struct _Instance Instance;
typedef struct _Pager       Pager;
typedef struct _Pager_Desk  Pager_Desk;
typedef struct _Pager_Win   Pager_Win;
typedef struct _Pager_Popup Pager_Popup;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_pager; /* table */
   Pager           *pager;
};

struct _Pager
{
   Instance       *inst;
   E_Drop_Handler *drop_handler;
   Pager_Popup    *popup;
   Evas_Object    *o_table;
   E_Zone         *zone;
   int             xnum, ynum;
   Eina_List      *desks;
   unsigned char   dragging : 1;
   unsigned char   just_dragged : 1;
   Evas_Coord      dnd_x, dnd_y;
   Pager_Desk     *active_drop_pd;
};

struct _Pager_Desk
{
   Pager           *pager;
   E_Desk          *desk;
   Eina_List       *wins;
   Evas_Object     *o_desk;
   Evas_Object     *o_layout;
   int              xpos, ypos;
   int              current : 1;
   int              urgent;
   struct {
      Pager         *from_pager;
      unsigned char  in_pager : 1;
      unsigned char  start : 1;
      int            x, y;
      int            dx, dy;
      int            button;
   } drag;
};

struct _Pager_Win
{
   E_Border     *border;
   Pager_Desk   *desk;
   Evas_Object  *o_window;
   Evas_Object  *o_icon;
   unsigned char skip_winlist : 1;
   struct {
      Pager         *from_pager;
      unsigned char  start : 1;
      unsigned char  in_pager : 1;
      unsigned char  no_place : 1;
      unsigned char  desktop  : 1;
      int            x, y;
      int            dx, dy;
      int            button;
   } drag;
};

struct _Pager_Popup
{
   E_Popup      *popup;
   Pager        *pager;
   Evas_Object  *o_bg;
   Ecore_Timer  *timer;
   unsigned char urgent : 1;
};

static void _pager_cb_obj_moveresize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _menu_cb_post(void *data, E_Menu *m);
static void _pager_inst_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi);
static void _pager_inst_cb_menu_virtual_desktops_dialog(void *data, E_Menu *m, E_Menu_Item *mi);
static void _pager_instance_drop_zone_recalc(Instance *inst);
static int _pager_cb_event_border_resize(void *data, int type, void *event);
static int _pager_cb_event_border_move(void *data, int type, void *event);
static int _pager_cb_event_border_add(void *data, int type, void *event);
static int _pager_cb_event_border_remove(void *data, int type, void *event);
static int _pager_cb_event_border_iconify(void *data, int type, void *event);
static int _pager_cb_event_border_uniconify(void *data, int type, void *event);
static int _pager_cb_event_border_stick(void *data, int type, void *event);
static int _pager_cb_event_border_unstick(void *data, int type, void *event);
static int _pager_cb_event_border_desk_set(void *data, int type, void *event);
static int _pager_cb_event_border_stack(void *data, int type, void *event);
static int _pager_cb_event_border_icon_change(void *data, int type, void *event);
static int _pager_cb_event_border_urgent_change(void *data, int type, void *event);
static int _pager_cb_event_border_focus_in(void *data, int type, void *event);
static int _pager_cb_event_border_focus_out(void *data, int type, void *event);
static int _pager_cb_event_border_property(void *data, int type, void *event);
static int _pager_cb_event_zone_desk_count_set(void *data, int type, void *event);
static int _pager_cb_event_desk_show(void *data, int type, void *event);
static int _pager_cb_event_desk_name_change(void *data, int type, void *event);
static int _pager_cb_event_container_resize(void *data, int type, void *event);
static void _pager_window_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _pager_window_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _pager_window_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _pager_window_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _pager_window_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void *_pager_window_cb_drag_convert(E_Drag *drag, const char *type);
static void _pager_window_cb_drag_finished(E_Drag *drag, int dropped);
static void _pager_drop_cb_enter(void *data, const char *type, void *event_info);
static void _pager_drop_cb_move(void *data, const char *type, void *event_info);
static void _pager_drop_cb_leave(void *data, const char *type, void *event_info);
static void _pager_drop_cb_drop(void *data, const char *type, void *event_info);
static void _pager_inst_cb_scroll(void *data);
static void _pager_update_drop_position(Pager *p, Evas_Coord x, Evas_Coord y);
static void _pager_desk_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _pager_desk_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _pager_desk_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _pager_desk_cb_drag_finished(E_Drag *drag, int dropped);
static void _pager_desk_cb_mouse_wheel(void *data, Evas *e, Evas_Object *obj, void *event_info);
static int _pager_popup_cb_timeout(void *data);
static Pager *_pager_new(Evas *evas, E_Zone *zone);
static void _pager_free(Pager *p);
static void _pager_fill(Pager *p);
static void _pager_empty(Pager *p);
static Pager_Desk *_pager_desk_new(Pager *p, E_Desk *desk, int xpos, int ypos);
static void _pager_desk_free(Pager_Desk *pd);
static Pager_Desk *_pager_desk_at_coord(Pager *p, Evas_Coord x, Evas_Coord y);
static void _pager_desk_select(Pager_Desk *pd);
static Pager_Desk *_pager_desk_find(Pager *p, E_Desk *desk);
static void _pager_desk_switch(Pager_Desk *pd1, Pager_Desk *pd2);
static Pager_Win *_pager_window_new(Pager_Desk *pd, E_Border *border);
static void _pager_window_free(Pager_Win *pw);
static void _pager_window_move(Pager_Win *pw);
static Pager_Win *_pager_window_find(Pager *p, E_Border *border);
static Pager_Win *_pager_desk_window_find(Pager_Desk *pd, E_Border *border);
static Pager_Popup *_pager_popup_new(E_Zone *zone, int keyaction);
static void _pager_popup_free(Pager_Popup *pp);
static Pager_Popup *_pager_popup_find(E_Zone *zone);
static E_Config_Dialog *_pager_config_dialog(E_Container *con, const char *params);

/* functions for pager popup on key actions */
static int _pager_popup_show();
static void _pager_popup_hide(int switch_desk);
static int _pager_popup_cb_mouse_down(void *data, int type, void *event);
static int _pager_popup_cb_mouse_up(void *data, int type, void *event);
static int _pager_popup_cb_mouse_move(void *data, int type, void *event);
static int _pager_popup_cb_mouse_wheel(void *data, int type, void *event);
static void _pager_popup_desk_switch(int x, int y);
static void _pager_popup_modifiers_set(int mod);
static int _pager_popup_cb_key_down(void *data, int type, void *event);
static int _pager_popup_cb_key_up(void *data, int type, void *event);
static void _pager_popup_cb_action_show(E_Object *obj, const char *params, Ecore_Event_Key *ev);
static void _pager_popup_cb_action_switch(E_Object *obj, const char *params, Ecore_Event_Key *ev);

/* variables for pager popup on key actions */
static E_Action *act_popup_show = NULL;
static E_Action *act_popup_switch = NULL;
static Ecore_X_Window input_window = 0;
static Eina_List *handlers = NULL;
static Pager_Popup *act_popup = NULL; /* active popup */
static int hold_count = 0;
static int hold_mod = 0;
static E_Desk *current_desk = NULL;

static E_Config_DD *conf_edd = NULL;

static Eina_List *pagers = NULL;

Config *pager_config = NULL;


static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Pager *p;
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;
   Evas_Coord x, y, w, h;
   const char *drop[] = { "enlightenment/pager_win", "enlightenment/border", "enlightenment/vdesktop"};

   inst = E_NEW(Instance, 1);

   p = _pager_new(gc->evas, gc->zone);
   p->inst = inst;
   inst->pager = p;
   o = p->o_table;
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;

   inst->gcc = gcc;
   inst->o_pager = o;

   evas_object_geometry_get(o, &x, &y, &w, &h);
   p->drop_handler =
   e_drop_handler_add(E_OBJECT(inst->gcc), p,
			_pager_drop_cb_enter, _pager_drop_cb_move,
			_pager_drop_cb_leave, _pager_drop_cb_drop,
			drop, 3, x, y, w, h);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOVE,
				  _pager_cb_obj_moveresize, inst);
   evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE,
				  _pager_cb_obj_moveresize, inst);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  _button_cb_mouse_down, inst);
   pager_config->instances = eina_list_append(pager_config->instances, inst);
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   inst = gcc->data;
   pager_config->instances = eina_list_remove(pager_config->instances, inst);
   e_drop_handler_del(inst->pager->drop_handler);
   _pager_free(inst->pager);
   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   Instance *inst;

   inst = gcc->data;
   e_gadcon_client_aspect_set(gcc,
			      inst->pager->xnum * inst->pager->zone->w,
			      inst->pager->ynum * inst->pager->zone->h);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class)
{
   return _("Pager");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas)
{
   Evas_Object *o;
   char buf[4096];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-pager.edj",
	    e_module_dir_get(pager_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class)
{
   return _gadcon_class.name;
}

static Pager *
_pager_new(Evas *evas, E_Zone *zone)
{
   Pager *p;

   p = E_NEW(Pager, 1);
   p->inst = NULL;
   p->popup = NULL;
   p->o_table = e_table_add(evas);
   e_table_homogenous_set(p->o_table, 1);
   p->zone = zone;
   _pager_fill(p);
   pagers = eina_list_append(pagers, p);
   return p;
}

static void
_pager_free(Pager *p)
{
   _pager_empty(p);
   evas_object_del(p->o_table);
   pagers = eina_list_remove(pagers, p);
   free(p);
}

static void
_pager_fill(Pager *p)
{
   int x, y;

   e_zone_desk_count_get(p->zone, &(p->xnum), &(p->ynum));
   for (x = 0; x < p->xnum; x++)
     {
	for (y = 0; y < p->ynum; y++)
	  {
	     Pager_Desk *pd;
	     E_Desk     *desk;

	     desk = e_desk_at_xy_get(p->zone, x, y);
	     pd = _pager_desk_new(p, desk, x, y);
	     if (pd)
	       {
		  p->desks = eina_list_append(p->desks, pd);
		  if (desk == e_desk_current_get(desk->zone))
		    _pager_desk_select(pd);
	       }
	  }
     }
}

static void
_pager_empty(Pager *p)
{
   while (p->desks)
     {
	_pager_desk_free(p->desks->data);
	p->desks = eina_list_remove_list(p->desks, p->desks);
     }
}

static Pager_Desk *
_pager_desk_new(Pager *p, E_Desk *desk, int xpos, int ypos)
{
   Pager_Desk    *pd;
   Evas_Object   *o;
   E_Border_List *bl;
   E_Border      *bd;

   pd = E_NEW(Pager_Desk, 1);
   if (!pd) return NULL;

   pd->xpos = xpos;
   pd->ypos = ypos;

   pd->urgent = 0;

   pd->desk = desk;
   e_object_ref(E_OBJECT(desk));
   pd->pager = p;

   o = edje_object_add(evas_object_evas_get(p->o_table));
   pd->o_desk = o;
   e_theme_edje_object_set(o, "base/theme/modules/pager",
			   "e/modules/pager/desk");
   if (pager_config->show_desk_names)
     edje_object_part_text_set(o, "e.text.label", desk->name);
   e_table_pack(p->o_table, o, xpos, ypos, 1, 1);
   e_table_pack_options_set(o, 1, 1, 1, 1, 0.5, 0.5, 0, 0, -1, -1);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _pager_desk_cb_mouse_down, pd);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _pager_desk_cb_mouse_up, pd);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _pager_desk_cb_mouse_move, pd);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_WHEEL, _pager_desk_cb_mouse_wheel, pd);

   evas_object_show(o);

   o = e_layout_add(evas_object_evas_get(p->o_table));
   pd->o_layout = o;

   e_layout_virtual_size_set(o, desk->zone->w, desk->zone->h);
   edje_object_part_swallow(pd->o_desk, "e.swallow.content", pd->o_layout);
   evas_object_show(o);

   bl = e_container_border_list_first(desk->zone->container);
   while ((bd = e_container_border_list_next(bl)))
     {
	Pager_Win   *pw;

	if ((bd->new_client) || ((bd->desk != desk) && (!bd->sticky))) continue;
	pw = _pager_window_new(pd, bd);
	if (pw) pd->wins = eina_list_append(pd->wins, pw);
     }
   e_container_border_list_free(bl);
   return pd;
}

static void
_pager_desk_free(Pager_Desk *pd)
{
   Eina_List *l;

   evas_object_del(pd->o_desk);
   evas_object_del(pd->o_layout);
   for (l = pd->wins; l; l = l->next)
     _pager_window_free(l->data);
   eina_list_free(pd->wins);
   e_object_unref(E_OBJECT(pd->desk));
   free(pd);
}

static Pager_Desk *
_pager_desk_at_coord(Pager *p, Evas_Coord x, Evas_Coord y)
{
   Eina_List *l;

   for (l = p->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	Evas_Coord dx, dy, dw, dh;

	pd = l->data;
	evas_object_geometry_get(pd->o_desk, &dx, &dy, &dw, &dh);
	if (E_INSIDE(x, y, dx, dy, dw, dh)) return pd;
     }
   return NULL;
}

static void
_pager_desk_select(Pager_Desk *pd)
{
   Eina_List *l;

   if (pd->current) return;
   for (l = pd->pager->desks; l; l = l->next)
     {
	Pager_Desk *pd2;

	pd2 = l->data;
	if (pd == pd2)
	  {
	     pd2->current = 1;
	     evas_object_raise(pd2->o_desk);
	     edje_object_signal_emit(pd2->o_desk, "e,state,selected", "e");
	  }
	else
	  {
	     if (pd2->current)
	       {
		  pd2->current = 0;
		  edje_object_signal_emit(pd2->o_desk, "e,state,unselected", "e");
	       }
	  }
     }
}

static Pager_Desk *
_pager_desk_find(Pager *p, E_Desk *desk)
{
   Eina_List *l;

   for (l = p->desks; l; l = l->next)
     {
	Pager_Desk *pd;

	pd = l->data;
	if (pd->desk == desk) return pd;
     }
   return NULL;
}

static void
_pager_desk_switch(Pager_Desk *pd1, Pager_Desk *pd2)
{
   int c;
   E_Zone     *zone1, *zone2;
   E_Desk     *desk1, *desk2;
   Pager_Win  *pw;
   Eina_List  *l;

   if ((!pd1) || (!pd2) || (!pd1->desk) || (!pd2->desk)) return;
   if (pd1 == pd2) return;

   desk1 = pd1->desk;
   desk2 = pd2->desk;
   zone1 = pd1->desk->zone;
   zone2 = pd2->desk->zone;

   /* Move opened windows from on desk to the other */
   for (l = pd1->wins; l; l = l->next)
     {
	pw = l->data;
	if (!pw || !pw->border || pw->border->iconic) continue;
	e_border_desk_set(pw->border, desk2);
     }
   for (l = pd2->wins; l; l = l->next)
     {
	pw = l->data;
	if (!pw || !pw->border || pw->border->iconic) continue;
	e_border_desk_set(pw->border, desk1);
     }

   /* Modify desktop names in the config */
   for (l = e_config->desktop_names, c = 0; l && c < 2; l = l->next)
     {
	E_Config_Desktop_Name *tmp_dn;

	tmp_dn = l->data;
	if (!tmp_dn) continue;
	if (tmp_dn->desk_x == desk1->x &&
	    tmp_dn->desk_y == desk1->y &&
	    tmp_dn->zone   == desk1->zone->num)
	  {
	     tmp_dn->desk_x = desk2->x;
	     tmp_dn->desk_y = desk2->y;
	     tmp_dn->zone   = desk2->zone->num;
	     c++;
	  }
	else if (tmp_dn->desk_x == desk2->x &&
		 tmp_dn->desk_y == desk2->y &&
		 tmp_dn->zone   == desk2->zone->num)
	  {
	     tmp_dn->desk_x = desk1->x;
	     tmp_dn->desk_y = desk1->y;
	     tmp_dn->zone   = desk1->zone->num;
	     c++;
	  }
     }
   if (c > 0) e_config_save();
   e_desk_name_update();

   /* Modify desktop backgrounds in the config */
   for (l = e_config->desktop_backgrounds, c = 0; l && c < 2; l = l->next)
     {
	E_Config_Desktop_Background *tmp_db;

	tmp_db = l->data;
	if (!tmp_db) continue;
	if (tmp_db->desk_x == desk1->x &&
	    tmp_db->desk_y == desk1->y &&
	    tmp_db->zone   == desk1->zone->num)
	  {
	     tmp_db->desk_x = desk2->x;
	     tmp_db->desk_y = desk2->y;
	     tmp_db->zone   = desk2->zone->num;
	     c++;
	  }
	else if (tmp_db->desk_x == desk2->x &&
		 tmp_db->desk_y == desk2->y &&
		 tmp_db->zone   == desk2->zone->num)
	  {
	     tmp_db->desk_x = desk1->x;
	     tmp_db->desk_y = desk1->y;
	     tmp_db->zone   = desk1->zone->num;
	     c++;
	  }
     }
   if (c > 0) e_config_save();

   /* If the current desktop has been switched, force to update of the screen */
   if (desk2 == e_desk_current_get(zone2))
     {
	desk2->visible = 0;
	e_desk_show(desk2);
     }
   if (desk1 == e_desk_current_get(zone1))
     {
	desk1->visible = 0;
	e_desk_show(desk1);
     }
}

static Pager_Win  *
_pager_window_new(Pager_Desk *pd, E_Border *border)
{
   Pager_Win   *pw;
   Evas_Object *o;
   int          visible;

   if (!border) return NULL;
   pw = E_NEW(Pager_Win, 1);
   if (!pw) return NULL;

   pw->border = border;
   e_object_ref(E_OBJECT(border));

   visible = !border->iconic && !border->client.netwm.state.skip_pager;
   pw->skip_winlist = border->client.netwm.state.skip_pager;
   pw->desk = pd;

   o = edje_object_add(evas_object_evas_get(pd->pager->o_table));
   pw->o_window = o;
   e_theme_edje_object_set(o, "base/theme/modules/pager",
			   "e/modules/pager/window");
   if (visible) evas_object_show(o);

   e_layout_pack(pd->o_layout, pw->o_window);
   e_layout_child_raise(pw->o_window);

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN,  _pager_window_cb_mouse_in,  pw);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT, _pager_window_cb_mouse_out, pw);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _pager_window_cb_mouse_down, pw);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _pager_window_cb_mouse_up, pw);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _pager_window_cb_mouse_move, pw);

   o = e_border_icon_add(border, evas_object_evas_get(pd->pager->o_table));
   if (o)
     {
	pw->o_icon = o;
	evas_object_show(o);
	edje_object_part_swallow(pw->o_window, "e.swallow.icon", o);
     }

   if (border->client.icccm.urgent)
     {
	if (!(border->iconic))
	  edje_object_signal_emit(pd->o_desk,
				  "e,state,urgent", "e");
	edje_object_signal_emit(pw->o_window,
				"e,state,urgent", "e");
     }

   evas_object_show(o);

   _pager_window_move(pw);
   return pw;
}

static void
_pager_window_free(Pager_Win *pw)
{
   if ((pw->drag.from_pager) && (pw->desk->pager->dragging))
     pw->desk->pager->dragging = 0;
   if (pw->o_window) evas_object_del(pw->o_window);
   if (pw->o_icon) evas_object_del(pw->o_icon);
   e_object_unref(E_OBJECT(pw->border));
   free(pw);
}

static void
_pager_window_move(Pager_Win *pw)
{
   e_layout_child_move(pw->o_window,
		       pw->border->x - pw->desk->desk->zone->x,
		       pw->border->y - pw->desk->desk->zone->y);
   e_layout_child_resize(pw->o_window, pw->border->w, pw->border->h);
}

static Pager_Win *
_pager_window_find(Pager *p, E_Border *border)
{
   Eina_List  *l;

   for (l = p->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	Pager_Win *pw;

	pd = l->data;
	pw = _pager_desk_window_find(pd, border);
	if (pw) return pw;
     }
   return NULL;
}

static Pager_Win *
_pager_desk_window_find(Pager_Desk *pd, E_Border *border)
{
   Eina_List  *l;

   for (l = pd->wins; l; l = l->next)
     {
	Pager_Win  *pw;

	pw = l->data;
	if (pw->border == border) return pw;
     }
   return NULL;
}

static Pager_Popup *
_pager_popup_new(E_Zone *zone, int keyaction)
{
   Pager_Popup *pp;
   Evas_Coord w, h;
   int x, y, height, width;
   E_Desk *desk;

   pp = E_NEW(Pager_Popup, 1);
   if (!pp) return NULL;

   /* Show popup */
   pp->popup = e_popup_new(zone, 0, 0, 1, 1);
   if (!pp->popup)
     {
	free(pp);
	return NULL;
     }
   e_popup_layer_set(pp->popup, 255);

   pp->pager = _pager_new(pp->popup->evas, zone);
   pp->pager->popup = pp;

   pp->urgent = 0;

   e_zone_desk_count_get(zone, &x, &y);

   if (keyaction)
     height = pager_config->popup_act_height * y;
   else
     height = pager_config->popup_height * y;

   width = height * (zone->w * x)/(zone->h * y);

   evas_object_move(pp->pager->o_table, 0, 0);
   evas_object_resize(pp->pager->o_table, width, height);

   pp->o_bg = edje_object_add(pp->popup->evas);
   e_theme_edje_object_set(pp->o_bg,
			   "base/theme/modules/pager",
			   "e/modules/pager/popup");
   desk = e_desk_current_get(zone);
   if (desk)
     edje_object_part_text_set(pp->o_bg, "e.text.label", desk->name);
   evas_object_show(pp->o_bg);

   edje_extern_object_min_size_set(pp->pager->o_table, width, height);
   edje_object_part_swallow(pp->o_bg, "e.swallow.content", pp->pager->o_table);
   edje_object_size_min_calc(pp->o_bg, &w, &h);

   evas_object_move(pp->o_bg, 0, 0);
   evas_object_resize(pp->o_bg, w, h);
   e_popup_edje_bg_object_set(pp->popup, pp->o_bg);
   //e_popup_ignore_events_set(pp->popup, 1);
   e_popup_move_resize(pp->popup, ((zone->w - w) / 2),
		       ((zone->h - h) / 2), w, h);
   e_bindings_mouse_grab(E_BINDING_CONTEXT_POPUP, pp->popup->evas_win);
   e_bindings_wheel_grab(E_BINDING_CONTEXT_POPUP, pp->popup->evas_win);

   e_popup_show(pp->popup);

   pp->timer = NULL;

   return pp;
}

static void
_pager_popup_free(Pager_Popup *pp)
{
   if (pp->timer) ecore_timer_del(pp->timer);
   evas_object_del(pp->o_bg);
   _pager_free(pp->pager);
   e_bindings_mouse_ungrab(E_BINDING_CONTEXT_POPUP, pp->popup->evas_win);
   e_bindings_wheel_ungrab(E_BINDING_CONTEXT_POPUP, pp->popup->evas_win);
   e_object_del(E_OBJECT(pp->popup));
   free(pp);
}

static Pager_Popup *
_pager_popup_find(E_Zone *zone)
{
   Pager *p;
   Eina_List *l;

   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	if (p->popup && p->zone == zone)
	  return p->popup;
     }
   return NULL;
}

static void
_pager_cb_obj_moveresize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Instance *inst;

   inst = data;
   _pager_instance_drop_zone_recalc(inst);
}

static void
_button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Instance *inst;
   Evas_Event_Mouse_Down *ev;

   inst = data;
   ev = event_info;
   if ((ev->button == 3) && (!pager_config->menu))
     {
	E_Menu *mn;
	E_Menu_Item *mi;
	int cx, cy;

	mn = e_menu_new();
	e_menu_post_deactivate_callback_set(mn, _menu_cb_post, inst);
	pager_config->menu = mn;

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Settings"));
	e_util_menu_item_theme_icon_set(mi, "configure");
	e_menu_item_callback_set(mi, _pager_inst_cb_menu_configure, NULL);

	if (e_configure_registry_exists("screen/virtual_desktops"))
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Virtual Desktops Settings"));
	     e_util_menu_item_theme_icon_set(mi, "preferences-desktop");
	     e_menu_item_callback_set(mi, _pager_inst_cb_menu_virtual_desktops_dialog, inst);
	  }

	e_gadcon_client_util_menu_items_append(inst->gcc, mn, 0);

	e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy, NULL, NULL);
	e_menu_activate_mouse(mn,
			      e_util_zone_current_get(e_manager_current_get()),
			      cx + ev->output.x, cy + ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
     }
}

static void
_menu_cb_post(void *data, E_Menu *m)
{
   if (!pager_config->menu) return;
   e_object_del(E_OBJECT(pager_config->menu));
   pager_config->menu = NULL;
}

static void
_pager_inst_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi)
{
   if (!pager_config) return;
   if (pager_config->config_dialog) return;
   /* FIXME: pass zone config item */
   _config_pager_module(NULL);
}

static E_Config_Dialog *
_pager_config_dialog(E_Container *con, const char *params)
{
   if (!pager_config) return NULL;
   if (pager_config->config_dialog) return NULL;
   /* FIXME: pass zone config item */
   _config_pager_module(NULL);
   return pager_config->config_dialog;
}

static void
_pager_inst_cb_menu_virtual_desktops_dialog(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Instance *inst;

   inst = data;
   e_configure_registry_call("screen/virtual_desktops", inst->gcc->gadcon->zone->container, NULL);
}

static void
_pager_instance_drop_zone_recalc(Instance *inst)
{
   Evas_Coord x, y, w, h;

   evas_object_geometry_get(inst->o_pager, &x, &y, &w, &h);
   e_drop_handler_geometry_set(inst->pager->drop_handler, x, y, w, h);
}

void
_pager_cb_config_updated(void)
{
   if (!pager_config) return;
}

static int
_pager_cb_event_border_resize(void *data, int type, void *event)
{
   E_Event_Border_Resize *ev;
   Eina_List *l, *l2;
   Pager *p;

   ev = event;
   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	if (p->zone != ev->border->zone) continue;
	for (l2 = p->desks; l2; l2 = l2->next)
	  {
	     Pager_Desk *pd;
	     Pager_Win *pw;

	     pd = l2->data;
	     pw = _pager_desk_window_find(pd, ev->border);
	     if (pw) _pager_window_move(pw);
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_move(void *data, int type, void *event)
{
   E_Event_Border_Move *ev;
   Eina_List *l, *l2;
   Pager_Win *pw;
   Pager_Desk *pd;
   Pager *p;

   ev = event;
   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	if (p->zone != ev->border->zone) continue;
	for (l2 = p->desks; l2; l2 = l2->next)
	  {
	     pd = l2->data;
	     pw = _pager_desk_window_find(pd, ev->border);
	     if (pw) _pager_window_move(pw);
	  }
     }

   if (act_popup && act_popup->pager->zone == ev->border->zone)
     {
	for (l = act_popup->pager->desks; l; l = l->next)
	  {
	     pd = l->data;
	     pw = _pager_desk_window_find(pd, ev->border);
	     if (pw) _pager_window_move(pw);
	  }
     }

   return 1;
}

static int
_pager_cb_event_border_add(void *data, int type, void *event)
{
   E_Event_Border_Add *ev;
   Eina_List *l;
   Pager *p;
   Pager_Desk *pd;
   Pager_Win *pw;

   ev = event;
   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	if ((p->zone != ev->border->zone) ||
	    (_pager_window_find(p, ev->border)))
	  continue;
	pd = _pager_desk_find(p, ev->border->desk);
	if (pd)
	  {
	     pw = _pager_window_new(pd, ev->border);
	     if (pw) pd->wins = eina_list_append(pd->wins, pw);
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_remove(void *data, int type, void *event)
{
   E_Event_Border_Remove *ev;
   Eina_List *l, *l2;
   Pager *p;
   Pager_Desk *pd;
   Pager_Win *pw;

   ev = event;
   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	if (p->zone != ev->border->zone) continue;
	for (l2 = p->desks; l2; l2 = l2->next)
	  {
	     pd = l2->data;
	     pw = _pager_desk_window_find(pd, ev->border);
	     if (pw)
	       {
		  pd->wins = eina_list_remove(pd->wins, pw);
		  _pager_window_free(pw);
	       }
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_iconify(void *data, int type, void *event)
{
   E_Event_Border_Iconify *ev;
   Eina_List *l, *l2;
   Pager *p;
   Pager_Desk *pd;
   Pager_Win *pw;

   ev = event;
   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	if (p->zone != ev->border->zone) continue;
	for (l2 = p->desks; l2; l2 = l2->next)
	  {
	     pd = l2->data;
	     pw = _pager_desk_window_find(pd, ev->border);
	     if (pw)
	       {
		  if ((pw->drag.from_pager) && (pw->desk->pager->dragging))
		    pw->desk->pager->dragging = 0;
		  evas_object_hide(pw->o_window);
	       }
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_uniconify(void *data, int type, void *event)
{
   E_Event_Border_Uniconify *ev;
   Eina_List *l, *l2;
   Pager *p;
   Pager_Desk *pd;
   Pager_Win *pw;

   ev = event;
   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	if (p->zone != ev->border->zone) continue;
	for (l2 = p->desks; l2; l2 = l2->next)
	  {
	     pd = l2->data;
	     pw = _pager_desk_window_find(pd, ev->border);
	     if (pw && !pw->skip_winlist) evas_object_show(pw->o_window);
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_stick(void *data, int type, void *event)
{
   E_Event_Border_Stick *ev;
   Eina_List *l, *l2;
   Pager *p;
   Pager_Desk *pd;
   Pager_Win *pw;

   ev = event;
   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	if (p->zone != ev->border->zone) continue;
	pw = _pager_window_find(p, ev->border);
	if (!pw) continue;
	for (l2 = p->desks; l2; l2 = l2->next)
	  {
	     pd = l2->data;
	     if (ev->border->desk != pd->desk)
	       {
		  pw = _pager_window_new(pd, ev->border);
		  if (pw) pd->wins = eina_list_append(pd->wins, pw);
	       }
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_unstick(void *data, int type, void *event)
{
   E_Event_Border_Unstick *ev;
   Eina_List *l, *l2;
   Pager *p;
   Pager_Desk *pd;
   Pager_Win *pw;

   ev = event;
   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	if (p->zone != ev->border->zone) continue;
	for (l2 = p->desks; l2; l2 = l2->next)
	  {
	     pd = l2->data;
	     if (ev->border->desk != pd->desk)
	       {
		  pw = _pager_desk_window_find(pd, ev->border);
		  if (pw)
		    {
		       pd->wins = eina_list_remove(pd->wins, pw);
		       _pager_window_free(pw);
		    }
	       }
	  }
     }
   return 1;
}

static void
_pager_window_desk_change(Pager *pager, E_Border *bd)
{
   Eina_List *l;
   Pager_Win  *pw;
   Pager_Desk *pd;

   /* if this pager is not for the zone of the border */
   if (pager->zone != bd->zone)
     {
	/* look at all desks in the pager */
	for (l = pager->desks; l; l = l->next)
	  {
	     pd = l->data;
	     /* find this border in this desk */
	     pw = _pager_desk_window_find(pd, bd);
	     if (pw)
	       {
		  /* if it is found - remove it. it does not belong in this
		   * pager as it probably moves zones */
		  pd->wins = eina_list_remove(pd->wins, pw);
		  _pager_window_free(pw);
	       }
	  }
	return;
     }
   /* and this pager zone is for this border */
   /* see if the window is in this pager at all */
   pw = _pager_window_find(pager, bd);
   if (pw)
     {
	/* is it sticky */
	if (bd->sticky)
	  {
	     /* if its sticky and in this pager - its already everywhere, so abort
	      * doing anything else */
	     return;
	  }
	/* move it to the right desk */
	/* find the pager desk of the target desk */
	pd = _pager_desk_find(pager, bd->desk);
	if (pd)
	  {
	     Pager_Win *pw2 = NULL;
	     E_Border *bd;
	     /* remove it from whatever desk it was on */
	     pw->desk->wins = eina_list_remove(pw->desk->wins, pw);
	     e_layout_unpack(pw->o_window);

	     /* add it to the one its MEANT to be on */
	     pw->desk = pd;
	     pd->wins = eina_list_append(pd->wins, pw);
	     e_layout_pack(pd->o_layout, pw->o_window);

	     bd = e_util_desk_border_above(pw->border);
	     if (bd)
	       pw2 = _pager_desk_window_find(pd, bd);
	     if (pw2)
	       e_layout_child_lower_below(pw->o_window, pw2->o_window);
	     else
	       e_layout_child_raise(pw->o_window);

	     _pager_window_move(pw);
	  }
     }
   /* the border isnt in this pager at all - it must have moved zones */
   else
     {
	if (!bd->sticky)
	  {
	     /* find the pager desk it needs to go to */
	     pd = _pager_desk_find(pager, bd->desk);
	     if (pd)
	       {
		  /* create it and add it */
		  pw = _pager_window_new(pd, bd);
		  if (pw)
		    {
		       Pager_Win *pw2 = NULL;
		       E_Border *bd;
		       pd->wins = eina_list_append(pd->wins, pw);
		       bd = e_util_desk_border_above(pw->border);
		       if (bd)
			 pw2 = _pager_desk_window_find(pd, bd);
		       if (pw2)
			 e_layout_child_lower_below(pw->o_window, pw2->o_window);
		       else
			 e_layout_child_raise(pw->o_window);
		       _pager_window_move(pw);
		    }
	       }
	  }
	else
	  {
	     /* go through all desks */
	     for (l = pager->desks; l; l = l->next)
	       {
		  pd = l->data;
		  /* create it and add it */
		  pw = _pager_window_new(pd, bd);
		  if (pw)
		    {
		       Pager_Win *pw2 = NULL;
		       E_Border *bd;
		       pd->wins = eina_list_append(pd->wins, pw);
		       bd = e_util_desk_border_above(pw->border);
		       if (bd)
			 pw2 = _pager_desk_window_find(pd, bd);
		       if (pw2)
			 e_layout_child_lower_below(pw->o_window, pw2->o_window);
		       else
			 e_layout_child_raise(pw->o_window);
		       _pager_window_move(pw);
		    }
	       }
	  }
     }
}

static int
_pager_cb_event_border_desk_set(void *data, int type, void *event)
{
   E_Event_Border_Desk_Set *ev;
   Eina_List *l;
   Pager *p;

   ev = event;
   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	_pager_window_desk_change(p, ev->border);
     }

   return 1;
}

static int
_pager_cb_event_border_stack(void *data, int type, void *event)
{
   E_Event_Border_Stack *ev;
   Eina_List *l, *l2;
   Pager *p;

   ev = event;
   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	if (p->zone != ev->border->zone) continue;
	for (l2 = p->desks; l2; l2 = l2->next)
	  {
	     Pager_Desk *pd;
	     Pager_Win *pw, *pw2 = NULL;

	     pd = l2->data;
	     pw = _pager_desk_window_find(pd, ev->border);
	     if (pw)
	       {
		  if (ev->stack)
		    {
		       pw2 = _pager_desk_window_find(pd, ev->stack);
		       if (!pw2)
			 {
			    /* This border is on another desk... */
			    E_Border *bd = NULL;

			    if (ev->type == E_STACKING_ABOVE)
			      bd = e_util_desk_border_below(ev->border);
			    else if (ev->type == E_STACKING_BELOW)
			      bd = e_util_desk_border_above(ev->border);
			    if (bd) pw2 = _pager_desk_window_find(pd, bd);
			 }
		    }
		  if (ev->type == E_STACKING_ABOVE)
		    {
		       if (pw2)
			 e_layout_child_raise_above(pw->o_window, pw2->o_window);
		       else
			 /* If we aren't above any window, we are at the bottom */
			 e_layout_child_lower(pw->o_window);
		    }
		  else if (ev->type == E_STACKING_BELOW)
		    {
		       if (pw2)
			 e_layout_child_lower_below(pw->o_window, pw2->o_window);
		       else
			 /* If we aren't below any window, we are at the top */
			 e_layout_child_raise(pw->o_window);
		    }
	       }
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_icon_change(void *data, int type, void *event)
{
   E_Event_Border_Icon_Change *ev;
   Eina_List *l, *l2;
   Pager *p;

   ev = event;
   for (l = pagers; l; l = l->next)
     {
	p = l->data;

	if (p->zone != ev->border->zone) continue;
	for (l2 = p->desks; l2; l2 = l2->next)
	  {
	     Pager_Desk *pd;
	     Pager_Win *pw;

	     pd = l2->data;
	     pw = _pager_desk_window_find(pd, ev->border);
	     if (pw)
	       {
		  Evas_Object *o;

		  if (pw->o_icon)
		    {
		       evas_object_del(pw->o_icon);
		       pw->o_icon = NULL;
		    }
		  o = e_border_icon_add
		    (ev->border, evas_object_evas_get(p->o_table));
		  if (o)
		    {
		       pw->o_icon = o;
		       evas_object_show(o);
		       edje_object_part_swallow(pw->o_window, "e.swallow.icon", o);
		    }
	       }
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_urgent_change(void *data, int type, void *event)
{
   E_Event_Border_Urgent_Change *ev;
   Eina_List *l, *l2;
   Pager_Popup *pp;
   E_Zone *zone;
   int urgent;
   Pager *p;
   Pager_Desk *pd;
   Pager_Win *pw;

   ev = event;
   zone = ev->border->zone;
   urgent = ev->border->client.icccm.urgent;

   if (pager_config->popup_urgent)
     {
	pp = _pager_popup_find(zone);

	if (!pp && urgent && !(ev->border->iconic))
	  {
	     pp = _pager_popup_new(zone, 0);

	     if (pp && !pager_config->popup_urgent_stick)
	       pp->timer = ecore_timer_add(pager_config->popup_urgent_speed,
					   _pager_popup_cb_timeout, pp);
	     if (pp)
	       pp->urgent = 1;
	  }
     }

   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	if (p->zone != zone) continue;
	for (l2 = p->desks; l2; l2 = l2->next)
	  {
	     pd = l2->data;
	     pw = _pager_desk_window_find(pd, ev->border);
	     if (pw)
	       {
		  if (urgent)
		    {
		       if (!(ev->border->iconic))
			 {
			    if (pd->pager && pd->pager->inst && !pager_config->popup_urgent)
			      e_gadcon_urgent_show(pd->pager->inst->gcc->gadcon);
			    edje_object_signal_emit(pd->o_desk,
						    "e,state,urgent", "e");
			 }
		       edje_object_signal_emit(pw->o_window,
					       "e,state,urgent", "e");
		    }
		  else
		    {
		       if (!(ev->border->iconic))
			 edje_object_signal_emit(pd->o_desk,
						 "e,state,not_urgent", "e");
		       edje_object_signal_emit(pw->o_window,
					       "e,state,not_urgent", "e");
		    }
	       }
	  }
     }
 
   return 1;
}

static int
_pager_cb_event_border_focus_in(void *data, int type, void *event)
{
   E_Event_Border_Focus_In *ev;
   Eina_List *l, *l2;
   Pager_Popup *pp;
   E_Zone *zone;

   ev = event;
   zone = ev->border->zone;

   for (l = pager_config->instances; l; l = l->next)
     {
	Instance *inst;

	inst = l->data;
	if (inst->pager->zone != zone) continue;
	for (l2 = inst->pager->desks; l2; l2 = l2->next)
	  {
	     Pager_Desk *pd;
	     Pager_Win *pw;

	     pd = l2->data;
	     pw = _pager_desk_window_find(pd, ev->border);
	     if (pw)
	       {
		  edje_object_signal_emit(pw->o_window,
					  "e,state,focused", "e");
		  break;
	       }
	  }
     }

   pp = _pager_popup_find(zone);
   if (!pp) return 1;
   for (l = pp->pager->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	Pager_Win *pw;
	
	pd = l->data;
	pw = _pager_desk_window_find(pd, ev->border);
	if (pw)
	  {
	     edje_object_signal_emit(pw->o_window,
				     "e,state,focused", "e");
	     break;
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_focus_out(void *data, int type, void *event)
{
   E_Event_Border_Focus_Out *ev;
   Eina_List *l, *l2;
   Pager_Popup *pp;
   E_Zone *zone;

   ev = event;
   zone = ev->border->zone;

   for (l = pager_config->instances; l; l = l->next)
     {
	Instance *inst;

	inst = l->data;
	if (inst->pager->zone != zone) continue;
	for (l2 = inst->pager->desks; l2; l2 = l2->next)
	  {
	     Pager_Desk *pd;
	     Pager_Win *pw;

	     pd = l2->data;
	     pw = _pager_desk_window_find(pd, ev->border);
	     if (pw)
	       {
		  edje_object_signal_emit(pw->o_window,
					  "e,state,unfocused", "e");
		  break;
	       }
	  }
     }

   pp = _pager_popup_find(zone);
   if (!pp) return 1;
   for (l = pp->pager->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	Pager_Win *pw;
	
	pd = l->data;
	pw = _pager_desk_window_find(pd, ev->border);
	if (pw)
	  {
	     edje_object_signal_emit(pw->o_window,
				     "e,state,unfocused", "e");
	     break;
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_property(void *data, int type, void *event)
{
   E_Event_Border_Property *ev;
   Eina_List *l, *l2;
   int found = 0;
   Pager *p;
   Pager_Win *pw;
   Pager_Desk *pd;

   ev = event;
   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	if (p->zone != ev->border->zone) continue;
	for (l2 = p->desks; l2; l2 = l2->next)
	  {
	     pd = l2->data;
	     pw = _pager_desk_window_find(pd, ev->border);
	     if (pw)
	       {
		  found = 1;
		  if (ev->border->client.netwm.state.skip_pager)
		    {
		       pd->wins = eina_list_remove(pd->wins, pw);
		       _pager_window_free(pw);
		    }
	       }
	  }
     }
   if (found) return 1;

   /* If we did not find this window in the pager, then add it because
    * the skip_pager state may have changed to 1 */
   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	if ((p->zone != ev->border->zone) ||
	    (_pager_window_find(p, ev->border)))
	  continue;
	if (!ev->border->sticky)
	  {
	     pd = _pager_desk_find(p, ev->border->desk);
	     if (pd)
	       {
		  Pager_Win *pw;

		  pw = _pager_window_new(pd, ev->border);
		  if (pw)
		    {
		       Pager_Win *pw2 = NULL;
		       E_Border *bd;

		       pd->wins = eina_list_append(pd->wins, pw);
		       bd = e_util_desk_border_above(pw->border);
		       if (bd)
			 pw2 = _pager_desk_window_find(pd, bd);
		       if (pw2)
			 e_layout_child_lower_below(pw->o_window, pw2->o_window);
		       else
			 e_layout_child_raise(pw->o_window);
		       _pager_window_move(pw);
		    }
	       }
	  }
	else
	  {
	     for (l2 = p->desks; l2; l2 = l2->next)
	       {
		  pd = l2->data;
		  pw = _pager_window_new(pd, ev->border);
		  if (pw)
		    {
		       Pager_Win *pw2 = NULL;
		       E_Border *bd;

		       pd->wins = eina_list_append(pd->wins, pw);
		       bd = e_util_desk_border_above(pw->border);
		       if (bd)
			 pw2 = _pager_desk_window_find(pd, bd);
		       if (pw2)
			 e_layout_child_lower_below(pw->o_window, pw2->o_window);
		       else
			 e_layout_child_raise(pw->o_window);
		       _pager_window_move(pw);
		    }
	       }
	  }
     }
   return 1;
}

static int
_pager_cb_event_zone_desk_count_set(void *data, int type, void *event)
{
   Eina_List *l;
   Pager *p;

   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	_pager_empty(p);
	_pager_fill(p);
	if (p->inst) _gc_orient(p->inst->gcc, p->inst->gcc->gadcon->orient);
     }
   return 1;
}

static int
_pager_cb_event_desk_show(void *data, int type, void *event)
{
   E_Event_Desk_Show *ev;
   Eina_List *l;
   Pager *p;
   Pager_Desk *pd;
   Pager_Popup *pp;

   ev = event;
   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	if (p->zone != ev->desk->zone) continue;
	pd = _pager_desk_find(p, ev->desk);
	if (pd) _pager_desk_select(pd);

	if (p->popup)
	  {
	     edje_object_part_text_set(p->popup->o_bg, "text", ev->desk->name);
	  }
     }

   if (pager_config->popup && !act_popup)
     {
	if ((pp = _pager_popup_find(ev->desk->zone)))
	  {
	     ecore_timer_del(pp->timer);
	  }
	else
	  {
	     pp = _pager_popup_new(ev->desk->zone, 0);
	  }

	if (pp)
	  {
	     pp->timer = ecore_timer_add(pager_config->popup_speed,
					 _pager_popup_cb_timeout, pp);

	     pd = _pager_desk_find(pp->pager, ev->desk);
	     if (pd)
	       {
		  _pager_desk_select(pd);
		  edje_object_part_text_set(pp->o_bg, "e.text.label",
					    ev->desk->name);
	       }
	     ecore_timer_del(pp->timer);
	     pp->timer = ecore_timer_add(pager_config->popup_speed,
					 _pager_popup_cb_timeout, pp);
	  }
     }
   return 1;
}

static int
_pager_cb_event_desk_name_change(void *data, int type, void *event)
{
   E_Event_Desk_Name_Change *ev;
   Eina_List *l;
   Pager *p;
   Pager_Desk *pd;

   ev = event;
   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	if (p->zone != ev->desk->zone) continue;
	pd = _pager_desk_find(p, ev->desk);
	if (pager_config->show_desk_names)
	  {
	     if (pd)
	       edje_object_part_text_set(pd->o_desk, "e.text.label", ev->desk->name);
	  }
     }
   return 1;
}

static int
_pager_cb_event_container_resize(void *data, int type, void *event)
{
   E_Event_Container_Resize *ev;
   Eina_List *l, *l2;
   Pager *p;
   Pager_Desk *pd;

   ev = event;
   for (l = pagers; l; l = l->next)
     {
	p = l->data;
	if (p->zone->container != ev->container) continue;
	for (l2 = p->desks; l2; l2 = l2->next)
	  {
	     pd = l2->data;
	     e_layout_virtual_size_set(pd->o_layout, pd->desk->zone->w,
				       pd->desk->zone->h);
	  }
	if (p->inst) _gc_orient(p->inst->gcc, p->inst->gcc->gadcon->orient);
	/* TODO if (p->popup) */
     }
   return 1;
}

static void
_pager_window_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_In *ev;
   Pager_Win *pw;

   ev = event_info;
   pw = data;
   /* FIXME: display window title in some tooltip thing */
}

static void
_pager_window_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Out *ev;
   Pager_Win *pw;

   ev = event_info;
   pw = data;
   /* FIXME: close tooltip */
}

static void
_pager_window_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Pager_Win *pw;

   ev = event_info;
   pw = data;

   if (!pw) return;
   if (pw->desk->pager->popup && !act_popup) return;
   if (!pw->desk->pager->popup && ev->button == 3) return;
   if (ev->button == pager_config->btn_desk) return;
   if ((ev->button == pager_config->btn_drag) ||
       (ev->button == pager_config->btn_noplace))
     {
	Evas_Coord ox, oy;
	evas_object_geometry_get(pw->o_window, &ox, &oy, NULL, NULL);
	pw->drag.in_pager = 1;
	pw->drag.x = ev->canvas.x;
	pw->drag.y = ev->canvas.y;
	pw->drag.dx = ox - ev->canvas.x;
	pw->drag.dy = oy - ev->canvas.y;
	pw->drag.start = 1;
	pw->drag.no_place = 0;
	pw->drag.button = ev->button;
	if (ev->button == pager_config->btn_noplace)
	  pw->drag.no_place = 1;
     }
}

static void
_pager_window_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   Pager_Win *pw;
   Pager *p;

   ev = event_info;
   pw = data;
   if (!pw) return;

   p = pw->desk->pager;

   if (pw->desk->pager->popup && !act_popup) return;
   if (ev->button == pager_config->btn_desk) return;
   if ((ev->button == pager_config->btn_drag) ||
       (ev->button == pager_config->btn_noplace))
     {
	if (!pw->drag.from_pager)
	  {
	     if (!pw->drag.start) p->just_dragged = 1;
	     pw->drag.in_pager = 0;
	     pw->drag.start = 0;
	     p->dragging = 0;
	  }
     }
}

static void
_pager_window_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   Pager_Win *pw;
   E_Drag *drag;
   Evas_Object *o, *oo;
   Evas_Coord x, y, w, h;
   const char *drag_types[] = { "enlightenment/pager_win", "enlightenment/border" };
   Evas_Coord dx, dy;
   unsigned int resist = 0;
   Evas_Coord mx, my, vx, vy;
   Pager_Desk *pd;

   ev = event_info;
   pw = data;

   if (!pw) return;
   if (pw->border->lock_user_location) return;
   if (pw->desk->pager->popup && !act_popup) return;
   /* prevent drag for a few pixels */
   if (pw->drag.start)
     {
	dx = pw->drag.x - ev->cur.output.x;
	dy = pw->drag.y - ev->cur.output.y;
	if (pw->desk && pw->desk->pager)
	  resist = pager_config->drag_resist;

	if (((dx * dx) + (dy * dy)) <= (resist * resist)) return;

	pw->desk->pager->dragging = 1;
	pw->drag.start = 0;
     }

   /* dragging this win around inside the pager */
   if (pw->drag.in_pager)
     {
	/* m for mouse */
	mx = ev->cur.canvas.x;
	my = ev->cur.canvas.y;

	/* find desk at pointer */
	pd = _pager_desk_at_coord(pw->desk->pager, mx, my);
	if ((pd) && (!pw->drag.no_place))
	  {
	     e_layout_coord_canvas_to_virtual(pd->o_layout,
					      mx + pw->drag.dx,
					      my + pw->drag.dy, &vx, &vy);
	     if (pd != pw->desk)
	       e_border_desk_set(pw->border, pd->desk);
	     e_border_move(pw->border,
			   vx + pd->desk->zone->x,
			   vy + pd->desk->zone->y);
	  }
	else
	  {
	     evas_object_geometry_get(pw->o_window, &x, &y, &w, &h);
	     evas_object_hide(pw->o_window);

	     drag = e_drag_new(pw->desk->pager->zone->container,
			       x, y, drag_types, 2, pw, -1,
			       _pager_window_cb_drag_convert,
			       _pager_window_cb_drag_finished);

	     o = edje_object_add(drag->evas);
	     e_theme_edje_object_set(o, "base/theme/modules/pager",
				     "e/modules/pager/window");
	     evas_object_show(o);

	     oo = e_border_icon_add(pw->border, drag->evas);
	     if (oo)
	       {
		  evas_object_show(oo);
		  edje_object_part_swallow(o, "e.swallow.icon", oo);
	       }

	     e_drag_object_set(drag, o);
	     e_drag_resize(drag, w, h);
	     e_drag_start(drag, x - pw->drag.dx, y - pw->drag.dy);

	     /* this prevents the desk from switching on drags */
	     pw->drag.from_pager = pw->desk->pager;
	     pw->drag.from_pager->dragging = 1;
	     pw->drag.in_pager = 0;
	  }
     }
}

static void *
_pager_window_cb_drag_convert(E_Drag *drag, const char *type)
{
   Pager_Win *pw;

   pw = drag->data;
   if (!strcmp(type, "enlightenment/pager_win")) return pw;
   if (!strcmp(type, "enlightenment/border")) return pw->border;
   return NULL;
}

static void
_pager_window_cb_drag_finished(E_Drag *drag, int dropped)
{
   Pager_Win *pw;
   E_Container *cont;
   E_Zone *zone;
   E_Desk *desk;
   int x, y, dx, dy;

   pw = drag->data;
   if (!pw) return;
   evas_object_show(pw->o_window);
   if (!dropped)
     {
	/* wasn't dropped (on pager). move it to position of mouse on screen */
	cont = e_container_current_get(e_manager_current_get());
	zone = e_zone_current_get(cont);
	desk = e_desk_current_get(zone);

	e_border_zone_set(pw->border, zone);
	e_border_desk_set(pw->border, desk);

	ecore_x_pointer_last_xy_get(&x, &y);

	dx = (pw->border->w / 2);
	dy = (pw->border->h / 2);

	/* offset so that center of window is on mouse, but keep within desk bounds */
	if (dx < x)
	  {
	     x -= dx;
	     if ((pw->border->w < zone->w) &&
		 (x + pw->border->w > zone->x + zone->w))
	       x -= x + pw->border->w - (zone->x + zone->w);
	  }
	else x = 0;

	if (dy < y)
	  {
	     y -= dy;
	     if ((pw->border->h < zone->h) &&
		 (y + pw->border->h > zone->y + zone->h))
	       y -= y + pw->border->h - (zone->y + zone->h);
	  }
	else y = 0;
	e_border_move(pw->border, x, y);

	if (!(pw->border->lock_user_stacking))
	  e_border_raise(pw->border);
     }
   if (pw && pw->drag.from_pager) pw->drag.from_pager->dragging = 0;
   pw->drag.from_pager = NULL;

  if (act_popup)
     {
	e_grabinput_get(input_window, 0, input_window);

	if (!hold_count) _pager_popup_hide(1);
     }
}

static void
_pager_inst_cb_scroll(void *data)
{
   Pager *p;

   p = data;
   _pager_update_drop_position(p, p->dnd_x, p->dnd_y);
}

static void
_pager_update_drop_position(Pager *p, Evas_Coord x, Evas_Coord y)
{
   Pager_Desk *pd, *pd2;
   Evas_Coord xx, yy;
   int ox, oy;
   Eina_List *l;

   ox = oy = 0;

   p->dnd_x = x;
   p->dnd_y = y;
   evas_object_geometry_get(p->o_table, &xx, &yy, NULL, NULL);
   if (p->inst) e_box_align_pixel_offset_get(p->inst->gcc->o_box, &ox, &oy);
   pd = _pager_desk_at_coord(p, x + xx + ox, y + yy + oy);
   if (pd == p->active_drop_pd) return;
   for (l = p->desks; l; l = l->next)
     {
	pd2 = l->data;
	if (pd == pd2)
	  edje_object_signal_emit(pd2->o_desk, "e,action,drag,in", "e");
	else if (pd2 == p->active_drop_pd)
	  edje_object_signal_emit(pd2->o_desk, "e,action,drag,out", "e");
     }
   p->active_drop_pd = pd;
}

static void
_pager_drop_cb_enter(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Enter *ev;
   Pager *p;

   ev = event_info;
   p = data;

   /* FIXME this fixes a segv, but the case is not easy
    * reproduceable. this makes no sense either since
    * the same 'pager' is passed to e_drop_handler_add
    * and it works without this almost all the time.
    * so this must be an issue with e_dnd code... i guess */
   if (act_popup) p = act_popup->pager;

   _pager_update_drop_position(p, ev->x, ev->y);

   if (p->inst)
     {
	e_gadcon_client_autoscroll_cb_set(p->inst->gcc, _pager_inst_cb_scroll, p);
	e_gadcon_client_autoscroll_update(p->inst->gcc, ev->x, ev->y);
     }
}

static void
_pager_drop_cb_move(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Move *ev;
   Pager *p;

   ev = event_info;
   p = data;

   if (act_popup) p = act_popup->pager;

   _pager_update_drop_position(p, ev->x, ev->y);

   if (p->inst)
     {
	e_gadcon_client_autoscroll_update(p->inst->gcc, ev->x, ev->y);
     }
}

static void
_pager_drop_cb_leave(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Leave *ev;
   Pager *p;
   Eina_List *l;

   ev = event_info;
   p = data;

   if (act_popup) p = act_popup->pager;

   for (l = p->desks; l && p->active_drop_pd; l = l->next)
     {
	Pager_Desk *pd;

	pd = l->data;
	if (pd == p->active_drop_pd)
	  {
	     edje_object_signal_emit(pd->o_desk, "e,action,drag,out", "e");
	     p->active_drop_pd = NULL;
	  }
     }

   if (p->inst) e_gadcon_client_autoscroll_cb_set(p->inst->gcc, NULL, NULL);
}

static void
_pager_drop_cb_drop(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Drop *ev;
   Pager_Desk *pd;
   Pager_Desk *pd2 = NULL;
   E_Border *bd = NULL;
   Eina_List *l;
   int dx = 0, dy = 0;
   Pager_Win *pw = NULL;
   Evas_Coord xx, yy;
   int x = 0, y = 0;
   Evas_Coord wx, wy, wx2, wy2;
   Evas_Coord nx, ny;
   Pager *p;

   ev = event_info;
   p = data;

   if (act_popup) p = act_popup->pager;

   evas_object_geometry_get(p->o_table, &xx, &yy, NULL, NULL);
   if (p->inst) e_box_align_pixel_offset_get(p->inst->gcc->o_box, &x, &y);
   pd = _pager_desk_at_coord(p, ev->x + xx + x, ev->y + yy + y);
   if (pd)
     {
	if (!strcmp(type, "enlightenment/pager_win"))
	  {
	     pw = (Pager_Win *)(ev->data);
	     if (pw)
	       {
		  bd = pw->border;
		  dx = pw->drag.dx;
		  dy = pw->drag.dy;
	       }
	  }
	else if (!strcmp(type, "enlightenment/border"))
	  {
	     bd = ev->data;
	     e_layout_coord_virtual_to_canvas(pd->o_layout, bd->x, bd->y, &wx, &wy);
	     e_layout_coord_virtual_to_canvas(pd->o_layout, bd->x + bd->w, bd->y + bd->h, &wx2, &wy2);
	     dx = (wx - wx2) / 2;
	     dy = (wy - wy2) / 2;
	  }
	else if (!strcmp(type, "enlightenment/vdesktop"))
	  {
	     pd2 = ev->data;
	     if (!pd2) return;
	     _pager_desk_switch(pd, pd2);
	  }
	else
	  return;

	if (bd)
	  {
	     if (bd->iconic) e_border_uniconify(bd);
	     e_border_desk_set(bd, pd->desk);
	     if ((!pw) || ((pw) && (!pw->drag.no_place)))
	       {
		  e_layout_coord_canvas_to_virtual(pd->o_layout,
						   ev->x + xx + x + dx,
						   ev->y + yy + y + dy,
						   &nx, &ny);
		  e_border_move(bd, nx + pd->desk->zone->x, ny + pd->desk->zone->y);
	       }
	  }
     }

   for (l = p->desks; l && p->active_drop_pd; l = l->next)
     {
	pd = l->data;
	if (pd == p->active_drop_pd)
	  {
	     edje_object_signal_emit(pd->o_desk, "e,action,drag,out", "e");
	     p->active_drop_pd = NULL;
	  }
     }

   if (p->inst) e_gadcon_client_autoscroll_cb_set(p->inst->gcc, NULL, NULL);
}

static void
_pager_desk_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Pager_Desk *pd;
   Evas_Coord ox, oy;

   ev = event_info;
   pd = data;
   if (!pd) return;
   if (!pd->pager->popup && ev->button == 3) return;
   if (ev->button == pager_config->btn_desk)
     {
	evas_object_geometry_get(pd->o_desk, &ox, &oy, NULL, NULL);
	pd->drag.start = 1;
	pd->drag.in_pager = 1;
	pd->drag.dx = ox - ev->canvas.x;
	pd->drag.dy = oy - ev->canvas.y;
	pd->drag.x = ev->canvas.x;
	pd->drag.y = ev->canvas.y;
	pd->drag.button = ev->button;
     }
}

static void
_pager_desk_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   Pager_Desk *pd;
   Pager *p;

   ev = event_info;
   pd = data;

   if (!pd) return;
   p = pd->pager;

   /* FIXME: pd->pager->dragging is 0 when finishing a drag from desk to desk */
   if ((ev->button == 1) && (!pd->pager->dragging) &&
       (!pd->pager->just_dragged))
     {
	current_desk = pd->desk;
	e_desk_show(pd->desk);
	pd->drag.start = 0;
	pd->drag.in_pager = 0;
     }
   pd->pager->just_dragged = 0;

   if (p->popup && p->popup->urgent)
     {
	_pager_popup_free(p->popup);
     }
}
static void
_pager_desk_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   Pager_Desk *pd;
   Evas_Coord dx, dy;
   unsigned int resist = 0;
   E_Drag *drag;
   Evas_Object *o, *oo, *o_icon;
   Evas_Coord x, y, w, h;
   const char *drag_types[] = { "enlightenment/vdesktop" };
   Pager_Win *pw;
   Eina_List *l;

   ev = event_info;

   pd = data;
   if (!pd) return;
   /* prevent drag for a few pixels */
   if (pd->drag.start)
     {
	dx = pd->drag.x - ev->cur.output.x;
	dy = pd->drag.y - ev->cur.output.y;
	if (pd->pager && pd->pager->inst)
	  resist = pager_config->drag_resist;

	if (((dx * dx) + (dy * dy)) <= (resist * resist)) return;

	pd->pager->dragging = 1;
	pd->drag.start = 0;
     }

   if (pd->drag.in_pager)
     {
	evas_object_geometry_get(pd->o_desk, &x, &y, &w, &h);
	drag = e_drag_new(pd->pager->zone->container,
			  x, y, drag_types, 1, pd, -1,
			  NULL, _pager_desk_cb_drag_finished);

	/* set a background to the drag icon */
	o = evas_object_rectangle_add(drag->evas);
	evas_object_color_set(o, 255, 255, 255, 255);
	evas_object_resize(o, w, h);
	evas_object_show(o);

	/* redraw the desktop theme above */
	o = edje_object_add(drag->evas);
	e_theme_edje_object_set(o, "base/theme/modules/pager",
				"e/modules/pager/desk");
	evas_object_show(o);
	e_drag_object_set(drag, o);

	/* and redraw is content */
	oo = e_layout_add(drag->evas);
	e_layout_virtual_size_set(oo, pd->pager->zone->w, pd->pager->zone->h);
	edje_object_part_swallow(o, "e.swallow.content", oo);
	evas_object_show(oo);

	for (l = pd->wins; l; l = l->next)
	  {
	     pw = l->data;
	     if (!pw || pw->border->iconic
		 || pw->border->client.netwm.state.skip_pager)
	       continue;

	     o = edje_object_add(drag->evas);
	     e_theme_edje_object_set(o, "base/theme/modules/pager",
				     "e/modules/pager/window");
	     e_layout_pack(oo, o);
	     e_layout_child_raise(o);
	     e_layout_child_move(o,
				 pw->border->x - pw->desk->desk->zone->x,
				 pw->border->y - pw->desk->desk->zone->y);
	     e_layout_child_resize(o, pw->border->w, pw->border->h);
	     evas_object_show(o);

	     if ((o_icon = e_border_icon_add(pw->border, drag->evas)))
	       {
		  evas_object_show(o_icon);
		  edje_object_part_swallow(o, "e.swallow.icon", o_icon);
	       }
	  }
	e_drag_resize(drag, w, h);
	e_drag_start(drag, x - pd->drag.dx, y - pd->drag.dy);

	pd->drag.from_pager = pd->pager;
	pd->drag.from_pager->dragging = 1;
	pd->drag.in_pager = 0;
     }
}

static void
_pager_desk_cb_drag_finished(E_Drag *drag, int dropped)
{
   Pager_Desk *pd;
   Pager_Desk *pd2 = NULL;
   Eina_List *l;
   E_Desk *desk;
   E_Zone *zone;
   Pager *p;

   pd = drag->data;
   if (!pd) return;
   if (!dropped)
     {
	/* wasn't dropped on pager, switch with current desktop */
	if (!pd->desk) return;
	zone = e_util_zone_current_get(e_manager_current_get());
	desk = e_desk_current_get(zone);
	for (l = pagers; l && !pd2; l = l->next)
	  {
	     p = l->data;
	     pd2 = _pager_desk_find(p, desk);
	  }
	_pager_desk_switch(pd, pd2);
     }
   if (pd->drag.from_pager)
     {
	pd->drag.from_pager->dragging = 0;
	pd->drag.from_pager->just_dragged = 0;
     }
   pd->drag.from_pager = NULL;

  if (act_popup)
     {
	e_grabinput_get(input_window, 0, input_window);

	if (!hold_count) _pager_popup_hide(1);
     }
}

static void
_pager_desk_cb_mouse_wheel(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Wheel *ev;
   Pager_Desk *pd;

   ev = event_info;
   pd = data;

   if (pd->pager->popup) return;

   if (pager_config->flip_desk)
     e_zone_desk_linear_flip_by(pd->desk->zone, ev->z);
}

static int
_pager_popup_cb_timeout(void *data)
{
   Pager_Popup *pp;

   pp = data;
   _pager_popup_free(pp);
   return 0;
}

/************************************************************************/
/* popup-on-keyaction functions */
static int
_pager_popup_show()
{
   E_Zone *zone;
   int x, y, w, h;
   Pager_Popup *pp;
   const char *drop[] = { "enlightenment/pager_win", "enlightenment/border", "enlightenment/vdesktop"};

   if (act_popup) return 0;

   zone = e_util_zone_current_get(e_manager_current_get());

   pp = _pager_popup_find(zone);
   if (pp) _pager_popup_free(pp);

   input_window = ecore_x_window_input_new(zone->container->win, 0, 0, 1, 1);
   ecore_x_window_show(input_window);
   if (!e_grabinput_get(input_window, 0, input_window))
     {
	ecore_x_window_free(input_window);
	input_window = 0;
	return 0;
     }

   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_KEY_DOWN, _pager_popup_cb_key_down, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_KEY_UP, _pager_popup_cb_key_up, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_BUTTON_DOWN, _pager_popup_cb_mouse_down, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_BUTTON_UP, _pager_popup_cb_mouse_up, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_WHEEL, _pager_popup_cb_mouse_wheel, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_MOVE, _pager_popup_cb_mouse_move, NULL));

   act_popup = _pager_popup_new(zone, 1);

   e_popup_ignore_events_set(act_popup->popup, 1);

   evas_object_geometry_get(act_popup->pager->o_table, &x, &y, &w, &h);

   e_drop_handler_add(E_OBJECT(act_popup->popup), act_popup->pager,
		      _pager_drop_cb_enter, _pager_drop_cb_move,
		      _pager_drop_cb_leave, _pager_drop_cb_drop,
		      drop, 3, x, y, w, h);

   e_bindings_key_grab(E_BINDING_CONTEXT_POPUP, act_popup->popup->evas_win);

   evas_event_freeze(act_popup->popup->evas);
   evas_event_feed_mouse_in(act_popup->popup->evas, ecore_x_current_time_get(), NULL);
   evas_event_feed_mouse_move(act_popup->popup->evas, -1000000, -1000000, ecore_x_current_time_get(), NULL);
   evas_event_thaw(act_popup->popup->evas);

   current_desk = e_desk_current_get(zone);

   return 1;
}

static void
_pager_popup_hide(int switch_desk)
{
   e_bindings_key_ungrab(E_BINDING_CONTEXT_POPUP, act_popup->popup->evas_win);
   _pager_popup_free(act_popup);
   act_popup = NULL;
   hold_count = 0;
   hold_mod = 0;
   while (handlers)
     {
	ecore_event_handler_del(handlers->data);
	handlers = eina_list_remove_list(handlers, handlers);
     }
   ecore_x_window_free(input_window);
   e_grabinput_release(input_window, input_window);
   input_window = 0;

   if (switch_desk && current_desk) e_desk_show(current_desk);
}

static void
_pager_popup_modifiers_set(int mod)
{
   if (!act_popup) return;
   hold_mod = mod;
   hold_count = 0;
   if (hold_mod & ECORE_EVENT_MODIFIER_SHIFT) hold_count++;
   if (hold_mod & ECORE_EVENT_MODIFIER_CTRL) hold_count++;
   if (hold_mod & ECORE_EVENT_MODIFIER_ALT) hold_count++;
   if (hold_mod & ECORE_EVENT_MODIFIER_WIN) hold_count++;
}

static void
_pager_popup_desk_switch(int x, int y)
{
   int max_x,max_y, desk_x, desk_y;
   Pager_Desk *pd;
   Pager_Popup *pp = act_popup;

   e_zone_desk_count_get(pp->pager->zone, &max_x, &max_y);

   desk_x = current_desk->x + x;
   desk_y = current_desk->y + y;

   if (desk_x < 0)
     desk_x = max_x - 1;
   else if (desk_x >= max_x)
     desk_x = 0;

   if (desk_y < 0)
     desk_y = max_y - 1;
   else if (desk_y >= max_y)
     desk_y = 0;

   current_desk = e_desk_at_xy_get(pp->pager->zone, desk_x, desk_y);

   pd = _pager_desk_find(pp->pager, current_desk);
   if (pd) _pager_desk_select(pd);

   edje_object_part_text_set(pp->o_bg, "e.text.label", current_desk->name);
}

static void
_pager_popup_cb_action_show(E_Object *obj, const char *params, Ecore_Event_Key *ev)
{
   if (_pager_popup_show())
     _pager_popup_modifiers_set(ev->modifiers);
}

static void
_pager_popup_cb_action_switch(E_Object *obj, const char *params, Ecore_Event_Key *ev)
{
   int x = 0;
   int y = 0;

   if (!act_popup)
     {
	if (_pager_popup_show())
	  _pager_popup_modifiers_set(ev->modifiers);
	else
	  return;
     }
   if (!strcmp(params, "left"))
     x = -1;
   else if (!strcmp(params, "right"))
     x = 1;
   else if (!strcmp(params, "up"))
     y = -1;
   else if (!strcmp(params, "down"))
     y = 1;

   _pager_popup_desk_switch(x, y);
}

static int
_pager_popup_cb_mouse_down(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Button *ev;
   Pager_Popup *pp = act_popup;

   ev = event;
   if (ev->window != input_window) return 1;

   evas_event_feed_mouse_down(pp->popup->evas, ev->buttons,
			      0, ev->timestamp, NULL);
   return 1;
}

static int
_pager_popup_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Button *ev;
   Pager_Popup *pp = act_popup;

   ev = event;
   if (ev->window != input_window) return 1;

   evas_event_feed_mouse_up(pp->popup->evas, ev->buttons,
			    0, ev->timestamp, NULL);
   return 1;
}

static int
_pager_popup_cb_mouse_move(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Move *ev;
   Pager_Popup *pp = act_popup;

   ev = event;
   if (ev->window != input_window) return 1;

   evas_event_feed_mouse_move(pp->popup->evas,
			      ev->x - pp->popup->x + pp->pager->zone->x,
			      ev->y - pp->popup->y + pp->pager->zone->y,
			      ev->timestamp, NULL);
   return 1;
}

static int
_pager_popup_cb_mouse_wheel(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Wheel *ev = event;
   Pager_Popup *pp = act_popup;
   int max_x;
   
   e_zone_desk_count_get(pp->pager->zone, &max_x, NULL);

   if (current_desk->x + ev->z >= max_x)
     _pager_popup_desk_switch(1, 1);
   else if (current_desk->x + ev->z < 0)
     _pager_popup_desk_switch(-1, -1);
   else
     _pager_popup_desk_switch(ev->z, 0);

   return 1;
}

static int
_pager_popup_cb_key_down(void *data, int type, void *event)
{
   Ecore_Event_Key *ev;

   ev = event;
   if (ev->window != input_window) return 1;
   if      (!strcmp(ev->key, "Up"))
     _pager_popup_desk_switch(0, -1);
   else if (!strcmp(ev->key, "Down"))
     _pager_popup_desk_switch(0, 1);
   else if (!strcmp(ev->key, "Left"))
     _pager_popup_desk_switch(-1, 0);
   else if (!strcmp(ev->key, "Right"))
     _pager_popup_desk_switch(1, 0);
   else if (!strcmp(ev->key, "Escape"))
     _pager_popup_hide(0);
   else
     {
	Eina_List *l;

	for (l = e_config->key_bindings; l; l = l->next)
	  {
	     E_Config_Binding_Key *bind;
	     E_Binding_Modifier mod = 0;

	     bind = l->data;

	     if (bind->action && strcmp(bind->action,"pager_switch")) continue;

	     if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT) mod |= E_BINDING_MODIFIER_SHIFT;
	     if (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) mod |= E_BINDING_MODIFIER_CTRL;
	     if (ev->modifiers & ECORE_EVENT_MODIFIER_ALT) mod |= E_BINDING_MODIFIER_ALT;
	     if (ev->modifiers & ECORE_EVENT_MODIFIER_WIN) mod |= E_BINDING_MODIFIER_WIN;

	     if (bind->key && (!strcmp(bind->key, ev->keyname)) && ((bind->modifiers == mod)))
	       {
		  E_Action *act;

		  act = e_action_find(bind->action);

		  if (act)
		    {
		       if (act->func.go_key)
			 act->func.go_key(NULL, bind->params, ev);
		    }
	       }
	  }
     }
   return 1;
}

static int
_pager_popup_cb_key_up(void *data, int type, void *event)
{
   Ecore_Event_Key *ev;

   ev = event;
   if (!(act_popup)) return 1;

   if (hold_mod)
     {
	if      ((hold_mod & ECORE_EVENT_MODIFIER_SHIFT) && (!strcmp(ev->key, "Shift_L"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_SHIFT) && (!strcmp(ev->key, "Shift_R"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_CTRL) && (!strcmp(ev->key, "Control_L"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_CTRL) && (!strcmp(ev->key, "Control_R"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_ALT) && (!strcmp(ev->key, "Alt_L"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_ALT) && (!strcmp(ev->key, "Alt_R"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_ALT) && (!strcmp(ev->key, "Meta_L"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_ALT) && (!strcmp(ev->key, "Meta_R"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_ALT) && (!strcmp(ev->key, "Super_L"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_ALT) && (!strcmp(ev->key, "Super_R"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_WIN) && (!strcmp(ev->key, "Super_L"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_WIN) && (!strcmp(ev->key, "Super_R"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_WIN) && (!strcmp(ev->key, "Mode_switch"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_WIN) && (!strcmp(ev->key, "Meta_L"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_WIN) && (!strcmp(ev->key, "Meta_R"))) hold_count--;
	if (hold_count <= 0 && !act_popup->pager->dragging)
	  {
	     _pager_popup_hide(1);
	     return 1;
	  }
     }

   return 1;
}

/***************************************************************************/
/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Pager"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   conf_edd = E_CONFIG_DD_NEW("Pager_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, popup, UINT);
   E_CONFIG_VAL(D, T, popup_speed, DOUBLE);
   E_CONFIG_VAL(D, T, popup_urgent, UINT);
   E_CONFIG_VAL(D, T, popup_urgent_stick, UINT);
   E_CONFIG_VAL(D, T, popup_urgent_speed, DOUBLE);
   E_CONFIG_VAL(D, T, show_desk_names, UINT);
   E_CONFIG_VAL(D, T, popup_height, INT);
   E_CONFIG_VAL(D, T, popup_act_height, INT);
   E_CONFIG_VAL(D, T, drag_resist, UINT);
   E_CONFIG_VAL(D, T, btn_drag, UCHAR);
   E_CONFIG_VAL(D, T, btn_noplace, UCHAR);
   E_CONFIG_VAL(D, T, btn_desk, UCHAR);
   E_CONFIG_VAL(D, T, flip_desk, UCHAR);

   pager_config = e_config_domain_load("module.pager", conf_edd);

   if (!pager_config)
     {
	pager_config = E_NEW(Config, 1);
	pager_config->popup = 1;
	pager_config->popup_speed = 1.0;
	pager_config->popup_urgent = 0;
	pager_config->popup_urgent_stick = 0;
	pager_config->popup_urgent_speed = 1.5;
	pager_config->show_desk_names = 1;
	pager_config->popup_height = 60;
	pager_config->popup_act_height = 60;
	pager_config->drag_resist = 3;
	pager_config->btn_drag = 1;
	pager_config->btn_noplace = 2;
	pager_config->btn_desk = 0;
	pager_config->flip_desk = 0;
     }
   E_CONFIG_LIMIT(pager_config->popup, 0, 1);
   E_CONFIG_LIMIT(pager_config->popup_speed, 0.1, 10.0);
   E_CONFIG_LIMIT(pager_config->popup_urgent, 0, 1);
   E_CONFIG_LIMIT(pager_config->popup_urgent_stick, 0, 1);
   E_CONFIG_LIMIT(pager_config->popup_urgent_speed, 0.1, 10.0);
   E_CONFIG_LIMIT(pager_config->show_desk_names, 0, 1);
   E_CONFIG_LIMIT(pager_config->popup_height, 20, 200);
   E_CONFIG_LIMIT(pager_config->popup_act_height, 20, 200);
   E_CONFIG_LIMIT(pager_config->drag_resist, 0, 50);
   E_CONFIG_LIMIT(pager_config->flip_desk, 0, 1);
   E_CONFIG_LIMIT(pager_config->btn_drag, 0, 32);
   E_CONFIG_LIMIT(pager_config->btn_noplace, 0, 32);
   E_CONFIG_LIMIT(pager_config->btn_desk, 0, 32);

   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_RESIZE, _pager_cb_event_border_resize, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_MOVE, _pager_cb_event_border_move, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_ADD, _pager_cb_event_border_add, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_REMOVE, _pager_cb_event_border_remove, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_ICONIFY, _pager_cb_event_border_iconify, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_UNICONIFY, _pager_cb_event_border_uniconify, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_STICK, _pager_cb_event_border_stick, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_UNSTICK, _pager_cb_event_border_unstick, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_DESK_SET, _pager_cb_event_border_desk_set, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_STACK, _pager_cb_event_border_stack, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_ICON_CHANGE, _pager_cb_event_border_icon_change, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_URGENT_CHANGE, _pager_cb_event_border_urgent_change, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_FOCUS_IN,
       _pager_cb_event_border_focus_in, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_FOCUS_OUT,
       _pager_cb_event_border_focus_out, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_PROPERTY, _pager_cb_event_border_property, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_ZONE_DESK_COUNT_SET, _pager_cb_event_zone_desk_count_set, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_DESK_SHOW, _pager_cb_event_desk_show, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_DESK_NAME_CHANGE, _pager_cb_event_desk_name_change, NULL));
   pager_config->handlers = eina_list_append
     (pager_config->handlers, ecore_event_handler_add
      (E_EVENT_CONTAINER_RESIZE, _pager_cb_event_container_resize, NULL));

   pager_config->module = m;

   e_gadcon_provider_register(&_gadcon_class);

   e_configure_registry_item_add("extensions/pager", 40, N_("Pager"), NULL,
				 "preferences-pager", _pager_config_dialog);

   act_popup_show = e_action_add("pager_show");
   if (act_popup_show)
     {
	act_popup_show->func.go_key = _pager_popup_cb_action_show;
	e_action_predef_name_set(N_("Pager"), N_("Show Pager Popup"), "pager_show", "<none>", NULL, 0);
     }
   act_popup_switch = e_action_add("pager_switch");
   if (act_popup_switch)
     {
	act_popup_switch->func.go_key = _pager_popup_cb_action_switch;
	e_action_predef_name_set(N_("Pager"), N_("Popup Desk Right"), "pager_switch", "right", NULL, 0);
	e_action_predef_name_set(N_("Pager"), N_("Popup Desk Left"),  "pager_switch", "left",  NULL, 0);
	e_action_predef_name_set(N_("Pager"), N_("Popup Desk Up"),    "pager_switch", "up",    NULL, 0);
	e_action_predef_name_set(N_("Pager"), N_("Popup Desk Down"),  "pager_switch", "down",  NULL, 0);
     }

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   e_gadcon_provider_unregister(&_gadcon_class);

   if (pager_config->config_dialog)
     e_object_del(E_OBJECT(pager_config->config_dialog));
   while (pager_config->handlers)
     {
	ecore_event_handler_del(pager_config->handlers->data);
	pager_config->handlers = eina_list_remove_list(pager_config->handlers, pager_config->handlers);
     }

   if (pager_config->menu)
     {
	e_menu_post_deactivate_callback_set(pager_config->menu, NULL, NULL);
	e_object_del(E_OBJECT(pager_config->menu));
	pager_config->menu = NULL;
     }

   e_configure_registry_item_del("extensions/pager");

   e_action_del("pager_show");
   e_action_del("pager_switch");

   e_action_predef_name_del(N_("Pager"), N_("Popup Desk Right"));
   e_action_predef_name_del(N_("Pager"), N_("Popup Desk Left"));
   e_action_predef_name_del(N_("Pager"), N_("Popup Desk Up"));
   e_action_predef_name_del(N_("Pager"), N_("Popup Desk Down"));

   E_FREE(pager_config);
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   e_config_domain_save("module.pager", conf_edd, pager_config);
   return 1;
}
