#include "e.h"
#include "e_mod_main.h"

#include <sys/time.h>
#include <time.h>
#ifdef HAVE_SYS_TIMERFD_H
# include <sys/timerfd.h>
#endif

/* actual module specifics */
typedef struct _Instance Instance;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_clock, *o_table, *o_popclock, *o_cal;
   E_Gadcon_Popup  *popup;
   Eina_List *handlers;

   int                  madj;

   char                 year[8];
   char                 month[64];
   char                 month_dec[6];
   int                  weeks[53];
   const char          *daynames[7];
   unsigned char        daynums[7][6];
   Eina_Bool            dayweekends[7][6];
   Eina_Bool            dayvalids[7][6];
   Eina_Bool            daytoday[7][6];
   Config_Item         *cfg;
   Ecore_X_Window       input_win;
   Ecore_Event_Handler *hand_mouse_down;
   Ecore_Event_Handler *hand_key_down;
};

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void             _gc_shutdown(E_Gadcon_Client *gcc);
static void             _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char      *_gc_label(const E_Gadcon_Client_Class *client_class);
static Evas_Object     *_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas);
static const char      *_gc_id_new(const E_Gadcon_Client_Class *client_class);
static Config_Item     *_conf_item_get(const char *id);
static void             _clock_popup_free(Instance *inst);
static void             _clock_popup_new(Instance *inst);

static Eio_Monitor *clock_tz_monitor = NULL;
static Eio_Monitor *clock_tz2_monitor = NULL;
static Eina_List *clock_eio_handlers = NULL;
Config *clock_config = NULL;

static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;
static Eina_List *clock_instances = NULL;
static E_Action *act = NULL;
static Ecore_Timer *update_today = NULL;
#ifdef HAVE_SYS_TIMERFD_H
static Ecore_Fd_Handler *timerfd_handler = NULL;
#endif

/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
   "clock",
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, NULL
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

static void
_clear_timestrs(Instance *inst)
{
   int x;

   for (x = 0; x < 7; x++)
     {
        if (inst->daynames[x])
          {
             eina_stringshare_del(inst->daynames[x]);
             inst->daynames[x] = NULL;
          }
     }
}

static void
_todaystr_eval(Instance *inst, char *buf, int bufsz)
{
   
    struct timeval timev;
    struct tm *tm;
    time_t tt;

    tzset();
    gettimeofday(&timev, NULL);
    tt = (time_t)(timev.tv_sec);
    tm = localtime(&tt);
    if (tm)
      {
         if (inst->cfg->show_date == 1)
           strftime(buf, bufsz, "%a, %e %b, %Y", (const struct tm *)tm);
         else if (inst->cfg->show_date == 2)
           strftime(buf, bufsz, "%a, %x", (const struct tm *)tm);
         else if (inst->cfg->show_date == 3)
           strftime(buf, bufsz, "%x", (const struct tm *)tm);
         else if (inst->cfg->show_date == 4)
           strftime(buf, bufsz, "%F", (const struct tm *)tm);
         else if (inst->cfg->show_date == 5)
          strftime(buf, bufsz, inst->cfg->custom_date_const, (const struct tm *)tm);

         inst->cfg->timeset.hour = tm->tm_hour;
         inst->cfg->timeset.minute = tm->tm_min;
      }
    else
      buf[0] = 0;

    if (!inst->cfg->show_date)
      buf[0] = 0;
}

