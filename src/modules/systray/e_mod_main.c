/**
 * systray implementation following freedesktop.org specification.
 *
 * @see: http://standards.freedesktop.org/systemtray-spec/latest/
 *
 * @todo: implement xembed, mostly done, at least relevant parts are done.
 *        http://standards.freedesktop.org/xembed-spec/latest/
 *
 * @todo: implement messages/popup part of the spec (anyone using this at all?)
 */

#include "e.h"
#include "e_mod_main.h"

#define RETRY_TIMEOUT 2.0

#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2
#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0
#define _NET_SYSTEM_TRAY_ORIENTATION_VERT 1

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY		0
#define XEMBED_WINDOW_ACTIVATE  	1
#define XEMBED_WINDOW_DEACTIVATE  	2
#define XEMBED_REQUEST_FOCUS	 	3
#define XEMBED_FOCUS_IN 	 	4
#define XEMBED_FOCUS_OUT  		5
#define XEMBED_FOCUS_NEXT 		6
#define XEMBED_FOCUS_PREV 		7
/* 8-9 were used for XEMBED_GRAB_KEY/XEMBED_UNGRAB_KEY */
#define XEMBED_MODALITY_ON 		10
#define XEMBED_MODALITY_OFF 		11
#define XEMBED_REGISTER_ACCELERATOR     12
#define XEMBED_UNREGISTER_ACCELERATOR   13
#define XEMBED_ACTIVATE_ACCELERATOR     14

/* Details for  XEMBED_FOCUS_IN: */
#define XEMBED_FOCUS_CURRENT		0
#define XEMBED_FOCUS_FIRST 		1
#define XEMBED_FOCUS_LAST		2

typedef struct _Instance Instance;
typedef struct _Icon Icon;

struct _Icon
{
   Ecore_X_Window win;
   Evas_Object *o;
   Instance *inst;
};

struct _Instance
{
   E_Gadcon_Client *gcc;
   E_Container *con;
   Evas *evas;
   struct
   {
      Ecore_X_Window parent;
      Ecore_X_Window base;
      Ecore_X_Window selection;
   } win;
   struct
   {
      Evas_Object *gadget;
   } ui;
   struct
   {
      Ecore_Event_Handler *message;
      Ecore_Event_Handler *destroy;
      Ecore_Event_Handler *show;
      Ecore_Event_Handler *reparent;
      Ecore_Event_Handler *sel_clear;
      Ecore_Event_Handler *configure;
   } handler;
   struct
   {
      Ecore_Timer *retry;
   } timer;
   struct
   {
      Ecore_Job *size_apply;
   } job;
   Eina_List *icons;
   E_Menu *menu;
};

static const char _Name[] = "Systray";
static const char _name[] = "systray";
static const char _group_gadget[] = "e/modules/systray/main";
static const char _part_box[] = "e.box";
static const char _part_size[] = "e.size";
static const char _sig_source[] = "e";
static const char _sig_enable[] = "e,action,enable";
static const char _sig_disable[] = "e,action,disable";

static Ecore_X_Atom _atom_manager = 0;
static Ecore_X_Atom _atom_st_orient = 0;
static Ecore_X_Atom _atom_st_visual = 0;
static Ecore_X_Atom _atom_st_op_code = 0;
static Ecore_X_Atom _atom_st_msg_data = 0;
static Ecore_X_Atom _atom_xembed = 0;
static Ecore_X_Atom _atom_xembed_info = 0;
static Ecore_X_Atom _atom_st_num = 0;
static int _last_st_num = -1;

static E_Module *systray_mod = NULL;
static Instance *instance = NULL; /* only one systray ever possible */
static char tmpbuf[PATH_MAX]; /* general purpose buffer, just use immediately */

static Eina_Bool 
_systray_site_is_safe(E_Gadcon_Site site) 
{
   if (e_gadcon_site_is_shelf(site))
     return EINA_TRUE;

   return EINA_FALSE;
}

static const char *
_systray_theme_path(void)
{
#define TF "/e-module-systray.edj"
   unsigned int dirlen;
   const char *moddir = e_module_dir_get(systray_mod);

   dirlen = strlen(moddir);
   if (dirlen >= sizeof(tmpbuf) - sizeof(TF))
     return NULL;

   memcpy(tmpbuf, moddir, dirlen);
   memcpy(tmpbuf + dirlen, TF, sizeof(TF));

   return tmpbuf;
#undef TF
}

