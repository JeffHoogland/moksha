#include "e.h"
#include "e_mod_main.h"

/* TODO List:
 *
 * should support proepr resize and move handles in the edje.
 * 
 */

/* module private routines */
static Temperature *_temperature_init(E_Module *m);
static void     _temperature_shutdown(Temperature *e);
static E_Menu  *_temperature_config_menu_new(Temperature *e);
static void     _temperature_config_menu_del(Temperature *e, E_Menu *m);
static void     _temperature_face_init(Temperature_Face *ef);
static void     _temperature_face_free(Temperature_Face *ef);
static void     _temperature_face_reconfigure(Temperature_Face *ef);
static void     _temperature_cb_face_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void     _temperature_cb_face_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void     _temperature_cb_face_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static int      _temperature_cb_event_container_resize(void *data, int type, void *event);
static int      _temperature_cb_check(void *data);
static void     _temperature_level_set(Temperature_Face *ef, double level);

/* public module routines. all modules must have these */
void *
init(E_Module *m)
{
   Temperature *e;
   
   /* check module api version */
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show("Module API Error",
			    "Error initializing Module: Temperature\n"
			    "It requires a minimum module API version of: %i.\n"
			    "The module API advertized by Enlightenment is: %i.\n"
			    "Aborting module.",
			    E_MODULE_API_VERSION,
			    m->api->version);
	return NULL;
     }
   /* actually init temperature */
   e = _temperature_init(m);
   m->config_menu = _temperature_config_menu_new(e);
   return e;
}

int
shutdown(E_Module *m)
{
   Temperature *e;
   
   e = m->data;
   if (e)
     {
	if (m->config_menu)
	  {
	     _temperature_config_menu_del(e, e->config_menu1);
	     _temperature_config_menu_del(e, e->config_menu2);
	     _temperature_config_menu_del(e, e->config_menu3);
	     _temperature_config_menu_del(e, m->config_menu);
	     m->config_menu = NULL;
	  }
	_temperature_shutdown(e);
     }
   return 1;
}

int
save(E_Module *m)
{
   Temperature *e;

   e = m->data;
   e_config_domain_save("module.temperature", e->conf_edd, e->conf);
   return 1;
}

int
info(E_Module *m)
{
   char buf[4096];
   
   m->label = strdup("Temperature");
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

int
about(E_Module *m)
{
   e_error_dialog_show("Enlightenment Temperature Module",
		       "A module to measure the ACPI Thermal sensor on Linux.\n"
		       "It is especially useful for modern Laptops with high speed\n"
		       "CPUs that generate a lot of heat.");
   return 1;
}

/* module private routines */
static Temperature *
_temperature_init(E_Module *m)
{
   Temperature *e;
   Evas_List *managers, *l, *l2;
   
   e = calloc(1, sizeof(Temperature));
   if (!e) return NULL;
   
   e->conf_edd = E_CONFIG_DD_NEW("Temperature_Config", Config);
#undef T
#undef D
#define T Config
#define D e->conf_edd   
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_VAL(D, T, x, DOUBLE);
   E_CONFIG_VAL(D, T, y, DOUBLE);
   E_CONFIG_VAL(D, T, poll_time, DOUBLE);
   E_CONFIG_VAL(D, T, low, INT);
   E_CONFIG_VAL(D, T, high, INT);

   e->conf = e_config_domain_load("module.temperature", e->conf_edd);
   if (!e->conf)
     {
	e->conf = E_NEW(Config, 1);
	e->conf->width = 64;
	e->conf->x = 0.9;
	e->conf->y = 1.0;
	e->conf->poll_time = 10.0;
	e->conf->low = 30;
	e->conf->high = 80;
     }
   E_CONFIG_LIMIT(e->conf->width, 2, 256);
   E_CONFIG_LIMIT(e->conf->x, 0.0, 1.0);
   E_CONFIG_LIMIT(e->conf->y, 0.0, 1.0);
   E_CONFIG_LIMIT(e->conf->poll_time, 0.5, 1000.0);
   E_CONFIG_LIMIT(e->conf->low, 0, 100);
   E_CONFIG_LIMIT(e->conf->high, 0, 200);
   
   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     Temperature_Face *ef;
	     
	     con = l2->data;
	     ef = calloc(1, sizeof(Temperature_Face));
	     if (ef)
	       {
		  ef->temp = e;
		  ef->con = con;
		  ef->evas = con->bg_evas;
		  _temperature_face_init(ef);
		  e->face = ef;
	       }
	  }
     }
   return e;
}

