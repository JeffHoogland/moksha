#include "e.h"

/***************************************************************************/
typedef struct _Instance Instance;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object *obj;
};

static void _cb_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info);

/***************************************************************************/
/**/
/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc);
static char *_gc_label(void);
static Evas_Object *_gc_icon(Evas *evas);
static const char *_gc_id_new(void);
/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
     "illume-cfg",
     {
	_gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL
     },
   E_GADCON_CLIENT_STYLE_PLAIN
};
static E_Module *mod = NULL;
/**/
/***************************************************************************/

/* called from the module core */
void
_e_mod_gad_cfg_init(E_Module *m)
{
   mod = m;
   e_gadcon_provider_register(&_gadcon_class);
}

void
_e_mod_gad_cfg_shutdown(void)
{
   e_gadcon_provider_unregister(&_gadcon_class);
   mod = NULL;
}

/* internal calls */
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
	     
	     snprintf(buf, sizeof(buf), "%s/illume.edj", custom_dir);
	     if (edje_object_file_set(o, buf, group))
	       {
		  printf("OK FALLBACK %s\n", buf);
	       }
	  }
     }
   return o;
}

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;
   
   inst = E_NEW(Instance, 1);
   o = _theme_obj_new(gc->evas, e_module_dir_get(mod),
		      "e/modules/illume/gadget/cfg");
   evas_object_show(o);
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;
   inst->gcc = gcc;
   inst->obj = o;
   e_gadcon_client_util_menu_attach(gcc);

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP,
				  _cb_mouse_up, inst);
    
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;
   
   inst = gcc->data;
   evas_object_del(inst->obj);
   free(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc)
{
   Instance *inst;
   Evas_Coord mw, mh;
   
   inst = gcc->data;
   mw = 0, mh = 0;
   edje_object_size_min_get(inst->obj, &mw, &mh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(inst->obj, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   e_gadcon_client_aspect_set(gcc, mw, mh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static char *
_gc_label(void)
{
   return "Configuration (Illume)";
}

static Evas_Object *
_gc_icon(Evas *evas)
{
/* FIXME: need icon
   Evas_Object *o;
   char buf[4096];
   
   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-clock.edj",
	    e_module_dir_get(clock_module));
   edje_object_file_set(o, buf, "icon");
   return o;
 */
   return NULL;
}

static const char *
_gc_id_new(void)
{
   return _gadcon_class.name;
}

static void _cfg(void);

static void
_cb_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   Instance *inst;
   
   ev = event_info;
   inst = data;
   
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   _cfg();
}

static void _cb_signal_ok(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _cb_delete(E_Win *win);
static void _cb_resize(E_Win *win);
static void _cb_cfg_item(void *data);

static E_Win *win = NULL;
static Eina_List *cfgstr = NULL;
static Evas_Object *o_cfg;

static void
_cfg(void)
{
   E_Zone *zone;
   Evas_Object *o, *il;
   Eina_List *l, *ll;
   
   if (win)
     {
	e_win_show(win);
	e_win_raise(win);
	return;
     }
   
   zone = e_util_zone_current_get(e_manager_current_get());
   win = e_win_new(zone->container);
   e_win_delete_callback_set(win, _cb_delete);
   e_win_resize_callback_set(win, _cb_resize);
   e_win_name_class_set(win, "E", "configuration");
   e_win_title_set(win, "Configuration");
   
   o = _theme_obj_new(e_win_evas_get(win), e_module_dir_get(mod),
		      "e/modules/illume/config/dialog");
   o_cfg = o;
   edje_object_part_text_set(o, "e.text.label", "Close");
   edje_object_signal_callback_add(o, "e,action,do,ok", "", _cb_signal_ok, win);
   evas_object_move(o, 0, 0);
   evas_object_show(o);
   
   il = e_widget_ilist_add(e_win_evas_get(win), 64, 64, NULL);
   e_widget_ilist_selector_set(il, 1);
   evas_event_freeze(evas_object_evas_get(il));
   e_widget_ilist_freeze(il);
   edje_freeze();
   
   for (l = e_configure_registry; l; l = l->next)
     {
	E_Configure_Cat *ecat;
	
	ecat = l->data;
	if ((ecat->pri >= 0) && (ecat->items))
	  {
	     for (ll = ecat->items; ll; ll = ll->next)
	       {
		  E_Configure_It *eci;
		  char buf[1024];
		  
		  eci = ll->data;
		  if (eci->pri >= 0)
		    {
		       char *s;
		       Evas_Object *oi;
		       
		       if (e_util_edje_icon_check(eci->icon))
			 {
			    oi = edje_object_add(e_win_evas_get(win));
			    e_util_edje_icon_set(oi, eci->icon);
			 }
		       else
			 oi = e_util_icon_add(eci->icon, e_win_evas_get(win));
		       snprintf(buf, sizeof(buf), "%s/%s", ecat->cat, eci->item);
		       s = evas_stringshare_add(buf);
		       cfgstr = eina_list_append(cfgstr, s);
		       e_widget_ilist_append(il, oi, eci->label, _cb_cfg_item,
					     s, NULL);
		    }
	       }
	  }
     }

   e_widget_ilist_go(il);
   edje_thaw();
   e_widget_ilist_thaw(il);
   evas_event_thaw(evas_object_evas_get(il));

   e_widget_focus_set(il, 1);
   evas_object_focus_set(il, 1);
   
   edje_object_part_swallow(o, "e.swallow.content", il);

   e_win_show(win);
   return;
}

static void
_cb_signal_ok(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   e_object_del(E_OBJECT(win));
   win = NULL;
   while (cfgstr)
     {
	evas_stringshare_del(cfgstr->data);
	cfgstr = eina_list_remove_list(cfgstr, cfgstr);
     }
}

static void
_cb_delete(E_Win *w)
{
   e_object_del(E_OBJECT(win));
   win = NULL;
   while (cfgstr)
     {
	evas_stringshare_del(cfgstr->data);
	cfgstr = eina_list_remove_list(cfgstr, cfgstr);
     }
}

static void
_cb_resize(E_Win *w)
{
   evas_object_resize(o_cfg, win->w, win->h);
}

static void
_cb_cfg_item(void *data)
{
   e_configure_registry_call(data, 
			     e_util_zone_current_get(e_manager_current_get())->container,
			     NULL);
}
