#include "e.h"
#include "e_mod_main.h"
#ifdef HAVE_ENOTIFY
#include "E_Notify.h"
#endif

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void             _gc_shutdown(E_Gadcon_Client *gcc);
static void             _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char      *_gc_label(const E_Gadcon_Client_Class *client_class);
static Evas_Object     *_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas);
static const char      *_gc_id_new(const E_Gadcon_Client_Class *client_class);

Eina_List *device_batteries;
Eina_List *device_ac_adapters;
double init_time;
double current_power;
int current_time_left;
Eina_Bool bat_charging;
Eina_Bool mouse_down = EINA_FALSE;

/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
   "battery",
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, NULL
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

/* actual module specifics */
typedef struct _Instance Instance;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_battery;
   Evas_Object     *popup_battery;
   E_Gadcon_Popup  *warning;
};

static void      _battery_update(int full, int time_left, int time_full, Eina_Bool have_battery, Eina_Bool have_power);
static Eina_Bool _battery_cb_exe_data(void *data, int type, void *event);
static Eina_Bool _battery_cb_exe_del(void *data, int type, void *event);
static void      _button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void      _battery_face_level_set(Evas_Object *battery, double level);
static void      _battery_face_time_set(Evas_Object *battery, int time);
static void      _battery_face_cb_menu_powermanagement(void *data, E_Menu *m, E_Menu_Item *mi);
static void      _battery_face_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi);

static Eina_Bool _battery_cb_warning_popup_timeout(void *data);
static void      _battery_cb_warning_popup_hide(void *data, Evas *e, Evas_Object *obj, void *event);
static void      _battery_warning_popup_destroy(Instance *inst);
static void      _battery_warning_popup(Instance *inst, int time, double percent, int warn);

static Eina_Bool _powersave_cb_config_update(void *data, int type, void *event);

static E_Config_DD *conf_edd = NULL;
static Ecore_Event_Handler *_handler = NULL;

Config *battery_config = NULL;

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;

   battery_config->full = -2;
   battery_config->time_left = -2;
   battery_config->time_full = -2;
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
   inst->warning = NULL;
   inst->popup_battery = NULL;

#ifdef HAVE_EEZE
   eeze_init();
#elif !defined __OpenBSD__
   e_dbus_init();
   e_hal_init();
#endif

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
                                  _button_cb_mouse_down, inst);
   battery_config->instances =
     eina_list_append(battery_config->instances, inst);
   _battery_config_updated();

   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

#ifdef HAVE_EEZE
   eeze_shutdown();
#elif !defined __OpenBSD__
   e_hal_shutdown();
   e_dbus_shutdown();
