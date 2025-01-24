#include "e.h"
#include "e_mod_main.h"
#include <Elementary.h>

#define TIME_STRING_SET "date +%T -s"
#define DATE_STRING_SET "date +%Y%m%d -s"

struct _E_Config_Dialog_Data
{
   Config_Item cfg;
   char *custom_dat;
   double hour, minute, sec;
   int year, month, day;
   Evas_Object *ck, *win, *win2, *cal;
};

/* Protos */
static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd,
                               E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd,
                                          Evas *evas,
                                          E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data(E_Config_Dialog *cfd,
                                      E_Config_Dialog_Data *cfdata);
static void         _clock_date_time_set(E_Config_Dialog_Data *cfdata,
                                         Eina_Bool data);


void
e_int_config_clock_module(E_Container *con, Config_Item *ci)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   //~ char buf[4096];

   if (e_config_dialog_find("E", "utils/clock")) return;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;

   //~ snprintf(buf, sizeof(buf), "%s/e-module-clock.edj",
            //~ e_module_dir_get(clock_config->module));
   cfd = e_config_dialog_new(con, _("Clock Settings"),
                             "E", "utils/clock", "clock", 0, v, ci);
   clock_config->config_dialog = cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = NULL;
   Config_Item *ci;
   
   ci = cfd->data;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);

   memcpy(&(cfdata->cfg), ci, sizeof(Config_Item));
   if (ci->custom_date_const)
     cfdata->custom_dat = strdup(ci->custom_date_const);
   else
     cfdata->custom_dat = strdup("");

   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd  __UNUSED__,
           E_Config_Dialog_Data *cfdata)
{
   free(cfdata->custom_dat);
   clock_config->config_dialog = NULL;
   E_FREE(cfdata);
}

static void
_bt_date_apply(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__,
                 void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   struct tm sel_time;

   if (!elm_calendar_selected_time_get(cfdata->cal, &sel_time))
     return;

   cfdata->year = sel_time.tm_year + 1900;
   cfdata->month = sel_time.tm_mon + 1;
   cfdata->day = sel_time.tm_mday;
   _clock_date_time_set(cfdata, EINA_TRUE);

   evas_object_del(cfdata->win2);
   cfdata->win2 = NULL;
}

static void
_bt_time_apply(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__,
                 void *event_info __UNUSED__)
{  
   int hrs, mins, secs;
   E_Config_Dialog_Data *cfdata = data;

   elm_clock_time_get(cfdata->ck, &hrs, &mins, &secs);
   cfdata->hour = (double) hrs;
   cfdata->minute = (double) mins;
   cfdata->sec = (double) secs;
   _clock_date_time_set(cfdata, EINA_FALSE);

   evas_object_del(cfdata->win);
   cfdata->win = NULL;
}

static void
_close_cb(void *data, Evas *e __UNUSED__, 
          Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;

   if (cfdata->win)
     {
       evas_object_del(cfdata->win);
       cfdata->win = NULL;
     }
   if (cfdata->win2)
     {
       evas_object_del(cfdata->win2);
       cfdata->win2 = NULL;
     }
}