static void
_time_eval(Instance *inst)
{
   struct timeval timev;
   struct tm *tm, tms, tmm, tm2;
   time_t tt;
   int started = 0, num, i;

   tzset();
   gettimeofday(&timev, NULL);
   tt = (time_t)(timev.tv_sec);
   tm = localtime(&tt);

   _clear_timestrs(inst);
   if (tm)
     {
        int day;

        // tms == current date time "saved"
        // tm2 == date to look at adjusting for madj
        // tm2 == month baseline @ 1st
        memcpy(&tms, tm, sizeof(struct tm));
        num = 0;
        for (day = (0 - 6); day < (31 + 16); day++)
          {
             memcpy(&tmm, &tms, sizeof(struct tm));
             tmm.tm_sec = 0;
             tmm.tm_min = 0;
             tmm.tm_hour = 10;
             tmm.tm_mon += inst->madj;
             tmm.tm_mday = 1; // start at the 1st of the month
             tmm.tm_wday = 0; // ignored by mktime
             tmm.tm_yday = 0; // ignored by mktime
             tmm.tm_isdst = 0; // ignored by mktime
             tt = mktime(&tmm);
             tm = localtime(&tt);
             memcpy(&tm2, tm, sizeof(struct tm));

             tt = mktime(&tmm);
             tt += (day * 60 * 60 * 24);
             tm = localtime(&tt);
             memcpy(&tmm, tm, sizeof(struct tm));
             
             // retrieve a week number
             char weeknum[3];
             strftime(weeknum, sizeof(weeknum), "%V", tm); // %V ISO-8601
                                                           // %W Start on Monday
                                                           // %U Start on Sunday
             inst->weeks[day] = atoi(weeknum);
             
             if (!started)
               {
                  if (tm->tm_wday == inst->cfg->week.start)
                    {
                       char buf[32];

                       for (i = 0; i < 7; i++, tm->tm_wday = (tm->tm_wday + 1) % 7)
                         {
                            strftime(buf, sizeof(buf), "%a", tm);
                            inst->daynames[i] = eina_stringshare_add(buf);
                         }
                       started = 1;
                    }
               }
             if (started)
               {
                  int y = num / 7;
                  int x = num % 7;

                  if (y < 6)
                    {
                       inst->daynums[x][y] = tmm.tm_mday;

                       inst->dayvalids[x][y] = 0;
                       if (tmm.tm_mon == tm2.tm_mon) inst->dayvalids[x][y] = 1;

                       inst->daytoday[x][y] = 0;
                       if ((tmm.tm_mon == tms.tm_mon) &&
                           (tmm.tm_year == tms.tm_year) &&
                           (tmm.tm_mday == tms.tm_mday))
                         inst->daytoday[x][y] = 1;

                       inst->dayweekends[x][y] = 0;
                       for (i = inst->cfg->weekend.start;
                            i < (inst->cfg->weekend.start + inst->cfg->weekend.len);
                            i++)
                         {
                            if (tmm.tm_wday == (i % 7))
                              {
                                 inst->dayweekends[x][y] = 1;
                                 break;
                              }
                         }
                    }
                  num++;
               }
          }

        memcpy(&tmm, &tms, sizeof(struct tm));
        tmm.tm_sec = 0;
        tmm.tm_min = 0;
        tmm.tm_hour = 10;
        tmm.tm_mon += inst->madj;
        tmm.tm_mday = 1; // start at the 1st of the month
        tmm.tm_wday = 0; // ignored by mktime
        tmm.tm_yday = 0; // ignored by mktime
        tmm.tm_isdst = 0; // ignored by mktime
        tt = mktime(&tmm);
        tm = localtime(&tt);
        memcpy(&tm2, tm, sizeof(struct tm));
        inst->year[sizeof(inst->year) - 1] = 0;
        strftime(inst->year, sizeof(inst->year) - 1, "%Y", (const struct tm *)&tm2);
        inst->month[sizeof(inst->month) - 1] = 0;
        strftime(inst->month, sizeof(inst->month) - 1, "%B", (const struct tm *)&tm2); // %b for short month
        strftime(inst->month_dec, sizeof(inst->month_dec) - 1, "%m", (const struct tm *)&tm2); // %m month as decimal
     }
}

static void
_clock_month_update(Instance *inst)
{
   Evas_Object *od, *oi, *ow;
   int x, y;

   oi = inst->o_cal;
   edje_object_part_text_set(oi, "e.text.month", inst->month);
   edje_object_part_text_set(oi, "e.text.year", inst->year);
   for (x = 0; x < 7; x++)
     {
        od = edje_object_part_table_child_get(oi, "e.table.daynames", x, 0);
        edje_object_part_text_set(od, "e.text.label", inst->daynames[x]);
        edje_object_message_signal_process(od);
        if (inst->dayweekends[x][0])
          edje_object_signal_emit(od, "e,state,weekend", "e");
        else
          edje_object_signal_emit(od, "e,state,weekday", "e");
     }
     od = edje_object_part_table_child_get(oi, "e.table.daynames", 7, 0);
     edje_object_part_text_set(od, "e.text.label", ""); // week number header

   for (y = 0; y < 6; y++)
     {
        for (x = 0; x < 7; x++)
          {
             char buf[32];

             od = edje_object_part_table_child_get(oi, "e.table.days", x, y);
             snprintf(buf, sizeof(buf), "%i", (int)inst->daynums[x][y]);
             edje_object_part_text_set(od, "e.text.label", buf);

             if (inst->dayweekends[x][y])
               edje_object_signal_emit(od, "e,state,weekend", "e");
             else
               edje_object_signal_emit(od, "e,state,weekday", "e");
             if (inst->dayvalids[x][y])
               edje_object_signal_emit(od, "e,state,visible", "e");
             else
               edje_object_signal_emit(od, "e,state,hidden", "e");
             if (inst->daytoday[x][y])
               edje_object_signal_emit(od, "e,state,today", "e");
             else
               edje_object_signal_emit(od, "e,state,someday", "e");

             edje_object_message_signal_process(od);

          }
        // week number 
          
        char buf[3]; 
        ow = edje_object_part_table_child_get(oi, "e.table.days", 7, y);
        snprintf(buf, sizeof(buf), "%i", inst->weeks[y * 7]);
        edje_object_part_text_set(ow, "e.text.label", buf); 

        edje_object_signal_emit(ow, "e,state,week", "e");

     }
   edje_object_message_signal_process(oi);
}

