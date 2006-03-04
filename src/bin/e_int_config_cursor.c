#include "e.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void        _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int         _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int         _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void         _cursor_check_toggle(void *data, Evas_Object *obj, const char *emission, const char *source);

struct _E_Config_Dialog_Data 
{
   int use_e_cursor;

   /* Advanced */
   int cursor_size;
};

typedef struct _E_Widget_Check_Data E_Widget_Check_Data;
struct _E_Widget_Check_Data 
{
   Evas_Object *o_check;
   int *valptr;
};

EAPI E_Config_Dialog *
e_int_config_cursor(E_Container *con) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   
   cfd = e_config_dialog_new(con, _("Cursor Settings"), NULL, 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   cfdata->use_e_cursor = e_config->use_e_cursor;
   cfdata->cursor_size = e_config->cursor_size;
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
   free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   int changed = 0;
   
   if (e_config->use_e_cursor != cfdata->use_e_cursor) changed = 1;
   
   e_border_button_bindings_ungrab_all();
   e_config->use_e_cursor = cfdata->use_e_cursor;
   e_border_button_bindings_grab_all();
   e_config_save_queue();
   
   if (changed) 
     {
	Evas_List *l;
	
	for (l = e_manager_list(); l; l = l->next) 
	  {
	     E_Manager *man;
	     man = l->data;
	     if (man->pointer) e_object_del(E_OBJECT(man->pointer));
	     man->pointer = e_pointer_window_new(man->root);
	  }
     }   
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   E_Widget_Check_Data *wd;
   
   o = e_widget_list_add(evas, 0, 0);
      
   of = e_widget_framelist_add(evas, _("Cursor Settings"), 0);
   ob = e_widget_check_add(evas, _("Use Enlightenment Cursor"), &(cfdata->use_e_cursor));
   wd = e_widget_data_get(ob);
   edje_object_signal_callback_add(wd->o_check, "toggle_on", "", _cursor_check_toggle, ob);
   edje_object_signal_callback_add(wd->o_check, "toggle_off", "", _cursor_check_toggle, ob);
   
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);   

   return o;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   int changed = 0;
   
   if (e_config->use_e_cursor != cfdata->use_e_cursor) changed = 1;
   if (e_config->cursor_size != cfdata->cursor_size) changed = 1;
   
   e_border_button_bindings_ungrab_all();
   e_config->use_e_cursor = cfdata->use_e_cursor;
   if (cfdata->cursor_size <= 0) cfdata->cursor_size = 1;
   e_config->cursor_size = cfdata->cursor_size;
   
   e_border_button_bindings_grab_all();
   e_config_save_queue();
   if (changed) 
     {
	Evas_List *l;
	
	e_pointers_size_set(e_config->cursor_size);
	for (l = e_manager_list(); l; l = l->next) 
	  {
	     E_Manager *man;
	     man = l->data;
	     if (man->pointer) e_object_del(E_OBJECT(man->pointer));
	     man->pointer = e_pointer_window_new(man->root);
	  }	
     }   
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *ob, *of;
   E_Widget_Check_Data *wd;
      
   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Cursor Settings"), 0);
   ob = e_widget_check_add(evas, _("Use Enlightenment Cursor"), &(cfdata->use_e_cursor));
   wd = e_widget_data_get(ob);
   edje_object_signal_callback_add(wd->o_check, "toggle_on", "", _cursor_check_toggle, ob);
   edje_object_signal_callback_add(wd->o_check, "toggle_off", "", _cursor_check_toggle, ob);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Cursor Size"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f pixels"), 8, 128, 4, 0, NULL, &(cfdata->cursor_size), 150);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);   
   return o;
}

static void 
_cursor_check_toggle(void *data, Evas_Object *obj, const char *emission, const char *source) 
{
   E_Widget_Check_Data *wd;
   Evas_Object *o;
   
   if (!(o = data)) return;
   wd = e_widget_data_get(o);
   if (!wd->valptr)
     return;

   if (*(wd->valptr) == 0)
     edje_object_part_text_set(wd->o_check, "label", _("Use X Cursor"));
   else
     edje_object_part_text_set(wd->o_check, "label", _("Use Enlightenment Cursor"));
}
