#include "e.h"
#include "e_mod_main.h"

struct _E_Config_Dialog_Data
{
   int poll_interval;
   int restore_governor;
   int auto_powersave;
   char *powersave_governor;
   char *governor;
   int pstate_min;
   int pstate_max;
};

/* Protos */
static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

E_Config_Dialog *
e_int_config_cpufreq_module(E_Container *parent __UNUSED__, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[PATH_MAX];

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->basic.check_changed = _basic_check_changed;

   snprintf(buf, sizeof(buf), "%s/e-module-cpufreq.edj",
            e_module_dir_get(cpufreq_config->module));
   cfd = e_config_dialog_new(NULL, _("Cpu Frequency Control Settings"),
                             "E", "_e_mod_cpufreq_config_dialog",
                             buf, 0, v, NULL);
   cpufreq_config->config_dialog = cfd;
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   if (!cpufreq_config) return;
   cfdata->poll_interval = cpufreq_config->poll_interval;
   cfdata->restore_governor = cpufreq_config->restore_governor;
   cfdata->auto_powersave = cpufreq_config->auto_powersave;
   cfdata->pstate_min = cpufreq_config->pstate_min - 1;
   cfdata->pstate_max = cpufreq_config->pstate_max - 1;
   if (cpufreq_config->powersave_governor)
     cfdata->powersave_governor = strdup(cpufreq_config->powersave_governor);
   if (cpufreq_config->governor)
     cfdata->governor = strdup(cpufreq_config->governor);
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (!cpufreq_config) return;
   cpufreq_config->config_dialog = NULL;
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ob, *of;
   E_Radio_Group *rg;
   
   o = e_widget_list_add(evas, 0, 0);
   ob = e_widget_check_add(evas, _("Restore CPU Power Policy"), &cfdata->restore_governor);
   e_widget_list_object_append(o, ob, 1, 0, 0.5);
   
   ob = e_widget_check_add(evas, _("Automatic powersaving"), &cfdata->auto_powersave);
   e_widget_list_object_append(o, ob, 1, 0, 0.5);
   
   of = e_widget_framelist_add(evas, _("Update poll interval"), 0);
   rg = e_widget_radio_group_new(&cfdata->poll_interval);
   ob = e_widget_radio_add(evas, _("Fast (4 ticks)"), 4, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Medium (8 ticks)"), 8, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Normal (32 ticks)"), 32, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Slow (64 ticks)"), 64, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Very Slow (256 ticks)"), 256, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 0, 0.5);

   if ((cpufreq_config->status) && (cpufreq_config->status->pstate))
     {
        ob = e_widget_label_add(evas, _("Minimum Power State"));
        e_widget_list_object_append(o, ob, 1, 0, 0.5);
        ob = e_widget_slider_add(evas, 1, 0, _("%3.0f"), 0, 100,
                                 1, 0, NULL, &cfdata->pstate_min, 100);
        e_widget_list_object_append(o, ob, 1, 0, 0.5);

        ob = e_widget_label_add(evas, _("Maximum Power State"));
        e_widget_list_object_append(o, ob, 1, 0, 0.5);
        ob = e_widget_slider_add(evas, 1, 0, _("%3.0f"), 2, 100,
                                 1, 0, NULL, &cfdata->pstate_max, 100);
        e_widget_list_object_append(o, ob, 1, 0, 0.5);
     }
   else
     {
        // XXX: list governors
     }
   return o;
}

static int
_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (!cpufreq_config) return 0;
   cpufreq_config->poll_interval = cfdata->poll_interval;
   cpufreq_config->restore_governor = cfdata->restore_governor;
   cpufreq_config->auto_powersave = cfdata->auto_powersave;
   cpufreq_config->pstate_min = cfdata->pstate_min + 1;
   cpufreq_config->pstate_max = cfdata->pstate_max + 1;
   eina_stringshare_replace(&cpufreq_config->powersave_governor, cfdata->powersave_governor);
   eina_stringshare_replace(&cpufreq_config->governor, cfdata->governor);
   _cpufreq_poll_interval_update();
   if (cpufreq_config->governor)
     _cpufreq_set_governor(cpufreq_config->governor);
   if (cpufreq_config->pstate_max < cpufreq_config->pstate_min)
     cpufreq_config->pstate_max = cpufreq_config->pstate_min;
   if (cpufreq_config->pstate_min > cpufreq_config->pstate_max)
     cpufreq_config->pstate_min = cpufreq_config->pstate_max;
   _cpufreq_set_pstate(cpufreq_config->pstate_min - 1,
                       cpufreq_config->pstate_max - 1);
   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata __UNUSED__)
{
   Eina_Bool ret = EINA_TRUE;
   return ret;
}