static void
show_date_cb(void *data, void *data2 __UNUSED__)
{
   Evas_Object *win, *bx, *cal, *bt;
   struct tm selected_time;
   time_t current_time;
   E_Config_Dialog_Data *cfdata = data;

   if (cfdata->win2) return;

   win = elm_win_util_standard_add("calendar", _("Date set"));
   elm_win_autodel_set(win, EINA_TRUE);
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE, _close_cb, cfdata);
   cfdata->win2 = win;

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   cal = elm_calendar_add(win);
   evas_object_size_hint_weight_set(cal, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(cal, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(cal, 200 * e_scale, 140 * e_scale);
   current_time = time(NULL);
   localtime_r(&current_time, &selected_time);
   elm_calendar_selected_time_set(cal, &selected_time);
   evas_object_show(cal);
   elm_box_pack_end(bx, cal);
   cfdata->cal = cal;

   bt = elm_button_add(win);
   elm_object_text_set(bt, _("Set"));
   evas_object_event_callback_add(bt, EVAS_CALLBACK_MOUSE_DOWN, _bt_date_apply, cfdata);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   evas_object_show(win);
}

static void
show_time_cb(void *data, void *data2 __UNUSED__)
{
   Evas_Object *win, *bx, *ck, *bt;
   E_Config_Dialog_Data *cfdata = data;
   
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   
   if (cfdata->win) return;

   win = elm_win_util_standard_add("clock", _("Time set"));
   cfdata->win = win;
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE, _close_cb, cfdata);
   evas_object_resize(win, 280, 100);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, 100, 100);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);
   
   ck = elm_clock_add(win);
   elm_clock_show_seconds_set(ck, EINA_TRUE);
   elm_clock_edit_set(ck, EINA_TRUE);
   evas_object_size_hint_min_set(ck, 80 * e_scale, 40 * e_scale);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);
   
   bt = elm_button_add(win);
   elm_object_text_set(bt, _("Set"));
   cfdata->ck = ck;
   evas_object_event_callback_add(bt, EVAS_CALLBACK_MOUSE_DOWN, _bt_time_apply, cfdata);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   evas_object_show(win);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd __UNUSED__,
                      Evas *evas,
                      E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ob, *tab, *of;
   E_Radio_Group *rg;
   char daynames[7][64];
   struct tm tm;
   int i;
   
   memset(&tm, 0, sizeof(struct tm));
   for (i = 0; i < 7; i++)
     {
        tm.tm_wday = i;
        strftime(daynames[i], sizeof(daynames[i]), "%A", &tm);
     }

   tab = e_widget_table_add(evas, 0);

   of = e_widget_frametable_add(evas, _("Clock"), 0);

   rg = e_widget_radio_group_new(&(cfdata->cfg.digital_clock));
   ob = e_widget_radio_add(evas, _("Analog"), 0, rg);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Digital"), 1, rg);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 0, 0);
   ob = e_widget_check_add(evas, _("Seconds"), &(cfdata->cfg.show_seconds));
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 1, 1, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->cfg.digital_24h));
   ob = e_widget_radio_add(evas, _("12 h"), 0, rg);
   e_widget_frametable_object_append(of, ob, 0, 3, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("24 h"), 1, rg);
   e_widget_frametable_object_append(of, ob, 0, 4, 1, 1, 1, 1, 0, 0);
   
   e_widget_table_object_append(tab, of, 0, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_frametable_add(evas, _("Date"), 0);
   
   rg = e_widget_radio_group_new(&(cfdata->cfg.show_date));
   ob = e_widget_radio_add(evas, _("None"), 0, rg);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Full"), 1, rg);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Numbers"), 2, rg);
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Date Only"), 3, rg);
   e_widget_frametable_object_append(of, ob, 0, 3, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("ISO 8601"), 4, rg);
   e_widget_frametable_object_append(of, ob, 0, 4, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Strftime"), 5, rg);
   e_widget_frametable_object_append(of, ob, 0, 5, 1, 1, 1, 1, 0, 0);
   ob = e_widget_entry_add(evas, &(cfdata->custom_dat), NULL, NULL, NULL);
   e_widget_frametable_object_append(of, ob, 0, 6, 1, 1, 1, 1, 0, 0); 

   e_widget_table_object_append(tab, of, 0, 1, 1, 1, 1, 1, 1, 1);
   
   of = e_widget_frametable_add(evas, _("Week"), 0);

   ob = e_widget_label_add(evas, _("Start"));
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 0, 1, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->cfg.week.start));
   for (i = 0; i < 7; i++)
     {
        ob = e_widget_radio_add(evas, daynames[i], i, rg);
        e_widget_frametable_object_append(of, ob, 0, i + 1, 1, 1, 1, 1, 0, 0);
     }

   e_widget_table_object_append(tab, of, 1, 0, 1, 2, 1, 1, 1, 1);
   
   of = e_widget_frametable_add(evas, _("Weekend"), 0);
   ob = e_widget_label_add(evas, _("Start"));
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 0, 1, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->cfg.weekend.start));
   for (i = 0; i < 7; i++)
     {
        ob = e_widget_radio_add(evas, daynames[i], i, rg);
        e_widget_frametable_object_append(of, ob, 0, i + 1, 1, 1, 1, 1, 0, 0);
     }

   ob = e_widget_label_add(evas, _("Days"));
   e_widget_frametable_object_append(of, ob, 1, 0, 1, 1, 0, 1, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->cfg.weekend.len));
   ob = e_widget_radio_add(evas, _("None"), 0, rg);
   e_widget_frametable_object_append(of, ob, 1, 1, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, "1", 1, rg);
   e_widget_frametable_object_append(of, ob, 1, 2, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, "2", 2, rg);
   e_widget_frametable_object_append(of, ob, 1, 3, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, "3", 3, rg);
   e_widget_frametable_object_append(of, ob, 1, 4, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, "4", 4, rg);
   e_widget_frametable_object_append(of, ob, 1, 5, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, "5", 5, rg);
   e_widget_frametable_object_append(of, ob, 1, 6, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, "6", 6, rg);
   e_widget_frametable_object_append(of, ob, 1, 7, 1, 1, 1, 1, 0, 0);

   e_widget_table_object_append(tab, of, 2, 0, 1, 2, 1, 1, 1, 1);

   of = e_widget_frametable_add(evas, _("Time set"), 0);
   
   ob = e_widget_button_add(evas, _("Set..."), "configure", show_time_cb, cfdata, NULL);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 0, 0);
   e_widget_table_object_append(tab, of, 3, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_frametable_add(evas, _("Date set"), 0);

   ob = e_widget_button_add(evas, _("Set... "), "configure", show_date_cb, cfdata, NULL);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 0, 0);
   e_widget_table_object_append(tab, of, 3, 1, 1, 1, 1, 1, 1, 1);

   return tab;
}

