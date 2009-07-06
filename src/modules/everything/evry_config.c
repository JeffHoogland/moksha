#include "e.h"
#include "e_mod_main.h"
#include "evry.h"

/* typedef struct _E_Config_Dialog_Data E_Config_Dialog_Data; */

static void        *_create_data             (E_Config_Dialog *cfd);
static void         _free_data               (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data        (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets    (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);


struct _E_Config_Dialog_Data 
{
  int width, height;

  int scroll_animate;
};


EAPI E_Config_Dialog *
evry_config_dialog(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_everything_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = NULL;
   v->advanced.create_widgets = NULL;
   cfd = e_config_dialog_new(con,
			     _("Everything Settings"),
			     "E", "_config_everything_dialog",
			     "system-run", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   cfdata->scroll_animate = evry_conf->scroll_animate;
   cfdata->height = evry_conf->height;
   cfdata->width = evry_conf->width;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   evry_conf->width = cfdata->width;
   evry_conf->height = cfdata->height;
   evry_conf->scroll_animate = cfdata->scroll_animate;

   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   
   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_label_add(evas, _("Popup Width"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 300, 800, 5, 0, NULL, &(cfdata->width), 200);
   e_widget_framelist_object_append(of, ob);   

   ob = e_widget_label_add(evas, _("Popup Height"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 200, 800, 5, 0, NULL, &(cfdata->height), 200);
   e_widget_framelist_object_append(of, ob);   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Scroll Settings"), 0);
   ob = e_widget_check_add(evas, _("Scroll Animate"), &(cfdata->scroll_animate));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   return o;
}
