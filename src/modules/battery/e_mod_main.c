#include "e.h"
#include "e_mod_main.h"

/* TODO List:
 *
 * should support proepr resize and move handles in the edje.
 * 
 */

/* module private routines */
static Battery *_battery_init(E_Module *m);
static void     _battery_shutdown(Battery *e);
static E_Menu  *_battery_config_menu_new(Battery *e);
static void     _battery_config_menu_del(Battery *e, E_Menu *m);
static void     _battery_face_init(Battery_Face *ef);
static void     _battery_face_free(Battery_Face *ef);
static void     _battery_face_reconfigure(Battery_Face *ef);
static void     _battery_cb_face_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void     _battery_cb_face_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void     _battery_cb_face_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static int      _battery_cb_event_container_resize(void *data, int type, void *event);
static int      _battery_cb_check(void *data);
static int      _battery_linux_acpi_check(Battery_Face *ef);
static int      _battery_linux_apm_check(Battery_Face *ef);
static void     _battery_level_set(Battery_Face *ef, double level);

/* public module routines. all modules must have these */
void *
init(E_Module *m)
{
   Battery *e;
   
   /* check module api version */
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show("Module API Error",
			    "Error initializing Module: IBar\n"
			    "It requires a minimum module API version of: %i.\n"
			    "The module API advertized by Enlightenment is: %i.\n"
			    "Aborting module.",
			    E_MODULE_API_VERSION,
			    m->api->version);
	return NULL;
     }
   /* actually init battery */
   e = _battery_init(m);
   m->config_menu = _battery_config_menu_new(e);
   return e;
}

int
shutdown(E_Module *m)
{
   Battery *e;
   
   e = m->data;
   if (e)
     {
	if (m->config_menu)
	  {
	     _battery_config_menu_del(e, m->config_menu);
	     m->config_menu = NULL;
	  }
	_battery_shutdown(e);
     }
   return 1;
}

int
save(E_Module *m)
{
   Battery *e;

   e = m->data;
   e_config_domain_save("module.battery", e->conf_edd, e->conf);
   return 1;
}

int
info(E_Module *m)
{
   char buf[4096];
   
   m->label = strdup("Battery");
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

int
about(E_Module *m)
{
   e_error_dialog_show("Enlightenment Battery Module",
		       "A simple module to give E17 a battery meter.");
   return 1;
}

/* module private routines */
static Battery *
_battery_init(E_Module *m)
{
   Battery *e;
   Evas_List *managers, *l, *l2;
   
   e = calloc(1, sizeof(Battery));
   if (!e) return NULL;
   
   e->conf_edd = E_CONFIG_DD_NEW("Battery_Config", Config);
#undef T
#undef D
#define T Config
#define D e->conf_edd   
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_VAL(D, T, x, DOUBLE);
   E_CONFIG_VAL(D, T, y, DOUBLE);

   e->conf = e_config_domain_load("module.battery", e->conf_edd);
   if (!e->conf)
     {
	e->conf = E_NEW(Config, 1);
	e->conf->width = 64;
	e->conf->x = 1.0;
	e->conf->y = 1.0;
     }
   E_CONFIG_LIMIT(e->conf->width, 2, 256);
   E_CONFIG_LIMIT(e->conf->x, 0.0, 1.0);
   E_CONFIG_LIMIT(e->conf->y, 0.0, 1.0);
   
   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     Battery_Face *ef;
	     
	     con = l2->data;
	     ef = calloc(1, sizeof(Battery_Face));
	     if (ef)
	       {
		  ef->bat = e;
		  ef->con = con;
		  ef->evas = con->bg_evas;
		  _battery_face_init(ef);
		  e->face = ef;
	       }
	  }
     }
   return e;
}

static void
_battery_shutdown(Battery *e)
{
   free(e->conf);
   E_CONFIG_DD_FREE(e->conf_edd);
   
   _battery_face_free(e->face);
   free(e);
}

static E_Menu *
_battery_config_menu_new(Battery *e)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   /* FIXME: hook callbacks to each menu item */
   mn = e_menu_new();
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "(Unused)");
/*   e_menu_item_callback_set(mi, _battery_cb_time_set, e);*/
   e->config_menu = mn;
   
   return mn;
}