static void
_clock_date_time_set(E_Config_Dialog_Data *cfdata, Eina_Bool date)
{
   char pkexec_cmd[PATH_MAX];
   const char *cmd_sudo;
   char buf[4096];

   snprintf(pkexec_cmd, PATH_MAX, "pkexec env DISPLAY=%s XAUTHORITY=%s", getenv("DISPLAY"), getenv("XAUTHORITY"));
   cmd_sudo = eina_stringshare_add(pkexec_cmd);
   if (date)
     snprintf(buf, sizeof(buf), "%s %s %d%02d%02d", cmd_sudo,
              DATE_STRING_SET, cfdata->year, cfdata->month, cfdata->day);
   else
     snprintf(buf, sizeof(buf), "%s %s %0.0f:%0.0f:%0.0f", cmd_sudo,
              TIME_STRING_SET, cfdata->hour, cfdata->minute, cfdata->sec);
   e_util_exe_safe_run(buf, NULL);
   eina_stringshare_del(cmd_sudo);
}

static int
_basic_apply_data(E_Config_Dialog *cfd  __UNUSED__,
                  E_Config_Dialog_Data *cfdata)
{
   Config_Item *ci;

   ci = cfd->data;
   memcpy(ci, &(cfdata->cfg), sizeof(Config_Item));
   
   if (ci->custom_date_const) eina_stringshare_del(ci->custom_date_const);
   ci->custom_date_const = eina_stringshare_add(cfdata->custom_dat);
   
   ci->changed = EINA_TRUE;
   e_config_save_queue();
   e_int_clock_instances_redo(EINA_FALSE);
   ci->changed = EINA_FALSE;
   
   return 1;
}