static void
_clock_month_prev_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Instance *inst = data;
   inst->madj--;
   _time_eval(inst);
   _clock_month_update(inst);
}

static void
_clock_month_next_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Instance *inst = data;
   inst->madj++;
   _time_eval(inst);
   _clock_month_update(inst);
}

static void
_clock_cb_mouse_wheel(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   Instance *inst = data;
    Evas_Event_Mouse_Wheel *ev = event;

   if (ev->z > 0)
     inst->madj= inst->madj - 12;
   else if (ev->z < 0)
     inst->madj= inst->madj + 12;

  _time_eval(inst);
  _clock_month_update(inst);
}

static void
_clock_settings_cb(void *d1, void *d2 __UNUSED__)
{
   Instance *inst = d1;
   e_int_config_clock_module(inst->popup->win->zone->container, inst->cfg);
   _clock_popup_free(inst);
   //~ e_object_del(E_OBJECT(inst->popup));
   //~ inst->popup = NULL;
   //~ inst->o_popclock = NULL;
}

static Eina_Bool
_clock_popup_fullscreen_change(Instance *inst, int type __UNUSED__, void *ev __UNUSED__)
{
   _clock_popup_free(inst);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_clock_popup_desk_change(Instance *inst, int type __UNUSED__, E_Event_Desk_After_Show *ev)
{
   if ((!inst->gcc) || (!inst->gcc->gadcon) || (!inst->gcc->gadcon->shelf)) return ECORE_CALLBACK_RENEW;
   if (e_shelf_desk_visible(inst->gcc->gadcon->shelf, ev->desk)) return ECORE_CALLBACK_RENEW;
   _clock_popup_free(inst);
   return ECORE_CALLBACK_RENEW;
}

static void
_popclock_del_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *info __UNUSED__)
{
   Instance *inst = data;
   if (inst->o_popclock == obj)
     {
        inst->o_popclock = NULL;
     }
}

static Eina_Bool
_clock_input_win_mouse_down_cb(void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   Instance *inst = data;

   if (ev->window != inst->input_win) return ECORE_CALLBACK_PASS_ON;
   _clock_popup_free(inst);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_clock_input_win_key_down_cb(void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Key *ev = event;
   Instance *inst = data;
   const char *keysym;

   if (ev->window != inst->input_win) return ECORE_CALLBACK_PASS_ON;

   keysym = ev->key;
   if (!strcmp(keysym, "Escape"))
      _clock_popup_free(inst);
   return ECORE_CALLBACK_PASS_ON;
}

static void
_clock_input_win_del(Instance *inst)
{
   if (!inst->input_win) return;
   e_grabinput_release(0, inst->input_win);
   ecore_x_window_free(inst->input_win);
   inst->input_win = 0;
   ecore_event_handler_del(inst->hand_mouse_down);
   inst->hand_mouse_down = NULL;
   ecore_event_handler_del(inst->hand_key_down);
   inst->hand_key_down = NULL;
}

static void
_clock_input_win_new(Instance *inst)
{
   Ecore_X_Window_Configure_Mask mask;
   Ecore_X_Window w, popup_w;
   E_Manager *man;

   man = inst->gcc->gadcon->zone->container->manager;

   w = ecore_x_window_input_new(man->root, 0, 0, man->w, man->h);
   mask = (ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE |
           ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING);
   popup_w = inst->popup->win->evas_win;
   ecore_x_window_configure(w, mask, 0, 0, 0, 0, 0, popup_w,
                            ECORE_X_WINDOW_STACK_BELOW);
   ecore_x_window_show(w);

   inst->hand_mouse_down =
      ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                              _clock_input_win_mouse_down_cb, inst);
   inst->hand_key_down =
      ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                              _clock_input_win_key_down_cb, inst);
   inst->input_win = w;
   e_grabinput_get(0, 0, inst->input_win);
}

static void
_check_cb_change(void *data, Evas_Object *obj __UNUSED__)
{
   Instance *inst = data;

   if (!inst) return;
   if (!inst->cfg->always_on_top)
     _clock_input_win_new(inst);
   else
     _clock_input_win_del(inst);
}

