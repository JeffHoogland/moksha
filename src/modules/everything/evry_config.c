#include "e.h"
#include "e_mod_main.h"
#include "evry.h"


static void        *_create_data             (E_Config_Dialog *cfd);
static void         _free_data               (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data        (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets    (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);


struct _E_Config_Dialog_Data 
{
  /* Basic */
  int max_exe_list;
  int max_eap_list;
  int max_hist_list;
  int scroll_animate;
  /* Advanced */
  double scroll_speed;
  double pos_align_x;
  double pos_align_y;
  double pos_size_w;
  double pos_size_h;
  int pos_min_w;
  int pos_min_h;
  int pos_max_w;
  int pos_max_h;
  char *term_cmd;
};


E_Config_Dialog *
e_int_config_exebuf(E_Container *con, const char *params __UNUSED__) 
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
   /* Basic */
   cfdata->max_exe_list = e_config->exebuf_max_exe_list;
   cfdata->max_eap_list = e_config->exebuf_max_eap_list;
   cfdata->max_hist_list = e_config->exebuf_max_hist_list;
   cfdata->scroll_animate = e_config->exebuf_scroll_animate;
   /* Advanced */
   cfdata->scroll_speed = e_config->exebuf_scroll_speed;
   cfdata->pos_align_x = e_config->exebuf_pos_align_x;
   cfdata->pos_align_y = e_config->exebuf_pos_align_y;
   cfdata->pos_size_w = e_config->exebuf_pos_size_w;
   cfdata->pos_size_h = e_config->exebuf_pos_size_h;
   cfdata->pos_min_w = e_config->exebuf_pos_min_w;
   cfdata->pos_min_h = e_config->exebuf_pos_min_h;
   cfdata->pos_max_w = e_config->exebuf_pos_max_w;
   cfdata->pos_max_h = e_config->exebuf_pos_max_h;
   if (e_config->exebuf_term_cmd)
     cfdata->term_cmd = strdup(e_config->exebuf_term_cmd);
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
   scroll_list = eina_list_free(scroll_list);

   E_FREE(cfdata->term_cmd);
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   e_config->exebuf_max_exe_list = cfdata->max_exe_list;
   e_config->exebuf_max_eap_list = cfdata->max_eap_list;
   e_config->exebuf_max_hist_list = cfdata->max_hist_list;
   e_config->exebuf_scroll_animate = cfdata->scroll_animate;
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   
   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_label_add(evas, _("Maximum Number of Matched Apps to List"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 10, 50, 5, 0, NULL, &(cfdata->max_eap_list), 200);
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_label_add(evas, _("Maximum Number of Matched Exes to List"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 10, 50, 5, 0, NULL, &(cfdata->max_exe_list), 200);
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_label_add(evas, _("Maximum History to List"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 10, 200, 5, 0, NULL, &(cfdata->max_hist_list), 200);
   e_widget_framelist_object_append(of, ob);   
   e_widget_list_object_append(o, of, 1, 1, 0.5);   
   
   of = e_widget_framelist_add(evas, _("Scroll Settings"), 0);
   ob = e_widget_check_add(evas, _("Scroll Animate"), &(cfdata->scroll_animate));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   return o;
}