static void
_battery_config_menu_del(Battery *e, E_Menu *m)
{
   e_object_del(E_OBJECT(m));
}

static void
_battery_face_init(Battery_Face *ef)
{
   Evas_Coord ww, hh, bw, bh;
   Evas_Object *o;
   
   ef->ev_handler_container_resize =
     ecore_event_handler_add(E_EVENT_CONTAINER_RESIZE,
			     _battery_cb_event_container_resize,
			     ef);
   evas_output_viewport_get(ef->evas, NULL, NULL, &ww, &hh);
   ef->fx = ef->bat->conf->x * (ww - ef->bat->conf->width);
   ef->fy = ef->bat->conf->y * (hh - ef->bat->conf->width);
      
   evas_event_freeze(ef->evas);
   o = edje_object_add(ef->evas);
   ef->bat_object = o;

   edje_object_file_set(o,
			/* FIXME: "default.eet" needs to come from conf */
			e_path_find(path_themes, "default.eet"),
			"modules/battery/main");
   evas_object_show(o);
   
   o = evas_object_rectangle_add(ef->evas);
   ef->event_object = o;
   evas_object_layer_set(o, 2);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _battery_cb_face_down, ef);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _battery_cb_face_up, ef);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _battery_cb_face_move, ef);
   evas_object_show(o);
   
   edje_object_size_min_calc(ef->bat_object, &bw, &bh);
   ef->minsize = bh;
   ef->minsize = bw;

   _battery_face_reconfigure(ef);
   
   ef->battery_check_mode = CHECK_NONE;
   ef->battery_prev_drain = 1;
   ef->battery_prev_ac = -1;
   ef->battery_prev_battery = -1;
   ef->battery_check_timer = ecore_timer_add(1.0, _battery_cb_check, ef);
   
   _battery_cb_check(ef);
   
   evas_event_thaw(ef->evas);
}

static void
_battery_face_free(Battery_Face *ef)
{
   ecore_timer_derl(ef->battery_check_timer);
   ecore_event_handler_del(ef->ev_handler_container_resize);
   evas_object_del(ef->bat_object);
   evas_object_del(ef->event_object);
   free(ef);
}

static void
_battery_face_reconfigure(Battery_Face *ef)
{
   Evas_Coord minw, minh, maxw, maxh, ww, hh;

   edje_object_size_min_calc(ef->bat_object, &minw, &maxh);
   edje_object_size_max_get(ef->bat_object, &maxw, &minh);
   evas_output_viewport_get(ef->evas, NULL, NULL, &ww, &hh);
   ef->fx = ef->bat->conf->x * (ww - ef->bat->conf->width);
   ef->fy = ef->bat->conf->y * (hh - ef->bat->conf->width);
   ef->fw = ef->bat->conf->width;
   ef->minsize = minw;
   ef->maxsize = maxw;

   evas_object_move(ef->bat_object, ef->fx, ef->fy);
   evas_object_resize(ef->bat_object, ef->bat->conf->width, ef->bat->conf->width);
   evas_object_move(ef->event_object, ef->fx, ef->fy);
   evas_object_resize(ef->event_object, ef->bat->conf->width, ef->bat->conf->width);
}

static void
_battery_cb_face_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Battery_Face *ef;
   
   ev = event_info;
   ef = data;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(ef->bat->config_menu, ef->con,
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
_battery_cb_face_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   Battery_Face *ef;
   Evas_Coord ww, hh;
   
   ev = event_info;
   ef = data;
   ef->move = 0;
   ef->resize = 0;
   evas_output_viewport_get(ef->evas, NULL, NULL, &ww, &hh);
   ef->bat->conf->width = ef->fw;
   ef->bat->conf->x = (double)ef->fx / (double)(ww - ef->bat->conf->width);
   ef->bat->conf->y = (double)ef->fy / (double)(hh - ef->bat->conf->width);
   e_config_save_queue();
}

