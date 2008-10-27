#include <e.h>

/***************************************************************************/
typedef struct _Instance Instance;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object *obj;
   Ecore_Exe *wifiget_exe;
   Ecore_Event_Handler *wifiget_data_handler;
   Ecore_Event_Handler *wifiget_del_handler;
   int strength;
};

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
     "illume-wifi",
     {
	_gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL
     },
   E_GADCON_CLIENT_STYLE_PLAIN
};
static E_Module *mod = NULL;
/**/
/***************************************************************************/

static void _wifiget_spawn(Instance *inst);
static void _wifiget_kill(Instance *inst);
static int _wifiget_cb_exe_data(void *data, int type, void *event);
static int _wifiget_cb_exe_del(void *data, int type, void *event);

/* called from the module core */
void
_e_mod_gad_wifi_init(E_Module *m)
{
   mod = m;
   e_gadcon_provider_register(&_gadcon_class);
}

void
_e_mod_gad_wifi_shutdown(void)
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
		      "e/modules/illume/gadget/wifi");
   evas_object_show(o);
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;
   inst->gcc = gcc;
   inst->obj = o;
   e_gadcon_client_util_menu_attach(gcc);
   
   inst->strength = -1;
   _wifiget_spawn(inst);
   
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;
   
   inst = gcc->data;
   _wifiget_kill(inst);
   evas_object_del(inst->obj);
   free(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc)
{
   Instance *inst;
   Evas_Coord mw, mh, mxw, mxh;
   
   inst = gcc->data;
   mw = 0, mh = 0;
   edje_object_size_min_get(inst->obj, &mw, &mh);
   edje_object_size_max_get(inst->obj, &mxw, &mxh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(inst->obj, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   if ((mxw > 0) && (mxh > 0))
     e_gadcon_client_aspect_set(gcc, mxw, mxh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static char *
_gc_label(void)
{
   return "Wifi (Illume)";
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

static void
_wifiget_spawn(Instance *inst)
{
   char buf[4096];
   
   if (inst->wifiget_exe) return;
   snprintf(buf, sizeof(buf),
	             "%s/%s/wifiget %i",
	             e_module_dir_get(mod), MODULE_ARCH,
	             8);
   inst->wifiget_exe = ecore_exe_pipe_run(buf,
					  ECORE_EXE_PIPE_READ |
					  ECORE_EXE_PIPE_READ_LINE_BUFFERED |
					  ECORE_EXE_NOT_LEADER,
					  inst);
   inst->wifiget_data_handler =
     ecore_event_handler_add(ECORE_EXE_EVENT_DATA, _wifiget_cb_exe_data,
			     inst);
   inst->wifiget_del_handler =
     ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
			     _wifiget_cb_exe_del,
			     inst);
   
}

static void
_wifiget_kill(Instance *inst)
{
   if (!inst->wifiget_exe) return;
   ecore_exe_terminate(inst->wifiget_exe);
   ecore_exe_free(inst->wifiget_exe);
   inst->wifiget_exe = NULL;
   ecore_event_handler_del(inst->wifiget_data_handler);
   inst->wifiget_data_handler = NULL;
   ecore_event_handler_del(inst->wifiget_del_handler);
   inst->wifiget_del_handler = NULL;
}

static int
_wifiget_cb_exe_data(void *data, int type, void *event)
{
   Ecore_Exe_Event_Data *ev;
   Instance *inst;
   int pstrength;
   
   inst = data;
   ev = event;
   if (ev->exe != inst->wifiget_exe) return 1;
   pstrength = inst->strength;
   if ((ev->lines) && (ev->lines[0].line))
     {
	int i;
	
	for (i = 0; ev->lines[i].line; i++)
	  {
	     if (!strcmp(ev->lines[i].line, "ERROR"))
	       inst->strength = -999;
	     else
	       inst->strength = atoi(ev->lines[i].line);
	  }
     }
   
   if (inst->strength != pstrength)
     {
	Edje_Message_Float msg;
	double level;
	
	level = (double)inst->strength / 100.0;
	if (level < 0.0) level = 0.0;
	else if (level > 1.0) level = 1.0;
	msg.val = level;
	edje_object_message_send(inst->obj, EDJE_MESSAGE_FLOAT, 1, &msg);
     }
   return 0;
}

static int 
_wifiget_cb_exe_del(void *data, int type, void *event)
{
   Ecore_Exe_Event_Del *ev;
   Instance *inst;
  
   inst = data;
   ev = event;
   if (ev->exe != inst->wifiget_exe) return 1;
   inst->wifiget_exe = NULL;
   return 1;
}
