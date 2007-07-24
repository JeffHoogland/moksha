#include "e.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void        _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int         _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int         _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data 
{
   int use_e_cursor;

   /* Advanced */
   int cursor_size;
};

EAPI E_Config_Dialog *
e_int_config_cursor(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_cursor_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   
   cfd = e_config_dialog_new(con,
			     _("Cursor Settings"),
			     "E", "_config_cursor_dialog",
			     "enlightenment/mouse", 0, v, NULL);
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
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   int changed = 0;
   
   if (e_config->use_e_cursor != cfdata->use_e_cursor) changed = 1;
   
   e_config->use_e_cursor = cfdata->use_e_cursor;
   e_config_save_queue();
   
   if (changed) 
     {
	Evas_List *l;
	
	for (l = e_manager_list(); l; l = l->next) 
	  {
	     E_Manager *man;
	     man = l->data;
	     if (man->pointer) e_object_del(E_OBJECT(man->pointer));
	     man->pointer = e_pointer_window_new(man->root, 1);
	  }
     }   
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   
   o = e_widget_list_add(evas, 0, 0);
      
   of = e_widget_framelist_add(evas, _("Cursor Settings"), 0);
   rg = e_widget_radio_group_new(&cfdata->use_e_cursor);
   ob = e_widget_radio_add(evas, _("Use Enlightenment Cursor"), 1, rg);   
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Use X Cursor"), 0, rg);   
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
   
   e_config->use_e_cursor = cfdata->use_e_cursor;
   if (cfdata->cursor_size <= 0) cfdata->cursor_size = 1;
   e_config->cursor_size = cfdata->cursor_size;
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
	     man->pointer = e_pointer_window_new(man->root, 1);
	  }	
     }   
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *ob, *of;
   E_Radio_Group *rg;
      
   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Cursor Settings"), 0);
   rg = e_widget_radio_group_new(&cfdata->use_e_cursor);
   ob = e_widget_radio_add(evas, _("Use Enlightenment Cursor"), 1, rg);   
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Use X Cursor"), 0, rg);   
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Cursor Size"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f pixels"), 8, 128, 4, 0, NULL, &(cfdata->cursor_size), 150);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);   
   return o;
}