static void
_systray_menu_cb_post(void *data, E_Menu *menu __UNUSED__)
{
   Instance *inst = data;
   if (!inst->menu) return;
   e_object_del(E_OBJECT(inst->menu));
   inst->menu = NULL;
}

static void
_systray_menu_new(Instance *inst, Evas_Event_Mouse_Down *ev)
{
   E_Zone *zone;
   E_Menu *m;
   int x, y;

   zone = e_util_zone_current_get(e_manager_current_get());

   m = e_menu_new();
   m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
   e_menu_post_deactivate_callback_set(m, _systray_menu_cb_post, inst);
   inst->menu = m;
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y, NULL, NULL);
   e_menu_activate_mouse(m, zone, x + ev->output.x, y + ev->output.y,
			 1, 1, E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
}

static void
_systray_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
   Instance *inst = data;
   Evas_Event_Mouse_Down *ev = event;

   if ((ev->button == 3) && (!inst->menu))
     _systray_menu_new(inst, ev);
}

static void
_systray_size_apply_do(Instance *inst)
{
   const Evas_Object *o;
   Evas_Coord x, y, w, h;

   edje_object_message_signal_process(inst->ui.gadget);
   o = edje_object_part_object_get(inst->ui.gadget, _part_box);
   if (!o) return;
   evas_object_size_hint_min_get(o, &w, &h);

   if (w < 1) w = 1;
   if (h < 1) h = 1;

   if (eina_list_count(inst->icons) == 0)
      ecore_x_window_hide(inst->win.base);
   else
      ecore_x_window_show(inst->win.base);

   e_gadcon_client_aspect_set(inst->gcc, w, h);
   e_gadcon_client_min_size_set(inst->gcc, w, h);

   evas_object_geometry_get(o, &x, &y, &w, &h);
   ecore_x_window_move_resize(inst->win.base, x, y, w, h);
}

static void
_systray_size_apply_delayed(void *data)
{
   Instance *inst = data;
   _systray_size_apply_do(inst);
   inst->job.size_apply = NULL;
}

static void
_systray_size_apply(Instance *inst)
{
   if (inst->job.size_apply) return;
   inst->job.size_apply = ecore_job_add(_systray_size_apply_delayed, inst);
}

static void
_systray_cb_move(void *data, Evas *evas __UNUSED__, Evas_Object *o __UNUSED__, void *event __UNUSED__)
{
   Instance *inst = data;
   _systray_size_apply(inst);
}

static void
_systray_cb_resize(void *data, Evas *evas __UNUSED__, Evas_Object *o __UNUSED__, void *event __UNUSED__)
{
   Instance *inst = data;
   _systray_size_apply(inst);
}

static void
_systray_icon_geometry_apply(Icon *icon)
{
   const Evas_Object *o;
   Evas_Coord x, y, w, h, wx, wy;

   o = edje_object_part_object_get(icon->inst->ui.gadget, _part_size);
   if (!o) return;

   evas_object_geometry_get(icon->o, &x, &y, &w, &h);
   evas_object_geometry_get(o, &wx, &wy, NULL, NULL);
   ecore_x_window_move_resize(icon->win, x - wx, y - wy, w, h);
}

static void
_systray_icon_cb_move(void *data, Evas *evas __UNUSED__, Evas_Object *o __UNUSED__, void *event __UNUSED__)
{
   Icon *icon = data;
   _systray_icon_geometry_apply(icon);
}

static void
_systray_icon_cb_resize(void *data, Evas *evas __UNUSED__, Evas_Object *o __UNUSED__, void *event __UNUSED__)
{
   Icon *icon = data;
   _systray_icon_geometry_apply(icon);
}

static Ecore_X_Gravity
_systray_gravity(const Instance *inst)
{
   switch (inst->gcc->gadcon->orient)
     {
      case E_GADCON_ORIENT_FLOAT:
	 return ECORE_X_GRAVITY_STATIC;
      case E_GADCON_ORIENT_HORIZ:
	 return ECORE_X_GRAVITY_CENTER;
      case E_GADCON_ORIENT_VERT:
	 return ECORE_X_GRAVITY_CENTER;
      case E_GADCON_ORIENT_LEFT:
	 return ECORE_X_GRAVITY_CENTER;
      case E_GADCON_ORIENT_RIGHT:
	 return ECORE_X_GRAVITY_CENTER;
      case E_GADCON_ORIENT_TOP:
	 return ECORE_X_GRAVITY_CENTER;
      case E_GADCON_ORIENT_BOTTOM:
	 return ECORE_X_GRAVITY_CENTER;
      case E_GADCON_ORIENT_CORNER_TL:
	 return ECORE_X_GRAVITY_S;
      case E_GADCON_ORIENT_CORNER_TR:
	 return ECORE_X_GRAVITY_S;
      case E_GADCON_ORIENT_CORNER_BL:
	 return ECORE_X_GRAVITY_N;
      case E_GADCON_ORIENT_CORNER_BR:
	 return ECORE_X_GRAVITY_N;
      case E_GADCON_ORIENT_CORNER_LT:
	 return ECORE_X_GRAVITY_E;
      case E_GADCON_ORIENT_CORNER_RT:
	 return ECORE_X_GRAVITY_W;
      case E_GADCON_ORIENT_CORNER_LB:
	 return ECORE_X_GRAVITY_E;
      case E_GADCON_ORIENT_CORNER_RB:
	 return ECORE_X_GRAVITY_W;
      default:
	 return ECORE_X_GRAVITY_STATIC;
     }
}