static void
_clock_popup_new(Instance *inst)
{
   Evas *evas;
   Evas_Object *o, *oi;
   Evas_Coord mw = 128, mh = 128;
   char todaystr[128];

   if (inst->popup) return;

   _todaystr_eval(inst, todaystr, sizeof(todaystr) - 1);

   inst->madj = 0;

   _time_eval(inst);

   inst->popup = e_gadcon_popup_new(inst->gcc);
   evas = inst->popup->win->evas;

   inst->o_table = e_widget_table_add(evas, 0);

   oi = edje_object_add(evas);
   inst->o_popclock = oi;
   evas_object_event_callback_add(oi, EVAS_CALLBACK_DEL, _popclock_del_cb, inst);

   if (inst->cfg->digital_clock)
     e_theme_edje_object_set(oi, "base/theme/modules/clock",
                             "e/modules/clock/digital");
   else
     e_theme_edje_object_set(oi, "base/theme/modules/clock",
                             "e/modules/clock/main");
   if (inst->cfg->show_date)
     edje_object_signal_emit(oi, "e,state,date,on", "e");
   else
     edje_object_signal_emit(oi, "e,state,date,off", "e");
   if (inst->cfg->digital_24h)
     edje_object_signal_emit(oi, "e,state,24h,on", "e");
   else
     edje_object_signal_emit(oi, "e,state,24h,off", "e");
   if (inst->cfg->show_seconds)
     edje_object_signal_emit(oi, "e,state,seconds,on", "e");
   else
     edje_object_signal_emit(oi, "e,state,seconds,off", "e");

   edje_object_part_text_set(oi, "e.text.today", todaystr);

   o = e_widget_image_add_from_object(evas, oi, 112, 85 * e_scale);
   evas_object_show(oi);
   e_widget_table_object_align_append(inst->o_table, o,
                                      0, 0, 1, 1, 0, 0, 0, 0, 0.5, 0.5);

   o = e_widget_check_add(evas, _("Always on Top"), &inst->cfg->always_on_top);
   e_widget_on_change_hook_set(o, _check_cb_change, inst);
   e_widget_table_object_align_append(inst->o_table, o,
                                      0, 1, 1, 1, 0, 0, 0, 0, 0.5, 0.0);

   o = e_widget_button_add(evas, _("Settings"), "preferences-system",
                           _clock_settings_cb, inst, NULL);
   e_widget_table_object_align_append(inst->o_table, o,
                                      0, 1, 1, 1, 0, 0, 0, 0, 0.5, 1.0);

   oi = edje_object_add(evas);
   inst->o_cal = oi;
   e_theme_edje_object_set(oi, "base/theme/modules/clock",
                           "e/modules/clock/calendar");
   _clock_month_update(inst);

   edje_object_signal_callback_add(oi, "e,action,prev", "*",
                                   _clock_month_prev_cb, inst);
   edje_object_signal_callback_add(oi, "e,action,next", "*",
                                   _clock_month_next_cb, inst);
   evas_object_event_callback_add(oi,
                                  EVAS_CALLBACK_MOUSE_WHEEL,
                                  _clock_cb_mouse_wheel,
                                  inst);
   edje_object_message_signal_process(oi);
   evas_object_resize(oi, 500, 500);
   edje_object_size_min_restricted_calc(oi, &mw, &mh, 128, 128);

   o = e_widget_image_add_from_object(evas, oi, mw, mh);
   evas_object_show(oi);
   e_widget_table_object_align_append(inst->o_table, o,
                                      1, 0, 1, 2, 0, 0, 0, 0, 0.5, 0.5);

   e_gadcon_popup_content_set(inst->popup, inst->o_table);
   e_gadcon_popup_show(inst->popup);

   E_LIST_HANDLER_APPEND(inst->handlers, E_EVENT_DESK_AFTER_SHOW, _clock_popup_desk_change, inst);
   E_LIST_HANDLER_APPEND(inst->handlers, E_EVENT_BORDER_FULLSCREEN, _clock_popup_fullscreen_change, inst);
   if (!inst->cfg->always_on_top)
     _clock_input_win_new(inst);
}

