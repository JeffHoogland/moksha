/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

#ifdef __FreeBSD__
# include <sys/types.h>
# include <sys/sysctl.h>
# include <fcntl.h>
# ifdef __i386__
#  include <machine/apm_bios.h>
# endif
# include <stdio.h>
#endif

#ifdef HAVE_CFBASE_H
#include <CFBase.h>
#include <CFNumber.h>
#include <CFArray.h>
#include <CFDictionary.h>
#include <CFRunLoop.h>
#include <ps/IOPSKeys.h>
#include <ps/IOPowerSources.h>
#endif

/* TODO List:
 *
 * which options should be in main menu, and which in face menu?
 */

/* module private routines */
static Battery      *_battery_new();
static void          _battery_shutdown(Battery *e);
static void          _battery_config_menu_new(Battery *e);
static int           _battery_cb_check(void *data);
static Status       *_battery_linux_acpi_check(Battery *ef);
static Status       *_battery_linux_apm_check(Battery *ef);
/* Should these be  #ifdef'd ?  */
#ifdef __FreeBSD__
static Status       *_battery_bsd_acpi_check(Battery *ef);
static Status       *_battery_bsd_apm_check(Battery *ef);
#endif

#ifdef HAVE_CFBASE_H
static Status       *_battery_darwin_check(Battery *ef);
#endif

static Battery_Face *_battery_face_new(E_Container *con);
static void          _battery_face_free(Battery_Face *ef);
static void          _battery_face_menu_new(Battery_Face *face);
static void          _battery_face_enable(Battery_Face *face);
static void          _battery_face_disable(Battery_Face *face);
static void          _battery_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);
static void          _battery_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void          _battery_face_level_set(Battery_Face *ef, double level);
static void          _battery_face_cb_menu_enabled(void *data, E_Menu *m, E_Menu_Item *mi);
static void          _battery_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);

static E_Config_DD *conf_edd;
static E_Config_DD *conf_face_edd;

static int battery_count;

/* public module routines. all modules must have these */
void *
e_modapi_init(E_Module *m)
{
   Battery *e;

   /* check module api version */
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show(_("Module API Error"),
			    _("Error initializing Module: Battery\n"
			      "It requires a minimum module API version of: %i.\n"
			      "The module API advertized by Enlightenment is: %i.\n"
			      "Aborting module."),
			    E_MODULE_API_VERSION,
			    m->api->version);
	return NULL;
     }
   /* actually init battery */
   e = _battery_new();
   m->config_menu = e->config_menu;
   return e;
}

int
e_modapi_shutdown(E_Module *m)
{
   Battery *e;
   if (m->config_menu)
     m->config_menu = NULL;

   e = m->data;
   if (e)
     _battery_shutdown(e);

   return 1;
}

int
e_modapi_save(E_Module *m)
{
   Battery *e;

   e = m->data;
   e_config_domain_save("module.battery", conf_edd, e->conf);
   return 1;
}

int
e_modapi_info(E_Module *m)
{
   char buf[4096];

   m->label = strdup(_("Battery"));
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

int
e_modapi_about(E_Module *m)
{
   e_error_dialog_show(_("Enlightenment Battery Module"),
		       _("A basic battery meter that uses either ACPI or APM\n"
			 "on Linux to monitor your battery and AC power adaptor\n"
			 "status. This will work under Linux and FreeBSD and is only\n"
			 "as accurate as your BIOS or kernel drivers."));
   return 1;
}

/* module private routines */
static Battery *
_battery_new()
{
   Battery *e;
   Evas_List *managers, *l, *l2, *cl;

   E_Menu_Item *mi;

   battery_count = 0;
   e = E_NEW(Battery, 1);
   if (!e) return NULL;

   conf_face_edd = E_CONFIG_DD_NEW("Battery_Config_Face", Config_Face);
#undef T
#undef D
#define T Config_Face
#define D conf_face_edd
   E_CONFIG_VAL(D, T, enabled, UCHAR);

   conf_edd = E_CONFIG_DD_NEW("Battery_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, poll_time, DOUBLE);
   E_CONFIG_VAL(D, T, alarm, INT);
   E_CONFIG_LIST(D, T, faces, conf_face_edd);

   e->conf = e_config_domain_load("module.battery", conf_edd);
   if (!e->conf)
     {
       e->conf = E_NEW(Config, 1);
       e->conf->poll_time = 30.0;
       e->conf->alarm = 30;
     }
   E_CONFIG_LIMIT(e->conf->poll_time, 0.5, 1000.0);
   E_CONFIG_LIMIT(e->conf->alarm, 0, 60);

   _battery_config_menu_new(e);

   e->battery_check_mode = CHECK_NONE;
   e->battery_prev_drain = 1;
   e->battery_prev_ac = -1;
   e->battery_prev_battery = -1;
   e->battery_check_timer = ecore_timer_add(e->conf->poll_time, _battery_cb_check, e);

   managers = e_manager_list();
   cl = e->conf->faces;
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;

	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     Battery_Face *ef;

	     con = l2->data;
	     ef = _battery_face_new(con);
	     if (ef)
	       {
		  e->faces = evas_list_append(e->faces, ef);

		  /* Config */
		  if (!cl)
		    {
		       ef->conf = E_NEW(Config_Face, 1);
		       ef->conf->enabled = 1;
		       e->conf->faces = evas_list_append(e->conf->faces, ef->conf);
		    }
		  else
		    {
		       ef->conf = cl->data;
		       cl = cl->next;
		    }

		  /* Menu */
		  /* This menu must be initialized after conf */
		  _battery_face_menu_new(ef);

		  /* Add main menu to face menu */
		  mi = e_menu_item_new(ef->menu);
		  e_menu_item_label_set(mi, _("Set Poll Time"));
		  e_menu_item_submenu_set(mi, e->config_menu_poll);

		  mi = e_menu_item_new(ef->menu);
		  e_menu_item_label_set(mi, _("Set Alarm"));
		  e_menu_item_submenu_set(mi, e->config_menu_alarm);

		  mi = e_menu_item_new(e->config_menu);
		  e_menu_item_label_set(mi, con->name);
		  e_menu_item_submenu_set(mi, ef->menu);

		  /* Setup */
		  if (!ef->conf->enabled)
		    _battery_face_disable(ef);
	       }
	  }
     }

   _battery_cb_check(e);

   return e;
}

