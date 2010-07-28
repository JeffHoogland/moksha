#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data
{
   double framerate;
   int priority;
   int cache_flush_poll_interval;
   double font_cache;
   double image_cache;
   int edje_cache;
   int edje_collection_cache;
};

E_Config_Dialog *
e_int_config_performance(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "advanced/performance")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;
   v->basic.check_changed = _basic_check_changed;

   cfd = e_config_dialog_new(con, _("Performance Settings"),
			     "E", "advanced/performance",
			     "preferences-system-performance", 0, v, NULL);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;
   cfdata->framerate = e_config->framerate;
   cfdata->priority = e_config->priority;
   cfdata->font_cache = ((double)e_config->font_cache / 1024);
   cfdata->image_cache = ((double)e_config->image_cache / 1024);
   cfdata->edje_cache = e_config->edje_cache;
   cfdata->edje_collection_cache = e_config->edje_collection_cache;
   cfdata->cache_flush_poll_interval = e_config->cache_flush_poll_interval;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfdata);
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
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

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->framerate <= 0.0) cfdata->framerate = 1.0;
   return ((e_config->framerate != cfdata->framerate) ||
	   (e_config->cache_flush_poll_interval != cfdata->cache_flush_poll_interval) ||
	   (e_config->font_cache != (cfdata->font_cache * 1024)) ||
	   (e_config->image_cache != (cfdata->image_cache * 1024)) ||
	   (e_config->edje_cache != cfdata->edje_cache) ||
	   (e_config->edje_collection_cache != cfdata->edje_collection_cache) ||
	   (e_config->priority != cfdata->priority));
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *otb, *ob, *ol;

   otb = e_widget_toolbook_add(evas, (24 * e_scale), (24 * e_scale));

   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_label_add(evas, _("Framerate"));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f fps"), 5.0, 200.0, 1.0, 0, 
                            &(cfdata->framerate), NULL, 100);
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   ob = e_widget_label_add(evas, _("Applications priority"));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, "%1.0f", 0, 19, 1, 0, NULL, 
                            &(cfdata->priority), 100);
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("General"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_label_add(evas, _("Cache flush interval"));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f ticks"), 8, 4096, 8, 0, NULL,
                            &(cfdata->cache_flush_poll_interval), 100);
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   ob = e_widget_label_add(evas, _("Font cache size"));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.1f MB"), 0, 4, 0.1, 0,
                            &(cfdata->font_cache), NULL, 100);
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   ob = e_widget_label_add(evas, _("Image cache size"));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f MB"), 0, 32, 1, 0,
                            &(cfdata->image_cache), NULL, 100);
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("Caches"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_label_add(evas, _("Number of Edje files to cache"));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f files"), 0, 256, 1, 0, NULL,
                            &(cfdata->edje_cache), 100);
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   ob = e_widget_label_add(evas, _("Number of Edje collections to cache"));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f collections"), 0, 512, 1, 0,
                            NULL, &(cfdata->edje_collection_cache), 100);
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Edje Cache"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);

   return otb;
}