static void
_eval_instance_size(Instance *inst)
{
   Evas_Coord mw, mh, omw, omh;

   edje_object_size_min_get(inst->o_clock, &mw, &mh);
   omw = mw;
   omh = mh;

   if ((mw < 1) || (mh < 1))
     {
        Evas_Coord x, y, sw = 0, sh = 0, ow, oh;
        //~ Evas_Coord sw = 0, sh = 0, ow, oh;
        Eina_Bool horiz;
        const char *orient;

        switch (inst->gcc->gadcon->orient)
          {
           case E_GADCON_ORIENT_TOP:
           case E_GADCON_ORIENT_CORNER_TL:
           case E_GADCON_ORIENT_CORNER_TR:
           case E_GADCON_ORIENT_BOTTOM:
           case E_GADCON_ORIENT_CORNER_BL:
           case E_GADCON_ORIENT_CORNER_BR:
           case E_GADCON_ORIENT_HORIZ:
             horiz = EINA_TRUE;
             orient = "e,state,horizontal";
             break;

           case E_GADCON_ORIENT_LEFT:
           case E_GADCON_ORIENT_CORNER_LB:
           case E_GADCON_ORIENT_CORNER_LT:
           case E_GADCON_ORIENT_RIGHT:
           case E_GADCON_ORIENT_CORNER_RB:
           case E_GADCON_ORIENT_CORNER_RT:
           case E_GADCON_ORIENT_VERT:
             horiz = EINA_FALSE;
             orient = "e,state,vertical";
             break;

           default:
             horiz = EINA_TRUE;
             orient = "e,state,float";
          }

        if (inst->gcc->gadcon->shelf)
          {
             if (horiz)
               sh = inst->gcc->gadcon->shelf->h;
             else
               sw = inst->gcc->gadcon->shelf->w;
          }

        evas_object_geometry_get(inst->o_clock, NULL, NULL, &ow, &oh);
        if (orient)
          edje_object_signal_emit(inst->o_clock, orient, "e");
        evas_object_resize(inst->o_clock, sw, sh);
        edje_object_message_signal_process(inst->o_clock);

        edje_object_parts_extends_calc(inst->o_clock, &x, &y, &mw, &mh);
        //~ edje_object_size_min_calc(inst->o_clock, &mw, &mh);

        evas_object_resize(inst->o_clock, ow, oh);
     }

   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;

   if (mw < omw) mw = omw;
   if (mh < omh) mh = omh;

   e_gadcon_client_aspect_set(inst->gcc, mw, mh);
   e_gadcon_client_min_size_set(inst->gcc, mw, mh);
}

void
e_int_clock_instances_redo(Eina_Bool all)
{
   Eina_List *l;
   Instance *inst;
   char todaystr[128];

   EINA_LIST_FOREACH(clock_instances, l, inst)
     {
        Evas_Object *o = inst->o_clock;

        if ((!all) && (!inst->cfg->changed)) continue;
        _todaystr_eval(inst, todaystr, sizeof(todaystr) - 1);
        if (inst->cfg->digital_clock)
          e_theme_edje_object_set(o, "base/theme/modules/clock",
                                  "e/modules/clock/digital");
        else
          e_theme_edje_object_set(o, "base/theme/modules/clock",
                                  "e/modules/clock/main");
        if (inst->cfg->show_date)
          edje_object_signal_emit(o, "e,state,date,on", "e");
        else
          edje_object_signal_emit(o, "e,state,date,off", "e");
        if (inst->cfg->digital_24h)
          edje_object_signal_emit(o, "e,state,24h,on", "e");
        else
          edje_object_signal_emit(o, "e,state,24h,off", "e");
        if (inst->cfg->show_seconds)
          edje_object_signal_emit(o, "e,state,seconds,on", "e");
        else
          edje_object_signal_emit(o, "e,state,seconds,off", "e");

        edje_object_part_text_set(o, "e.text.today", todaystr);
        edje_object_message_signal_process(o);
        _eval_instance_size(inst);
        
        if (inst->o_popclock)
          {
             o = inst->o_popclock;

             if (inst->cfg->digital_clock)
               e_theme_edje_object_set(o, "base/theme/modules/clock",
                                       "e/modules/clock/digital");
             else
               e_theme_edje_object_set(o, "base/theme/modules/clock",
                                       "e/modules/clock/main");
             if (inst->cfg->show_date)
               edje_object_signal_emit(o, "e,state,date,on", "e");
             else
               edje_object_signal_emit(o, "e,state,date,off", "e");
             if (inst->cfg->digital_24h)
               edje_object_signal_emit(o, "e,state,24h,on", "e");
             else
               edje_object_signal_emit(o, "e,state,24h,off", "e");
             if (inst->cfg->show_seconds)
               edje_object_signal_emit(o, "e,state,seconds,on", "e");
             else
               edje_object_signal_emit(o, "e,state,seconds,off", "e");

             edje_object_part_text_set(o, "e.text.today", todaystr);
             edje_object_message_signal_process(o);
          }
     }
}