static void
_temperature_shutdown(Temperature *e)
{
   free(e->conf);
   E_CONFIG_DD_FREE(e->conf_edd);
   
   _temperature_face_free(e->face);
   free(e);
}

static void
_temperature_menu_fast(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->poll_time = 1.0;
   ecore_timer_del(e->face->temperature_check_timer);
   e->face->temperature_check_timer = ecore_timer_add(e->face->temp->conf->poll_time, _temperature_cb_check, e->face);
   e_config_save_queue();
}

static void
_temperature_menu_medium(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->poll_time = 5.0;
   ecore_timer_del(e->face->temperature_check_timer);
   e->face->temperature_check_timer = ecore_timer_add(e->face->temp->conf->poll_time, _temperature_cb_check, e->face);
   e_config_save_queue();
}

static void
_temperature_menu_normal(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->poll_time = 10.0;
   ecore_timer_del(e->face->temperature_check_timer);
   e->face->temperature_check_timer = ecore_timer_add(e->face->temp->conf->poll_time, _temperature_cb_check, e->face);
   e_config_save_queue();
}

static void
_temperature_menu_slow(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->poll_time = 30.0;
   ecore_timer_del(e->face->temperature_check_timer);
   e->face->temperature_check_timer = ecore_timer_add(e->face->temp->conf->poll_time, _temperature_cb_check, e->face);
   e_config_save_queue();
}

static void
_temperature_menu_very_slow(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->poll_time = 60.0;
   ecore_timer_del(e->face->temperature_check_timer);
   e->face->temperature_check_timer = ecore_timer_add(e->face->temp->conf->poll_time, _temperature_cb_check, e->face);
   e_config_save_queue();
}

static void
_temperature_menu_low_10(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->low = 10;
   _temperature_cb_check(e->face);
   e_config_save_queue();
}

static void
_temperature_menu_low_20(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->low = 20;
   _temperature_cb_check(e->face);
   e_config_save_queue();
}

static void
_temperature_menu_low_30(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->low = 30;
   _temperature_cb_check(e->face);
   e_config_save_queue();
}

static void
_temperature_menu_low_40(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->low = 40;
   _temperature_cb_check(e->face);
   e_config_save_queue();
}

static void
_temperature_menu_low_50(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->low = 50;
   _temperature_cb_check(e->face);
   e_config_save_queue();
}

static void
_temperature_menu_high_20(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->high = 20;
   _temperature_cb_check(e->face);
   e_config_save_queue();
}

static void
_temperature_menu_high_30(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->high = 30;
   _temperature_cb_check(e->face);
   e_config_save_queue();
}

static void
_temperature_menu_high_40(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->high = 40;
   _temperature_cb_check(e->face);
   e_config_save_queue();
}

static void
_temperature_menu_high_50(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->high = 50;
   _temperature_cb_check(e->face);
   e_config_save_queue();
}

static void
_temperature_menu_high_60(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->high = 60;
   _temperature_cb_check(e->face);
   e_config_save_queue();
}

static void
_temperature_menu_high_70(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->high = 70;
   _temperature_cb_check(e->face);
   e_config_save_queue();
}

static void
_temperature_menu_high_80(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->high = 80;
   _temperature_cb_check(e->face);
   e_config_save_queue();
}

static void
_temperature_menu_high_90(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->high = 90;
   _temperature_cb_check(e->face);
   e_config_save_queue();
}

static void
_temperature_menu_high_100(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Temperature *e;
   
   e = data;
   e->conf->high = 100;
   _temperature_cb_check(e->face);
   e_config_save_queue();
}