static void
_battery_cb_face_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{ 
   Evas_Event_Mouse_Move *ev;
   Battery_Face *ef;
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
	evas_object_move(ef->bat_object, ef->fx, ef->fy);
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
	evas_object_resize(ef->bat_object, ef->fw, ef->fw);
	evas_object_resize(ef->event_object, ef->fw, ef->fw);
   }
   ef->xx = ev->cur.canvas.x;
   ef->yy = ev->cur.canvas.y;
}  

static int
_battery_cb_event_container_resize(void *data, int type, void *event)
{
   Battery_Face *ef;
   
   ef = data;
   _battery_face_reconfigure(ef);
   return 1;
}

static int
_battery_cb_check(void *data)
{
   Battery_Face *ef;
   int ret = 0;
   
   ef = data;
   if (ef->battery_check_mode == 0)
     {
	if (e_file_is_dir("/proc/acpi"))
	  ef->battery_check_mode = CHECK_LINUX_ACPI;
	else if (e_file_is_dir("/proc/apm"))
	  ef->battery_check_mode = CHECK_LINUX_APM;
     }
   switch (ef->battery_check_mode)
     {
      case CHECK_LINUX_ACPI:
	ret = _battery_linux_acpi_check(ef);
	break;
      case CHECK_LINUX_APM:
	ret = _battery_linux_acpi_check(ef);
	break;
      default:
	break;
     }
   if (!ret)
     {
	if (ef->battery_prev_battery != -2)
	  {
	     edje_object_signal_emit(ef->bat_object, "unknown", "");
	     ef->battery_prev_battery = -2;
	  }
	edje_object_part_text_set(ef->bat_object, "reading", "NO INFO");
	edje_object_part_text_set(ef->bat_object, "time", "--:--");
	ef->battery_check_mode = CHECK_NONE;
	_battery_level_set(ef, (double)(rand() & 0xff) / 255.0);
     }
   return 1;
}