static Evas_Coord
_systray_icon_size_normalize(Evas_Coord size)
{
   const Evas_Coord *itr, sizes[] = {
     16, 22, 24, 32, 36, 48, 64, 72, 96, 128, 192, 256, -1
   };
   for (itr = sizes; *itr > 0; itr++)
     if (*itr == size)
       return size;
     else if (*itr > size)
       {
	  if (itr > sizes)
	    return itr[-1];
	  else
	    return sizes[0];
       }
   return sizes[0];
}

static Icon *
_systray_icon_add(Instance *inst, const Ecore_X_Window win)
{
   Ecore_X_Gravity gravity;
   Evas_Object *o;
   Evas_Coord w, h;
   Icon *icon;

   edje_object_part_geometry_get(inst->ui.gadget, _part_size,
				 NULL, NULL, &w, &h);
   if (w > h)
     w = h;
   else
     h = w;

   w = h = _systray_icon_size_normalize(w);

   o = evas_object_rectangle_add(inst->evas);
   if (!o)
     return NULL;
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_resize(o, w, h);
   evas_object_show(o);

   icon = malloc(sizeof(*icon));
   if (!icon)
     {
	evas_object_del(o);
	return NULL;
     }
   icon->win = win;
   icon->inst = inst;
   icon->o = o;

   gravity = _systray_gravity(inst);
   ecore_x_icccm_size_pos_hints_set(win, 1, gravity,
				    w, h, w, h, w, h, 0, 0,
				    1.0, (double)w / (double)h);

   ecore_x_window_reparent(win, inst->win.base, 0, 0);
   ecore_x_window_resize(win, w, h);
   ecore_x_window_raise(win);
   ecore_x_window_client_manage(win);
   ecore_x_window_save_set_add(win);
   ecore_x_window_shape_events_select(win, 1);

   //ecore_x_window_geometry_get(win, NULL, NULL, &w, &h);

   evas_object_event_callback_add
     (o, EVAS_CALLBACK_MOVE, _systray_icon_cb_move, icon);
   evas_object_event_callback_add
     (o, EVAS_CALLBACK_RESIZE, _systray_icon_cb_resize, icon);

   inst->icons = eina_list_append(inst->icons, icon);
   edje_object_part_box_append(inst->ui.gadget, _part_box, o);
   _systray_size_apply_do(inst);
   _systray_icon_geometry_apply(icon);

   ecore_x_window_show(win);

   return icon;
}

static void
_systray_icon_del_list(Instance *inst, Eina_List *l, Icon *icon)
{
   inst->icons = eina_list_remove_list(inst->icons, l);

   ecore_x_window_save_set_del(icon->win);
   ecore_x_window_reparent(icon->win, 0, 0, 0);
   evas_object_del(icon->o);
   free(icon);

   _systray_size_apply(inst);
}

static Ecore_X_Atom
_systray_atom_st_get(int screen_num)
{
   if ((_last_st_num == -1) || (_last_st_num != screen_num))
     {
	char buf[32];
	snprintf(buf, sizeof(buf), "_NET_SYSTEM_TRAY_S%d", screen_num);
	_atom_st_num = ecore_x_atom_get(buf);
	_last_st_num = screen_num;
     }

   return _atom_st_num;
}

static Eina_Bool
_systray_selection_owner_set(int screen_num, Ecore_X_Window win)
{
   Ecore_X_Atom atom;
   Ecore_X_Window cur_selection;
   Eina_Bool ret;

   atom = _systray_atom_st_get(screen_num);
   ecore_x_selection_owner_set(win, atom, ecore_x_current_time_get());
   ecore_x_sync();
   cur_selection = ecore_x_selection_owner_get(atom);

   ret = (cur_selection == win);
   if (!ret)
     fprintf(stderr, "SYSTRAY: tried to set selection to %#x, but got %#x\n",
	     win, cur_selection);

   return ret;
}