#endif

   inst = gcc->data;
   if (battery_config)
     battery_config->instances =
       eina_list_remove(battery_config->instances, inst);
   evas_object_del(inst->o_battery);
   if (inst->warning)
     {
        e_object_del(E_OBJECT(inst->warning));
        inst->popup_battery = NULL;
     }
   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   Instance *inst;
   Evas_Coord mw, mh, mxw, mxh;

   inst = gcc->data;
   mw = 0, mh = 0;
   edje_object_size_min_get(inst->o_battery, &mw, &mh);
   edje_object_size_max_get(inst->o_battery, &mxw, &mxh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(inst->o_battery, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   if ((mxw > 0) && (mxh > 0))
     e_gadcon_client_aspect_set(gcc, mxw, mxh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("Battery");
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
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
_gc_id_new(const E_Gadcon_Client_Class *client_class)
{
   static char buf[4096];

   snprintf(buf, sizeof(buf), "%s.%d", client_class->name,
            eina_list_count(battery_config->instances) + 1);
   return buf;
}

static void
_button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Instance *inst;
   Evas_Event_Mouse_Down *ev;

   inst = data;
   ev = event_info;
   if ((ev->button == 1) && (!mouse_down))
     _battery_warning_popup(inst, current_time_left, current_power, 0);
   else
     _battery_cb_warning_popup_hide(data, e, obj, event_info);

   if (ev->button == 3)
     {
        E_Menu *m;
        E_Menu_Item *mi;
        int cx, cy;

        m = e_menu_new();
        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, _("Settings"));
        e_util_menu_item_theme_icon_set(mi, "configure");
        e_menu_item_callback_set(mi, _battery_face_cb_menu_configure, NULL);
        if (e_configure_registry_exists("advanced/powermanagement"))
          {
             mi = e_menu_item_new(m);
             e_menu_item_label_set(mi, _("Power Management Timing"));
             e_util_menu_item_theme_icon_set(mi, "preferences-system-power-management");
             e_menu_item_callback_set(mi, _battery_face_cb_menu_powermanagement, NULL);
          }

        m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);

        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon,
                                          &cx, &cy, NULL, NULL);
        e_menu_activate_mouse(m,
                              e_util_zone_current_get(e_manager_current_get()),
                              cx + ev->output.x, cy + ev->output.y, 1, 1,
                              E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
        evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                                 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
}

static void
_battery_face_level_set(Evas_Object *battery, double level)
{
   Edje_Message_Float msg;
   char buf[256];

   snprintf(buf, sizeof(buf), "%i%%", (int)(level * 100.0));
   edje_object_part_text_set(battery, "e.text.reading", buf);

   if (level < 0.0) level = 0.0;
   else if (level > 1.0)
     level = 1.0;
   msg.val = level;
   edje_object_message_send(battery, EDJE_MESSAGE_FLOAT, 1, &msg);
}

static void
_battery_face_time_set(Evas_Object *battery, int t)
{
   char buf[256];
   int hrs, mins;

   if (t < 0) return;

   hrs = (t / 3600);
   mins = ((t) / 60 - (hrs * 60));
   if (mins < 0) mins = 0;
   snprintf(buf, sizeof(buf), "%i:%02i", hrs, mins);
   edje_object_part_text_set(battery, "e.text.time", buf);
}

static void
_battery_face_cb_menu_powermanagement(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   e_configure_registry_call("advanced/powermanagement", m->zone->container, NULL);
}

static void
_battery_face_cb_menu_configure(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   if (!battery_config) return;
   if (battery_config->config_dialog) return;
   e_int_config_battery_module(m->zone->container, NULL);
}

Battery *
_battery_battery_find(const char *udi)
{
   Eina_List *l;
   Battery *bat;
   EINA_LIST_FOREACH(device_batteries, l, bat)
     { /* these are always stringshared */
       if (udi == bat->udi) return bat;
     }

   return NULL;
}

Ac_Adapter *
_battery_ac_adapter_find(const char *udi)
{
   Eina_List *l;
   Ac_Adapter *ac;
   EINA_LIST_FOREACH(device_ac_adapters, l, ac)
     { /* these are always stringshared */
       if (udi == ac->udi) return ac;
     }

   return NULL;
}

void
_battery_device_update(void)
{
   Eina_List *l;
   Battery *bat;
   Ac_Adapter *ac;
   int full = -1;
   int time_left = -1;
   int time_full = -1;
   int have_battery = 0;
   int have_power = 0;
   int charging = 0;

   int batnum = 0;
   int acnum = 0;

   EINA_LIST_FOREACH(device_ac_adapters, l, ac)
     if (ac->present) acnum++;

   EINA_LIST_FOREACH(device_batteries, l, bat)
     {
        if ((!bat->got_prop) || (!bat->technology))
          continue;
        have_battery = 1;
        batnum++;
        if (bat->charging == 1) have_power = 1;
        if (full == -1) full = 0;
        if (bat->percent >= 0)
          full += bat->percent;
        else if (bat->last_full_charge > 0)
          full += (bat->current_charge * 100) / bat->last_full_charge;
        else if (bat->design_charge > 0)
          full += (bat->current_charge * 100) / bat->design_charge;
        if (bat->time_left > 0)
          {
             if (time_left < 0) time_left = bat->time_left;
             else time_left += bat->time_left;
          }
        if (bat->time_full > 0)
          {
             if (time_full < 0) time_full = bat->time_full;
             else time_full += bat->time_full;
          }
        charging += bat->charging;
     }

   if ((device_batteries) && (batnum == 0))
     return;  /* not ready yet, no properties received for any battery */

   if (batnum > 0) full /= batnum;
   if ((full == 100) && have_power)
     {
        time_left = -1;
        time_full = -1;
     }
   if (time_left < 1) time_left = -1;
   if (time_full < 1) time_full = -1;

   _battery_update(full, time_left, time_full, have_battery, have_power);
}

void
_battery_config_updated(void)
{
   Eina_List *l;
   Instance *inst;
   char buf[4096];
   int ok = 0;

   if (!battery_config) return;

   if (battery_config->instances)
     {
        EINA_LIST_FOREACH(battery_config->instances, l, inst)
          _battery_warning_popup_destroy(inst);
     }
   if (battery_config->batget_exe)
     {
        ecore_exe_terminate(battery_config->batget_exe);
        ecore_exe_free(battery_config->batget_exe);
        battery_config->batget_exe = NULL;
     }

   if ((battery_config->force_mode == UNKNOWN) ||
       (battery_config->force_mode == SUBSYSTEM))
     {
#ifdef HAVE_EEZE
        ok = _battery_udev_start();
#elif defined __OpenBSD__
        ok = _battery_openbsd_start();
#else
        ok = _battery_dbus_start();
#endif
     }
   if (ok) return;

   if ((battery_config->force_mode == UNKNOWN) ||
       (battery_config->force_mode == NOSUBSYSTEM))
     {
        snprintf(buf, sizeof(buf), "%s/%s/batget %i",
                 e_module_dir_get(battery_config->module), MODULE_ARCH,
                 battery_config->poll_interval);

        battery_config->batget_exe =
          ecore_exe_pipe_run(buf, ECORE_EXE_PIPE_READ |
                             ECORE_EXE_PIPE_READ_LINE_BUFFERED |
                             ECORE_EXE_NOT_LEADER, NULL);
     }
}

static Eina_Bool
_battery_cb_warning_popup_timeout(void *data)
{
   Instance *inst;

   inst = data;
   e_gadcon_popup_hide(inst->warning);
   battery_config->alert_timer = NULL;
   mouse_down = EINA_FALSE;
   return ECORE_CALLBACK_DONE;
}

static void
_battery_cb_warning_popup_hide(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
   Instance *inst = NULL;

   inst = (Instance *)data;
   mouse_down = EINA_FALSE;
   if ((!inst) || (!inst->warning)) return;
   e_gadcon_popup_hide(inst->warning);
}

static void
_battery_warning_popup_destroy(Instance *inst)
{
   if (battery_config->alert_timer)
     {
        ecore_timer_del(battery_config->alert_timer);
        battery_config->alert_timer = NULL;
     }
   if ((!inst) || (!inst->warning)) return;
   e_object_del(E_OBJECT(inst->warning));
   inst->warning = NULL;
   inst->popup_battery = NULL;
}

static void
_battery_warning_popup(Instance *inst, int t, double percent, int warn)
{
   Evas *e = NULL;
   Evas_Object *rect = NULL, *popup_bg = NULL;;
   int x, y, w, h;

   if (warn)       //warn 1 = warning, warn 0 = actual state
     {
       if ((!inst) || (inst->warning)) return;
     }
   else
     {
       if ((!inst)) return;
     }

   mouse_down = EINA_TRUE;
   if (warn)
     {
       #ifdef HAVE_ENOTIFY
       static E_Notification *notification;

       if (battery_config->desktop_notifications)
         {
           if (notification) return;
           notification = e_notification_full_new
           (
              _("Battery"),
              0,
              "battery-low",
              _("Your battery is low!"),
              _("AC power is recommended."),
              (battery_config->alert_timeout * 1000)
           );
           e_notification_send(notification, NULL, NULL);
           e_notification_unref(notification);
           notification = NULL;
           return;
         }
       #endif
      }

   inst->warning = e_gadcon_popup_new(inst->gcc);
   if (!inst->warning) return;

   e = inst->warning->win->evas;

   popup_bg = edje_object_add(e);
   inst->popup_battery = edje_object_add(e);

   if ((!popup_bg) || (!inst->popup_battery))
     {
        e_object_free(E_OBJECT(inst->warning));
        inst->warning = NULL;
        return;
     }

   e_theme_edje_object_set(popup_bg, "base/theme/modules/battery/popup",
                           "e/modules/battery/popup");
   e_theme_edje_object_set(inst->popup_battery, "base/theme/modules/battery",
                           "e/modules/battery/main");
   if (edje_object_part_exists(popup_bg, "e.swallow.battery"))
    {
       edje_object_part_swallow(popup_bg, "e.swallow.battery", inst->popup_battery);
       if (bat_charging)
         edje_object_signal_emit(popup_bg, "e,state,charging", "e");
       else
         edje_object_signal_emit(popup_bg, "e,state,discharging", "e");
     }
   else
     edje_object_part_swallow(popup_bg, "battery", inst->popup_battery);

   if (warn)
     {
       edje_object_part_text_set(popup_bg, "e.text.title",
                                 _("Your battery is low!"));
       edje_object_part_text_set(popup_bg, "e.text.label",
                             _("AC power is recommended."));
     }
   else
     {
       char buf[64] = "";
       snprintf(buf, sizeof(buf), "%s%s", _("Power now: "),
            edje_object_part_text_get(inst->o_battery, "e.text.reading"));
       edje_object_part_text_set(popup_bg, "e.text.title", buf);
      
       if (edje_object_part_text_get(inst->o_battery, "e.text.time"))
         {
           snprintf(buf, sizeof(buf), "%s%s %s", _("Remaining time: "),
              edje_object_part_text_get(inst->o_battery, "e.text.time"), _("hr"));
           edje_object_part_text_set(popup_bg, "e.text.label", buf);
         }
       else
         edje_object_part_text_set(popup_bg, "e.text.label", "");
     }

   e_gadcon_popup_content_set(inst->warning, popup_bg);
   e_gadcon_popup_show(inst->warning);

   evas_object_geometry_get(inst->warning->o_bg, &x, &y, &w, &h);

   rect = evas_object_rectangle_add(e);
   if (rect)
     {
        evas_object_move(rect, x, y);
        evas_object_resize(rect, w, h);
        evas_object_color_set(rect, 0, 0, 0, 0);
        evas_object_event_callback_add(rect, EVAS_CALLBACK_MOUSE_DOWN,
                                       _battery_cb_warning_popup_hide, inst);
        evas_object_repeat_events_set(rect, 1);
        evas_object_show(rect);
     }
   
   _battery_face_time_set(inst->popup_battery, t);
   _battery_face_level_set(inst->popup_battery, percent);
   
   if (warn)
     {
       edje_object_signal_emit(inst->popup_battery, "e,state,discharging", "e");

       if ((battery_config->alert_timeout > 0) &&
          (!battery_config->alert_timer))
         {
           battery_config->alert_timer =
           ecore_timer_add(battery_config->alert_timeout,
                          _battery_cb_warning_popup_timeout, inst);
         }
     }
   else
     ecore_timer_add(5.0 ,_battery_cb_warning_popup_timeout, inst);
}

static Eina_Bool
_powersave_cb_config_update(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   if (!battery_config->have_battery)
     e_powersave_mode_set(E_POWERSAVE_MODE_LOW);
   else
     {
        if (battery_config->have_power)
          e_powersave_mode_set(E_POWERSAVE_MODE_LOW);
        else if (battery_config->full > 95)
          e_powersave_mode_set(E_POWERSAVE_MODE_MEDIUM);
        else if (battery_config->full > 30)
          e_powersave_mode_set(E_POWERSAVE_MODE_HIGH);
        else
          e_powersave_mode_set(E_POWERSAVE_MODE_EXTREME);
     }
   return ECORE_CALLBACK_RENEW;
}

static void
_battery_update(int full, int time_left, int time_full, Eina_Bool have_battery, Eina_Bool have_power)
{
   Eina_List *l;
   Instance *inst;
   static double debounce_time = 0.0;

   EINA_LIST_FOREACH(battery_config->instances, l, inst)
     {
        if (have_power != battery_config->have_power)
          {
             if (have_power && (full < 100))
               {
                  edje_object_signal_emit(inst->o_battery,
                                       "e,state,charging",
                                       "e");
                  bat_charging = 1;
               }
             else
               {
                  edje_object_signal_emit(inst->o_battery,
                                          "e,state,discharging",
                                          "e");
                  bat_charging = 0;
                  if (inst->popup_battery)
                    edje_object_signal_emit(inst->popup_battery,
                                            "e,state,discharging", "e");
               }
          }
        if (have_battery)
          {
             if (battery_config->full != full)
               {
                  double val;

                  if (full >= 100) val = 1.0;
                  else val = (double)full / 100.0;
                  _battery_face_level_set(inst->o_battery, val);
                  current_power = val;
                  if (inst->popup_battery)
                    _battery_face_level_set(inst->popup_battery, val);
               }
          }
        else
          {
             _battery_face_level_set(inst->o_battery, 0.0);
             edje_object_part_text_set(inst->o_battery,
                                       "e.text.reading",
                                       _("N/A"));
          }

        if ((time_full < 0) && (time_left != battery_config->time_left))
          {
             _battery_face_time_set(inst->o_battery, time_left);
             current_time_left = time_left;
             if (inst->popup_battery)
               _battery_face_time_set(inst->popup_battery,
                                      time_left);
          }
        else if ((time_left < 0) && (time_full != battery_config->time_full))
          {
             _battery_face_time_set(inst->o_battery, time_full);
             if (inst->popup_battery)
               _battery_face_time_set(inst->popup_battery,
                                      time_full);
          }
        if (have_battery &&
            (!have_power) &&
            (full < 100) &&
            (
              (
                (time_left > 0) &&
                battery_config->alert &&
                ((time_left / 60) <= battery_config->alert)
              ) ||
              (
                battery_config->alert_p &&
                (full <= battery_config->alert_p)
              )
            )
            )
          {
             double t;

             printf("-------------------------------------- bat warn .. why below\n");
             printf("have_battery = %i\n", (int)have_battery);
             printf("have_power = %i\n", (int)have_power);
             printf("full = %i\n", (int)full);
             printf("time_left = %i\n", (int)time_left);
             printf("battery_config->alert = %i\n", (int)battery_config->alert);
             printf("battery_config->alert_p = %i\n", (int)battery_config->alert_p);
             t = ecore_time_get();
             if ((t - debounce_time) > 30.0)
               {
                  printf("t-debounce = %3.3f\n", (t - debounce_time));
                  debounce_time = t;
                  if ((t - init_time) > 5.0)
                    _battery_warning_popup(inst, time_left, (double)full / 100.0, 1);
               }
          }
        else if (have_power || ((time_left / 60) > battery_config->alert))
          _battery_warning_popup_destroy(inst);
        if ((have_battery) && (!have_power) && (full >= 0) &&
            (battery_config->suspend_below > 0) &&
            (full < battery_config->suspend_below))
          {
             if (battery_config->suspend_method == SUSPEND)
               e_sys_action_do(E_SYS_SUSPEND, NULL);
             else if (battery_config->suspend_method == HIBERNATE)
               e_sys_action_do(E_SYS_HIBERNATE, NULL);
             else if (battery_config->suspend_method == SHUTDOWN)
               e_sys_action_do(E_SYS_HALT, NULL);
          }
     }
   if (!have_battery)
     e_powersave_mode_set(E_POWERSAVE_MODE_LOW);
   else
     {
        if (have_power)
          e_powersave_mode_set(E_POWERSAVE_MODE_LOW);
        else if (full > 95)
          e_powersave_mode_set(E_POWERSAVE_MODE_MEDIUM);
        else if (full > 30)
          e_powersave_mode_set(E_POWERSAVE_MODE_HIGH);
        else
          e_powersave_mode_set(E_POWERSAVE_MODE_EXTREME);
     }
   battery_config->full = full;
   battery_config->time_left = time_left;
   battery_config->have_battery = have_battery;
   battery_config->have_power = have_power;
}

static Eina_Bool
_battery_cb_exe_data(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Data *ev;
   Instance *inst;
   Eina_List *l;
   int i;

   ev = event;
   if (ev->exe != battery_config->batget_exe) return ECORE_CALLBACK_PASS_ON;
   if ((ev->lines) && (ev->lines[0].line))
     {
        for (i = 0; ev->lines[i].line; i++)
          {
             if (!strcmp(ev->lines[i].line, "ERROR"))
               EINA_LIST_FOREACH(battery_config->instances, l, inst)
                 {
                    edje_object_signal_emit(inst->o_battery,
                                            "e,state,unknown", "e");
                    edje_object_part_text_set(inst->o_battery,
                                              "e.text.reading", _("ERROR"));
                    edje_object_part_text_set(inst->o_battery,
                                              "e.text.time", _("ERROR"));

                    if (inst->popup_battery)
                      {
                         edje_object_signal_emit(inst->popup_battery,
                                                 "e,state,unknown", "e");
                         edje_object_part_text_set(inst->popup_battery,
                                                   "e.text.reading", _("ERROR"));
                         edje_object_part_text_set(inst->popup_battery,
                                                   "e.text.time", _("ERROR"));
                      }
                 }
             else
               {
                  int full = 0;
                  int time_left = 0;
                  int time_full = 0;
                  int have_battery = 0;
                  int have_power = 0;

                  if (sscanf(ev->lines[i].line, "%i %i %i %i %i", &full, &time_left, &time_full,
                             &have_battery, &have_power) == 5)
                    _battery_update(full, time_left, time_full,
                                    have_battery, have_power);
                  else
                    e_powersave_mode_set(E_POWERSAVE_MODE_LOW);
               }
          }
     }
   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_battery_cb_exe_del(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Del *ev;

   ev = event;
   if (ev->exe != battery_config->batget_exe) return ECORE_CALLBACK_PASS_ON;
   battery_config->batget_exe = NULL;
   return ECORE_CALLBACK_PASS_ON;
}

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION, "Battery"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   //~ char buf[4096];

#ifdef HAVE_ENOTIFY
   e_notification_init();
#endif

   conf_edd = E_CONFIG_DD_NEW("Battery_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, poll_interval, INT);
   E_CONFIG_VAL(D, T, alert, INT);
   E_CONFIG_VAL(D, T, alert_p, INT);
   E_CONFIG_VAL(D, T, alert_timeout, INT);
   E_CONFIG_VAL(D, T, suspend_below, INT);
   E_CONFIG_VAL(D, T, force_mode, INT);
   E_CONFIG_VAL(D, T, suspend_method, INT);
#if defined HAVE_EEZE || defined __OpenBSD__
   E_CONFIG_VAL(D, T, fuzzy, INT);
#endif
#ifdef HAVE_ENOTIFY
   E_CONFIG_VAL(D, T, desktop_notifications, INT);
#endif

   battery_config = e_config_domain_load("module.battery", conf_edd);
   if (!battery_config)
     {
        battery_config = E_NEW(Config, 1);
        battery_config->poll_interval = 512;
        battery_config->alert = 30;
        battery_config->alert_p = 10;
        battery_config->alert_timeout = 0;
        battery_config->suspend_below = 0;
        battery_config->force_mode = 0;
        battery_config->suspend_method = 0;
#if defined HAVE_EEZE || defined __OpenBSD__
        battery_config->fuzzy = 0;
#endif
#ifdef HAVE_ENOTIFY
        battery_config->desktop_notifications = 0;
#endif
     }
   E_CONFIG_LIMIT(battery_config->poll_interval, 4, 4096);
   E_CONFIG_LIMIT(battery_config->alert, 0, 60);
   E_CONFIG_LIMIT(battery_config->alert_p, 0, 100);
   E_CONFIG_LIMIT(battery_config->alert_timeout, 0, 300);
   E_CONFIG_LIMIT(battery_config->suspend_below, 0, 50);
   E_CONFIG_LIMIT(battery_config->force_mode, 0, 2);
   E_CONFIG_LIMIT(battery_config->suspend_method, 0, 2);
#ifdef HAVE_ENOTIFY
   E_CONFIG_LIMIT(battery_config->desktop_notifications, 0, 1);
#endif

   battery_config->module = m;
   battery_config->full = -2;
   battery_config->time_left = -2;
   battery_config->time_full = -2;
   battery_config->have_battery = -2;
   battery_config->have_power = -2;

   battery_config->batget_data_handler =
     ecore_event_handler_add(ECORE_EXE_EVENT_DATA,
                             _battery_cb_exe_data, NULL);
   battery_config->batget_del_handler =
     ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
                             _battery_cb_exe_del, NULL);
   _handler = ecore_event_handler_add(E_EVENT_POWERSAVE_CONFIG_UPDATE,
                                      _powersave_cb_config_update, NULL);

   e_gadcon_provider_register(&_gadcon_class);

   //~ snprintf(buf, sizeof(buf), "%s/e-module-battery.edj", e_module_dir_get(m));
   e_configure_registry_category_add("advanced", 80, _("Advanced"), NULL,
                                     "preferences-advanced");
   e_configure_registry_item_add("advanced/battery", 100, _("Battery Meter"),
                                 NULL, "battery", e_int_config_battery_module);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   e_configure_registry_item_del("advanced/battery");
   e_configure_registry_category_del("advanced");
   e_gadcon_provider_unregister(&_gadcon_class);

   battery_config->alert_timer = NULL;

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

#ifdef HAVE_EEZE
   _battery_udev_stop();
#elif defined __OpenBSD__
   _battery_openbsd_stop();
#else
   _battery_dbus_stop();
#endif

#ifdef HAVE_ENOTIFY
   e_notification_shutdown();
#endif

   free(battery_config);
   battery_config = NULL;
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.battery", conf_edd, battery_config);
   return 1;
}