static int
_battery_linux_acpi_check(Battery_Face *ef)
{
   Evas_List *bats;
   char buf[4096], buf2[4096];
   
   int bat_max = 0;
   int bat_filled = 0;
   int bat_level = 0;
   int bat_drain = 1;
   
   int bat_val = 0;
   
   char current_status[256];
   int discharging = 0;
   int charging = 0;
   int battery = 0;
   
   int design_cap_unknown = 0;
   int last_full_unknown = 0;
   int rate_unknown = 0;
   int level_unknown = 0;
   
   int hours, minutes;

   /* Read some information on first run. */
   bats = e_file_ls("/proc/acpi/battery");
   while (bats) 
     {
	FILE *f;
	char *name;
	
	name = bats->data;
	if ((!strcmp(name, ".")) || (!strcmp(name, "..")))
	  {
	     bats = evas_list_remove_list(bats, bats);
	     free(name);
	     continue;
	  }
	snprintf(buf, sizeof(buf), "/proc/acpi/battery/%s/info", name);
	f = fopen(buf, "r");
	if (f)
	  {
	     int design_cap = 0;
	     int last_full = 0;
	     
	     fgets(buf2, sizeof(buf2), f); buf[sizeof(buf2) - 1] = 0;
	     fgets(buf2, sizeof(buf2), f); buf[sizeof(buf2) - 1] = 0;
	     sscanf(buf2, "%*[^:]: %250s %*s", buf);
	     if (!strcmp(buf, "unknown")) design_cap_unknown = 1;
	     else sscanf(buf2, "%*[^:]: %i %*s", &design_cap);
	     fgets(buf2, sizeof(buf2), f); buf[sizeof(buf2) - 1] = 0;
	     sscanf(buf2, "%*[^:]: %250s %*s", buf);
	     if (!strcmp(buf, "unknown")) last_full_unknown = 1;
	     else sscanf(buf2, "%*[^:]: %i %*s", &last_full);
	     fclose(f);
	     bat_max += design_cap;
	     bat_filled += last_full;
	  }
	snprintf(buf, sizeof(buf), "/proc/acpi/battery/%s/state", name);
	f = fopen(buf, "r");
	if (f)
	  {
	     char present[256];
	     char capacity_state[256];
	     char charging_state[256];
	     int rate = 1;
	     int level = 0;
	     
	     fgets(buf2, sizeof(buf2), f); buf[sizeof(buf2) - 1] = 0;
	     sscanf(buf2, "%*[^:]: %250s", present);
	     fgets(buf2, sizeof(buf2), f); buf[sizeof(buf2) - 1] = 0;
	     sscanf(buf2, "%*[^:]: %250s", capacity_state);
	     fgets(buf2, sizeof(buf2), f); buf[sizeof(buf2) - 1] = 0;
	     sscanf(buf2, "%*[^:]: %250s", charging_state);
	     fgets(buf2, sizeof(buf2), f); buf[sizeof(buf2) - 1] = 0;
	     sscanf(buf2, "%*[^:]: %250s %*s", buf);
	     if (!strcmp(buf, "unknown")) rate_unknown = 1;
	     else sscanf(buf2, "%*[^:]: %i %*s", &rate);
	     fgets(buf2, sizeof(buf2), f); buf[sizeof(buf2) - 1] = 0;
	     sscanf(buf2, "%*[^:]: %250s %*s", buf);
	     if (!strcmp(buf, "unknown")) level_unknown = 1;
	     else sscanf(buf2, "%*[^:]: %i %*s", &level);
	     fclose(f);
	     if (!strcmp(present, "yes")) battery++;
	     if (!strcmp(charging_state, "discharging")) discharging++;
	     if (!strcmp(charging_state, "charging")) charging++;
	     bat_drain += rate;
	     bat_level += level;
	  }
	bats = evas_list_remove_list(bats, bats);
	free(name);
     }
   
   if (ef->battery_prev_drain < 1) ef->battery_prev_drain = 1;
   if (bat_drain < 1) bat_drain = ef->battery_prev_drain;
   ef->battery_prev_drain = bat_drain;
   
   if (bat_filled > 0) bat_val = (100 * bat_level) / bat_filled;
   else bat_val = 100;
   
   if (discharging) minutes = (60 * bat_level) / bat_drain;
   else
     {
	if (bat_filled > 0)
	  minutes = (60 * (bat_filled - bat_level)) / bat_drain;
	else
	  minutes = 0;
     }
   hours = minutes / 60;
   minutes -= (hours * 60);
   
   if (hours < 0) hours = 0;
   if (minutes < 0) minutes = 0;
   
   if ((charging) || (discharging))
     {
	ef->battery_prev_battery = 1;
	if ((charging ) && (ef->battery_prev_ac != 1))
	  {
	     edje_object_signal_emit(ef->bat_object, "charge", "");
	     ef->battery_prev_ac = 1;
	  }
	else if ((discharging) && (ef->battery_prev_ac != 0))
	  {
	     edje_object_signal_emit(ef->bat_object, "discharge", "");
	     ef->battery_prev_ac = 0;
	  }
	if (level_unknown)
	  {
	     edje_object_part_text_set(ef->bat_object, "reading", "BAD DIRVER");
	     edje_object_part_text_set(ef->bat_object, "time", "--:--");
	     _battery_level_set(ef, 0.0);
	  }
	else if (rate_unknown)
	  {
	     snprintf(buf, sizeof(buf), "%i%%", bat_val);
	     edje_object_part_text_set(ef->bat_object, "reading", buf);
	     edje_object_part_text_set(ef->bat_object, "time", "--:--");
	     _battery_level_set(ef, (double)bat_val / 100.0);
	  }
	else
	  {
	     snprintf(buf, sizeof(buf), "%i%%", bat_val);
	     edje_object_part_text_set(ef->bat_object, "reading", buf);
	     snprintf(buf, sizeof(buf), "%i:%02i", hours, minutes);
	     edje_object_part_text_set(ef->bat_object, "time", buf);
	     _battery_level_set(ef, (double)bat_val / 100.0);
	  }
     }
   else if (!battery)
     {
	if (ef->battery_prev_battery != 0)
	  {
	     edje_object_signal_emit(ef->bat_object, "unknown", "");
	     ef->battery_prev_battery = 0;
	  }
	edje_object_part_text_set(ef->bat_object, "reading", "NO BAT");
	edje_object_part_text_set(ef->bat_object, "time", "--:--");
	_battery_level_set(ef, 1.0);
     }
   else
     {
	if (ef->battery_prev_battery == 0)
	  {
	     edje_object_signal_emit(ef->bat_object, "charge", "");
	     ef->battery_prev_battery = 1;
	  }
	if (ef->battery_prev_ac != 1)
	  {
	     edje_object_signal_emit(ef->bat_object, "charge", "");
	     ef->battery_prev_ac = 1;
	  }
	edje_object_part_text_set(ef->bat_object, "reading", "FULL");
	edje_object_part_text_set(ef->bat_object, "time", "--:--");
	_battery_level_set(ef, 1.0);
     }
   return 1;
}

