/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

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
     "battery",
     {
        _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL
     },
   E_GADCON_CLIENT_STYLE_PLAIN
};
/**/
/***************************************************************************/

/***************************************************************************/
/**/
/* actual module specifics */

typedef struct _Instance Instance;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_battery;
};

static int _battery_cb_exe_data(void *data, int type, void *event);
static int _battery_cb_exe_del(void *data, int type, void *event);
static void _button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _menu_cb_post(void *data, E_Menu *m);
static void _battery_face_level_set(Instance *inst, double level);
static void _battery_face_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi);

static E_Config_DD *conf_edd = NULL;

Config *battery_config = NULL;

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;
   
   battery_config->full = -2;
   battery_config->time_left = -2;
   battery_config->have_battery = -2;
   battery_config->have_power = -2;
   
   inst = E_NEW(Instance, 1);
   
   o = edje_object_add(gc->evas);
   e_theme_edje_object_set(o, "base/theme/modules/battery",
			   "e/modules/battery/main");
   
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;
   
   inst->gcc = gcc;
   inst->o_battery = o;   
   
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  _button_cb_mouse_down, inst);
   battery_config->instances = evas_list_append(battery_config->instances, inst);
   _battery_config_updated();
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;
   
   inst = gcc->data;
   battery_config->instances = evas_list_remove(battery_config->instances, inst);
   evas_object_del(inst->o_battery);
   free(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc)
{
   Instance *inst;
   Evas_Coord mw, mh;
   
   inst = gcc->data;
   mw = 0, mh = 0;
   edje_object_size_min_get(inst->o_battery, &mw, &mh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(inst->o_battery, &mw, &mh); 
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   e_gadcon_client_aspect_set(gcc, mw, mh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static char *
_gc_label(void)
{
   return _("Battery");
}

static Evas_Object *
_gc_icon(Evas *evas)
{
   Evas_Object *o;
   char buf[4096];
   
   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-battery.edj",
	    e_module_dir_get(battery_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(void)
{
   return _gadcon_class.name;
}

/**/
/***************************************************************************/

/***************************************************************************/
/**/
static void
_button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Instance *inst;
   Evas_Event_Mouse_Down *ev;
   
   inst = data;
   ev = event_info;
   if ((ev->button == 3) && (!battery_config->menu))
     {
	E_Menu *mn;
	E_Menu_Item *mi;
	int cx, cy, cw, ch;
	
	mn = e_menu_new();
	e_menu_post_deactivate_callback_set(mn, _menu_cb_post, inst);
	battery_config->menu = mn;

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Configuration"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/configuration");   
	e_menu_item_callback_set(mi, _battery_face_cb_menu_configure, NULL);
	
	e_gadcon_client_util_menu_items_append(inst->gcc, mn, 0);
	
	e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon,
					  &cx, &cy, &cw, &ch);
	e_menu_activate_mouse(mn,
			      e_util_zone_current_get(e_manager_current_get()),
			      cx + ev->output.x, cy + ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	e_util_evas_fake_mouse_up_later(inst->gcc->gadcon->evas,
					ev->button);
     }
}

static void
_menu_cb_post(void *data, E_Menu *m)
{
   if (!battery_config->menu) return;
   e_object_del(E_OBJECT(battery_config->menu));
   battery_config->menu = NULL;
}

static void
_battery_face_level_set(Instance *inst, double level)
{
   Edje_Message_Float msg;

   if (level < 0.0) level = 0.0;
   else if (level > 1.0) level = 1.0;
   msg.val = level;
   edje_object_message_send(inst->o_battery, EDJE_MESSAGE_FLOAT, 1, &msg);
}

static void 
_battery_face_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   if (!battery_config) return;
   if (battery_config->config_dialog) return;
   e_int_config_battery_module(m->zone->container, NULL);
}

void
_battery_config_updated(void)
{
   char buf[4096];
   
   if (!battery_config) return;
   if (battery_config->batget_exe)
     {
	ecore_exe_terminate(battery_config->batget_exe);
	ecore_exe_free(battery_config->batget_exe);
     }
   snprintf(buf, sizeof(buf),
	    "%s/%s/batget %i",
	    e_module_dir_get(battery_config->module), MODULE_ARCH,
	    battery_config->poll_interval);
   battery_config->batget_exe = ecore_exe_pipe_run(buf,
						   ECORE_EXE_PIPE_READ |
						   ECORE_EXE_PIPE_READ_LINE_BUFFERED |
						   ECORE_EXE_NOT_LEADER,
						   NULL);
}

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
     "Battery"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[4096];

   conf_edd = E_CONFIG_DD_NEW("Battery_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, poll_interval, INT);
   E_CONFIG_VAL(D, T, alarm, INT);
   E_CONFIG_VAL(D, T, alarm_p, INT);

   battery_config = e_config_domain_load("module.battery", conf_edd);
   if (!battery_config)
     {
       battery_config = E_NEW(Config, 1);
       battery_config->poll_interval = 512;
       battery_config->alarm = 30;
       battery_config->alarm_p = 10;
     }
   E_CONFIG_LIMIT(battery_config->poll_interval, 4, 4096);
   E_CONFIG_LIMIT(battery_config->alarm, 0, 60);
   E_CONFIG_LIMIT(battery_config->alarm_p, 0, 100);

   battery_config->module = m;
   
   battery_config->full = -2;
   battery_config->time_left = -2;
   battery_config->have_battery = -2;
   battery_config->have_power = -2;
   
   battery_config->batget_data_handler =
     ecore_event_handler_add(ECORE_EXE_EVENT_DATA,
			     _battery_cb_exe_data,
			     NULL);
   battery_config->batget_del_handler =
     ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
			     _battery_cb_exe_del,
			     NULL);
   
   e_gadcon_provider_register(&_gadcon_class);
   
   snprintf(buf, sizeof(buf), "%s/e-module-battery.edj", e_module_dir_get(m));
   e_configure_registry_category_add("advanced", 80, _("Advanced"), NULL, "enlightenment/advanced");
   e_configure_registry_item_add("advanced/battery", 100, _("Battery Meter"), NULL, buf, e_int_config_battery_module);
   
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   e_configure_registry_item_del("advanced/battery");
   e_configure_registry_category_del("advanced");
   
   e_gadcon_provider_unregister(&_gadcon_class);

   if (battery_config->batget_exe)
     {
	ecore_exe_terminate(battery_config->batget_exe);
	ecore_exe_free(battery_config->batget_exe);
	battery_config->batget_exe = NULL;
     }

   if (battery_config->batget_data_handler)
     {
	ecore_event_handler_del(battery_config->batget_data_handler);
	battery_config->batget_data_handler = NULL;
     }
   if (battery_config->batget_del_handler)
     {
	ecore_event_handler_del(battery_config->batget_del_handler);
	battery_config->batget_del_handler = NULL;
     }
   
   if (battery_config->config_dialog)
     e_object_del(E_OBJECT(battery_config->config_dialog));
   if (battery_config->menu)
     {
        e_menu_post_deactivate_callback_set(battery_config->menu, NULL, NULL);
	e_object_del(E_OBJECT(battery_config->menu));
	battery_config->menu = NULL;
     }
   free(battery_config);
   battery_config = NULL;
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   e_config_domain_save("module.battery", conf_edd, battery_config);
   return 1;
}
/**/
/***************************************************************************/

/***************************************************************************/
/**/
static int
_battery_cb_exe_data(void *data, int type, void *event)
{
   Ecore_Exe_Event_Data *ev;
                                
   ev = event;
   if (ev->exe != battery_config->batget_exe) return 1;
   if ((ev->lines) && (ev->lines[0].line))
     {
	int i;
	
	for (i = 0; ev->lines[i].line; i++)
	  {
	     if (!strcmp(ev->lines[i].line, "ERROR"))
	       {
		  Evas_List *l;
		  
		  for (l = battery_config->instances; l; l = l->next)
		    {
		       Instance *inst;
		       
		       inst = l->data;
		       edje_object_signal_emit(inst->o_battery, "e,state,unknown", "e");
		       edje_object_part_text_set(inst->o_battery, "e.text.reading", _("ERROR"));
		       edje_object_part_text_set(inst->o_battery, "e.text.time", _("ERROR"));
		    }
	       }
	     else
	       {
		  int full = 0;
		  int time_left = 0;
		  int have_battery = 0;
		  int have_power = 0;
		  Evas_List *l;
		  
		  if (sscanf(ev->lines[i].line, "%i %i %i %i",
			     &full, &time_left, &have_battery, &have_power)
		      == 4)
		    {
		       for (l = battery_config->instances; l; l = l->next)
			 {
			    Instance *inst;
			    
			    inst = l->data;
			    if (have_power != battery_config->have_power)
			      {
				 if (have_power)
				   edje_object_signal_emit(inst->o_battery, "e,state,charging", "e");
				 else
				   edje_object_signal_emit(inst->o_battery, "e,state,discharging", "e");
			      }
			    if (have_battery)
			      {
				 if (battery_config->full != full)
				   {
				      char buf[256];
				      
				      snprintf(buf, sizeof(buf), "%i%%", full);
				      edje_object_part_text_set(inst->o_battery, "e.text.reading", buf);
				      _battery_face_level_set(inst, (double)full / 100.0);
				   }
			      }
			    else
			      {
				 edje_object_part_text_set(inst->o_battery, "e.text.reading", _("N/A"));
				 _battery_face_level_set(inst, 0.0);
			      }
			    if (time_left != battery_config->time_left)
			      {
				 char buf[256];
				 int mins, hrs;
				 
				 hrs = time_left / 3600;
				 mins = (time_left) / 60 - (hrs * 60);
				 snprintf(buf, sizeof(buf), "%i:%02i", hrs, mins);
				 if (hrs < 0) hrs = 0;
				 if (mins < 0) mins = 0;
				 edje_object_part_text_set(inst->o_battery, "e.text.time", buf);
			      }
			 }
		    }
		  battery_config->full = full;
		  battery_config->time_left = time_left;
		  battery_config->have_battery = have_battery;
		  battery_config->have_power = have_power;
	       }
	  }
     }
   return 0;
}                          

static int
_battery_cb_exe_del(void *data, int type, void *event)
{
   Ecore_Exe_Event_Del *ev;
   
   ev = event;
   if (ev->exe != battery_config->batget_exe) return 1;
   battery_config->batget_exe = NULL;
   return 0;
}