static void
_battery_shutdown(Battery *e)
{
   Evas_List *l;

   E_CONFIG_DD_FREE(conf_edd);
   E_CONFIG_DD_FREE(conf_face_edd);

   for (l = e->faces; l; l = l->next)
     _battery_face_free(l->data);
   evas_list_free(e->faces);

   e_object_del(E_OBJECT(e->config_menu));
   e_object_del(E_OBJECT(e->config_menu_poll));
   e_object_del(E_OBJECT(e->config_menu_alarm));

   ecore_timer_del(e->battery_check_timer);

   evas_list_free(e->conf->faces);
   free(e->conf);
   free(e);
}

static void
_battery_menu_alarm_10(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Battery *e;

   e = data;
   e->conf->alarm = 10;
   e_config_save_queue();
}

static void
_battery_menu_alarm_20(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Battery *e;

   e = data;
   e->conf->alarm = 20;
   e_config_save_queue();
}

static void
_battery_menu_alarm_30(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Battery *e;

   e = data;
   e->conf->alarm = 30;
   e_config_save_queue();
}

static void
_battery_menu_alarm_40(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Battery *e;

   e = data;
   e->conf->alarm = 40;
   e_config_save_queue();
}

static void
_battery_menu_alarm_50(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Battery *e;

   e = data;
   e->conf->alarm = 50;
   e_config_save_queue();
}

static void
_battery_menu_alarm_60(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Battery *e;

   e = data;
   e->conf->alarm = 60;
   e_config_save_queue();
}

static void
_battery_menu_alarm_disable(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Battery *e;

   e = data;
   e->conf->alarm = 0;
   e_config_save_queue();
}

static void
_battery_menu_fast(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Battery *e;

   e = data;
   e->conf->poll_time = 1.0;
   ecore_timer_del(e->battery_check_timer);
   e->battery_check_timer = ecore_timer_add(e->conf->poll_time, _battery_cb_check, e);
   e_config_save_queue();
}

static void
_battery_menu_medium(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Battery *e;

   e = data;
   e->conf->poll_time = 5.0;
   ecore_timer_del(e->battery_check_timer);
   e->battery_check_timer = ecore_timer_add(e->conf->poll_time, _battery_cb_check, e);
   e_config_save_queue();
}

static void
_battery_menu_normal(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Battery *e;

   e = data;
   e->conf->poll_time = 10.0;
   ecore_timer_del(e->battery_check_timer);
   e->battery_check_timer = ecore_timer_add(e->conf->poll_time, _battery_cb_check, e);
   e_config_save_queue();
}

static void
_battery_menu_slow(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Battery *e;

   e = data;
   e->conf->poll_time = 30.0;
   ecore_timer_del(e->battery_check_timer);
   e->battery_check_timer = ecore_timer_add(e->conf->poll_time, _battery_cb_check, e);
   e_config_save_queue();
}

static void
_battery_menu_very_slow(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Battery *e;

   e = data;
   e->conf->poll_time = 60.0;
   ecore_timer_del(e->battery_check_timer);
   e->battery_check_timer = ecore_timer_add(e->conf->poll_time, _battery_cb_check, e);
   e_config_save_queue();
}