static E_Menu *
_temperature_config_menu_new(Temperature *e)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   mn = e_menu_new();
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Fast (1 sec)");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time == 1.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_fast, e);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Medium (5 sec)");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time == 5.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_medium, e);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Normal (10 sec)");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time == 10.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_normal, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Slow (30 sec)");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time == 30.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_slow, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Very Slow (60 sec)");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time == 60.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_very_slow, e);
   
   e->config_menu1 = mn;

   mn = e_menu_new();
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "10°C");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->low == 10) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_low_10, e);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "20°C");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->low == 20) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_low_20, e);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "30°C");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->low == 30) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_low_30, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "40°C");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->low == 40) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_low_40, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "50°C");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->low == 50) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_low_50, e);
   
   e->config_menu2 = mn;
   
   mn = e_menu_new();
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "20°C");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 20) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_20, e);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "30°C");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 30) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_30, e);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "40°C");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 40) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_40, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "50°C");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 50) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_50, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "60°C");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 60) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_60, e);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "70°C");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 70) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_70, e);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "80°C");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 80) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_80, e);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "90°C");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 90) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_90, e);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "100°C");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->high == 100) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _temperature_menu_high_100, e);
   
   e->config_menu3 = mn;
   
   mn = e_menu_new();
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Check Interval");
   e_menu_item_submenu_set(mi, e->config_menu1);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Low Temperature");
   e_menu_item_submenu_set(mi, e->config_menu2);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "High Temperature");
   e_menu_item_submenu_set(mi, e->config_menu3);
   
   e->config_menu = mn;
   
   return mn;
}

static void
_temperature_config_menu_del(Temperature *e, E_Menu *m)
{
   e_object_del(E_OBJECT(m));
}

static void
_temperature_face_init(Temperature_Face *ef)
{
   Evas_Coord ww, hh, bw, bh;
   Evas_Object *o;
   
   ef->ev_handler_container_resize =
     ecore_event_handler_add(E_EVENT_CONTAINER_RESIZE,
			     _temperature_cb_event_container_resize,
			     ef);
   evas_output_viewport_get(ef->evas, NULL, NULL, &ww, &hh);
   ef->fx = ef->temp->conf->x * (ww - ef->temp->conf->width);
   ef->fy = ef->temp->conf->y * (hh - ef->temp->conf->width);
      
   evas_event_freeze(ef->evas);
   o = edje_object_add(ef->evas);
   ef->temp_object = o;

   edje_object_file_set(o,
			/* FIXME: "default.eet" needs to come from conf */
			e_path_find(path_themes, "default.eet"),
			"modules/temperature/main");
   evas_object_show(o);
   
   o = evas_object_rectangle_add(ef->evas);
   ef->event_object = o;
   evas_object_layer_set(o, 2);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _temperature_cb_face_down, ef);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _temperature_cb_face_up, ef);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _temperature_cb_face_move, ef);
   evas_object_show(o);
   
   edje_object_size_min_calc(ef->temp_object, &bw, &bh);
   ef->minsize = bh;
   ef->minsize = bw;

   _temperature_face_reconfigure(ef);
   
   ef->temperature_check_timer = ecore_timer_add(ef->temp->conf->poll_time, _temperature_cb_check, ef);
   
   _temperature_cb_check(ef);
   
   evas_event_thaw(ef->evas);
}

static void
_temperature_face_free(Temperature_Face *ef)
{
   ecore_timer_del(ef->temperature_check_timer);
   ecore_event_handler_del(ef->ev_handler_container_resize);
   evas_object_del(ef->temp_object);
   evas_object_del(ef->event_object);
   free(ef);
}

static void
_temperature_face_reconfigure(Temperature_Face *ef)
{
   Evas_Coord minw, minh, maxw, maxh, ww, hh;

   edje_object_size_min_calc(ef->temp_object, &minw, &maxh);
   edje_object_size_max_get(ef->temp_object, &maxw, &minh);
   evas_output_viewport_get(ef->evas, NULL, NULL, &ww, &hh);
   ef->fx = ef->temp->conf->x * (ww - ef->temp->conf->width);
   ef->fy = ef->temp->conf->y * (hh - ef->temp->conf->width);
   ef->fw = ef->temp->conf->width;
   ef->minsize = minw;
   ef->maxsize = maxw;

   evas_object_move(ef->temp_object, ef->fx, ef->fy);
   evas_object_resize(ef->temp_object, ef->temp->conf->width, ef->temp->conf->width);
   evas_object_move(ef->event_object, ef->fx, ef->fy);
   evas_object_resize(ef->event_object, ef->temp->conf->width, ef->temp->conf->width);
}

static void
_temperature_cb_face_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Temperature_Face *ef;
   
   ev = event_info;
   ef = data;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(ef->temp->config_menu, ef->con,
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(ef->con);
     }
   else if (ev->button == 2)
     {
	ef->resize = 1;
     }
   else if (ev->button == 1)
     {
	ef->move = 1;
     }
   evas_pointer_canvas_xy_get(e, &ef->xx, &ef->yy);
}