static Eina_Bool
_update_today_timer(void *data __UNUSED__)
{
   time_t t, t_tomorrow;
   const struct tm *now;
   struct tm today;

   e_int_clock_instances_redo(EINA_TRUE);
   if (!clock_instances)
     {
        update_today = NULL;
        return EINA_FALSE;
     }

   t = time(NULL);
   now = localtime(&t);
   memcpy(&today, now, sizeof(today));
   today.tm_sec = 1;
   today.tm_min = 0;
   today.tm_hour = 0;

   t_tomorrow = mktime(&today) + 24 * 60 * 60;
   if (update_today) ecore_timer_interval_set(update_today, t_tomorrow - t);
   else update_today = ecore_timer_add(t_tomorrow - t, _update_today_timer, NULL);
   return EINA_TRUE;
}

static void
_clock_popup_free(Instance *inst)
{
   if (!inst->popup) return;
   if (inst->popup)
     {
       _clock_input_win_del(inst);
       e_object_del(E_OBJECT(inst->popup));
       inst->popup = NULL;
     }

   E_FREE_LIST(inst->handlers, ecore_event_handler_del);
   inst->o_popclock = NULL;
}

static void
_clock_menu_cb_cfg(void *data, E_Menu *menu __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   Instance *inst = data;
   E_Container *con;

   if (inst->popup)
     {
        e_object_del(E_OBJECT(inst->popup));
        inst->popup = NULL;
     }
   con = e_container_current_get(e_manager_current_get());
   e_int_config_clock_module(con, inst->cfg);
}

static void
_clock_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   Instance *inst = data;
   Evas_Event_Mouse_Down *ev = event;

   if (ev->button == 1)
     {
        if (inst->popup) _clock_popup_free(inst);
        else _clock_popup_new(inst);
     }
   else if (ev->button == 3)
     {
        E_Zone *zone;
        E_Menu *m;
        E_Menu_Item *mi;
        int x, y;

        zone = e_util_zone_current_get(e_manager_current_get());

        m = e_menu_new();

        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, _("Settings"));
        e_util_menu_item_theme_icon_set(mi, "configure");
        e_menu_item_callback_set(mi, _clock_menu_cb_cfg, inst);

        m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);

        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y, NULL, NULL);
        e_menu_activate_mouse(m, zone, x + ev->output.x, y + ev->output.y,
                              1, 1, E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
        evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                                 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
}

static void
_clock_sizing_changed_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _eval_instance_size(data);
}

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;
   char todaystr[128];

   inst = E_NEW(Instance, 1);
   inst->cfg = _conf_item_get(id);

   _todaystr_eval(inst, todaystr, sizeof(todaystr) - 1);
   
   o = edje_object_add(gc->evas);
   edje_object_signal_callback_add(o, "e,state,sizing,changed", "*",
                                   _clock_sizing_changed_cb, inst);
   if (inst->cfg->digital_clock)
     e_theme_edje_object_set(o, "base/theme/modules/clock",
                             "e/modules/clock/digital");
   else
     e_theme_edje_object_set(o, "base/theme/modules/clock",
                             "e/modules/clock/main");
   if (inst->cfg->show_date)
     edje_object_signal_emit(o, "e,state,date,on", "e");
   else
     edje_object_signal_emit(o, "e,state,date,off", "e");
   if (inst->cfg->digital_24h)
     edje_object_signal_emit(o, "e,state,24h,on", "e");
   else
     edje_object_signal_emit(o, "e,state,24h,off", "e");
   if (inst->cfg->show_seconds)
     edje_object_signal_emit(o, "e,state,seconds,on", "e");
   else
     edje_object_signal_emit(o, "e,state,seconds,off", "e");

   edje_object_part_text_set(o, "e.text.today", todaystr);
   edje_object_message_signal_process(o);
   evas_object_show(o);

   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;

   inst->gcc = gcc;
   inst->o_clock = o;

   evas_object_event_callback_add(inst->o_clock,
                                  EVAS_CALLBACK_MOUSE_DOWN,
                                  _clock_cb_mouse_down,
                                  inst);
   clock_instances = eina_list_append(clock_instances, inst);

   if (!update_today) _update_today_timer(NULL);

   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   inst = gcc->data;
   clock_instances = eina_list_remove(clock_instances, inst);
   evas_object_del(inst->o_clock);
   _clock_popup_free(inst);
   _clear_timestrs(inst);
   free(inst);

   if ((!clock_instances) && (update_today))
     {
        ecore_timer_del(update_today);
        update_today = NULL;
     }
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   _eval_instance_size(gcc->data);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("Clock");
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[4096];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-clock.edj",
            e_module_dir_get(clock_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   Config_Item *ci = NULL;

   ci = _conf_item_get(NULL);
   return ci->id;
}

