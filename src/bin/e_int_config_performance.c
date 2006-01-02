#include "e.h"

typedef struct _CFData CFData;

static void        *_create_data(E_Config_Dialog *cfd);
static void        _free_data(E_Config_Dialog *cfd, CFData *cfdata);
static int         _basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);
static int         _advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);

struct _CFData 
{
   double framerate;

   /* Advanced */   
   double cache_flush_interval;
   int font_cache;
   int image_cache;
   int edje_cache;
   int edje_collection_cache;
};

E_Config_Dialog *
e_int_config_performance(E_Container *con) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View v;
   
   v.create_cfdata = _create_data;
   v.free_cfdata = _free_data;
   v.basic.apply_cfdata = _basic_apply_data;
   v.basic.create_widgets = _basic_create_widgets;
   v.advanced.apply_cfdata = _advanced_apply_data;
   v.advanced.create_widgets = _advanced_create_widgets;
   
   cfd = e_config_dialog_new(con, _("Performance Settings"), NULL, 0, &v, NULL);
   return cfd;
}

static void
_fill_data(CFData *cfdata) 
{
   cfdata->framerate = e_config->framerate;
   cfdata->font_cache = (e_config->font_cache / 1024);
   cfdata->image_cache = (e_config->image_cache / 1024);
   cfdata->edje_cache = e_config->edje_cache;
   cfdata->edje_collection_cache = e_config->edje_collection_cache;
   cfdata->cache_flush_interval = e_config->cache_flush_interval;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   CFData *cfdata;
   
   cfdata = E_NEW(CFData, 1);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, CFData *cfdata) 
{
   free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata) 
{   
   e_border_button_bindings_ungrab_all();
   if (cfdata->framerate <= 0.0) cfdata->framerate = 1.0;
   e_config->framerate = cfdata->framerate;
   e_border_button_bindings_grab_all();
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata) 
{
   Evas_Object *o, *of, *ob;

   _fill_data(cfdata);
   
   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_label_add(evas, _("Framerate"));
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f fps"), 0.0, 200.0, 5.0, 0, &(cfdata->framerate), NULL, 150);
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 1, 0.5);   
   return o;
}


static int
_advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata) 
{
   e_border_button_bindings_ungrab_all();
   if (cfdata->framerate <= 0.0) cfdata->framerate = 1.0;
   e_config->framerate = cfdata->framerate;
   e_config->cache_flush_interval = cfdata->cache_flush_interval;   
   e_config->font_cache = (cfdata->font_cache * 1024);
   e_config->image_cache = (cfdata->image_cache * 1024);
   e_config->edje_cache = cfdata->edje_cache;
   e_config->edje_collection_cache = cfdata->edje_collection_cache;
   e_border_button_bindings_grab_all();
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata) 
{
   Evas_Object *o, *ob, *of;
   
   _fill_data(cfdata);

   o = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_label_add(evas, _("Framerate"));
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f fps"), 0.0, 200.0, 5.0, 0, &(cfdata->framerate), NULL, 150);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);   

   of = e_widget_framelist_add(evas, _("Cache Settings"), 0);   
   ob = e_widget_label_add(evas, _("Cache Flush Interval"));
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f seconds"), 0.0, 600.0, 1.0, 0, &(cfdata->cache_flush_interval), NULL, 150);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(evas, _("Font Cache"));
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f MB"), 0, 32, 1, 0, NULL, &(cfdata->font_cache), 150);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(evas, _("Image Cache"));
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f MB"), 0, 256, 1, 0, NULL, &(cfdata->image_cache), 150);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(evas, _("Edje Cache"));
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f MB"), 0, 256, 1, 0, NULL, &(cfdata->edje_cache), 150);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(evas, _("Edje Collection Cache"));
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f MB"), 0, 512, 1, 0, NULL, &(cfdata->edje_collection_cache), 150);
   e_widget_framelist_object_append(of, ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);   
   return o;
}