static Eina_Bool
_systray_selection_owner_set_current(Instance *inst)
{
   return _systray_selection_owner_set
     (inst->con->manager->num, inst->win.selection);
}

static void
_systray_deactivate(Instance *inst)
{
   Ecore_X_Window old;

   if (inst->win.selection == 0) return;

   edje_object_signal_emit(inst->ui.gadget, _sig_disable, _sig_source);

   while (inst->icons)
     _systray_icon_del_list(inst, inst->icons, inst->icons->data);

   old = inst->win.selection;
   inst->win.selection = 0;
   _systray_selection_owner_set_current(inst);
   ecore_x_sync();
   ecore_x_window_free(old);
   ecore_x_window_free(inst->win.base);
   inst->win.base = 0;
}

static Eina_Bool
_systray_base_create(Instance *inst)
{
   const Evas_Object *o;
   Evas_Coord x, y, w, h;
   unsigned short r, g, b;
   const char *color;

   color = edje_object_data_get(inst->ui.gadget, inst->gcc->style);
   if (!color)
     color = edje_object_data_get(inst->ui.gadget, "default");

   if (color && (sscanf(color, "%hu %hu %hu", &r, &g, &b) == 3))
     {
	r = (65535 * (unsigned int)r) / 255;
	g = (65535 * (unsigned int)g) / 255;
	b = (65535 * (unsigned int)b) / 255;
     }
   else
     r = g = b = (unsigned short)65535;

   o = edje_object_part_object_get(inst->ui.gadget, _part_size);
   if (!o)
     return 0;

   evas_object_geometry_get(o, &x, &y, &w, &h);
   if (w < 1) w = 1;
   if (h < 1) h = 1;
   inst->win.base = ecore_x_window_new(0, 0, 0, w, h);
   ecore_x_window_reparent(inst->win.base, inst->win.parent, x, y); 
   ecore_x_window_background_color_set(inst->win.base, r, g, b);
   ecore_x_window_show(inst->win.base);
   return 1;
}

static Eina_Bool
_systray_activate(Instance *inst)
{
   unsigned int visual;
   Ecore_X_Atom atom;
   Ecore_X_Window old_win;
   Ecore_X_Window_Attributes attr;

   if (inst->win.selection != 0) return 1;

   atom = _systray_atom_st_get(inst->con->manager->num);
   old_win = ecore_x_selection_owner_get(atom);
   if (old_win != 0) return 0;

   if (inst->win.base == 0)
     {
	if (!_systray_base_create(inst))
	  return 0;
     }

   inst->win.selection = ecore_x_window_input_new(inst->win.base, 0, 0, 1, 1);
   if (inst->win.selection == 0)
     {
	ecore_x_window_free(inst->win.base);
	inst->win.base = 0;
	return 0;
     }

   if (!_systray_selection_owner_set_current(inst))
     {
	ecore_x_window_free(inst->win.selection);
	inst->win.selection = 0;
	ecore_x_window_free(inst->win.base);
	inst->win.base = 0;
	return 0;
     }

   ecore_x_window_attributes_get(inst->win.base, &attr);

   visual = ecore_x_visual_id_get(attr.visual);
   ecore_x_window_prop_card32_set(inst->win.selection, _atom_st_visual,
                                  (void *)&visual, 1);

   ecore_x_client_message32_send(inst->con->manager->root, _atom_manager,
				 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
				 ecore_x_current_time_get(), atom,
				 inst->win.selection, 0, 0);

   edje_object_signal_emit(inst->ui.gadget, _sig_enable, _sig_source);

   return 1;
}