static int
_battery_linux_apm_check(Battery_Face *ef)
{
   FILE *f;
   char s[256], s1[32], s2[32], s3[32], buf[4096];
   int  apm_flags, ac_stat, bat_stat, bat_flags, bat_val, time_val;
   int  hours, minutes;
   
   f = fopen("/proc/apm", "r");
   if (!f) return 0;
	     
   fgets(s, sizeof(s), f); s[sizeof(s) - 1] = 0;
   if (sscanf(s, "%*s %*s %x %x %x %x %s %s %s", 
	      &apm_flags, &ac_stat, &bat_stat, &bat_flags, s1, s2, s3) != 7)
     {
	fclose(f);
	return 0;
     }
   s1[strlen(s1) - 1] = 0;
   bat_val = atoi(s1);
   if (!strcmp(s3, "sec")) time_val = atoi(s2);
   else if (!strcmp(s3, "min")) time_val = atoi(s2) * 60;
   fclose(f);
   
   if ((bat_flags != 0xff) && (bat_flags & 0x80))
     {
	if (ef->battery_prev_battery != 0)
	  {
	     edje_object_signal_emit(ef->bat_object, "unknown", "");
	     ef->battery_prev_battery = 0;
	  }
	edje_object_part_text_set(ef->bat_object, "reading", "NO BAT");
	edje_object_part_text_set(ef->bat_object, "time", "--:--");
	_battery_level_set(ef, 1.0);
     }
   else
     {
	ef->battery_prev_battery = 1;
	if (bat_val > 0)
	  {
	     snprintf(buf, sizeof(buf), "%i%%", bat_val);
	     edje_object_part_text_set(ef->bat_object, "reading", buf);
	     _battery_level_set(ef, (double)bat_val / 100.0);
	  }
	else
	  {
	     switch( bat_stat )
	       {
		case 0:
		  edje_object_part_text_set(ef->bat_object, "reading", "High");
		  _battery_level_set(ef, 1.0);
		  break;
		case 1:
		  edje_object_part_text_set(ef->bat_object, "reading", "Low");
		  _battery_level_set(ef, 0.50);
		  break;
		case 2:
		  edje_object_part_text_set(ef->bat_object, "reading", "Danger");
		  _battery_level_set(ef, 0.25);
		  break;
		case 3:
		  edje_object_part_text_set(ef->bat_object, "reading", "Charge");
		  _battery_level_set(ef, 1.0);
		  break;
	       }
	  }
     }
   
   if (ac_stat == 1)
     {
	if ((ac_stat == 1) && (ef->battery_prev_ac == 0))
	  {
	     edje_object_signal_emit(ef->bat_object, "charge", "");
	     ef->battery_prev_ac = 1;
	  }
	else if ((ac_stat == 0) && (ef->battery_prev_ac == 1))
	  {
	     edje_object_signal_emit(ef->bat_object, "discharge", "");
	     ef->battery_prev_ac = 0;
	  }
	edje_object_part_text_set(ef->bat_object, "time", "--:--");
     }
   else
     {
	hours = time_val / 3600;
	minutes = (time_val / 60) % 60;
	snprintf(buf, sizeof(buf), "%i:%02i", hours, minutes);
	edje_object_part_text_set(ef->bat_object, "time", buf);
     }
   return 1;
}

static void
_battery_level_set(Battery_Face *ef, double level)
{
   Edje_Message_Float msg;
   
   if (level < 0.0) level = 0.0;
   else if (level > 1.0) level = 1.0;
   msg.val = level;
   edje_object_message_send(ef->bat_object, EDJE_MESSAGE_FLOAT, 1, &msg);
}
