#include "e.h"
#include "e_mod_main.h"

struct _E_Config_Dialog_Data
{
   Config_Item cfg;
};

/* Protos */
static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog      *cfd,
                               E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog      *cfd,
                                          Evas                 *evas,
                                          E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog      *cfd,
                             E_Config_Dialog_Data *cfdata);

void
e_int_config_clock_module(E_Container *con, Config_Item *ci)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[4096];

   if (e_config_dialog_find("E", "utils/clock")) return;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;

   snprintf(buf, sizeof(buf), "%s/e-module-clock.edj",
            e_module_dir_get(clock_config->module));
   cfd = e_config_dialog_new(con,_("Clock Settings"),
                             "E", "utils/clock", buf, 0, v, ci);
   clock_config->config_dialog = cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   Config_Item *ci;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   ci = cfd->data;

   memcpy(&(cfdata->cfg), ci, sizeof(Config_Item));

   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd  __UNUSED__,
           E_Config_Dialog_Data *cfdata)
{
   clock_config->config_dialog = NULL;
   free(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog      *cfd __UNUSED__,
                      Evas                 *evas,
                      E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ob, *tab, *of;
   E_Radio_Group *rg;

   tab = e_widget_table_add(evas, 0);

   of = e_widget_frametable_add(evas, _("Clock"), 0);

   rg = e_widget_radio_group_new(&(cfdata->cfg.digital_clock));
   ob = e_widget_radio_add(evas, _("Analogue"), 0, rg);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Digital"), 1, rg);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 0, 0);
   ob = e_widget_label_add(evas, "");
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 0, 1, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->cfg.digital_24h));
   ob = e_widget_radio_add(evas, _("12HR"), 0, rg);
   e_widget_frametable_object_append(of, ob, 0, 3, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("24HR"), 1, rg);
   e_widget_frametable_object_append(of, ob, 0, 4, 1, 1, 1, 1, 0, 0);
   ob = e_widget_check_add(evas, _("Seconds"), &(cfdata->cfg.show_seconds));
   e_widget_frametable_object_append(of, ob, 0, 6, 1, 1, 1, 1, 0, 0);

   e_widget_table_object_append(tab, of, 0, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_frametable_add(evas, _("Week"), 0);

   ob = e_widget_label_add(evas, _("Start"));
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 0, 1, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->cfg.week.start));
   ob = e_widget_radio_add(evas, _("Sun"), 0, rg);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Mon"), 1, rg);
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Tue"), 2, rg);
   e_widget_frametable_object_append(of, ob, 0, 3, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Wed"), 3, rg);
   e_widget_frametable_object_append(of, ob, 0, 4, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Thu"), 4, rg);
   e_widget_frametable_object_append(of, ob, 0, 5, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Fri"), 5, rg);
   e_widget_frametable_object_append(of, ob, 0, 6, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Sat"), 6, rg);
   e_widget_frametable_object_append(of, ob, 0, 7, 1, 1, 1, 1, 0, 0);

   e_widget_table_object_append(tab, of, 1, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_frametable_add(evas, _("Weekend"), 0);

   ob = e_widget_label_add(evas, _("Start"));
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 0, 1, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->cfg.weekend.start));
   ob = e_widget_radio_add(evas, _("Sun"), 0, rg);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Mon"), 1, rg);
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Tue"), 2, rg);
   e_widget_frametable_object_append(of, ob, 0, 3, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Wed"), 3, rg);
   e_widget_frametable_object_append(of, ob, 0, 4, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Thu"), 4, rg);
   e_widget_frametable_object_append(of, ob, 0, 5, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Fri"), 5, rg);
   e_widget_frametable_object_append(of, ob, 0, 6, 1, 1, 1, 1, 0, 0);
   ob = e_widget_radio_add(evas, _("Sat"), 6, rg);
   e_widget_frametable_object_append(of, ob, 0, 7, 1, 1, 1, 1, 0, 0);

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

   e_widget_table_object_append(tab, of, 2, 0, 1, 1, 1, 1, 1, 1);
   return tab;
}

static int
_basic_apply_data(E_Config_Dialog *cfd  __UNUSED__,
                  E_Config_Dialog_Data *cfdata)
{
   Config_Item *ci;

   ci = cfd->data;
   memcpy(ci, &(cfdata->cfg), sizeof(Config_Item));
   e_int_clock_instances_redo();
   e_config_save_queue();
   return 1;
}

