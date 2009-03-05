#include "e.h"

static void        *_create_data             (E_Config_Dialog *cfd);
static void         _free_data               (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data        (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets    (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _advanced_apply_data     (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

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

EAPI E_Config_Dialog *
e_int_config_exebuf(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_exebuf_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   cfd = e_config_dialog_new(con,
			     _("Run Command Settings"),
			    "E", "_config_exebuf_dialog",
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

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   e_config->exebuf_max_exe_list = cfdata->max_exe_list;
   e_config->exebuf_max_eap_list = cfdata->max_eap_list;   
   e_config->exebuf_max_hist_list = cfdata->max_hist_list;   
   e_config->exebuf_scroll_animate = cfdata->scroll_animate;   
   e_config->exebuf_scroll_speed = cfdata->scroll_speed;
   e_config->exebuf_pos_align_x = cfdata->pos_align_x;
   e_config->exebuf_pos_align_y = cfdata->pos_align_y;
   e_config->exebuf_pos_min_w = cfdata->pos_min_w;
   e_config->exebuf_pos_min_h = cfdata->pos_min_h;
   e_config->exebuf_pos_max_w = cfdata->pos_max_w;
   e_config->exebuf_pos_max_h = cfdata->pos_max_h;
   if (e_config->exebuf_term_cmd)
     eina_stringshare_del(e_config->exebuf_term_cmd);
   e_config->exebuf_term_cmd = NULL;
   if (cfdata->term_cmd) 
     e_config->exebuf_term_cmd = eina_stringshare_add(cfdata->term_cmd);
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *of, *ob, *ot;

   ot = e_widget_table_add(evas, 0);

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
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Scroll Settings"), 0);   
   ob = e_widget_check_add(evas, _("Scroll Animate"), &(cfdata->scroll_animate));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Scroll Speed"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f"), 0.0, 1.0, 0.01, 0, &(cfdata->scroll_speed), NULL, 200);
   e_widget_framelist_object_append(of, ob);   
   e_widget_table_object_append(ot, of, 0, 1, 1, 1, 1, 1, 1, 1);

   of = e_widget_frametable_add(evas, _("Terminal Settings"), 0);      
   ob = e_widget_label_add(evas, _("Terminal Command (CTRL+RETURN to utilize)"));
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 0, 1, 0);
//   e_widget_framelist_object_append(of, ob);
   ob = e_widget_entry_add(evas, &(cfdata->term_cmd), NULL, NULL, NULL);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 0, 1, 0);
//   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 2, 1, 1, 1, 1, 1, 1);

   of = e_widget_frametable_add(evas, _("Size Settings"), 0);
   ob = e_widget_label_add(evas, _("Minimum Width"));
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 1, 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%4.0f"), 0, 4000, 50, 0, NULL, &(cfdata->pos_min_w), 200);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 1, 0);
   ob = e_widget_label_add(evas, _("Minimum Height"));
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 1, 1, 1, 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%4.0f"), 0, 4000, 50, 0, NULL, &(cfdata->pos_min_h), 200);
   e_widget_frametable_object_append(of, ob, 0, 3, 1, 1, 1, 1, 1, 0);
   ob = e_widget_label_add(evas, _("Maximum Width"));
   e_widget_frametable_object_append(of, ob, 0, 4, 1, 1, 1, 1, 1, 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%4.0f"), 0, 4000, 50, 0, NULL, &(cfdata->pos_max_w), 200);
   e_widget_frametable_object_append(of, ob, 0, 5, 1, 1, 1, 1, 1, 0);   
   ob = e_widget_label_add(evas, _("Maximum Height"));
   e_widget_frametable_object_append(of, ob, 0, 6, 1, 1, 1, 1, 1, 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%4.0f"), 0, 4000, 50, 0, NULL, &(cfdata->pos_max_h), 200);
   e_widget_frametable_object_append(of, ob, 0, 7, 1, 1, 1, 1, 1, 0);
   e_widget_table_object_append(ot, of, 1, 0, 1, 2, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Position Settings"), 0);   
   ob = e_widget_label_add(evas, _("X-Axis Alignment"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f"), 0.0, 1.0, 0.01, 0, &(cfdata->pos_align_x), NULL, 200);
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_label_add(evas, _("Y-Axis Alignment"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f"), 0.0, 1.0, 0.01, 0, &(cfdata->pos_align_y), NULL, 200);
   e_widget_framelist_object_append(of, ob);   
   e_widget_table_object_append(ot, of, 1, 2, 1, 1, 1, 1, 1, 1);

   return ot;
}
