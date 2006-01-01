#include "e.h"

#define FONT_CACHE_MAX (32 * 1024)
#define IMAGE_CACHE_MAX (256 * 1024)

typedef struct _CFData CFData;

static void        *_create_data(E_Config_Dialog *cfd);
static void        _free_data(E_Config_Dialog *cfd, CFData *cfdata);
static int         _basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);
static int         _advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);

static void _dialog_cb_ok(void *data, E_Dialog *dia);
static void _dialog_cb_cancel(void *data, E_Dialog *dia);

struct _CFData 
{
   int show_splash;
   double framerate;
   int use_e_cursor;
   int cursor_size;
 
   /* Advanced */
   int image_cache;
   int font_cache;
   int edje_cache;
   int edje_collection_cache;
 
   /* Not Implemented Yet
   char *transition_start;
   char *transition_desk;
   char *transition_change;
   int kill_if_close_not_possible;
   int kill_process;
   double kill_timer_wait;
   int ping_clients;
   double pint_clients_wait;
   double cache_flush_interval;
   */
};

E_Config_Dialog *
e_int_config_general(E_Container *con) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View v;
   
   v.create_cfdata = _create_data;
   v.free_cfdata = _free_data;
   v.basic.apply_cfdata = _basic_apply_data;
   v.basic.create_widgets = _basic_create_widgets;
   v.advanced.apply_cfdata = _advanced_apply_data;
   v.advanced.create_widgets = _advanced_create_widgets;
   
   cfd = e_config_dialog_new(con, _("General Settings"), NULL, 0, &v, NULL);
   return cfd;
}

static void
_fill_data(CFData *cfdata) 
{
   cfdata->show_splash = e_config->show_splash;
   cfdata->framerate = e_config->framerate;
   cfdata->use_e_cursor = e_config->use_e_cursor;
   cfdata->cursor_size = e_config->cursor_size;
   
   /* Advanced */
   cfdata->image_cache = e_config->image_cache;
   cfdata->font_cache = e_config->font_cache;
   cfdata->edje_cache = e_config->edje_cache;
   cfdata->edje_collection_cache = e_config->edje_collection_cache;
   
   /* Not Implemented Yet
   cfdata->transition_start = e_config->transition_start;
   cfdata->transition_desk = e_config->transition_desk;
   cfdata->transition_change = e_config->transition_change;
   */
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   CFData *cfdata;
   
   cfdata = E_NEW(CFData, 1);
   _fill_data(cfdata);   
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
   E_Action *a;
   int restart = 0;
   
   if (e_config->use_e_cursor != cfdata->use_e_cursor) restart = 1;
   if (e_config->cursor_size != cfdata->cursor_size) restart = 1;
   
   e_border_button_bindings_ungrab_all();
   e_config->show_splash = cfdata->show_splash;
   if (cfdata->framerate <= 0.0) cfdata->framerate = 1.0;
   e_config->framerate = cfdata->framerate;
   e_config->use_e_cursor = cfdata->use_e_cursor;
   /* Trap for idiots that may set cursor size == 0 */
   if (cfdata->cursor_size <= 0) cfdata->cursor_size = 1;
   e_config->cursor_size = cfdata->cursor_size;
   e_border_button_bindings_grab_all();
   e_config_save_queue();
   
   if (restart) 
     {
	E_Dialog *dia;
	
	dia = e_dialog_new(cfd->con);
	if (!dia) return 1;
	e_dialog_title_set(dia, _("Are you sure you want to restart ?"));
	e_dialog_text_set(dia, _("Your changes require Enlightenment to be restarted<br>before they can take effect.<br><br>Would you like to restart now ?"));
	e_dialog_icon_set(dia, "enlightenment/reset", 64);
	e_dialog_button_add(dia, _("Yes"), NULL, _dialog_cb_ok, NULL);
	e_dialog_button_add(dia, _("No"), NULL, _dialog_cb_cancel, NULL);
	e_dialog_button_focus_num(dia, 1);
	e_win_centered_set(dia->win, 1);
	e_dialog_show(dia);
     }   
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata) 
{
   Evas_Object *o, *of, *ob;
      
   o = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_check_add(evas, _("Show Splash Screen At Boot"), &(cfdata->show_splash));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Framerate"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 0.0, 200.0, 5.0, 0, &(cfdata->framerate), NULL, 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Mouse Pointer Settings"), 0);
   ob = e_widget_check_add(evas, _("Use E Mouse Pointer"), &(cfdata->use_e_cursor));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Mouse Pointer Size"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 0, 1024, 1, 0, NULL, &(cfdata->cursor_size), 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);   
   return o;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata) 
{
   _basic_apply_data(cfd, cfdata);
   
   e_border_button_bindings_ungrab_all();
   e_config->font_cache = cfdata->font_cache;
   e_config->image_cache = cfdata->image_cache;
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
   
   //_fill_data(cfdata);

   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_check_add(evas, _("Show Splash Screen At Boot"), &(cfdata->show_splash));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Framerate"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 0.0, 200.0, 5.0, 0, &(cfdata->framerate), NULL, 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Mouse Pointer Settings"), 0);
   ob = e_widget_check_add(evas, _("Use E Mouse Pointer"), &(cfdata->use_e_cursor));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Mouse Pointer Size"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 0, 1024, 1, 0, NULL, &(cfdata->cursor_size), 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);   

   of = e_widget_framelist_add(evas, _("Cache Settings"), 0);
   ob = e_widget_label_add(evas, _("Font Cache"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 0, FONT_CACHE_MAX, 100.0, 0, NULL, &(cfdata->font_cache), 200);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Image Cache"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 0, IMAGE_CACHE_MAX, 100.0, 0, NULL, &(cfdata->image_cache), 200);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Edje Cache"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 0, 256, 1, 0, NULL, &(cfdata->edje_cache), 200);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Edje Collection Cache"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 0, 512, 1, 0, NULL, &(cfdata->edje_collection_cache), 200);
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 1, 0.5);   
   return o;
}

static void
_dialog_cb_ok(void *data, E_Dialog *dia) 
{
   E_Action *a;

   e_object_del(E_OBJECT(dia));
   a = e_action_find("restart");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);   
}

static void
_dialog_cb_cancel(void *data, E_Dialog *dia) 
{
   e_object_del(E_OBJECT(dia));
}
