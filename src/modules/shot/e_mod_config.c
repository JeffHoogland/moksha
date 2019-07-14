#include <e.h>
#include "e_mod_main.h"

/* The typedef for this structure is declared inside the E code in order to
 * allow everybody to use this type, you dont need to declare the typedef, 
 * just use the E_Config_Dialog_Data for your data structures declarations */
struct _E_Config_Dialog_Data 
{
   int view_enable, notify, full_dialog, mode_dialog; 
   double delay, pict_quality;
   char *viewer;
   char *path;
   
};

/* Local Function Prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

/* External Functions */

/* Function for calling our personal dialog menu */
E_Config_Dialog *
e_int_config_shot_module(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd = NULL;
   E_Config_Dialog_View *v = NULL;
   char buf[4096];

   /* is this config dialog already visible ? */
   
  
   if (e_config_dialog_find("E", "extensions/takescreenshot")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   /* Icon in the theme */
   snprintf(buf, sizeof(buf), "%s/e-module-shot.edj", shot_conf->module->dir);

   /* create our config dialog */
   
 
   
   cfd = e_config_dialog_new(con, _("Screenshot Settings"), "E", 
                             "extensions/takescreenshot", buf, 0, v, NULL);

   e_dialog_resizable_set(cfd->dia, 0);
   shot_conf->cfd = cfd;
   return cfd;
}

/* Local Functions */
static void *
_create_data(E_Config_Dialog *cfd __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata = NULL;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
   shot_conf->cfd = NULL;
   E_FREE(cfdata->viewer);
   E_FREE(cfdata->path);
   E_FREE(cfdata);
}

static void 
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   /* load a temp copy of the config variables */

   cfdata->view_enable = shot_conf->view_enable;
   if (shot_conf->viewer) cfdata->viewer = strdup(shot_conf->viewer);
   if (shot_conf->path) cfdata->path = strdup(shot_conf->path);
   cfdata->delay = shot_conf->delay;
   cfdata->pict_quality = shot_conf->pict_quality;
   cfdata->notify = shot_conf->notify;
   cfdata->full_dialog = shot_conf->full_dialog;
   cfdata->mode_dialog = shot_conf->mode_dialog;

}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o = NULL, *of = NULL, *ow = NULL;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Settings"), 0);
   
   ow = e_widget_check_add(evas, _(" Show the screenshot mode dialog"), &(cfdata->mode_dialog));
   e_widget_framelist_object_append(of, ow);
   
   ow = e_widget_check_add(evas, _(" Screenshot guide (quality/share/save)"), &(cfdata->full_dialog));
   e_widget_framelist_object_append(of, ow);
   
   ow = e_widget_check_add(evas, _(" Show notifications"), &(cfdata->notify));
   e_widget_framelist_object_append(of, ow);
   
   ow = e_widget_check_add(evas, _(" Launch app after screenshot"), &(cfdata->view_enable));
   e_widget_framelist_object_append(of, ow);
      
   ow = e_widget_label_add(evas, _("Application for opening:"));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_entry_add(evas, &cfdata->viewer, NULL, NULL, NULL);
   e_widget_size_min_set(ow, 60, 28);
   e_widget_framelist_object_append(of, ow);
   
   ow = e_widget_label_add(evas, _("Folder for saving screenshots:"));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_entry_add(evas, &cfdata->path, NULL, NULL, NULL);
   e_widget_size_min_set(ow, 60, 28);
   e_widget_framelist_object_append(of, ow);
   
   ow = e_widget_label_add(evas, _("Delay time [s]"));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_slider_add(evas, 1, 0, "%1.0f", 0.0, 10.0, 1.0, 0, &(cfdata->delay), NULL, 100);
   e_widget_framelist_object_append(of, ow);
   
   ow = e_widget_label_add(evas, _("Image quality [50-90 = jpg, 100 = png]"));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_slider_add(evas, 1, 0, "%1.0f", 50.0, 100.0, 10.0, 0, &(cfdata->pict_quality), NULL, 100);
   e_widget_framelist_object_append(of, ow);
   
   e_widget_list_object_append(o, of, 1, 0, 0.5);
   
   of = e_widget_framelist_add(evas, _("Info"), 0);   
   ow = e_widget_label_add(evas, _("Bind Take Shot in Wndows:Action"));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_label_add(evas, _("for window screenshot!"));
   e_widget_framelist_object_append(of, ow);
   
   e_widget_list_object_append(o, of, 1, 0, 0.5);

   return o;
}

static int 
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
  
   if (shot_conf->viewer) eina_stringshare_del( shot_conf->viewer);
   if (cfdata->viewer)
      shot_conf->viewer = eina_stringshare_add(cfdata->viewer);
  
   if (shot_conf->path) eina_stringshare_del(shot_conf->path);
   if (cfdata->path) 
     shot_conf->path = eina_stringshare_add(cfdata->path);
   else
     shot_conf->path = eina_stringshare_add(e_user_homedir_get());
  
  
   shot_conf->notify = cfdata->notify;
   shot_conf->view_enable = cfdata->view_enable;
   shot_conf->delay = cfdata->delay;
   shot_conf->pict_quality = cfdata->pict_quality;
   shot_conf->full_dialog = cfdata->full_dialog;
   shot_conf->mode_dialog = cfdata->mode_dialog;
   e_config_save_queue();
   
   return 1;
}