static void
_battery_config_menu_new(Battery *e)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   /* Alarm */
   mn = e_menu_new();

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Disable"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->alarm == 0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _battery_menu_alarm_disable, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("10 mins"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->alarm == 10) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _battery_menu_alarm_10, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("20 mins"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->alarm == 20) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _battery_menu_alarm_20, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("30 mins"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->alarm == 30) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _battery_menu_alarm_30, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("40 mins"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->alarm == 40) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _battery_menu_alarm_40, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("50 mins"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->alarm == 50) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _battery_menu_alarm_50, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("1 hour"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->alarm == 60) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _battery_menu_alarm_60, e);

   e->config_menu_alarm = mn;

   /* Check interval */
   mn = e_menu_new();

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Check Fast (1 sec)"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time == 1.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _battery_menu_fast, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Check Medium (5 sec)"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time == 5.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _battery_menu_medium, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Check Normal (10 sec)"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time == 10.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _battery_menu_normal, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Check Slow (30 sec)"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time == 30.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _battery_menu_slow, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Check Very Slow (60 sec)"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time == 60.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _battery_menu_very_slow, e);

   e->config_menu_poll = mn;

   mn = e_menu_new();

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Set Poll Time"));
   e_menu_item_submenu_set(mi, e->config_menu_poll);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Set Alarm"));
   e_menu_item_submenu_set(mi, e->config_menu_alarm);

   e->config_menu = mn;
}

static Battery_Face *
_battery_face_new(E_Container *con)
{
   Evas_Object *o;
   Battery_Face *ef;

   ef = E_NEW(Battery_Face, 1);
   if (!ef) return NULL;

   ef->con = con;
   e_object_ref(E_OBJECT(con));

   evas_event_freeze(con->bg_evas);
   o = edje_object_add(con->bg_evas);
   ef->bat_object = o;

   e_theme_edje_object_set(o, "base/theme/modules/battery",
			   "modules/battery/main");
   evas_object_show(o);

   o = evas_object_rectangle_add(con->bg_evas);
   ef->event_object = o;
   evas_object_layer_set(o, 2);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _battery_face_cb_mouse_down, ef);
   evas_object_show(o);

   ef->gmc = e_gadman_client_new(con->gadman);
   e_gadman_client_domain_set(ef->gmc, "module.battery", battery_count++);
   e_gadman_client_policy_set(ef->gmc,
			      E_GADMAN_POLICY_ANYWHERE |
			      E_GADMAN_POLICY_HMOVE |
			      E_GADMAN_POLICY_VMOVE |
			      E_GADMAN_POLICY_HSIZE |
			      E_GADMAN_POLICY_VSIZE);
   e_gadman_client_min_size_set(ef->gmc, 4, 4);
   e_gadman_client_max_size_set(ef->gmc, 128, 128);
   e_gadman_client_auto_size_set(ef->gmc, 40, 40);
   e_gadman_client_align_set(ef->gmc, 1.0, 1.0);
   e_gadman_client_resize(ef->gmc, 40, 40);
   e_gadman_client_change_func_set(ef->gmc, _battery_face_cb_gmc_change, ef);
   e_gadman_client_load(ef->gmc);
   evas_event_thaw(con->bg_evas);

   return ef;
}

static void
_battery_face_free(Battery_Face *ef)
{
   e_object_unref(E_OBJECT(ef->con));
   e_object_del(E_OBJECT(ef->gmc));
   e_object_del(E_OBJECT(ef->menu));
   evas_object_del(ef->bat_object);
   evas_object_del(ef->event_object);

   free(ef->conf);
   free(ef);
   battery_count--;
}

static void
_battery_face_menu_new(Battery_Face *face)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   mn = e_menu_new();
   face->menu = mn;

   /* Enabled */
   /*   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Enabled"));
   e_menu_item_check_set(mi, 1);
   if (face->conf->enabled) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _battery_face_cb_menu_enabled, face);
    */
   
   /* Edit */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Edit Mode"));
   e_menu_item_callback_set(mi, _battery_face_cb_menu_edit, face);
}

static void
_battery_face_enable(Battery_Face *face)
{
   face->conf->enabled = 1;
   evas_object_show(face->bat_object);
   evas_object_show(face->event_object);
   e_config_save_queue();
}

static void
_battery_face_disable(Battery_Face *face)
{
   face->conf->enabled = 0;
   evas_object_hide(face->bat_object);
   evas_object_hide(face->event_object);
   e_config_save_queue();
}

static void
_battery_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   Battery_Face *ef;
   Evas_Coord x, y, w, h;

   ef = data;
   switch (change)
     {
      case E_GADMAN_CHANGE_MOVE_RESIZE:
	 e_gadman_client_geometry_get(ef->gmc, &x, &y, &w, &h);
	 evas_object_move(ef->bat_object, x, y);
	 evas_object_move(ef->event_object, x, y);
	 evas_object_resize(ef->bat_object, w, h);
	 evas_object_resize(ef->event_object, w, h);
	 break;
      case E_GADMAN_CHANGE_RAISE:
	 evas_object_raise(ef->bat_object);
	 evas_object_raise(ef->event_object);
	 break;
      case E_GADMAN_CHANGE_EDGE:
      case E_GADMAN_CHANGE_ZONE:
	 /* FIXME
	  * Must we do something here?
	  */
	 break;
     }
}

static void
_battery_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Battery_Face *ef;

   ev = event_info;
   ef = data;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(ef->menu, e_zone_current_get(ef->con),
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	e_util_container_fake_mouse_up_all_later(ef->con);
     }
}

