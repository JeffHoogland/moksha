#include "e.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void        _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int         _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int         _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data 
{
   double framerate;
   int priority;

   /* Advanced */   
   int cache_flush_poll_interval;
   double font_cache;
   double image_cache;
   int edje_cache;
   int edje_collection_cache;
};

EAPI E_Config_Dialog *
e_int_config_performance(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_performance_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   
   cfd = e_config_dialog_new(con,
			     _("Performance Settings"),
			    "E", "_config_performance_dialog",
			     "preferences-system-performance", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   cfdata->framerate = e_config->framerate;
   cfdata->priority = e_config->priority;
   cfdata->font_cache = ((double)e_config->font_cache / 1024);
   cfdata->image_cache = ((double)e_config->image_cache / 1024);
   cfdata->edje_cache = e_config->edje_cache;
   cfdata->edje_collection_cache = e_config->edje_collection_cache;
   cfdata->cache_flush_poll_interval = e_config->cache_flush_poll_interval;
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
   if (cfdata->framerate <= 0.0) cfdata->framerate = 1.0;
   e_config->framerate = cfdata->framerate;
   edje_frametime_set(1.0 / e_config->framerate);
   e_config->priority = cfdata->priority;
   ecore_exe_run_priority_set(e_config->priority);
   e_canvas_recache();
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   
   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_label_add(evas, _("Framerate"));
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f fps"), 5.0, 200.0, 5.0, 0, &(cfdata->framerate), NULL, 150);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(evas, _("Application Priority"));
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, "%1.0f", 0, 19, 1, 0, NULL, &(cfdata->priority), 150);
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 1, 0.5);   
   return o;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (cfdata->framerate <= 0.0) cfdata->framerate = 1.0;
   e_config->framerate = cfdata->framerate;
   e_config->cache_flush_poll_interval = cfdata->cache_flush_poll_interval;   
   e_config->font_cache = (cfdata->font_cache * 1024);
   e_config->image_cache = (cfdata->image_cache * 1024);
   e_config->edje_cache = cfdata->edje_cache;
   e_config->edje_collection_cache = cfdata->edje_collection_cache;
   edje_frametime_set(1.0 / e_config->framerate);
   e_config->priority = cfdata->priority;
   ecore_exe_run_priority_set(e_config->priority);
   e_canvas_recache();
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *ob, *of;

   o = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_label_add(evas, _("Framerate"));
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f fps"), 5.0, 240.0, 1.0, 0, &(cfdata->framerate), NULL, 150);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(evas, _("Application Priority"));
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, "%1.0f", 0, 19, 1, 0, NULL, &(cfdata->priority), 150);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);   

   of = e_widget_framelist_add(evas, _("Cache Settings"), 0);   
   ob = e_widget_label_add(evas, _("Cache Flush Interval"));
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f ticks"), 8, 4096, 8, 0, NULL, &(cfdata->cache_flush_poll_interval), 150);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(evas, _("Size Of Font Cache"));
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.1f MB"), 0, 4, 0.1, 0, &(cfdata->font_cache), NULL, 150);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(evas, _("Size Of Image Cache"));
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f MB"), 0, 32, 1, 0, &(cfdata->image_cache), NULL, 150);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(evas, _("Number Of Edje Files To Cache"));
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f files"), 0, 256, 1, 0, NULL, &(cfdata->edje_cache), 150);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(evas, _("Number Of Edje Collections To Cache"));
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f collections"), 0, 512, 1, 0, NULL, &(cfdata->edje_collection_cache), 150);
   e_widget_framelist_object_append(of, ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);   
   return o;
}
