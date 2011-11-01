#include <e.h>
#include "e_mod_main.h"

struct _E_Config_Dialog_Data
{
   int show_all;
   int minw, minh;
};

/* Protos */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

void
_config_tasks_module(Config_Item *ci)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   E_Container *con;

   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;

   con = e_container_current_get(e_manager_current_get());
   cfd = e_config_dialog_new(con, _("Tasks Configuration"), "Tasks", 
                             "_e_modules_tasks_config_dialog", NULL, 0, v, ci);
   if (tasks_config->config_dialog)
     e_object_del(E_OBJECT(tasks_config->config_dialog));
   tasks_config->config_dialog = cfd;
}

static void
_fill_data(Config_Item *ci, E_Config_Dialog_Data *cfdata)
{
   cfdata->show_all = ci->show_all;
   cfdata->minw = ci->minw;
   cfdata->minh = ci->minh;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   Config_Item *ci;

   ci = cfd->data;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(ci, cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (!tasks_config) return;
   tasks_config->config_dialog = NULL;
   free(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob, *ow;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Display"), 0);
   ob = e_widget_check_add(evas, _("Show windows from all desktops"),
                           &(cfdata->show_all));
   e_widget_framelist_object_append(of, ob);
   ow = e_widget_label_add(evas, _("Minimum Width"));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f px"), 20, 420, 1, 0,
                            NULL, &(cfdata->minw), 100);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_label_add(evas, _("Minimum Height"));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f px"), 20, 420, 1, 0,
                            NULL, &(cfdata->minh), 100);
   e_widget_framelist_object_append(of, ow);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   return o;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Config_Item *ci;

   ci = cfd->data;
   ci->show_all = cfdata->show_all;
   ci->minw = cfdata->minw;
   ci->minh = cfdata->minh;
   e_config_save_queue();
   _tasks_config_updated(ci);
   return 1;
}
