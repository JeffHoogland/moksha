#include "e.h"

/* PROTOTYPES - same all the time */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

/* Actual config data we will be playing with whil the dialog is active */
struct _E_Config_Dialog_Data
{
   int kill_if_close_not_possible;
   int kill_process;
   double kill_timer_wait;
   int ping_clients;
   int ping_clients_interval;
};

/* a nice easy setup function that does the dirty work */
E_Config_Dialog *
e_int_config_window_process(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "windows/window_process"))
     return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   /* methods */
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;
   v->basic.check_changed = _basic_check_changed;

   /* create config diaolg for NULL object/data */
   cfd = e_config_dialog_new(con, _("Window Process Management"),
			     "E", "windows/window_process",
			     "preferences-window-process", 0, v, NULL);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;
   cfdata->kill_if_close_not_possible = e_config->kill_if_close_not_possible;
   cfdata->kill_process = e_config->kill_process;
   cfdata->kill_timer_wait = e_config->kill_timer_wait;
   cfdata->ping_clients = e_config->ping_clients;
   cfdata->ping_clients_interval = e_config->ping_clients_interval;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfdata);
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   e_config->kill_if_close_not_possible = cfdata->kill_if_close_not_possible;
   e_config->kill_process = cfdata->kill_process;
   e_config->kill_timer_wait = cfdata->kill_timer_wait;
   e_config->ping_clients = cfdata->ping_clients;
   e_config->ping_clients_interval = cfdata->ping_clients_interval;
   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return ((e_config->kill_if_close_not_possible != cfdata->kill_if_close_not_possible) ||
           (e_config->kill_process != cfdata->kill_process) ||
           (e_config->kill_timer_wait != cfdata->kill_timer_wait) ||
           (e_config->ping_clients != cfdata->ping_clients) ||
           (e_config->ping_clients_interval != cfdata->ping_clients_interval));
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ob;

   o = e_widget_list_add(evas, 0, 0);

   ob = e_widget_check_add(evas, _("Kill process if unclosable"), &(cfdata->kill_if_close_not_possible));
   e_widget_list_object_append(o, ob, 1, 0, 0.5);
   ob = e_widget_check_add(evas, _("Kill process instead of client"), &(cfdata->kill_process));
   e_widget_list_object_append(o, ob, 1, 0, 0.5);
   ob = e_widget_label_add(evas, _("Kill timeout:"));
   e_widget_list_object_append(o, ob, 1, 0, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f s"), 1.0, 30.0, 1.0, 0, 
                            &(cfdata->kill_timer_wait), NULL, 100);
   e_widget_list_object_append(o, ob, 1, 0, 0.5);
   
   ob = e_widget_check_add(evas, _("Ping clients"), &(cfdata->ping_clients));
   e_widget_list_object_append(o, ob, 1, 0, 0.5);
   ob = e_widget_label_add(evas, _("Ping interval:"));
   e_widget_list_object_append(o, ob, 1, 0, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f ticks"), 1.0, 256.0, 1.0, 0, 
                            NULL, &(cfdata->ping_clients_interval), 100);
   e_widget_list_object_append(o, ob, 1, 0, 0.5);

   return o;
}