static int
_battery_cb_check(void *data)
{
   Battery *ef;
   Battery_Face *face;
   Evas_List *l;
   Status *ret = NULL;

   ef = data;
#ifdef __FreeBSD__
   int acline;
   size_t len;
   int apm_fd = -1;
   int acline_mib[3] = {-1};
   
   if (ef->battery_check_mode == 0)
     {
      	len = sizeof(acline);
      	if (sysctlbyname("hw.acpi.acline", &acline, &len, NULL, 0) == 0) 
      	  {
	     len = 3;
	     if (sysctlnametomib("hw.acpi.acline", acline_mib, &len) == 0)
      	        ef->battery_check_mode = CHECK_ACPI; 
	  }
	else
	  {
	     apm_fd = open("/dev/apm", O_RDONLY); 
	     if (apm_fd != -1)
	       ef->battery_check_mode = CHECK_APM;
	  }
     }
   switch (ef->battery_check_mode)
     {
      case CHECK_ACPI:
	ret = _battery_bsd_acpi_check(ef);
	break;
      case CHECK_APM:
	ret = _battery_bsd_apm_check(ef);
	break;
      default:
	break;
     }
#elif defined(HAVE_CFBASE_H) /* OS X */
   ret = _battery_darwin_check(ef);
#else
   if (ef->battery_check_mode == 0)
     {
	if (ecore_file_is_dir("/proc/acpi"))
	  ef->battery_check_mode = CHECK_ACPI;
	else if (ecore_file_exists("/proc/apm"))
	  ef->battery_check_mode = CHECK_APM;
     }
   switch (ef->battery_check_mode)
     {
      case CHECK_ACPI:
	ret = _battery_linux_acpi_check(ef);
	break;
      case CHECK_APM:
	ret = _battery_linux_apm_check(ef);
	break;
      default:
	break;
     }
#endif
   if (ret)
     {
	if (ret->has_battery)
	  {
	     if (ret->state == BATTERY_STATE_CHARGING)
	       {
		  for (l = ef->faces; l; l = l->next)
		    {
		       face = l->data;
		       if (ef->battery_prev_ac != 1)
			 edje_object_signal_emit(face->bat_object, "charge", "");
		       edje_object_signal_emit(face->bat_object, "pulsestop", "");
		       edje_object_part_text_set(face->bat_object, "reading", ret->reading);
		       edje_object_part_text_set(face->bat_object, "time", ret->time);
		       _battery_face_level_set(face, ret->level);

		    }
		  ef->battery_prev_ac = 1;
	       }
	     else if (ret->state == BATTERY_STATE_DISCHARGING)
	       {
		  for (l = ef->faces; l; l = l->next)
		    {
		       face = l->data;
		       if (ef->battery_prev_ac != 0)
			 edje_object_signal_emit(face->bat_object, "discharge", "");
		       if (ret->alarm)
			 {
			    if (!ef->alarm_triggered)
			      {
				 e_error_dialog_show(_("Battery Running Low"),
						     _("Your battery is running low.\n"
						       "You may wish to switch to an AC source."));
			      }
			    edje_object_signal_emit(face->bat_object, "pulse", "");
			 }
		       edje_object_part_text_set(face->bat_object, "reading", ret->reading);
		       edje_object_part_text_set(face->bat_object, "time", ret->time);
		       _battery_face_level_set(face, ret->level);

		    }
		  ef->battery_prev_ac = 0;
		  if (ret->alarm)
		    ef->alarm_triggered = 1;
	       }
	     else
	       {
		  /* ret->state == BATTERY_STATE_NONE */
		  for (l = ef->faces; l; l = l->next)
		    {
		       face = l->data;
		       if (ef->battery_prev_ac != 1)
			 edje_object_signal_emit(face->bat_object, "charge", "");
		       if (ef->battery_prev_battery == 0)
			 edje_object_signal_emit(face->bat_object, "charge", "");
		       edje_object_part_text_set(face->bat_object, "reading", ret->reading);
		       edje_object_part_text_set(face->bat_object, "time", ret->time);
		       _battery_face_level_set(face, ret->level);

		    }
		  ef->battery_prev_ac = 1;
		  ef->battery_prev_battery = 1;
	       }
	  }
	else
	  {
	     /* Hasn't battery */
	     for (l = ef->faces; l; l = l->next)
	       {
		  face = l->data;
		  if (ef->battery_prev_battery != 0)
		    edje_object_signal_emit(face->bat_object, "unknown", "");
		  edje_object_part_text_set(face->bat_object, "reading", ret->reading);
		  edje_object_part_text_set(face->bat_object, "time", ret->time);
		  _battery_face_level_set(face, ret->level);

	       }
	     ef->battery_prev_battery = 0;
	  }
	free(ret->reading);
	free(ret->time);
	free(ret);
     }
   else
     {
	/* Error reading status */
	for (l = ef->faces; l; l = l->next)
	  {
	     face = l->data;

	     if (ef->battery_prev_battery != -2)
	       edje_object_signal_emit(face->bat_object, "unknown", "");
	     edje_object_part_text_set(face->bat_object, "reading", _("NO INFO"));
	     edje_object_part_text_set(face->bat_object, "time", "--:--");
	     _battery_face_level_set(face, (double)(rand() & 0xff) / 255.0);
	  }
	ef->battery_prev_battery = -2;
	ef->battery_check_mode = CHECK_NONE;
     }
   return 1;
}