static Config_Item *
_conf_item_get(const char *id)
{
   Config_Item *ci;

   GADCON_CLIENT_CONFIG_GET(Config_Item, clock_config->items, _gadcon_class, id);

   ci = E_NEW(Config_Item, 1);
   ci->id = eina_stringshare_add(id);
   ci->custom_date_const = eina_stringshare_add("%a, %d. %b");
   ci->weekend.start = 6;
   ci->weekend.len = 2;
   ci->week.start = 1;
   ci->digital_clock = 0;
   ci->digital_24h = 0;
   ci->show_seconds = 0;
   ci->show_date = 0;
   ci->always_on_top = 0;

   clock_config->items = eina_list_append(clock_config->items, ci);
   e_config_save_queue();

   return ci;
}

static void
_e_mod_action(const char *params)
{
   Eina_List *l;
   Instance *inst;

   if (!params) return;
   if (strcmp(params, "show_calendar")) return;

   EINA_LIST_FOREACH(clock_instances, l, inst)
     if (inst->popup)
       _clock_popup_free(inst);
     else
       _clock_popup_new(inst);
}

static void
_e_mod_action_cb_edge(E_Object *obj __UNUSED__, const char *params, E_Event_Zone_Edge *ev __UNUSED__)
{
   _e_mod_action(params);
}

static void
_e_mod_action_cb(E_Object *obj __UNUSED__, const char *params)
{
   _e_mod_action(params);
}

static void
_e_mod_action_cb_key(E_Object *obj __UNUSED__, const char *params, Ecore_Event_Key *ev __UNUSED__)
{
   _e_mod_action(params);
}

static void
_e_mod_action_cb_mouse(E_Object *obj __UNUSED__, const char *params, Ecore_Event_Mouse_Button *ev __UNUSED__)
{
   _e_mod_action(params);
}