static Eina_Bool
_systray_activate_retry(void *data)
{
   Instance *inst = data;
   Eina_Bool ret;

   fputs("SYSTRAY: reactivate...\n", stderr);
   ret = _systray_activate(inst);
   if (ret)
     fputs("SYSTRAY: activate success!\n", stderr);
   else
     fprintf(stderr, "SYSTRAY: activate failure! retrying in %0.1f seconds\n",
	     RETRY_TIMEOUT);

   if (!ret)
     return ECORE_CALLBACK_RENEW;

   inst->timer.retry = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_systray_retry(Instance *inst)
{
   if (inst->timer.retry) return;
   inst->timer.retry = ecore_timer_add
     (RETRY_TIMEOUT, _systray_activate_retry, inst);
}

static Eina_Bool
_systray_activate_retry_first(void *data)
{
   Instance *inst = data;
   Eina_Bool ret;

   fputs("SYSTRAY: reactivate (first time)...\n", stderr);
   ret = _systray_activate(inst);
   if (ret)
     {
	fputs("SYSTRAY: activate success!\n", stderr);
	inst->timer.retry = NULL;
	return ECORE_CALLBACK_CANCEL;
     }

   edje_object_signal_emit(inst->ui.gadget, _sig_disable, _sig_source);

   fprintf(stderr, "SYSTRAY: activate failure! retrying in %0.1f seconds\n",
	   RETRY_TIMEOUT);

   inst->timer.retry = NULL;
   _systray_retry(inst);
   return ECORE_CALLBACK_CANCEL;
}

static void
_systray_handle_request_dock(Instance *inst, Ecore_X_Event_Client_Message *ev)
{
   Ecore_X_Window win = (Ecore_X_Window)ev->data.l[2];
   Ecore_X_Time time;
   Ecore_X_Window_Attributes attr;
   const Eina_List *l;
   Icon *icon;
   unsigned int val[2];
   int r;

   EINA_LIST_FOREACH(inst->icons, l, icon)
     if (icon->win == win)
       return;

   if (!ecore_x_window_attributes_get(win, &attr))
     {
	fprintf(stderr, "SYSTRAY: could not get attributes of win %#x\n", win);
	return;
     }

   icon = _systray_icon_add(inst, win);
   if (!icon)
     return;

   r  = ecore_x_window_prop_card32_get(win, _atom_xembed_info, val, 2);
   if (r < 2)
     {
	/*
	fprintf(stderr, "SYSTRAY: win %#x does not support _XEMBED_INFO (%d)\n",
		win, r);
	*/
	return;
     }

   time = ecore_x_current_time_get();
   ecore_x_client_message32_send(win, _atom_xembed,
				 ECORE_X_EVENT_MASK_NONE,
				 time, XEMBED_EMBEDDED_NOTIFY, 0,
				 inst->win.selection, 0);
}

static void
_systray_handle_op_code(Instance *inst, Ecore_X_Event_Client_Message *ev)
{
   unsigned long message = ev->data.l[1];

   switch (message)
     {
      case SYSTEM_TRAY_REQUEST_DOCK:
	 _systray_handle_request_dock(inst, ev);
	 break;
      case SYSTEM_TRAY_BEGIN_MESSAGE:
      case SYSTEM_TRAY_CANCEL_MESSAGE:
	 fputs("SYSTRAY TODO: handle messages (anyone uses this?)\n", stderr);
	 break;
      default:
	 fprintf(stderr,
		 "SYSTRAY: error, unknown message op code: %ld, win: %#lx\n",
		 message, ev->data.l[2]);
     }
}

static void
_systray_handle_message(Instance *inst __UNUSED__, Ecore_X_Event_Client_Message *ev)
{
   fprintf(stderr, "SYSTRAY TODO: message op: %ld, data: %ld, %ld, %ld\n",
	   ev->data.l[1], ev->data.l[2], ev->data.l[3], ev->data.l[4]);
}

static void
_systray_handle_xembed(Instance *inst __UNUSED__, Ecore_X_Event_Client_Message *ev __UNUSED__)
{
   unsigned long message = ev->data.l[1];

   switch (message)
     {
      case XEMBED_EMBEDDED_NOTIFY:
      case XEMBED_WINDOW_ACTIVATE:
      case XEMBED_WINDOW_DEACTIVATE:
      case XEMBED_REQUEST_FOCUS:
      case XEMBED_FOCUS_IN:
      case XEMBED_FOCUS_OUT:
      case XEMBED_FOCUS_NEXT:
      case XEMBED_FOCUS_PREV:
      case XEMBED_MODALITY_ON:
      case XEMBED_MODALITY_OFF:
      case XEMBED_REGISTER_ACCELERATOR:
      case XEMBED_UNREGISTER_ACCELERATOR:
      case XEMBED_ACTIVATE_ACCELERATOR:
      default:
	 fprintf(stderr,
		 "SYSTRAY: unsupported xembed: %#lx, %#lx, %#lx, %#lx\n",
		 ev->data.l[1], ev->data.l[2], ev->data.l[3], ev->data.l[4]);
     }
}

static Eina_Bool
_systray_cb_client_message(void *data, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Client_Message *ev = event;
   Instance *inst = data;

   if (ev->message_type == _atom_st_op_code)
     _systray_handle_op_code(inst, ev);
   else if (ev->message_type == _atom_st_msg_data)
     _systray_handle_message(inst, ev);
   else if (ev->message_type == _atom_xembed)
     _systray_handle_xembed(inst, ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_systray_cb_window_destroy(void *data, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Destroy *ev = event;
   Instance *inst = data;
   Icon *icon;
   Eina_List *l;

   EINA_LIST_FOREACH(inst->icons, l, icon)
     if (icon->win == ev->win)
       {
	  _systray_icon_del_list(inst, l, icon);
	  break;
       }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_systray_cb_window_show(void *data, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Show *ev = event;
   Instance *inst = data;
   Icon *icon;
   Eina_List *l;

   EINA_LIST_FOREACH(inst->icons, l, icon)
     if (icon->win == ev->win)
       {
	  _systray_icon_geometry_apply(icon);
	  break;
       }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_systray_cb_window_configure(void *data, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Configure *ev = event;
   Instance *inst = data;
   Icon *icon;
   const Eina_List *l;

   EINA_LIST_FOREACH(inst->icons, l, icon)
     if (icon->win == ev->win)
       {
	  _systray_icon_geometry_apply(icon);
	  break;
       }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_systray_cb_reparent_notify(void *data, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Reparent *ev = event;
   Instance *inst = data;
   Icon *icon;
   Eina_List *l;

   EINA_LIST_FOREACH(inst->icons, l, icon)
     if ((icon->win == ev->win) && (ev->parent != inst->win.base))
       {
	  _systray_icon_del_list(inst, l, icon);
	  break;
       }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_systray_cb_selection_clear(void *data, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Selection_Clear *ev = event;
   Instance *inst = data;

   if ((ev->win == inst->win.selection) && (inst->win.selection != 0) &&
       (ev->atom == _systray_atom_st_get(inst->con->manager->num)))
     {
	edje_object_signal_emit(inst->ui.gadget, _sig_disable, _sig_source);

	while (inst->icons)
	  _systray_icon_del_list(inst, inst->icons, inst->icons->data);

	ecore_x_window_free(inst->win.selection);
	inst->win.selection = 0;
	ecore_x_window_free(inst->win.base);
	inst->win.base = 0;
	_systray_retry(inst);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static void
_systray_theme(Evas_Object *o, const char *shelf_style, const char *gc_style)
{
   const char base_theme[] = "base/theme/modules/systray";
   const char *path = _systray_theme_path();
   char buf[128], *p;
   size_t len, avail;

   len = eina_strlcpy(buf, _group_gadget, sizeof(buf));
   if (len >= sizeof(buf))
     goto fallback;
   p = buf + len;
   *p = '/';
   p++;
   avail = sizeof(buf) - len - 1;

   if (shelf_style && gc_style)
     {
	size_t r;
	r = snprintf(p, avail, "%s/%s", shelf_style, gc_style);
	if (r < avail && e_theme_edje_object_set(o, base_theme, buf))
	  return;
     }

   if (shelf_style)
     {
	size_t r;
	r = eina_strlcpy(p, shelf_style, avail);
	if (r < avail && e_theme_edje_object_set(o, base_theme, buf))
	  return;
     }

   if (gc_style)
     {
	size_t r;
	r = eina_strlcpy(p, gc_style, avail);
	if (r < avail && e_theme_edje_object_set(o, base_theme, buf))
	  return;
     }

   if (e_theme_edje_object_set(o, base_theme, _group_gadget))
     return;

   if (shelf_style && gc_style)
     {
	size_t r;
	r = snprintf(p, avail, "%s/%s", shelf_style, gc_style);
	if (r < avail && edje_object_file_set(o, path, buf))
	  return;
     }

   if (shelf_style)
     {
	size_t r;
	r = eina_strlcpy(p, shelf_style, avail);
	if (r < avail && edje_object_file_set(o, path, buf))
	  return;
     }

   if (gc_style)
     {
	size_t r;
	r = eina_strlcpy(p, gc_style, avail);
	if (r < avail && edje_object_file_set(o, path, buf))
	  return;
     }

 fallback:
   edje_object_file_set(o, path, _group_gadget);
}


static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Instance *inst;

   // fprintf(stderr, "SYSTRAY: init name=%s, id=%s, style=%s\n", name, id, style);

   if (!systray_mod)
     return NULL;
   if ((!id) || (instance))
     {
	e_util_dialog_internal
	  (_("Another systray exists"),
	   _("There can be only one systray gadget and "
	     "another one already exists."));
	return NULL;
     }

   if ((gc->shelf) && (!gc->shelf->popup))
     {
	e_util_dialog_internal
	  (_("Systray Error"),
	   _("Systray cannot work in a shelf that is set to below everything."));
	return NULL;
     }

   inst = E_NEW(Instance, 1);
   if (!inst)
     return NULL;
   inst->evas = gc->evas;
   inst->con = e_container_current_get(e_manager_current_get());
   if (!inst->con)
     {
	E_FREE(inst);
	return NULL;
     }

   if ((gc->shelf) && (gc->shelf->popup))
     inst->win.parent = gc->shelf->popup->evas_win;
   else
     inst->win.parent = (Ecore_X_Window) ecore_evas_window_get(gc->ecore_evas);

   inst->win.base = 0;
   inst->win.selection = 0;

   inst->ui.gadget = edje_object_add(inst->evas);

   _systray_theme(inst->ui.gadget, gc->shelf ? gc->shelf->style : NULL, style);

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->ui.gadget);
   if (!inst->gcc)
     {
	evas_object_del(inst->ui.gadget);
	E_FREE(inst);
	return NULL;
     }

   inst->gcc->data = inst;

   if (!_systray_activate(inst))
     {
	if (!inst->timer.retry)
	  inst->timer.retry = ecore_timer_add
	    (0.1, _systray_activate_retry_first, inst);
	else
	  edje_object_signal_emit(inst->ui.gadget, _sig_disable, _sig_source);
     }

   evas_object_event_callback_add(inst->ui.gadget, EVAS_CALLBACK_MOUSE_DOWN,
				  _systray_cb_mouse_down, inst);
   evas_object_event_callback_add(inst->ui.gadget, EVAS_CALLBACK_MOVE,
				  _systray_cb_move, inst);
   evas_object_event_callback_add(inst->ui.gadget, EVAS_CALLBACK_RESIZE,
				  _systray_cb_resize, inst);

   inst->handler.message = ecore_event_handler_add
     (ECORE_X_EVENT_CLIENT_MESSAGE, _systray_cb_client_message, inst);
   inst->handler.destroy = ecore_event_handler_add
     (ECORE_X_EVENT_WINDOW_DESTROY, _systray_cb_window_destroy, inst);
   inst->handler.show = ecore_event_handler_add
     (ECORE_X_EVENT_WINDOW_SHOW, _systray_cb_window_show, inst);
   inst->handler.reparent = ecore_event_handler_add
     (ECORE_X_EVENT_WINDOW_REPARENT, _systray_cb_reparent_notify, inst);
   inst->handler.sel_clear = ecore_event_handler_add
     (ECORE_X_EVENT_SELECTION_CLEAR, _systray_cb_selection_clear, inst);
   inst->handler.configure = ecore_event_handler_add
     (ECORE_X_EVENT_WINDOW_CONFIGURE, _systray_cb_window_configure, inst);

   instance = inst;
   return inst->gcc;
}

/* Called when Gadget_Container says stop */
static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst = gcc->data;

   // fprintf(stderr, "SYSTRAY: shutdown %p, inst=%p\n", gcc, inst);

   if (!inst)
     return;

   if (inst->menu)
     {
	e_menu_post_deactivate_callback_set(inst->menu, NULL, NULL);
	e_object_del(E_OBJECT(inst->menu));
     }

   _systray_deactivate(inst);
   evas_object_del(inst->ui.gadget);

   if (inst->handler.message)
     ecore_event_handler_del(inst->handler.message);
   if (inst->handler.destroy)
     ecore_event_handler_del(inst->handler.destroy);
   if (inst->handler.show)
     ecore_event_handler_del(inst->handler.show);
   if (inst->handler.reparent)
     ecore_event_handler_del(inst->handler.reparent);
   if (inst->handler.sel_clear)
     ecore_event_handler_del(inst->handler.sel_clear);
   if (inst->handler.configure)
     ecore_event_handler_del(inst->handler.configure);
   if (inst->timer.retry)
     ecore_timer_del(inst->timer.retry);
   if (inst->job.size_apply)
     ecore_job_del(inst->job.size_apply);

   if (instance == inst)
     instance = NULL;

   E_FREE(inst);
   gcc->data = NULL;
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   Instance *inst = gcc->data;
   const char *signal;
   unsigned int systray_orient;

   if (!inst)
     return;

   switch (orient)
     {
      case E_GADCON_ORIENT_FLOAT:
	 signal = "e,action,orient,float";
	 systray_orient = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
	 break;
      case E_GADCON_ORIENT_HORIZ:
	 signal = "e,action,orient,horiz";
	 systray_orient = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
	 break;
      case E_GADCON_ORIENT_VERT:
	 signal = "e,action,orient,vert";
	 systray_orient = _NET_SYSTEM_TRAY_ORIENTATION_VERT;
	 break;
      case E_GADCON_ORIENT_LEFT:
	 signal = "e,action,orient,left";
	 systray_orient = _NET_SYSTEM_TRAY_ORIENTATION_VERT;
	 break;
      case E_GADCON_ORIENT_RIGHT:
	 signal = "e,action,orient,right";
	 systray_orient = _NET_SYSTEM_TRAY_ORIENTATION_VERT;
	 break;
      case E_GADCON_ORIENT_TOP:
	 signal = "e,action,orient,top";
	 systray_orient = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
	 break;
      case E_GADCON_ORIENT_BOTTOM:
	 signal = "e,action,orient,bottom";
	 systray_orient = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
	 break;
      case E_GADCON_ORIENT_CORNER_TL:
	 signal = "e,action,orient,corner_tl";
	 systray_orient = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
	 break;
      case E_GADCON_ORIENT_CORNER_TR:
	 signal = "e,action,orient,corner_tr";
	 systray_orient = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
	 break;
      case E_GADCON_ORIENT_CORNER_BL:
	 signal = "e,action,orient,corner_bl";
	 systray_orient = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
	 break;
      case E_GADCON_ORIENT_CORNER_BR:
	 signal = "e,action,orient,corner_br";
	 systray_orient = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
	 break;
      case E_GADCON_ORIENT_CORNER_LT:
	 signal = "e,action,orient,corner_lt";
	 systray_orient = _NET_SYSTEM_TRAY_ORIENTATION_VERT;
	 break;
      case E_GADCON_ORIENT_CORNER_RT:
	 signal = "e,action,orient,corner_rt";
	 systray_orient = _NET_SYSTEM_TRAY_ORIENTATION_VERT;
	 break;
      case E_GADCON_ORIENT_CORNER_LB:
	 signal = "e,action,orient,corner_lb";
	 systray_orient = _NET_SYSTEM_TRAY_ORIENTATION_VERT;
	 break;
      case E_GADCON_ORIENT_CORNER_RB:
	 signal = "e,action,orient,corner_rb";
	 systray_orient = _NET_SYSTEM_TRAY_ORIENTATION_VERT;
	 break;
      default:
	 signal = "e,action,orient,horiz";
	 systray_orient = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
     }

   ecore_x_window_prop_card32_set
     (inst->win.selection, _atom_st_orient, &systray_orient, 1);

   edje_object_signal_emit(inst->ui.gadget, signal, _sig_source);
   edje_object_message_signal_process(inst->ui.gadget);
   _systray_size_apply(inst);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("Systray");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;

   o = edje_object_add(evas);
   edje_object_file_set(o, _systray_theme_path(), "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   if (!instance)
     return _name;
   else
     return NULL;
}

static const E_Gadcon_Client_Class _gc_class =
{
  GADCON_CLIENT_CLASS_VERSION, _name,
  {
     _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
     _systray_site_is_safe
  },
  E_GADCON_CLIENT_STYLE_INSET
};

EAPI E_Module_Api e_modapi = {E_MODULE_API_VERSION, _Name};

EAPI void *
e_modapi_init(E_Module *m)
{
   systray_mod = m;

   e_gadcon_provider_register(&_gc_class);

   if (!_atom_manager)
     _atom_manager = ecore_x_atom_get("MANAGER");
   if (!_atom_st_orient)
     _atom_st_orient = ecore_x_atom_get("_NET_SYSTEM_TRAY_ORIENTATION");
   if (!_atom_st_visual)
     _atom_st_visual = ecore_x_atom_get("_NET_SYSTEM_TRAY_VISUAL");
   if (!_atom_st_op_code)
     _atom_st_op_code = ecore_x_atom_get("_NET_SYSTEM_TRAY_OPCODE");
   if (!_atom_st_msg_data)
     _atom_st_msg_data = ecore_x_atom_get("_NET_SYSTEM_TRAY_MESSAGE_DATA");
   if (!_atom_xembed)
     _atom_xembed = ecore_x_atom_get("_XEMBED");
   if (!_atom_xembed_info)
     _atom_xembed_info = ecore_x_atom_get("_XEMBED_INFO");

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   e_gadcon_provider_unregister(&_gc_class);
   systray_mod = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