static void
_temperature_cb_face_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   Temperature_Face *ef;
   Evas_Coord ww, hh;
   
   ev = event_info;
   ef = data;
   ef->move = 0;
   ef->resize = 0;
   evas_output_viewport_get(ef->evas, NULL, NULL, &ww, &hh);
   ef->temp->conf->width = ef->fw;
   ef->temp->conf->x = (double)ef->fx / (double)(ww - ef->temp->conf->width);
   ef->temp->conf->y = (double)ef->fy / (double)(hh - ef->temp->conf->width);
   e_config_save_queue();
}

static void
_temperature_cb_face_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{ 
   Evas_Event_Mouse_Move *ev;
   Temperature_Face *ef;
   Evas_Coord cx, cy, sw, sh;
   
   evas_pointer_canvas_xy_get(e, &cx, &cy);
   evas_output_viewport_get(e, NULL, NULL, &sw, &sh);

   ev = event_info;
   ef = data;
   if (ef->move)
     {
	ef->fx += cx - ef->xx;
	ef->fy += cy - ef->yy;
	if (ef->fx < 0) ef->fx = 0;
	if (ef->fy < 0) ef->fy = 0;
	if (ef->fx + ef->fw > sw) ef->fx = sw - ef->fw;
	if (ef->fy + ef->fw > sh) ef->fy = sh - ef->fw;
	evas_object_move(ef->temp_object, ef->fx, ef->fy);
	evas_object_move(ef->event_object, ef->fx, ef->fy);
     }
   else if (ef->resize)
     {
	Evas_Coord d;

	d = cx - ef->xx;
	ef->fw += d;
	if (ef->fw < ef->minsize) ef->fw = ef->minsize;
	if (ef->fw > ef->maxsize) ef->fw = ef->maxsize;
	if (ef->fx + ef->fw > sw) ef->fw = sw - ef->fx;
	if (ef->fy + ef->fw > sh) ef->fw = sh - ef->fy;
	evas_object_resize(ef->temp_object, ef->fw, ef->fw);
	evas_object_resize(ef->event_object, ef->fw, ef->fw);
   }
   ef->xx = ev->cur.canvas.x;
   ef->yy = ev->cur.canvas.y;
}  

static int
_temperature_cb_event_container_resize(void *data, int type, void *event)
{
   Temperature_Face *ef;
   
   ef = data;
   _temperature_face_reconfigure(ef);
   return 1;
}

static int
_temperature_cb_check(void *data)
{
   Temperature_Face *ef;
   int ret = 0;
   Evas_List *therms;
   
   ef = data;
   therms = e_file_ls("/proc/acpi/thermal_zone");
   if (!therms)
     {
	/* disable therm object */
     }
   else
     {
	while (therms)
	  {
	     char buf[4096], units[32];
	     char *name;
	     FILE *f;
	     int temp = 0;
	     
	     name = therms->data;
	     therms = evas_list_remove_list(therms, therms);
	     snprintf(buf, sizeof(buf), "/proc/acpi/thermal_zone/%s/temperature", name);
	     f = fopen(buf, "rb");
	     if (f)
	       {
		  fgets(buf, sizeof(buf), f); buf[sizeof(buf) - 1] = 0;
		  units[0] = 0;
		  if (sscanf(buf, "%*[^:]: %i %20s", &temp, units) == 2)
		    {
		       /* temp == temperature as a double */
		       /* units = string ("C" == celcius, "F" = farenheight) */
		       _temperature_level_set(ef, ((double)temp - ef->temp->conf->low) / ef->temp->conf->high);
		       snprintf(buf, sizeof(buf), "%i°C", temp, units);
		       edje_object_part_text_set(ef->temp_object, "reading", buf);
		    }
		  fclose(f);
	       }
	     free(name);
	  }
     }
   return 1;
}

static void
_temperature_level_set(Temperature_Face *ef, double level)
{
   Edje_Message_Float msg;
   
   if (level < 0.0) level = 0.0;
   else if (level > 1.0) level = 1.0;
   msg.val = level;
   edje_object_message_send(ef->temp_object, EDJE_MESSAGE_FLOAT, 1, &msg);
}