static Status *
_battery_linux_acpi_check(Battery *ef)
{
   Ecore_List *bats;
   char buf[4096], buf2[4096];
   char *name;

   int bat_max = 0;
   int bat_filled = 0;
   int bat_level = 0;
   int bat_drain = 1;

   int bat_val = 0;

   int discharging = 0;
   int charging = 0;
   int battery = 0;

   int design_cap_unknown = 0;
   int last_full_unknown = 0;
   int rate_unknown = 0;
   int level_unknown = 0;

   int hours, minutes;

   Status *stat;

   stat = E_NEW(Status, 1);

   /* Read some information on first run. */
   bats = ecore_file_ls("/proc/acpi/battery");
   while ((bats) && (name = ecore_list_next(bats)))
     {
	FILE *f;

	if ((!strcmp(name, ".")) || (!strcmp(name, "..")))
	  {
	     free(name);
	     continue;
	  }
	snprintf(buf, sizeof(buf), "/proc/acpi/battery/%s/info", name);
	f = fopen(buf, "r");
	if (f)
	  {
	     int design_cap = 0;
	     int last_full = 0;

	     fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
	     fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
	     sscanf(buf2, "%*[^:]: %250s %*s", buf);
	     if (!strcmp(buf, "unknown")) design_cap_unknown = 1;
	     else sscanf(buf2, "%*[^:]: %i %*s", &design_cap);
	     fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
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

	     fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
	     sscanf(buf2, "%*[^:]: %250s", present);
	     fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
	     sscanf(buf2, "%*[^:]: %250s", capacity_state);
	     fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
	     sscanf(buf2, "%*[^:]: %250s", charging_state);
	     fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
	     sscanf(buf2, "%*[^:]: %250s %*s", buf);
	     if (!strcmp(buf, "unknown")) rate_unknown = 1;
	     else sscanf(buf2, "%*[^:]: %i %*s", &rate);
	     fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
	     sscanf(buf2, "%*[^:]: %250s %*s", buf);
	     if (!strcmp(buf, "unknown")) level_unknown = 1;
	     else sscanf(buf2, "%*[^:]: %i %*s", &level);
	     fclose(f);
	     if (!strcmp(present, "yes")) battery++;
	     if (!strcmp(charging_state, "discharging"))
	       {
		  discharging++;
		  if ((rate == 0) && (rate_unknown == 0)) rate_unknown = 1;
	       }
	     if (!strcmp(charging_state, "charging"))
	       {
		  charging++;
		  if ((rate == 0) && (rate_unknown == 0)) rate_unknown = 1;
	       }
	     if (!strcmp(charging_state, "charged")) rate_unknown = 0;
	     bat_drain += rate;
	     bat_level += level;
	  }
	free(name);
     }
   if (bats) ecore_list_destroy(bats);

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

   if (!battery)
     {
	stat->has_battery = 0;
	stat->state = BATTERY_STATE_NONE;
	stat->reading = strdup(_("NO BAT"));
	stat->time = strdup("--:--");
	stat->level = 1.0;
     }
   else if ((charging) || (discharging))
     {
	ef->battery_prev_battery = 1;
	stat->has_battery = 1;
        if (charging)
          {
	     stat->state = BATTERY_STATE_CHARGING;
	     ef->alarm_triggered = 0;
          }
	else if (discharging)
	  {
	     stat->state = BATTERY_STATE_DISCHARGING;
	     if (stat->level < 0.1)
	       {
		  if (((hours * 60) + minutes) <= ef->conf->alarm)
		    stat->alarm = 1;
	       }
	  }
	if (level_unknown)
          {
	     stat->reading = strdup(_("BAD DRIVER"));
	     stat->time = strdup("--:--");
	     stat->level = 0.0;
          }
	else if (rate_unknown)
          {
             snprintf(buf, sizeof(buf), "%i%%", bat_val);
	     stat->reading = strdup(buf);
	     stat->time = strdup("--:--");
	     stat->level = (double)bat_val / 100.0;
          }
	else
          {
             snprintf(buf, sizeof(buf), "%i%%", bat_val);
	     stat->reading = strdup(buf);
             snprintf(buf, sizeof(buf), "%i:%02i", hours, minutes);
	     stat->time = strdup(buf);
	     stat->level = (double)bat_val / 100.0;
          }
     }
   else
     {
	stat->has_battery = 1;
	stat->state = BATTERY_STATE_NONE;
	stat->reading = strdup(_("FULL"));
	stat->time = strdup("--:--");
	stat->level = 1.0;
     }
   return stat;
}

