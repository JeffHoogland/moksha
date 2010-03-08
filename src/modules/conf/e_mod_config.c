#include "e.h"
#include "e_mod_main.h"

struct _E_Config_Dialog_Data
{
   int menu_augmentation;
};

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

E_Config_Dialog *
e_int_config_conf_module(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd = NULL;
   E_Config_Dialog_View *v = NULL;
   char buf[PATH_MAX];

   /* is this config dialog already visible ? */
   if (e_config_dialog_find("Conf", "advanced/conf")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.check_changed = _basic_check_changed;

   snprintf(buf, sizeof(buf), "%s/e-module-conf.edj", conf->module->dir);
   cfd = e_config_dialog_new(con, _("Configuration Panel"), "Conf",
                             "advanced/conf", buf, 0, v, NULL);

   conf->cfd = cfd;
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;
   cfdata->menu_augmentation = conf->menu_augmentation;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   conf->cfd = NULL;
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ow;

   o = e_widget_list_add(evas, 0, 0);

   ow = e_widget_check_add(evas, _("Show configurations in menu"),
                           &(cfdata->menu_augmentation));
   e_widget_list_object_append(o, ow, 1, 0, 0.5);

   return o;
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   conf->menu_augmentation = cfdata->menu_augmentation;
   if (conf->aug)
     {
        e_int_menus_menu_augmentation_del("config/2", conf->aug);
        conf->aug = NULL;
     }

   if (conf->menu_augmentation)
     {
	conf->aug =
          e_int_menus_menu_augmentation_add
	  ("config/2", e_mod_config_menu_add, NULL, NULL, NULL);
     }

   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return (conf->menu_augmentation != cfdata->menu_augmentation);
}
