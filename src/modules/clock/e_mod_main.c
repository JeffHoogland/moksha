#include "e.h"
#include "e_mod_main.h"

#include <sys/time.h>
#include <time.h>

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *client_class);

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

/* actual module specifics */
typedef struct _Instance Instance;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_clock, *o_table, *o_popclock, *o_cal;
   E_Gadcon_Popup  *popup;
   struct {
      int           start, len; // 0->6 0 == sun, 6 == sat, number of days
   } weekend;
   struct {
      int           start; // 0->6 0 == sun, 6 == sat
   } week;
   
   int madj;
   
   char year[8];
   char month[32];
   const char *daynames[7];
   unsigned char daynums[7][5];
   Eina_Bool dayweekends[7][5];
   Eina_Bool dayvalids[7][5];
   Eina_Bool daytoday[7][5];
};

static E_Module *clock_module = NULL;

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
_time_eval(Instance *inst)
{
   struct timeval      timev;
   struct tm          *tm, tms, tmm, tm2;
   time_t              tt;
   int                 started = 0, num, i;
   
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
        for (day = (0 - 6); day < (31 + 6); day++)
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
             if (!started)
               {
                  if (tm->tm_wday == inst->week.start) started = 1;
               }
             if (started)
               {
                  int y = num / 7;
                  int x = num % 7;
                  
                  if (y < 5)
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
                       for (i = inst->weekend.start; 
                            i < (inst->weekend.start + inst->weekend.len);
                            i++)
                         {
                            if (tmm.tm_wday == (i % 7))
                              {
                                 inst->dayweekends[x][y] = 1;
                                 break;
                              }
                         }
                       if (!inst->daynames[x])
                         {
                            char buf[32];
                            
                            buf[sizeof(buf) - 1] = 0;
                            strftime(buf, sizeof(buf) - 1, "%a", (const struct tm *)&tmm); // %A full weekeday
                            inst->daynames[x] = eina_stringshare_add(buf);
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
     }
}

static void
_clock_popup_new(Instance *inst)
{
   Evas *evas;
   Evas_Object *o, *oi, *od;
   int x, y;
   
   if (inst->popup) return;
   
   _time_eval(inst);
   
   inst->popup = e_gadcon_popup_new(inst->gcc);
   evas = inst->popup->win->evas;
   
   inst->o_table = e_widget_table_add(evas, 0);

   oi = edje_object_add(evas);
   inst->o_popclock = oi;
   e_theme_edje_object_set(oi, "base/theme/modules/clock",
                           "e/modules/clock/main");
   o = e_widget_image_add_from_object(evas, oi, 128, 128);
   evas_object_show(oi);
   e_widget_table_object_align_append(inst->o_table, o, 
                                      0, 0, 1, 1, 0, 0, 0, 0, 0.5, 0.5);
   
   oi = edje_object_add(evas);
   inst->o_popclock = oi;
   e_theme_edje_object_set(oi, "base/theme/modules/clock",
                           "e/modules/clock/calendar");
   
   edje_object_part_text_set(oi, "e.text.month", inst->month);
   edje_object_part_text_set(oi, "e.text.year", inst->year);
   for (x = 0; x < 7; x++)
     {
        od = edje_object_part_table_child_get(oi, "e.table.daynames", x, 0);
        edje_object_part_text_set(od, "e.text.label", inst->daynames[x]);
     }
   
   for (y = 0; y < 5; y++)
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
          }
     }
   
   // FIXME: set text part for month name and year
   // FIXME: hook signal callbacks for next and prev month
   
   // FIXME: set up day names (mon, tue, wed, ... sat, sun)
   
   // FIXME: calendar has a 7x5 grid, set up all the cells to ve either
   // hidden, visible, be weekend or weekday and have right date, be 
   // neighbouring month, be "active" today)
   
   o = e_widget_image_add_from_object(evas, oi, 182, 128);
   evas_object_show(oi);
   e_widget_table_object_align_append(inst->o_table, o, 
                                      1, 0, 1, 1, 0, 0, 0, 0, 0.5, 0.5);
   
   e_gadcon_popup_content_set(inst->popup, inst->o_table);
   e_gadcon_popup_show(inst->popup);
}

static void
_clock_popup_free(Instance *inst)
{
   if (!inst->popup) return;
   if (inst->popup) e_object_del(E_OBJECT(inst->popup));
   inst->popup = NULL;
}

static void
_clock_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   Instance *inst = data;
   Evas_Event_Mouse_Down *ev = event;
   
   if (ev->button != 1) return;
   if (inst->popup) _clock_popup_free(inst);
   else _clock_popup_new(inst);
}

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;
   
   inst = E_NEW(Instance, 1);
   
   o = edje_object_add(gc->evas);
   e_theme_edje_object_set(o, "base/theme/modules/clock",
			   "e/modules/clock/main");
   evas_object_show(o);
   
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;
   
   inst->gcc = gcc;
   inst->o_clock = o;
   
   inst->weekend.start = 6; // start weekend on sat
   inst->weekend.len = 2; // 2 days of weeked
   inst->week.start = 1; // start mon

   evas_object_event_callback_add(inst->o_clock, 
                                  EVAS_CALLBACK_MOUSE_DOWN,
                                  _clock_cb_mouse_down,
                                  inst);
   
   e_gadcon_client_util_menu_attach(gcc);
   
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;
   
   inst = gcc->data;
   evas_object_del(inst->o_clock);
   _clock_popup_free(inst);
   _clear_timestrs(inst);
   free(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   Instance *inst;
   Evas_Coord mw, mh;
   
   inst = gcc->data;
   mw = 0, mh = 0;
   edje_object_size_min_get(inst->o_clock, &mw, &mh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(inst->o_clock, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   e_gadcon_client_aspect_set(gcc, mw, mh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("Clock");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[4096];
   
   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-clock.edj",
	    e_module_dir_get(clock_module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _gadcon_class.name;
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
   clock_module = m;
   e_gadcon_provider_register(&_gadcon_class);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   clock_module = NULL;
   e_gadcon_provider_unregister(&_gadcon_class);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