static Status *
_battery_linux_apm_check(Battery *ef)
{
   FILE *f;
   char s[256], s1[32], s2[32], s3[32], buf[4096];
   int  apm_flags, ac_stat, bat_stat, bat_flags, bat_val, time_val;
   int  hours, minutes;

   Status *stat;

   f = fopen("/proc/apm", "r");
   if (!f) return NULL;

   fgets(s, sizeof(s), f); s[sizeof(s) - 1] = 0;
   if (sscanf(s, "%*s %*s %x %x %x %x %s %s %s",
	      &apm_flags, &ac_stat, &bat_stat, &bat_flags, s1, s2, s3) != 7)
     {
	fclose(f);
	return NULL;
     }
   s1[strlen(s1) - 1] = 0;
   bat_val = atoi(s1);
   if (!strcmp(s3, "sec")) time_val = atoi(s2);
   else if (!strcmp(s3, "min")) time_val = atoi(s2) * 60;
   fclose(f);

   stat = E_NEW(Status, 1);

   if ((bat_flags != 0xff) && (bat_flags & 0x80))
     {
	stat->has_battery = 0;
	stat->state = BATTERY_STATE_NONE;
	stat->reading = strdup("NO BAT");
	stat->time = strdup("--:--");
	stat->level = 1.0;
	return stat;
     }
   
   
   ef->battery_prev_battery = 1;
   stat->has_battery = 1;
   if (bat_val >= 0)
     {
	if (bat_val > 100) bat_val = 100;
	snprintf(buf, sizeof(buf), "%i%%", bat_val);
	stat->reading = strdup(buf);
	stat->level = (double)bat_val / 100.0;
     }
   else
     {
	switch (bat_stat)
	  {
	   case 0:
	      stat->reading = strdup(_("High"));
	      stat->level = 1.0;
	      break;
	   case 1:
	      stat->reading = strdup(_("Low"));
	      stat->level = 0.5;
	      break;
	   case 2:
	      stat->reading = strdup(_("Danger"));
	      stat->level = 0.25;
	      break;
	   case 3:
	      stat->reading = strdup(_("Charging"));
	      stat->level = 1.0;
	      break;
	  }
     }

   if (ac_stat == 1)
     {
	stat->state = BATTERY_STATE_CHARGING;
	stat->time = strdup("--:--");
     }
   else
     {
	/* ac_stat == 0 */
	stat->state = BATTERY_STATE_DISCHARGING;

	hours = time_val / 3600;
	minutes = (time_val / 60) % 60;
	snprintf(buf, sizeof(buf), "%i:%02i", hours, minutes);
	stat->time = strdup(buf);
	if (stat->level < 0.1)
	  {
	     if (((hours * 60) + minutes) <= ef->conf->alarm)
	       stat->alarm = 1;
	  }
     }

   return stat;
}

#ifdef __FreeBSD__
static Status *
_battery_bsd_acpi_check(Battery *ef)
{
   /* Assumes only a single battery - I don't know how multiple batts 
    * are represented in sysctl */

   Ecore_List *bats;
   char buf[4096], buf2[4096];
   char *name;

   int bat_max = 0;
   int bat_filled = 0;
   int bat_level = 0;
   int bat_drain = 1;

   int bat_val = 0;

   int discharging = 0;
   int charging = 0;
   int battery = 0;

   int design_cap_unknown = 0;
   int last_full_unknown = 0;
   int rate_unknown = 0;
   int level_unknown = 0;

   int hours, minutes;
   
   int mib_state[4];
   int mib_life[4];
   int mib_time[4];
   int mib_units[4];
   size_t len;
   int state;
   int level;
   int time;
   int life;
   int batteries;

   Status *stat;

   stat = E_NEW(Status, 1);

   /* Read some information on first run. */
   len = 4;
   sysctlnametomib("hw.acpi.battery.state", mib_state, &len);
   len = sizeof(state);
   if (sysctl(mib_state, 4, &state, &len, NULL, 0) == -1)
      /* ERROR */
      state = -1;     

   len = 4;
   sysctlnametomib("hw.acpi.battery.life", mib_life, &len);
   len = sizeof(life);
   if (sysctl(mib_life, 4, &life, &len, NULL, 0) == -1)
      /* ERROR */
      level = -1; 

   bat_val = life;
      
   len = 4;
   sysctlnametomib("hw.acpi.battery.time", mib_time, &len);
   len = sizeof(time);
   if (sysctl(mib_time, 4, &time, &len, NULL, 0) == -1)
      /* ERROR */
      time = -2;     

   len = 4;
   sysctlnametomib("hw.acpi.battery.units", mib_units, &len);
   len = sizeof(batteries);
   if (sysctl(mib_time, 4, &batteries, &len, NULL, 0) == -1)
      /* ERROR */
      batteries = 1;     
   
   if (ef->battery_prev_drain < 1)  
     ef->battery_prev_drain = 1;
   
   if (bat_drain < 1) 
     bat_drain = ef->battery_prev_drain;
   
   ef->battery_prev_drain = bat_drain;

   /*if (bat_filled > 0) 
     bat_val = (100 * bat_level) / bat_filled;
   else 
     bat_val = 100;

   if (state == BATTERY_STATE_DISCHARGING) 
     minutes = (60 * bat_level) / bat_drain;
   else
     {
	if (bat_filled > 0)
	  minutes = (60 * (bat_filled - bat_level)) / bat_drain;
	else
	  minutes = 0;
     }*/
   minutes = time;
   hours = minutes / 60;
   minutes -= (hours * 60);

   if (hours < 0) 
     hours = 0;
   if (minutes < 0) 
     minutes = 0;

   if (batteries == 1) /* hw.acpi.battery.units = 1 means NO BATTS */
     {
	stat->has_battery = 0;
	stat->state = BATTERY_STATE_NONE;
	stat->reading = strdup(_("NO BAT"));
	stat->time = strdup("--:--");
	stat->level = 1.0;
     }
   else if ((state == BATTERY_STATE_CHARGING) || 
	    (state == BATTERY_STATE_DISCHARGING)) 
     {
	ef->battery_prev_battery = 1;
	stat->has_battery = 1;
        if (state == BATTERY_STATE_CHARGING)
          {
	     stat->state = BATTERY_STATE_CHARGING;
	     ef->alarm_triggered = 0;
          }
	else if (state == BATTERY_STATE_DISCHARGING)
	  {
	     stat->state = BATTERY_STATE_DISCHARGING;
	     if (stat->level < 0.1) /* Why this if condition */
	       {
		  if (((hours * 60) + minutes) <= ef->conf->alarm)
		    stat->alarm = 1;
	       }
	  }
	if (!level)
          {
	     stat->reading = strdup(_("BAD DRIVER"));
	     stat->time = strdup("--:--");
	     stat->level = 0.0;
          }
	else if (time == -1)
          {
             snprintf(buf, sizeof(buf), "%i%%", bat_val);
	     stat->reading = strdup(buf);
	     stat->time = strdup("--:--");
	     stat->level = (double)bat_val / 100.0;
          }
	else
          {
             snprintf(buf, sizeof(buf), "%i%%", bat_val);
	     stat->reading = strdup(buf);
             snprintf(buf, sizeof(buf), "%i:%02i", hours, minutes);
	     stat->time = strdup(buf);
	     stat->level = (double)bat_val / 100.0;
          }
     }
   else
     {
	stat->has_battery = 1;
	stat->state = BATTERY_STATE_NONE;
	stat->reading = strdup(_("FULL"));
	stat->time = strdup("--:--");
	stat->level = 1.0;
     }
   return stat;
}