static Eina_Bool
_clock_eio_update(void *d __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   e_int_clock_instances_redo(EINA_TRUE);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_clock_eio_error(void *d __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   eio_monitor_del(clock_tz_monitor);
   clock_tz_monitor = eio_monitor_add("/etc/localtime");
   eio_monitor_del(clock_tz2_monitor);
   clock_tz2_monitor = eio_monitor_add("/etc/timezone");
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_clock_time_update(void *d __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   e_int_clock_instances_redo(EINA_TRUE);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_clock_fd_update(void *d __UNUSED__, Ecore_Fd_Handler *fdh)
{
   char buf[64];

   if (read(ecore_main_fd_handler_fd_get(fdh), buf, sizeof(buf)) < 0)
     {
        close(ecore_main_fd_handler_fd_get(fdh));
        timerfd_handler = ecore_main_fd_handler_del(timerfd_handler);
        return EINA_FALSE;
     }
   e_int_clock_instances_redo(EINA_TRUE);
   return EINA_TRUE;
}

static Eina_Bool
_clock_screensaver_on()
{
   E_FREE_FUNC(update_today, ecore_timer_del);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_clock_screensaver_off()
{
   if (clock_instances) _update_today_timer(NULL);
   return ECORE_CALLBACK_RENEW;
}

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Clock"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   conf_item_edd = E_CONFIG_DD_NEW("Config_Item", Config_Item);
#undef T
#undef D
#define T Config_Item
#define D conf_item_edd
   E_CONFIG_VAL(D, T, id, STR);
   E_CONFIG_VAL(D, T, custom_date_const, STR);
   E_CONFIG_VAL(D, T, weekend.start, INT);
   E_CONFIG_VAL(D, T, weekend.len, INT);
   E_CONFIG_VAL(D, T, week.start, INT);
   E_CONFIG_VAL(D, T, timeset.hour, DOUBLE);
   E_CONFIG_VAL(D, T, timeset.minute, DOUBLE);
   E_CONFIG_VAL(D, T, digital_clock, INT);
   E_CONFIG_VAL(D, T, digital_24h, INT);
   E_CONFIG_VAL(D, T, show_seconds, INT);
   E_CONFIG_VAL(D, T, show_date, INT);
   E_CONFIG_VAL(D, T, always_on_top, INT);

   conf_edd = E_CONFIG_DD_NEW("Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_LIST(D, T, items, conf_item_edd);

   clock_config = e_config_domain_load("module.clock", conf_edd);

   if (!clock_config)
     clock_config = E_NEW(Config, 1);

   act = e_action_add("clock");
   if (act)
     {
        act->func.go = _e_mod_action_cb;
        act->func.go_key = _e_mod_action_cb_key;
        act->func.go_mouse = _e_mod_action_cb_mouse;
        act->func.go_edge = _e_mod_action_cb_edge;

        e_action_predef_name_set(N_("Clock"), N_("Toggle calendar"), "clock", "show_calendar", NULL, 0);
     }

   clock_config->module = m;
    if (ecore_file_exists("/etc/localtime"))
     clock_tz_monitor = eio_monitor_add("/etc/localtime");
   if (ecore_file_exists("/etc/timezone"))
     clock_tz2_monitor = eio_monitor_add("/etc/timezone");
   E_LIST_HANDLER_APPEND(clock_eio_handlers, EIO_MONITOR_ERROR, _clock_eio_error, NULL);
   E_LIST_HANDLER_APPEND(clock_eio_handlers, EIO_MONITOR_FILE_CREATED, _clock_eio_update, NULL);
   E_LIST_HANDLER_APPEND(clock_eio_handlers, EIO_MONITOR_FILE_MODIFIED, _clock_eio_update, NULL);
   E_LIST_HANDLER_APPEND(clock_eio_handlers, EIO_MONITOR_FILE_DELETED, _clock_eio_update, NULL);
   E_LIST_HANDLER_APPEND(clock_eio_handlers, EIO_MONITOR_SELF_DELETED, _clock_eio_update, NULL);
   E_LIST_HANDLER_APPEND(clock_eio_handlers, EIO_MONITOR_SELF_RENAME, _clock_eio_update, NULL);
   E_LIST_HANDLER_APPEND(clock_eio_handlers, E_EVENT_SYS_RESUME, _clock_time_update, NULL);
   E_LIST_HANDLER_APPEND(clock_eio_handlers, ECORE_EVENT_SYSTEM_TIMEDATE_CHANGED, _clock_time_update, NULL);
   E_LIST_HANDLER_APPEND(clock_eio_handlers, E_EVENT_SCREENSAVER_ON, _clock_screensaver_on, NULL);
   E_LIST_HANDLER_APPEND(clock_eio_handlers, E_EVENT_SCREENSAVER_OFF, _clock_screensaver_off, NULL);
   
   e_gadcon_provider_register(&_gadcon_class);
#ifdef HAVE_SYS_TIMERFD_H

#ifndef TFD_TIMER_CANCELON_SET
# define TFD_TIMER_CANCELON_SET (1 << 1)
#endif
     {
        int timer_fd;
        int flags;
        struct itimerspec its;
   
        // on old systems, flags must be 0, so we'll play nice and do it always
        timer_fd = timerfd_create(CLOCK_REALTIME, 0);
        if (timer_fd < 0) return m;
        fcntl(timer_fd, F_SETFL, O_NONBLOCK);

        flags = fcntl(timer_fd, F_GETFD);
        flags |= FD_CLOEXEC;   
        fcntl(timer_fd, F_SETFD, flags);

        memset(&its, 0, sizeof(its));
        timerfd_settime(timer_fd, TFD_TIMER_ABSTIME | TFD_TIMER_CANCELON_SET, 
                        &its, NULL);

        timerfd_handler = ecore_main_fd_handler_add(timer_fd, ECORE_FD_READ,
                                                    _clock_fd_update, NULL, 
                                                    NULL, NULL);
     }
#endif
   return m;
}


EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_FREE_LIST(handlers, ecore_event_handler_del);
   if (act)
     {
        e_action_predef_name_del("Clock", "Toggle calendar");
        e_action_del("clock");
        act = NULL;
     }
   if (clock_config)
     {
        Config_Item *ci;

        if (clock_config->config_dialog)
          e_object_del(E_OBJECT(clock_config->config_dialog));

        EINA_LIST_FREE(clock_config->items, ci)
          {
             eina_stringshare_del(ci->id);
             eina_stringshare_del(ci->custom_date_const);
             free(ci);
          }

        free(clock_config);
        clock_config = NULL;
     }
   E_CONFIG_DD_FREE(conf_edd);
   E_CONFIG_DD_FREE(conf_item_edd);
   conf_item_edd = NULL;
   conf_edd = NULL;

   e_gadcon_provider_unregister(&_gadcon_class);

   if (update_today)
     {
        ecore_timer_del(update_today);
        update_today = NULL;
     }
   eio_monitor_del(clock_tz_monitor);
   eio_monitor_del(clock_tz2_monitor);
   clock_tz_monitor = NULL;
   clock_tz2_monitor = NULL;
#ifdef HAVE_SYS_TIMERFD_H
   timerfd_handler = ecore_main_fd_handler_del(timerfd_handler);
#endif

   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.clock", conf_edd, clock_config);
   return 1;
}

