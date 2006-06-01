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

/***************************************************************************/
/**/
/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc);
static char *_gc_label(void);
static Evas_Object *_gc_icon(Evas *evas);
/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
     "battery",
     {
        _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon
     }
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

static void _button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _menu_cb_post(void *data, E_Menu *m);
static Status *_battery_linux_acpi_check(void);
static Status *_battery_linux_apm_check(void);
static Status *_battery_linux_powerbook_check(void);
#ifdef __FreeBSD__
static Status *_battery_bsd_acpi_check(void);
static Status *_battery_bsd_apm_check(void);
#endif
#ifdef HAVE_CFBASE_H
static Status *_battery_darwin_check(void);
#endif
static void _battery_face_level_set(Instance *inst, double level);
static int _battery_int_get(char *buf);
static char *_battery_string_get(char *buf);
static int _battery_cb_check(void *data);
static void _battery_face_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi);

static E_Config_DD *conf_edd = NULL;

Config *battery_config = NULL;

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;
   
   inst = E_NEW(Instance, 1);
   
   o = edje_object_add(gc->evas);
   e_theme_edje_object_set(o, "base/theme/modules/battery",
			   "modules/battery/main");
   
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;
   
   inst->gcc = gcc;
   inst->o_battery = o;   
   
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  _button_cb_mouse_down, inst);
   battery_config->instances = evas_list_append(battery_config->instances, inst);
   battery_config->battery_check_mode = CHECK_NONE;
   battery_config->battery_prev_drain = 1;
   battery_config->battery_prev_ac = -1;
   battery_config->battery_prev_battery = -1;
   _battery_cb_check(NULL);
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
   
   inst = gcc->data;
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
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
   snprintf(buf, sizeof(buf), "%s/module.eap",
	    e_module_dir_get(battery_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
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
	evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
				 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
}

static void
_menu_cb_post(void *data, E_Menu *m)
{
   if (!battery_config->menu) return;
   e_object_del(E_OBJECT(battery_config->menu));
   battery_config->menu = NULL;
}

static int
_battery_cb_check(void *data)
{
   Instance *inst;
   Status *ret = NULL;
   Evas_List *l;
#ifdef __FreeBSD__
   int acline;
   size_t len;
   int apm_fd = -1;
   int acline_mib[3] = {-1};
#endif

#ifdef __FreeBSD__
   if (battery_config->battery_check_mode == 0)
     {
      	len = sizeof(acline);
      	if (sysctlbyname("hw.acpi.acline", &acline, &len, NULL, 0) == 0) 
      	  {
	     len = 3;
	     if (sysctlnametomib("hw.acpi.acline", acline_mib, &len) == 0)
      	        battery_config->battery_check_mode = CHECK_ACPI; 
	  }
	else
	  {
	     apm_fd = open("/dev/apm", O_RDONLY); 
	     if (apm_fd != -1)
	       battery_config->battery_check_mode = CHECK_APM;
	  }
     }
   switch (battery_config->battery_check_mode)
     {
      case CHECK_ACPI:
	ret = _battery_bsd_acpi_check();
	break;
      case CHECK_APM:
	ret = _battery_bsd_apm_check();
	break;
      default:
	break;
     }
#elif defined(HAVE_CFBASE_H) /* OS X */
   ret = _battery_darwin_check();
#else
   if (battery_config->battery_check_mode == 0)
     {
	if (ecore_file_is_dir("/proc/acpi"))
	  battery_config->battery_check_mode = CHECK_ACPI;
	else if (ecore_file_exists("/proc/apm"))
	  battery_config->battery_check_mode = CHECK_APM;
	else if (ecore_file_is_dir("/proc/pmu"))
	  battery_config->battery_check_mode = CHECK_PMU;
     }
   switch (battery_config->battery_check_mode)
     {
      case CHECK_ACPI:
	ret = _battery_linux_acpi_check();
	break;
      case CHECK_APM:
	ret = _battery_linux_apm_check();
	break;
      case CHECK_PMU:
	ret = _battery_linux_powerbook_check();
	break;
      default:
	break;
     }
#endif
   for (l = battery_config->instances; l; l = l->next)
     {
	Instance *inst;
	
	inst = l->data;
	if (ret)
	  {
	     if (ret->has_battery)
	       {
		  if (ret->state == BATTERY_STATE_CHARGING)
		    {
		       if (battery_config->battery_prev_ac != 1)
			 edje_object_signal_emit(inst->o_battery, "charge", "");
		       edje_object_signal_emit(inst->o_battery, "pulsestop", "");
		       edje_object_part_text_set(inst->o_battery, "reading", ret->reading);
		       edje_object_part_text_set(inst->o_battery, "time", ret->time);
		       _battery_face_level_set(inst, ret->level);
		       battery_config->battery_prev_ac = 1;
		    }
		  else if (ret->state == BATTERY_STATE_DISCHARGING)
		    {
		       if (battery_config->battery_prev_ac != 0)
			 edje_object_signal_emit(inst->o_battery, "discharge", "");
		       if (ret->alarm)
			 {
			    if (!battery_config->alarm_triggered)
			      {
				 E_Dialog *dia;
				 
				 dia = e_dialog_new(e_container_current_get(e_manager_current_get()));
				 if (!dia) return 0;
				 e_dialog_title_set(dia, "Enlightenment Battery Module");
				 e_dialog_icon_set(dia, "enlightenment/e", 64);
				 e_dialog_text_set(dia,
						   _("Battery Running Low<br>"
						     "Your battery is running low.<br>"
						     "You may wish to switch to an AC source."));
				 e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
				 e_win_centered_set(dia->win, 1);
				 e_dialog_show(dia);
			      }
			    edje_object_signal_emit(inst->o_battery, "pulse", "");
			 }
		       edje_object_part_text_set(inst->o_battery, "reading", ret->reading);
		       edje_object_part_text_set(inst->o_battery, "time", ret->time);
		       _battery_face_level_set(inst, ret->level);
		       battery_config->battery_prev_ac = 0;
		       if (ret->alarm)
			 battery_config->alarm_triggered = 1;
		    }
		  else
		    {
		       /* ret->state == BATTERY_STATE_NONE */
		       if (battery_config->battery_prev_ac != 1)
			 edje_object_signal_emit(inst->o_battery, "charge", "");
		       if (battery_config->battery_prev_battery == 0)
			 edje_object_signal_emit(inst->o_battery, "charge", "");
		       edje_object_part_text_set(inst->o_battery, "reading", ret->reading);
		       edje_object_part_text_set(inst->o_battery, "time", ret->time);
		       _battery_face_level_set(inst, ret->level);
		       battery_config->battery_prev_ac = 1;
		       battery_config->battery_prev_battery = 1;
		    }
	       }
	     else
	       {
		  /* Hasn't battery */
		  if (battery_config->battery_prev_battery != 0)
		    edje_object_signal_emit(inst->o_battery, "unknown", "");
		  edje_object_part_text_set(inst->o_battery, "reading", ret->reading);
		  edje_object_part_text_set(inst->o_battery, "time", ret->time);
		  _battery_face_level_set(inst, ret->level);
		  battery_config->battery_prev_battery = 0;
	       }
	     free(ret->reading);
	     free(ret->time);
	     free(ret);
	  }
	else
	  {
	     /* Error reading status */
	     if (battery_config->battery_prev_battery != -2)
	       edje_object_signal_emit(inst->o_battery, "unknown", "");
	     edje_object_part_text_set(inst->o_battery, "reading", _("NO INFO"));
	     edje_object_part_text_set(inst->o_battery, "time", "--:--");
	     _battery_face_level_set(inst, (double)(rand() & 0xff) / 255.0);
	     battery_config->battery_prev_battery = -2;
	     battery_config->battery_check_mode = CHECK_NONE;
	  }
     }
   return 1;
}

static Status *
_battery_linux_acpi_check(void)
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
   if (!stat) return NULL;

   /* Read some information on first run. */
   bats = ecore_file_ls("/proc/acpi/battery");
   if (bats)
     {
	while ((name = ecore_list_next(bats)))
	  {
	     FILE *f;
	     char *tmp;

	     snprintf(buf, sizeof(buf), "/proc/acpi/battery/%s/info", name);
	     f = fopen(buf, "r");
	     if (f)
	       {
		  int design_cap = 0;
		  int last_full = 0;

		  /* present */
		  fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
		  /* design capacity */
		  fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
		  tmp = _battery_string_get(buf2);
		  if (tmp)
		    {
		       if (!strcmp(tmp, "unknown")) design_cap_unknown = 1;
		       else design_cap = atoi(tmp);
		       free(tmp);
		    }
		  /* last full capacity */
		  fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
		  tmp = _battery_string_get(buf2);
		  if (tmp)
		    {
		       if (!strcmp(tmp, "unknown")) last_full_unknown = 1;
		       else last_full = atoi(tmp);
		       free(tmp);
		    }
		  fclose(f);
		  bat_max += design_cap;
		  bat_filled += last_full;
	       }
	     snprintf(buf, sizeof(buf), "/proc/acpi/battery/%s/state", name);
	     f = fopen(buf, "r");
	     if (f)
	       {
		  char *present;
		  char *capacity_state;
		  char *charging_state;
		  char *tmp;
		  int rate = 1;
		  int level = 0;

		  /* present */
		  fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
		  present = _battery_string_get(buf2);
		  /* capacity state */
		  fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
		  capacity_state = _battery_string_get(buf2);
		  /* charging state */
		  fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
		  charging_state = _battery_string_get(buf2);
		  /* present rate */
		  fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
		  tmp = _battery_string_get(buf2);
		  if (tmp)
		    {
		       if (!strcmp(tmp, "unknown")) rate_unknown = 1;
		       else rate = atoi(tmp);
		       free(tmp);
		    }
		  /* remaining capacity */
		  fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
		  tmp = _battery_string_get(buf2);
		  if (tmp)
		    {
		       if (!strcmp(tmp, "unknown")) level_unknown = 1;
		       else level = atoi(tmp);
		       free(tmp);
		    }
		  fclose(f);
		  if (present)
		    {
		       if (!strcmp(present, "yes")) battery++;
		       free(present);
		    }
		  if (charging_state)
		    {
		       if (!strcmp(charging_state, "discharging"))
			 {
			    discharging++;
			    if ((rate == 0) && (rate_unknown == 0)) rate_unknown = 1;
			 }
		       else if (!strcmp(charging_state, "charging"))
			 {
			    charging++;
			    if ((rate == 0) && (rate_unknown == 0)) rate_unknown = 1;
			 }
		       else if (!strcmp(charging_state, "charged"))
			 rate_unknown = 0;
		       free(charging_state);
		    }
		  E_FREE(capacity_state);

		  bat_drain += rate;
		  bat_level += level;
	       }
	  }
	ecore_list_destroy(bats);
     }

   if (battery_config->battery_prev_drain < 1) battery_config->battery_prev_drain = 1;
   if (bat_drain < 1) bat_drain = battery_config->battery_prev_drain;
   battery_config->battery_prev_drain = bat_drain;

   if (bat_filled > 0) bat_val = (100 * bat_level) / bat_filled;
   else bat_val = 100;
   
   if (discharging) minutes = (60 * bat_level) / bat_drain;
   else
     {
	/* FIXME: Batteries charge in paralell! */
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
	battery_config->battery_prev_battery = 1;
	stat->has_battery = 1;
        if (charging)
          {
	     stat->state = BATTERY_STATE_CHARGING;
	     battery_config->alarm_triggered = 0;
          }
	else if (discharging)
	  {
	     stat->state = BATTERY_STATE_DISCHARGING;
	     if (stat->level < 0.1)
	       {
		  if (((hours * 60) + minutes) <= battery_config->alarm)
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
_battery_linux_apm_check(void)
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
   if (!stat) return NULL;

   if ((bat_flags != 0xff) && (bat_flags & 0x80))
     {
	stat->has_battery = 0;
	stat->state = BATTERY_STATE_NONE;
	stat->reading = strdup("NO BAT");
	stat->time = strdup("--:--");
	stat->level = 1.0;
	return stat;
     }
   
   
   battery_config->battery_prev_battery = 1;
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
	     if (((hours * 60) + minutes) <= battery_config->alarm)
	       stat->alarm = 1;
	  }
     }

   return stat;
}


/* hack for pmu */

/* This function converts a string to an integer. Additionally to
 * atoi() it converts also hexadecimal values
 */
static int
axtoi(char *arg)
{
   int n, val, pwr=1, m, rc = 0;
   char hex[9], c;
   
   for (n = 0, m = 0; n < strlen(arg); n++)
     {
	if (arg[n] != ' ')
	  {
	     hex[m++] = c = toupper(arg[n]);
	     if ((m == sizeof(hex)) || (c < '0') || (c > 'F'))
	       return 0;   /* overflow or invalid */
	  }
     }
   hex[m] = '\0';  /* terminate string */
   
   for (n = 0; n < m; n++)
     {
	c = hex[m-n-1];
	if ((c >= 'A') && (c <= 'F'))
	  val = c -'A' + 10;
	else
	  val = c - '0';
	rc = rc + val * pwr;
	pwr *= 16;
     }
   return rc;
}

static Status *
_battery_linux_powerbook_check(void)
{
   Ecore_List *bats;
   char buf[4096], buf2[4096];
   char *name;
   char *token;
   FILE *f;
   int discharging = 0;
   int charging = 0;
   int battery = 0;
   int ac = 0;
   int seconds = 0;
   int hours, minutes;
   int flags;
   int voltage;
   int charge;
   int max_charge;
   double tmp;
   Status *stat;

   stat = E_NEW(Status, 1);
   if (!stat) return NULL;

   /* Read some information. */
   f = fopen("/proc/pmu/info", "r");
   if (f)
     {
	/* Skip driver */
	fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
	/* Skip firmware */
	fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
	/* Read ac */
	fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
	ac = _battery_int_get(buf2);
	fclose(f);
     }

   bats = ecore_file_ls("/proc/pmu");
   if (bats)
     {
	while ((name = ecore_list_next(bats)))
	  {
	     if (strncmp(name, "battery", 7))
	       continue;

	     snprintf(buf, sizeof(buf), "/proc/pmu/%s", name);
	     f = fopen(buf, "r");
	     if (f)
	       {
		  int time = 0;
		  int current = 0;

		  while (fgets (buf,sizeof (buf), f))
		    {
		       if ((token = strtok (buf, ":")))
			 {
			    if (!strncmp ("flags", token, 5))
			      flags = axtoi (strtok (0, ": "));
			    else if (!strncmp ("charge", token, 6)) 
			      charge = atoi(strtok(0, ": "));
			    else if (!strncmp ("max_charge", token, 9))
			      max_charge = atoi (strtok(0,": "));
			    else if (!strncmp ("current", token, 7))
			      current = atoi (strtok(0, ": "));
			    else if (!strncmp ("time rem", token, 8))
			      time = atoi (strtok(0, ": "));
			    else if (!strncmp ("voltage", token, 7))
			      voltage = atoi (strtok(0,": "));
			    else
			      strtok (0,": ");
			 }
		    }
		  /* Skip flag;
		  int tmp = 0;		  
		  fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
		  fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
		  tmp = _battery_int_get(buf2);
		  charge += tmp;
		  fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
		  tmp = _battery_int_get(buf2);
		  max_charge += tmp;
		  fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
		  current = _battery_int_get(buf2);
		  fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
		  fgets(buf2, sizeof(buf2), f); buf2[sizeof(buf2) - 1] = 0;
		  time = _battery_int_get(buf2);
		   */
		  fclose(f);

		  battery = 1;
		  if (!current)
		    {
		       /* Neither charging nor discharging */
		    }
		  else if (!ac)
		    {
		       /* When on dc, we are discharging */
		       discharging = 1;
		       seconds += time;
		    }
		  else
		    {
		       /* Charging */
		       charging = 1;
		       /* Charging works in paralell */
		       seconds = MAX(time, seconds);
		    }

	       }
	  }
	ecore_list_destroy(bats);
     }
   hours = seconds / (60 * 60);
   seconds -= hours * (60 * 60);
   minutes = seconds / 60;
   seconds -= minutes * 60;

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
	stat->has_battery = 1;
        if (charging)
          {
	     stat->state = BATTERY_STATE_CHARGING;
	     battery_config->alarm_triggered = 0;
          }
	else if (discharging)
	  {
	     stat->state = BATTERY_STATE_DISCHARGING;
	     if (stat->level < 0.1)
	       {
		  if (((hours * 60) + minutes) <= battery_config->alarm)
		    stat->alarm = 1;
	       }
	  }
	stat->level = (double)charge / (double)max_charge;
       if (stat->level > 1.0) stat->level = 1.0;
	tmp = (double)max_charge / 100;
	tmp = (double)charge / tmp;
	stat->level = (double)tmp / 100;

	snprintf(buf, sizeof(buf), "%.0f%%", tmp);
	stat->reading = strdup(buf);
	snprintf(buf, sizeof(buf), "%i:%02i", hours, minutes);
	stat->time = strdup(buf);
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

#ifdef __FreeBSD__
static Status *
_battery_bsd_acpi_check(void)
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
   if (!stat) return NULL;

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
   
   if (battery_config->battery_prev_drain < 1)  
     battery_config->battery_prev_drain = 1;
   
   if (bat_drain < 1) 
     bat_drain = battery_config->battery_prev_drain;
   
   battery_config->battery_prev_drain = bat_drain;

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
	battery_config->battery_prev_battery = 1;
	stat->has_battery = 1;
        if (state == BATTERY_STATE_CHARGING)
          {
	     stat->state = BATTERY_STATE_CHARGING;
	     battery_config->alarm_triggered = 0;
          }
	else if (state == BATTERY_STATE_DISCHARGING)
	  {
	     stat->state = BATTERY_STATE_DISCHARGING;
	     if (stat->level < 0.1) /* Why this if condition */
	       {
		  if (((hours * 60) + minutes) <= battery_config->alarm)
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
_battery_bsd_apm_check(void)
{
#ifdef __i386__
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
   if (!stat) return NULL;

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
   
   
   battery_config->battery_prev_battery = 1;
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
	     if (((hours * 60) + minutes) <= battery_config->alarm)
	       stat->alarm = 1;
	  }
     }
   
   return stat;
#else
   return NULL;
#endif
}
#endif

#ifdef HAVE_CFBASE_H
/*
 * There is a good chance this will work with a UPS as well as a battery.
 */
static Status *
_battery_darwin_check(void)
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
   if (!stat) return NULL;

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
	     /* CFRelease(values); */
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
   battery_config->battery_prev_battery = 1;
   stat->has_battery = 1;

   /*
    * Retrieve the current capacity key.
    */
   values = CFDictionaryGetValue(device_dict, CFSTR(kIOPSCurrentCapacityKey));
   CFNumberGetValue(values, kCFNumberSInt32Type, &currentval);
   /* CFRelease(values); */

   /*
    * Retrieve the max capacity key.
    */
   values = CFDictionaryGetValue(device_dict, CFSTR(kIOPSMaxCapacityKey));
   CFNumberGetValue(values, kCFNumberSInt32Type, &maxval);
   /* CFRelease(values); */

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
	/* CFRelease(values); */

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
	if (currentval <= battery_config->alarm)
	  stat->alarm = 1;
     }
   else
     {
	values = CFDictionaryGetValue(device_dict, CFSTR(kIOPSTimeToFullChargeKey));
	CFNumberGetValue(values, kCFNumberSInt32Type, &currentval);
	/* CFRelease(values); */

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
_battery_face_level_set(Instance *inst, double level)
{
   Edje_Message_Float msg;

   if (level < 0.0) level = 0.0;
   else if (level > 1.0) level = 1.0;
   msg.val = level;
   edje_object_message_send(inst->o_battery, EDJE_MESSAGE_FLOAT, 1, &msg);
}

static int
_battery_int_get(char *buf)
{
   char *p, *q;

   p = strchr(buf, ':');
   if (!p) return 0;
   p++;
   while (*p == ' ') p++;
   q = p;
   while ((*q != ' ') && (*q != '\n')) q++;
   if (q) *q = 0;
   return atoi(p);
}

static char *
_battery_string_get(char *buf)
{
   char *p, *q;

   p = strchr(buf, ':');
   if (!p) return NULL;
   p++;
   while (*p == ' ') p++;
   q = p;
   while ((*q != ' ') && (*q != '\n')) q++;
   if (q) *q = 0;
   return strdup(p);
}

static void 
_battery_face_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   if (!battery_config) return;
   if (battery_config->config_dialog) return;
   _config_battery_module();
}

void
_battery_config_updated(void)
{
   if (!battery_config) return;
   ecore_timer_del(battery_config->battery_check_timer);
   battery_config->battery_check_timer = ecore_timer_add(battery_config->poll_time,
						 _battery_cb_check, NULL);
   _battery_cb_check(NULL);
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
   conf_edd = E_CONFIG_DD_NEW("Battery_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, poll_time, DOUBLE);
   E_CONFIG_VAL(D, T, alarm, INT);

   battery_config = e_config_domain_load("module.battery", conf_edd);
   if (!battery_config)
     {
       battery_config = E_NEW(Config, 1);
       battery_config->poll_time = 30.0;
       battery_config->alarm = 30;
     }
   E_CONFIG_LIMIT(battery_config->poll_time, 0.5, 1000.0);
   E_CONFIG_LIMIT(battery_config->alarm, 0, 60);
   
   battery_config->battery_check_mode = CHECK_NONE;
   battery_config->battery_prev_drain = 1;
   battery_config->battery_prev_ac = -1;
   battery_config->battery_prev_battery = -1;
   battery_config->battery_check_timer = ecore_timer_add(battery_config->poll_time,
						 _battery_cb_check, NULL);
   battery_config->module = m;
   
   e_gadcon_provider_register(&_gadcon_class);
   return 1;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   e_gadcon_provider_unregister(&_gadcon_class);
   
   if (battery_config->config_dialog)
     e_object_del(E_OBJECT(battery_config->config_dialog));
   if (battery_config->battery_check_timer)
     ecore_timer_del(battery_config->battery_check_timer);
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

EAPI int
e_modapi_about(E_Module *m)
{
   e_module_dialog_show(_("Enlightenment Battery Module"),
		       _("A basic battery meter that uses either"
		 	 "<hilight>ACPI</hilight> or <hilight>APM</hilight><br>"
			 "on Linux to monitor your battery and AC power adaptor<br>"
			 "status. This will work under Linux and FreeBSD and is only<br>"
			 "as accurate as your BIOS or kernel drivers."));
   return 1;
}

EAPI int
e_modapi_config(E_Module *m)
{
   if (!battery_config) return 0;
   if (battery_config->config_dialog) return 0;
   _config_battery_module();
   return 1;
}
/**/
/***************************************************************************/