static Status *
_battery_bsd_apm_check(Battery *ef)
{
   int  ac_stat, bat_stat, bat_val, time_val;
   char buf[4096];
   int  hours, minutes;
   int apm_fd = -1;
   struct apm_info info;
   
   Status *stat;

   apm_fd = open("/dev/apm", O_RDONLY);
   
   if (apm_fd != -1 && ioctl(apm_fd, APMIO_GETINFO, &info) != -1)
     {
	/* set values */
	ac_stat = info.ai_acline;
	bat_stat = info.ai_batt_stat;
	bat_val = info.ai_batt_life;
	time_val = info.ai_batt_time;
     }
   else
     {
        return NULL;
     }

   stat = E_NEW(Status, 1);

   if (info.ai_batteries == 1) /* ai_batteries == 1 means NO battery,
				  ai_batteries == 2 means 1 battery */
     {
	stat->has_battery = 0;
	stat->state = BATTERY_STATE_NONE;
	stat->reading = strdup("NO BAT");
	stat->time = strdup("--:--");
	stat->level = 1.0;
	return stat;
     }
   
   
   ef->battery_prev_battery = 1;
   stat->has_battery = 1;

   if (ac_stat) /* Wallpowered */
     {
	stat->state = BATTERY_STATE_CHARGING;
	stat->time = strdup("--:--");
	switch (bat_stat) /* On FreeBSD the time_val is -1 when AC ist plugged
			     in. This means we don't know how long the battery
			     will recharge */ 
	  {
	   case 0:
	      stat->reading = strdup(_("High"));
	      stat->level = 1.0; 
	      break;
	   case 1:
	      stat->reading = strdup(_("Low"));
	      stat->level = 0.5;
	      break;
	   case 2:
	      stat->reading = strdup(_("Danger"));
	      stat->level = 0.25;
	      break;
	   case 3:
	      stat->reading = strdup(_("Charging"));
	      stat->level = 1.0;
	      break;
	  }
     }
   else /* Running on battery */
     {
	stat->state = BATTERY_STATE_DISCHARGING;
	
        snprintf(buf, sizeof(buf), "%i%%", bat_val);
        stat->reading = strdup(buf);
        stat->level = (double)bat_val / 100.0;
	
	hours = time_val / 3600;
	minutes = (time_val / 60) % 60;
	snprintf(buf, sizeof(buf), "%i:%02i", hours, minutes);
	stat->time = strdup(buf);

	if (stat->level < 0.1) /* Why this if condition? */
	  {
	     if (((hours * 60) + minutes) <= ef->conf->alarm)
	       stat->alarm = 1;
	  }
     }
   
   return stat;
}
#endif

#ifdef HAVE_CFBASE_H
/*
 * There is a good chance this will work with a UPS as well as a battery.
 */
static Status *
_battery_darwin_check(Battery *ef)
{
   const void *values;
   int device_num;
   int device_count;
   int  hours, minutes;
   int currentval = 0;
   int maxval = 0;
   char buf[4096];
   CFTypeRef blob;
   CFArrayRef sources;
   CFDictionaryRef device_dict;
   
   Status *stat;

   stat = E_NEW(Status, 1);

   /*
    * Retrieve the power source data and the array of sources.
    */
   blob = IOPSCopyPowerSourcesInfo();
   sources = IOPSCopyPowerSourcesList(blob);
   device_count = CFArrayGetCount(sources); 

   for (device_num = 0; device_num < device_count; device_num++)
     {
	CFTypeRef ps;

	printf("device %d of %d\n", device_num, device_count);

	/*
	 * Retrieve a dictionary of values for this device and the
	 * count of keys in the dictionary.
	 */
	ps = CFArrayGetValueAtIndex(sources, device_num);
	device_dict = IOPSGetPowerSourceDescription(blob, ps);

	/*
	 * Retrieve the charging key and save the present charging value if
	 * one exists.
	 */
	if (CFDictionaryGetValueIfPresent(device_dict, CFSTR(kIOPSIsChargingKey), &values))
	  {
	     stat->has_battery = 1;
	     printf("%s: %d\n", kIOPSIsChargingKey, CFBooleanGetValue(values));
	     if (CFBooleanGetValue(values) > 0)
	       {
		  stat->state = BATTERY_STATE_CHARGING;
	       }
	     else
	       {
		  stat->state = BATTERY_STATE_DISCHARGING;
	       }
	     // CFRelease(values);
	     break;
	  }

     }

   /*
    * Check for battery.
    */
   if (!stat->has_battery)
     {
	CFRelease(sources);
	CFRelease(blob);
	stat->state = BATTERY_STATE_NONE;
	stat->reading = strdup("NO BAT");
	stat->time = strdup("--:--");
	stat->level = 1.0;
	return stat;
     }

   /*
    * A battery was found so move along based on that assumption.
    */
   ef->battery_prev_battery = 1;
   stat->has_battery = 1;

   /*
    * Retrieve the current capacity key.
    */
   values = CFDictionaryGetValue(device_dict, CFSTR(kIOPSCurrentCapacityKey));
   CFNumberGetValue(values, kCFNumberSInt32Type, &currentval);
   // CFRelease(values);

   /*
    * Retrieve the max capacity key.
    */
   values = CFDictionaryGetValue(device_dict, CFSTR(kIOPSMaxCapacityKey));
   CFNumberGetValue(values, kCFNumberSInt32Type, &maxval);
   // CFRelease(values);

   /*
    * Calculate the percentage charged.
    */
   stat->level = (double)currentval / (double)maxval;
   printf("Battery charge %g\n", stat->level);

   /*
    * Retrieve the remaining battery power or time until charged in minutes.
    */
   if (stat->state == BATTERY_STATE_DISCHARGING)
     {
	values = CFDictionaryGetValue(device_dict, CFSTR(kIOPSTimeToEmptyKey));
	CFNumberGetValue(values, kCFNumberSInt32Type, &currentval);
	// CFRelease(values);

	/*
	 * Display remaining battery percentage.
	 */
	snprintf(buf, sizeof(buf), "%i%%", (int)(stat->level * 100));
	stat->reading = strdup(buf);

	hours = currentval / 60;
	minutes = currentval % 60;

	/*
	 * Check if an alarm should be raised.
	 */
	if (currentval <= ef->conf->alarm)
	  stat->alarm = 1;
     }
   else
     {
	values = CFDictionaryGetValue(device_dict, CFSTR(kIOPSTimeToFullChargeKey));
	CFNumberGetValue(values, kCFNumberSInt32Type, &currentval);
	// CFRelease(values);

	stat->reading = strdup(_("Charging"));

	hours = currentval / 60;
	minutes = currentval % 60;
     }

   /*
    * Store the time remaining.
    */
   if (minutes >= 0)
     {
	snprintf(buf, sizeof(buf), "%i:%02i", hours, minutes);
	stat->time = strdup(buf);
     }
   else
     stat->time = strdup("--:--");

   CFRelease(sources);
   CFRelease(blob);

   return stat;
}
#endif

static void
_battery_face_level_set(Battery_Face *ef, double level)
{
   Edje_Message_Float msg;

   if (level < 0.0) level = 0.0;
   else if (level > 1.0) level = 1.0;
   msg.val = level;
   edje_object_message_send(ef->bat_object, EDJE_MESSAGE_FLOAT, 1, &msg);
}

static void
_battery_face_cb_menu_enabled(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Battery_Face *face;
   unsigned char enabled;

   face = data;
   enabled = e_menu_item_toggle_get(mi);
   if ((face->conf->enabled) && (!enabled))
     {  
	_battery_face_disable(face);
     }
   else if ((!face->conf->enabled) && (enabled))
     { 
	_battery_face_enable(face);
     }
}

static void
_battery_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Battery_Face *face;

   face = data;
   e_gadman_mode_set(face->gmc->gadman, E_GADMAN_MODE_EDIT);
}
